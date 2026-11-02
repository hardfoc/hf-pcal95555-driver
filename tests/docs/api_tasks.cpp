/**
 * @file api_tasks.cpp
 * @brief The API guide's examples, task by task, compiled by the host build (never run).
 */
#include "my_i2c.hpp"

#include <cstdio>

using Gpio = pcal95555::PCAL95555<MyI2c>;
using pcal95555::ChipVariant;

// --8<-- [start:init]
bool bring_up(Gpio& gpio) {
  // Bus up, probe INPUT_PORT_0, detect the variant.
  if (!gpio.EnsureInitialized()) {
    return false;
  }
  const bool pcal = gpio.GetChipVariant() == ChipVariant::PCAL9555A;
  std::printf("0x%02X: %s\n", gpio.GetAddress(), pcal ? "PCAL9555A" : "PCA9555");
  return true;
}
// --8<-- [end:init]

// --8<-- [start:configure]
bool configure_pins(Gpio& gpio) {
  // Port 0 (pins 0-7) outputs, port 1 (pins 8-15) inputs.
  bool ok = gpio.SetMultipleDirections(0x00FF, GPIODir::Output);
  ok = ok && gpio.SetMultipleDirections(0xFF00, GPIODir::Input);
  // Pins 8 and 9 are active-low buttons: invert them so a press reads as true.
  ok = ok && gpio.SetPolarities({{8, Polarity::Inverted}, {9, Polarity::Inverted}});
  return ok;
}
// --8<-- [end:configure]

// --8<-- [start:write]
bool drive_outputs(Gpio& gpio) {
  bool ok = gpio.WritePin(0, true);                    // read-modify-write
  ok = ok && gpio.WritePins({{1, true}, {2, false}});  // each port once
  ok = ok && gpio.SetMultipleOutputs(0x00F0, false);  // pins 4-7 low
  ok = ok && gpio.TogglePin(3);
  return ok;
}
// --8<-- [end:write]

// --8<-- [start:read]
void read_inputs(Gpio& gpio) {
  bool button = false;
  if (!gpio.ReadPin(8, button)) {              // false: the read failed, button untouched
    std::printf("pin 8: read failed\n");
  }
  const bool led = gpio.ReadPin(0);            // shorthand: false on a bus error too
  const uint16_t all = gpio.ReadAllInputs();   // bit N = pin N
  const auto some = gpio.ReadPins({8, 9, 10});
  for (const auto& [pin, level] : some) {
    std::printf("pin %u: %d\n", pin, level);
  }
  if (gpio.HasError(Error::I2CReadFail)) {
    std::printf("read failed\n");
  }
  (void)button;
  (void)led;
  (void)all;
}
// --8<-- [end:read]

// --8<-- [start:agile]
bool configure_agile_io(Gpio& gpio) {
  if (!gpio.HasAgileIO()) {
    return true;  // PCA9555: none of this exists
  }
  // 100 kOhm (typ.) resistors: pin 10 pull-up, pin 11 pull-down.
  bool ok = gpio.SetPullEnables({{10, true}, {11, true}});
  ok = ok && gpio.SetPullDirections({{10, true}, {11, false}});
  // Half drive on the LED pin; port 0 open drain, port 1 push-pull.
  ok = ok && gpio.SetDriveStrength(0, DriveStrength::Level1);
  ok = ok && gpio.SetOutputMode(true, false);
  // Keep a short pulse on pin 12 until the input is read.
  ok = ok && gpio.EnableInputLatch(12, true);
  return ok;
}
// --8<-- [end:agile]

// --8<-- [start:interrupts]
bool watch_button(Gpio& gpio) {
  bool ok = gpio.SetPinDirection(8, GPIODir::Input);
  ok = ok && gpio.ConfigureInterrupt(8, InterruptState::Enabled);  // PCAL9555A
  auto on_fall = [](uint8_t pin, bool level) {
    std::printf("pin %u fell, now %d\n", pin, level);
  };
  ok = ok && gpio.RegisterPinInterrupt(8, InterruptEdge::Falling, on_fall);
  gpio.SetInterruptCallback([](uint16_t src) { std::printf("INT 0x%04X\n", src); });
  // The port now calls HandleInterrupt() when INT falls.
  return ok && gpio.RegisterInterruptHandler();
}
// --8<-- [end:interrupts]

// --8<-- [start:poll]
void poll_for_changes(Gpio& gpio) {
  // No INT line, or a port without RegisterInterruptHandler(): call it yourself.
  // Reads the sources (PCAL9555A) or diffs the inputs (PCA9555), then calls back.
  gpio.HandleInterrupt();
}
// --8<-- [end:poll]

// --8<-- [start:raw]
bool raw_registers(Gpio& gpio) {
  auto r = [](Pcal95555Reg reg) { return static_cast<uint8_t>(reg); };
  uint16_t config = 0;
  if (!gpio.ReadDualPortRegister(r(Pcal95555Reg::CONFIG_PORT_0),
                                 r(Pcal95555Reg::CONFIG_PORT_1), config)) {
    return false;
  }
  // Absolute write from a shadow you keep: no read-modify-write.
  return gpio.WriteDualPortRegister(r(Pcal95555Reg::OUTPUT_PORT_0),
                                    r(Pcal95555Reg::OUTPUT_PORT_1), 0xA55A);
}
// --8<-- [end:raw]

// --8<-- [start:errors]
void report_errors(Gpio& gpio) {
  if (!gpio.HasAnyError()) {
    return;
  }
  if (gpio.HasError(Error::I2CReadFail) || gpio.HasError(Error::I2CWriteFail)) {
    std::printf("bus: NACK or timeout at 0x%02X\n", gpio.GetAddress());
  }
  if (gpio.HasError(Error::UnsupportedFeature)) {
    std::printf("Agile I/O call on a PCA9555\n");
  }
  if (gpio.HasError(Error::InvalidPin)) {
    std::printf("pin index above 15\n");
  }
  // Sticky until cleared, or until a later success clears its own flag.
  gpio.ClearErrorFlags();
}
// --8<-- [end:errors]

// --8<-- [start:address]
bool two_expanders(MyI2c& bus) {
  Gpio left(&bus, uint8_t{0x20});   // A2 A1 A0 = 000
  Gpio right(&bus, uint8_t{0x21});  // A2 A1 A0 = 001
  return left.EnsureInitialized() && right.EnsureInitialized();
}
// --8<-- [end:address]
