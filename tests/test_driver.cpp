/**
 * @file test_driver.cpp
 * @brief Driver behaviour over the register model: variant detection, addressing, pin access, Agile I/O
 *        register encoding, interrupts and error flags.
 */
#include "mock_pcal95555.hpp"
#include "test_harness.hpp"

using pcal95555::ChipVariant;
using pcal95555_test::MockPcal95555;
using Driver = pcal95555::PCAL95555<MockPcal95555>;

namespace {
constexpr uint8_t reg(Pcal95555Reg r) { return static_cast<uint8_t>(r); }
}  // namespace

TEST_CASE("detects a PCAL9555A by its Agile I/O bank") {
  MockPcal95555 bus(true);
  Driver drv(&bus, false, false, false);
  REQUIRE(drv.EnsureInitialized());
  CHECK(drv.GetChipVariant() == ChipVariant::PCAL9555A);
  CHECK(drv.HasAgileIO());
}

TEST_CASE("detects a PCA9555 when the Agile I/O bank NACKs") {
  MockPcal95555 bus(false);
  Driver drv(&bus, false, false, false);
  REQUIRE(drv.EnsureInitialized());
  CHECK(drv.GetChipVariant() == ChipVariant::PCA9555);
  CHECK(!drv.HasAgileIO());
}

TEST_CASE("Agile I/O calls on a PCA9555 fail with UnsupportedFeature and touch nothing") {
  MockPcal95555 bus(false);
  Driver drv(&bus, false, false, false);
  REQUIRE(drv.EnsureInitialized());
  bus.clearLog();
  CHECK(!drv.SetPullEnable(3, true));
  CHECK(!drv.SetDriveStrength(3, DriveStrength::Level1));
  CHECK(!drv.SetOutputMode(true, false));
  CHECK(drv.HasError(Error::UnsupportedFeature));
  CHECK_EQ(bus.log().size(), 0U);
}

TEST_CASE("address pins map to 0x20 + A2 A1 A0") {
  MockPcal95555 bus(true, 0x25);
  Driver drv(&bus, true, false, true);  // A0 = 1, A1 = 0, A2 = 1
  CHECK_EQ(drv.GetAddress(), 0x25);
  CHECK_EQ(drv.GetAddressBits(), 5);
  CHECK(drv.EnsureInitialized());
}

TEST_CASE("the address constructor clamps out-of-range addresses") {
  MockPcal95555 bus(true);
  Driver low(&bus, uint8_t{0x10});
  Driver high(&bus, uint8_t{0x40});
  CHECK_EQ(low.GetAddress(), 0x20);
  CHECK_EQ(high.GetAddress(), 0x27);
}

TEST_CASE("a missing device fails initialisation with I2CReadFail") {
  MockPcal95555 bus(true, 0x21);
  Driver drv(&bus, uint8_t{0x20});
  CHECK(!drv.EnsureInitialized());
  CHECK(drv.HasError(Error::I2CReadFail));
}

TEST_CASE("direction, output and polarity bits land in the right port register") {
  MockPcal95555 bus(true);
  Driver drv(&bus, uint8_t{0x20});
  CHECK(drv.SetPinDirection(9, GPIODir::Output));       // pin 9 = port 1 bit 1
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::CONFIG_PORT_1)), 0xFD);
  CHECK(drv.WritePin(9, false));
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::OUTPUT_PORT_1)), 0xFD);
  CHECK(drv.TogglePin(9));
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::OUTPUT_PORT_1)), 0xFF);
  CHECK(drv.SetPinPolarity(2, Polarity::Inverted));
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::POLARITY_INV_0)), 0x04);
}

TEST_CASE("reads return the input level after polarity inversion") {
  MockPcal95555 bus(false);
  Driver drv(&bus, uint8_t{0x20});
  bus.setPins(0x0101);
  CHECK(drv.ReadPin(0));
  CHECK(drv.ReadPin(8));
  CHECK(!drv.ReadPin(1));
  CHECK_EQ(drv.ReadAllInputs(), 0x0101);
  CHECK(drv.SetPinPolarity(0, Polarity::Inverted));
  CHECK(!drv.ReadPin(0));
}

TEST_CASE("invalid pins set InvalidPin and send nothing") {
  MockPcal95555 bus(true);
  Driver drv(&bus, uint8_t{0x20});
  REQUIRE(drv.EnsureInitialized());
  bus.clearLog();
  CHECK(!drv.WritePin(16, true));
  CHECK(drv.HasError(Error::InvalidPin));
  CHECK_EQ(bus.log().size(), 0U);
  drv.ClearErrorFlags();
  CHECK(!drv.HasAnyError());
}

