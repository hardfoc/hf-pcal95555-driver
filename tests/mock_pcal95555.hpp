/**
 * @file mock_pcal95555.hpp
 * @brief A register-level model of a PCA9555 or PCAL9555A behind the driver's I2C interface.
 *
 * The model answers at one 7-bit address, holds the chip's registers with their power-up defaults
 * (PCA9555 Rev. 10 table 4 / PCAL9555A Rev. 2 table 4), NACKs the Agile I/O bank (0x40-0x4F) when
 * it models a PCA9555, and computes the Input Port registers from external pin levels and the
 * polarity inversion registers. It records every transfer so tests can check what went over the
 * bus.
 */
#pragma once

#include "pcal95555.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace pcal95555_test {

class MockPcal95555 : public pcal95555::I2cInterface<MockPcal95555> {
public:
  struct Transfer {
    bool write;
    uint8_t addr;
    uint8_t reg;
    uint8_t value;
  };

  explicit MockPcal95555(bool agile_io, uint8_t address = 0x20) noexcept
      : agile_io_(agile_io), address_(address) {
    regs_.fill(0x00);
    regs_[0x02] = 0xFF; // Output port 0
    regs_[0x03] = 0xFF; // Output port 1
    regs_[0x06] = 0xFF; // Configuration port 0: all inputs
    regs_[0x07] = 0xFF; // Configuration port 1: all inputs
    if (agile_io_) {
      for (uint8_t r = 0x40; r <= 0x43; ++r) {
        regs_[r] = 0xFF; // Drive strength: full
      }
      regs_[0x46] = regs_[0x47] = 0xFF; // Pull enable: on
      regs_[0x48] = regs_[0x49] = 0xFF; // Pull select: pull-up
      regs_[0x4A] = regs_[0x4B] = 0xFF; // Interrupt mask: all masked
    }
  }

  // ---- I2cInterface ----
  bool EnsureInitialized() noexcept {
    return true;
  }

  bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept {
    if (!accepts(addr, reg)) {
      log_.push_back({true, addr, reg, len > 0 ? data[0] : uint8_t{0}});
      return false; // NACK: wrong address, or the Agile I/O bank on a PCA9555
    }
    if (reg == 0x00 || reg == 0x01 || reg == 0x4C || reg == 0x4D) {
      log_.push_back({true, addr, reg, len > 0 ? data[0] : uint8_t{0}});
      return true; // read-only registers acknowledge and ignore the data
    }
    for (size_t i = 0; i < len; ++i) {
      regs_[reg + i] = data[i];
      log_.push_back({true, addr, static_cast<uint8_t>(reg + i), data[i]});
    }
    return true;
  }

  bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept {
    if (read_hook_) {
      read_hook_(reg);
    }
    if (nack_reads_ > 0) {
      --nack_reads_;
      log_.push_back({false, addr, reg, 0});
      return false;
    }
    if (!accepts(addr, reg)) {
      if (agile_io_ == false && reg >= 0x40) {
        nack_reads_ = nack_after_probe_;
      }
      log_.push_back({false, addr, reg, 0});
      return false;
    }
    for (size_t i = 0; i < len; ++i) {
      const auto r = static_cast<uint8_t>(reg + i);
      data[i] = value(r);
      log_.push_back({false, addr, r, data[i]});
      if (r == 0x00 || r == 0x01) {
        pending_irq_ &= static_cast<uint16_t>(r == 0x00 ? 0xFF00 : 0x00FF); // input read clears INT
      }
    }
    return true;
  }

  bool RegisterInterruptHandler(std::function<void()> handler) noexcept {
    handler_ = std::move(handler);
    return true;
  }

  // ---- Test controls ----
  /// Drive the external level of the 16 port pins and latch an interrupt for unmasked inputs that
  /// changed.
  void setPins(uint16_t levels) noexcept {
    const uint16_t changed = static_cast<uint16_t>(levels ^ pins_);
    pins_ = levels;
    pending_irq_ |=
        static_cast<uint16_t>(changed & configMask() & static_cast<uint16_t>(~intMask()));
  }
  void fireInterrupt() const {
    if (handler_) {
      handler_();
    }
  }
  [[nodiscard]] uint8_t reg(uint8_t r) const noexcept {
    return regs_[r];
  }
  void setReg(uint8_t r, uint8_t v) noexcept {
    regs_[r] = v;
  }
  [[nodiscard]] const std::vector<Transfer>& log() const noexcept {
    return log_;
  }
  void clearLog() noexcept {
    log_.clear();
  }
  void setAddress(uint8_t a) noexcept {
    address_ = a;
  }
  /// Model a bus that does not recover after a NACK of the Agile I/O probe: the next @p n reads
  /// fail as well.
  void nackReadsAfterProbe(int n) noexcept {
    nack_after_probe_ = n;
  }
  /// Called at the start of every Read() with the register; lets a test run code mid-transfer.
  void setReadHook(std::function<void(uint8_t)> hook) {
    read_hook_ = std::move(hook);
  }

private:
  [[nodiscard]] bool accepts(uint8_t addr, uint8_t reg) const noexcept {
    if (addr != address_) {
      return false;
    }
    if (reg <= 0x07) {
      return true;
    }
    return agile_io_ && reg >= 0x40 && reg <= 0x4F && reg != 0x4E;
  }

  [[nodiscard]] uint16_t configMask() const noexcept {
    return static_cast<uint16_t>(regs_[0x06] | (regs_[0x07] << 8));
  }
  [[nodiscard]] uint16_t intMask() const noexcept {
    return agile_io_ ? static_cast<uint16_t>(regs_[0x4A] | (regs_[0x4B] << 8)) : uint16_t{0};
  }

  [[nodiscard]] uint8_t value(uint8_t r) const noexcept {
    if (r == 0x00 || r == 0x01) {
      const auto shift = r == 0x00 ? 0 : 8;
      const auto level = static_cast<uint8_t>(pins_ >> shift);
      return static_cast<uint8_t>(level ^ regs_[0x04 + r]); // polarity inversion applies to inputs
    }
    if (r == 0x4C || r == 0x4D) {
      return static_cast<uint8_t>(pending_irq_ >> (r == 0x4C ? 0 : 8));
    }
    return regs_[r];
  }

  bool agile_io_;
  uint8_t address_;
  std::array<uint8_t, 0x50> regs_{};
  uint16_t pins_{0};
  uint16_t pending_irq_{0};
  std::function<void()> handler_;
  std::function<void(uint8_t)> read_hook_;
  int nack_after_probe_{0};
  int nack_reads_{0};
  std::vector<Transfer> log_;
};

} // namespace pcal95555_test
