/**
 * @file quickstart.cpp
 * @brief The Quickstart page's example, compiled by the host build (never run).
 */
#include "my_i2c.hpp"

// --8<-- [start:quickstart]
#include "pcal95555.hpp"

int led_follows_button(MyI2c& bus) {
  pcal95555::PCAL95555<MyI2c> gpio(&bus, false, false, false);  // A2 A1 A0 low: 0x20

  if (!gpio.EnsureInitialized()) {                    // 1. probe, detect the variant
    return -1;                                        // wiring, pull-ups or address
  }

  gpio.SetPinDirection(0, GPIODir::Output);           // 2. P0_0 drives an LED
  gpio.SetPinDirection(8, GPIODir::Input);            //    P1_0 reads a button

  const bool pressed = !gpio.ReadPin(8);              // 3. button pulls the pin low
  if (!gpio.WritePin(0, pressed)) {                   // 4. LED on while pressed
    return -2;                                        // see GetErrorFlags()
  }
  return 0;
}
// --8<-- [end:quickstart]