TEST_CASE("drive strength uses two bits per pin across registers 0x40-0x43") {
  MockPcal95555 bus(true);
  Driver drv(&bus, uint8_t{0x20});
  CHECK(drv.SetDriveStrength(5, DriveStrength::Level1));    // port 0 pin 5: 0x41 bits [3:2]
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::DRIVE_STRENGTH_1)), 0xF7);
  CHECK(drv.SetDriveStrength(8, DriveStrength::Level0));    // port 1 pin 0: 0x42 bits [1:0]
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::DRIVE_STRENGTH_2)), 0xFC);
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::DRIVE_STRENGTH_0)), 0xFF);
}

TEST_CASE("pull, latch and output-mode registers") {
  MockPcal95555 bus(true);
  Driver drv(&bus, uint8_t{0x20});
  CHECK(drv.SetPullEnable(12, false));
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::PULL_ENABLE_1)), 0xEF);
  CHECK(drv.SetPullDirection(0, false));                     // pull-down
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::PULL_SELECT_0)), 0xFE);
  CHECK(drv.EnableInputLatch(7, true));
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::INPUT_LATCH_0)), 0x80);
  CHECK(drv.SetOutputMode(false, true));                     // ODEN1 = 1
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::OUTPUT_CONF)), 0x02);
  uint16_t en = 0;
  uint16_t sel = 0;
  CHECK(drv.GetPullConfiguration(en, sel));
  CHECK_EQ(en, 0xEFFF);
  CHECK_EQ(sel, 0xFFFE);
}

TEST_CASE("ResetToDefault restores the datasheet power-up values") {
  MockPcal95555 bus(true);
  Driver drv(&bus, uint8_t{0x20});
  CHECK(drv.SetPinDirection(0, GPIODir::Output));
  CHECK(drv.SetOutputMode(true, true));
  CHECK(drv.ConfigureInterrupt(4, InterruptState::Enabled));
  drv.ResetToDefault();
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::CONFIG_PORT_0)), 0xFF);
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::OUTPUT_CONF)), 0x00);
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::INT_MASK_0)), 0xFF);
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::PULL_ENABLE_0)), 0xFF);
}

TEST_CASE("an unmasked input change reaches its pin callback with the edge") {
  MockPcal95555 bus(true);
  Driver drv(&bus, uint8_t{0x20});
  REQUIRE(drv.ConfigureInterrupt(4, InterruptState::Enabled));
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::INT_MASK_0)), 0xEF);
  int calls = 0;
  bool level = false;
  REQUIRE(drv.RegisterPinInterrupt(4, InterruptEdge::Rising, [&](uint8_t pin, bool state) {
    calls += pin == 4 ? 1 : 100;
    level = state;
  }));
  uint16_t global = 0;
  drv.SetInterruptCallback([&](uint16_t status) { global = status; });
  REQUIRE(drv.RegisterInterruptHandler());

  bus.setPins(0x0010);  // pin 4 rises
  bus.fireInterrupt();
  CHECK_EQ(calls, 1);
  CHECK(level);
  CHECK_EQ(global, 0x0010);

  bus.setPins(0x0000);  // pin 4 falls: not a rising edge
  bus.fireInterrupt();
  CHECK_EQ(calls, 1);
}

TEST_CASE("a PCA9555 reports changes by comparing input snapshots") {
  MockPcal95555 bus(false);
  Driver drv(&bus, uint8_t{0x20});
  int calls = 0;
  REQUIRE(drv.RegisterPinInterrupt(10, InterruptEdge::Both, [&](uint8_t, bool) { ++calls; }));
  REQUIRE(drv.RegisterInterruptHandler());
  bus.setPins(0x0400);
  bus.fireInterrupt();
  bus.setPins(0x0000);
  bus.fireInterrupt();
  CHECK_EQ(calls, 2);
}

TEST_CASE("multi-pin helpers write each port once") {
  MockPcal95555 bus(true);
  Driver drv(&bus, uint8_t{0x20});
  CHECK(drv.SetMultipleDirections(0x00FF, GPIODir::Output));
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::CONFIG_PORT_0)), 0x00);
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::CONFIG_PORT_1)), 0xFF);
  CHECK(drv.WritePins({{0, false}, {1, false}, {15, false}}));
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::OUTPUT_PORT_0)), 0xFC);
  CHECK_EQ(bus.reg(reg(Pcal95555Reg::OUTPUT_PORT_1)), 0x7F);
  bus.setPins(0x8001);
  const auto r = drv.ReadPins({0, 1, 15});
  CHECK_EQ(r.size(), 3);
  CHECK(r[0].second);
  CHECK(!r[1].second);
  CHECK(r[2].second);
}
