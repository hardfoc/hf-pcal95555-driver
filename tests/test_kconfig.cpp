/**
 * @file test_kconfig.cpp
 * @brief InitFromConfig() with an ESP-IDF style sdkconfig.h (tests/kconfig/sdkconfig.h).
 *
 * Kconfig writes a bool set to "n" as an absent macro. The driver must read that as 0, not fall back
 * to its "default y" value. This file uses its own bus type so its InitFromConfig() instantiation
 * (built with these macros) is distinct from the one in test_driver.cpp.
 */
#include "mock_pcal95555.hpp"
#include "test_harness.hpp"

namespace {
class KconfigBus : public pcal95555_test::MockPcal95555 {
public:
  KconfigBus() : MockPcal95555(true) {}
};
} // namespace

static_assert(CONFIG_PCAL95555_P0_0_DIR == 0, "absent Kconfig bool must read as 0");
static_assert(CONFIG_PCAL95555_INIT_DIRECTION == 0x7FFE, "P0_0 and P1_7 are outputs");

TEST_CASE("InitFromConfig applies sdkconfig.h, reading unset bools as 0") {
  KconfigBus bus;
  pcal95555::PCAL95555<KconfigBus> drv(&bus, false, false, false);
  REQUIRE(drv.EnsureInitialized());
  drv.InitFromConfig();
  CHECK_EQ(bus.reg(0x06), 0xFE); // CONFIG_PORT_0: P0_0 output
  CHECK_EQ(bus.reg(0x07), 0x7F); // CONFIG_PORT_1: P1_7 output
  CHECK_EQ(bus.reg(0x02), 0x02); // OUTPUT_PORT_0: P0_1 high
  CHECK_EQ(bus.reg(0x03), 0x00);
  CHECK_EQ(bus.reg(0x46), 0x00); // pulls off (PULL options unset)
  CHECK_EQ(bus.reg(0x48), 0xFB); // PULL_SELECT_0: P0_2 pull-down
  CHECK_EQ(bus.reg(0x49), 0xFF);
}
