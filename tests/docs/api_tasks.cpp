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
  if (!gpio.EnsureInitialized()) {        // bus up, probe INPUT_PORT_0, detect the variant
    return false;
  }
  const bool pcal = gpio.GetChipVariant() == ChipVariant::PCAL9555A;
  std::printf("expander at 0x%02X: %s\n", gpio.GetAddress(), pcal ? "PCAL9555A" : "PCA9555");
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
  bool ok = gpio.WritePin(0, true);                        // one pin: read-modify-write of one register
  ok = ok && gpio.WritePins({{1, true}, {2, false}});      // several pins, each port written once
  ok = ok && gpio.SetMultipleOutputs(0x00F0, false);       // pins 4-7 low
  ok = ok && gpio.TogglePin(3);
  return ok;
}
// --8<-- [end:write]

// --8<-- [start:read]
void read_inputs(Gpio& gpio) {
  const bool button = gpio.ReadPin(8);                     // false on a bus error too: check the flags
  const uint16_t all = gpio.ReadAllInputs();               // bit N = pin N, two register reads
  const auto some = gpio.ReadPins({8, 9, 10});
  for (const auto& [pin, level] : some) {
    std::printf("pin %u: %d\n", pin, level);
  }
  if (gpio.HasError(Error::I2CReadFail)) {
    std::printf("read failed\n");
  }
  (void)button;
  (void)all;
}
// --8<-- [end:read]

// --8<-- [start:agile]
bool configure_agile_io(Gpio& gpio) {
  if (!gpio.HasAgileIO()) {
    return true;                                           // PCA9555: none of this exists
  }
  bool ok = gpio.SetPullEnables({{10, true}, {11, true}}); // 100 kOhm (typ.) resistors on
  ok = ok && gpio.SetPullDirections({{10, true}, {11, false}});  // 10 pull-up, 11 pull-down
  ok = ok && gpio.SetDriveStrength(0, DriveStrength::Level1);    // 0.5 x full drive on the LED pin
  ok = ok && gpio.SetOutputMode(true, false);              // port 0 open drain, port 1 push-pull
  ok = ok && gpio.EnableInputLatch(12, true);              // keep a short pulse until it is read
  return ok;
}
// --8<-- [end:agile]

// --8<-- [start:interrupts]
bool watch_button(Gpio& gpio) {
  bool ok = gpio.SetPinDirection(8, GPIODir::Input);
  ok = ok && gpio.ConfigureInterrupt(8, InterruptState::Enabled);  // PCAL9555A: unmask pin 8
  ok = ok && gpio.RegisterPinInterrupt(8, InterruptEdge::Falling, [](uint8_t pin, bool level) {
    std::printf("pin %u fell, now %d\n", pin, level);
  });
  gpio.SetInterruptCallback([](uint16_t sources) { std::printf("INT: 0x%04X\n", sources); });
  return ok && gpio.RegisterInterruptHandler();            // the port calls HandleInterrupt()
}
// --8<-- [end:interrupts]

// --8<-- [start:poll]
void poll_for_changes(Gpio& gpio) {
  // Without an INT line or a port that registers the handler: call the handler yourself.
  gpio.HandleInterrupt();   // reads the sources (PCAL9555A) or diffs the inputs (PCA9555), calls back
}
// --8<-- [end:poll]

// --8<-- [start:raw]
bool raw_registers(Gpio& gpio) {
  uint16_t config = 0;
  if (!gpio.ReadDualPortRegister(static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_0),
                                 static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_1), config)) {
    return false;
  }
  // Absolute write from a shadow you keep: no read-modify-write.
  return gpio.WriteDualPortRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_0),
                                    static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_1), 0xA55A);
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
  gpio.ClearErrorFlags();   // flags are sticky until cleared (or a later success clears its own flag)
}
// --8<-- [end:errors]

// --8<-- [start:address]
bool two_expanders(MyI2c& bus) {
  Gpio left(&bus, uint8_t{0x20});                          // A2 A1 A0 = 000
  Gpio right(&bus, uint8_t{0x21});                         // A2 A1 A0 = 001
  return left.EnsureInitialized() && right.EnsureInitialized();
}
// --8<-- [end:address]
