/**
 * @file configuration.cpp
 * @brief The Configuration page's examples, compiled by the host build (never run).
 */
#include "my_i2c.hpp"

using Gpio = pcal95555::PCAL95555<MyI2c>;

// --8<-- [start:variant]
// Skip auto-detection when the board fixes the part (one fewer probe on the bus at start-up).
void make_drivers(MyI2c& bus) {
  Gpio auto_detect(&bus, uint8_t{0x20});                                       // probe 0x4F once
  Gpio known_pcal(&bus, uint8_t{0x21}, pcal95555::ChipVariant::PCAL9555A);    // trust the board
  Gpio known_pca(&bus, true, true, false, pcal95555::ChipVariant::PCA9555);    // 0x23
  (void)auto_detect;
  (void)known_pcal;
  (void)known_pca;
}
// --8<-- [end:variant]

// --8<-- [start:startup]
bool apply_startup_config(Gpio& gpio) {
  gpio.SetRetries(2);          // each register access tries up to 3 times
  gpio.ResetToDefault();       // datasheet power-up values, Agile I/O bank too on a PCAL9555A
  gpio.InitFromConfig();       // then the Kconfig values (no-op if CONFIG_PCAL95555_INIT_FROM_KCONFIG=0)
  return !gpio.HasAnyError();
}
// --8<-- [end:startup]
