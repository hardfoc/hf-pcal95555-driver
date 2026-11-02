/**
 * @file my_i2c.hpp
 * @brief The I2C port shown on the documentation's Quickstart and Porting pages, compiled by the host build.
 *
 * Every platform call goes through the `platform_*` functions declared below; a real port replaces them
 * with its SDK (ESP-IDF, STM32 HAL, Zephyr, Linux i2c-dev). The class is what the driver needs, nothing more.
 */
#pragma once

#include "pcal95555.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
bool platform_i2c_init() noexcept;
bool platform_i2c_write(uint8_t addr, const uint8_t* bytes, std::size_t len) noexcept;  // START .. STOP
bool platform_i2c_write_read(uint8_t addr, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                             std::size_t rx_len) noexcept;  // write, repeated START, read

// --8<-- [start:port]
class MyI2c : public pcal95555::I2cInterface<MyI2c> {
public:
  // Bring the bus up once; the driver calls this before its first transfer.
  bool EnsureInitialized() noexcept {
    if (!ready_) {
      ready_ = platform_i2c_init();
    }
    return ready_;
  }

  // One register write: [addr+W] [reg] [data...] STOP. Return false on a NACK.
  bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept {
    uint8_t frame[3] = {reg, 0, 0};
    if (len > 2) {
      return false;  // the driver writes one register at a time
    }
    for (size_t i = 0; i < len; ++i) {
      frame[1 + i] = data[i];
    }
    return platform_i2c_write(addr, frame, 1 + len);
  }

  // One register read: [addr+W] [reg] repeated START [addr+R] [data...] STOP.
  bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept {
    return platform_i2c_write_read(addr, &reg, 1, data, len);
  }

  // Optional: keep the driver's interrupt handler. Without it, poll instead.
  bool RegisterInterruptHandler(std::function<void()> handler) noexcept {
    handler_ = std::move(handler);
    return true;
  }

  // Call from a task, not from the ISR, after INT (open drain, active low) falls:
  // the handler reads the chip over I2C.
  void ServiceInterrupt() {
    if (handler_) {
      handler_();
    }
  }

private:
  bool ready_ = false;
  std::function<void()> handler_;
};
// --8<-- [end:port]
