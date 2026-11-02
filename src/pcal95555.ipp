/**
 * @file pcal95555.ipp
 * @brief Template implementation of PCAL95555 I/O expander driver
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#ifndef PCAL95555_IMPL
#define PCAL95555_IMPL

// When included from header, use relative path; when compiled directly, use standard include
#ifdef PCAL95555_HEADER_INCLUDED
#include "../inc/pcal95555.hpp"
#else
#include "../inc/pcal95555.hpp"
#endif

// Constructor: store I2C bus and pin levels (lazy initialization)
template <typename I2cType>
pcal95555::PCAL95555<I2cType>::PCAL95555(I2cType* bus, bool a0_level, bool a1_level, bool a2_level,
                                          ChipVariant variant)
    : i2c_(bus), previous_pin_states_(0), initialized_(false),
      a0_level_(a0_level), a1_level_(a1_level), a2_level_(a2_level),
      chip_variant_(ChipVariant::Unknown), user_variant_(variant) {
  // Calculate address bits from pin levels (A0=bit0, A1=bit1, A2=bit2)
  address_bits_ = (a0_level ? 1 : 0) | ((a1_level ? 1 : 0) << 1) | ((a2_level ? 1 : 0) << 2);
  dev_addr_ = calculateAddress(address_bits_);

  // Initialize pin callbacks array
  for (auto& callback : pin_callbacks_) {
    callback.registered = false;
    callback.edge = InterruptEdge::Both;
  }

  // No initialization here - use EnsureInitialized() when ready
}

// Constructor: store I2C bus and calculate pin levels from address (lazy initialization)
template <typename I2cType>
pcal95555::PCAL95555<I2cType>::PCAL95555(I2cType* bus, uint8_t address, ChipVariant variant)
    : i2c_(bus), previous_pin_states_(0), initialized_(false),
      chip_variant_(ChipVariant::Unknown), user_variant_(variant) {
  // Validate address range (0x20 to 0x27)
  constexpr uint8_t BASE_ADDRESS = 0x20;
  constexpr uint8_t MAX_ADDRESS = 0x27;
  constexpr uint8_t MAX_BITS = 0x07;

  if (address < BASE_ADDRESS || address > MAX_ADDRESS) {
    // Address out of range - clamp to valid range
    if (address < BASE_ADDRESS) {
      address = BASE_ADDRESS;
    } else {
      address = MAX_ADDRESS;
    }
    // Note: We don't set an error flag here as the address is still valid, just clamped
  }

  // Calculate address bits from address: address_bits = address - BASE_ADDRESS
  address_bits_ = (address - BASE_ADDRESS) & MAX_BITS;
  dev_addr_ = calculateAddress(address_bits_);

  // Calculate pin levels from address bits and store for lazy initialization
  a0_level_ = (address_bits_ & 0x01) != 0;
  a1_level_ = (address_bits_ & 0x02) != 0;
  a2_level_ = (address_bits_ & 0x04) != 0;

  // Initialize pin callbacks array
  for (auto& callback : pin_callbacks_) {
    callback.registered = false;
    callback.edge = InterruptEdge::Both;
  }

  // No initialization here - use EnsureInitialized() when ready
}

// Ensure initialization (lazy initialization)
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::EnsureInitialized() noexcept {
  if (initialized_) {
    return true;  // Already initialized
  }
  return initialize();
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::ForceMarkInitialized(ChipVariant variant) noexcept {
  if (variant == ChipVariant::Unknown) {
    return false;
  }
  user_variant_ = variant;
  chip_variant_ = variant;
  previous_pin_states_ = 0;
  clearError(Error::I2CReadFail);
  initialized_ = true;
  return true;
}

// Perform actual initialization
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::initialize() noexcept {
  // Ensure I2C bus is initialized and ready
  if (!i2c_->EnsureInitialized()) {
    setError(Error::I2CReadFail);
    initialized_ = false;
    return false;
  }

  // Attempt to set address pins via I2C interface (if supported)
  // This will return false if GPIO control is not implemented (hardwired pins)
  i2c_->SetAddressPins(a0_level_, a1_level_, a2_level_);

  // Verify communication by reading a register (INPUT_PORT_0)
  // This ensures the device is accessible at the calculated address
  uint8_t test_value = 0;
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_0), test_value)) {
    // Communication failed - this is expected if address pins are hardwired differently
    // or device is not connected. We still store the address for potential retry.
    setError(Error::I2CReadFail);
    initialized_ = false;
    return false;
  }

  // Communication successful - clear any previous errors
  clearError(Error::I2CReadFail);

  // Detect chip variant (PCA9555 vs PCAL9555A)
  if (user_variant_ != ChipVariant::Unknown) {
    // User specified variant - skip auto-detection
    chip_variant_ = user_variant_;
  } else {
    // Auto-detect by probing an Agile I/O register. An inconclusive probe (the bus did not answer
    // the read after it) leaves the variant unknown: fail so the next call retries instead of
    // running with no feature set.
    if (!detectChipVariant()) {
      setError(Error::I2CReadFail);
      initialized_ = false;
      return false;
    }
  }
  
  // Initialize previous pin states for edge detection
  previous_pin_states_ = readPinStates();
  
  // Mark as initialized
  initialized_ = true;
  return true;
}

// Configure retry count
template <typename I2cType>
void pcal95555::PCAL95555<I2cType>::SetRetries(int retries) noexcept {
  retries_ = retries;
}

// Low-level write with retries
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::writeRegister(uint8_t reg, uint8_t value) noexcept {
  /* Stage the payload in TU static storage before the I2C write.
   * Callers may pass a stack/external-RAM byte; some MCU memory maps do not
   * reliably support single-byte loads from those regions into the bus DMA
   * path, so the adapter always reads from internal SRAM/.bss. */
  static uint8_t s_byte = 0;
  s_byte = value;
  for (int attempt = 0; attempt <= retries_; ++attempt) {
    if (i2c_->Write(dev_addr_, reg, &s_byte, 1)) {
      clearError(Error::I2CWriteFail);
      return true;
    }
  }
  setError(Error::I2CWriteFail);
  return false;
}
// Low-level read with retries
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::readRegister(uint8_t reg, uint8_t& value) noexcept {
  static uint8_t s_byte = 0;
  for (int attempt = 0; attempt <= retries_; ++attempt) {
    if (i2c_->Read(dev_addr_, reg, &s_byte, 1)) {
      clearError(Error::I2CReadFail);
      value = s_byte;
      return true;
    }
  }
  setError(Error::I2CReadFail);
  return false;
}

// Reset all registers to defaults as per datasheet.
template <typename I2cType>
void pcal95555::PCAL95555<I2cType>::ResetToDefault() noexcept {
  if (!EnsureInitialized()) {
    return;
  }
  // Standard PCA9555 registers (always available)
  writeRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_0), 0xFF);
  writeRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_1), 0xFF);
  writeRegister(static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_0), 0x00);
  writeRegister(static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_1), 0x00);
  writeRegister(static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_0), 0xFF);
  writeRegister(static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_1), 0xFF);

  // PCAL9555A Agile I/O registers (only if chip supports them)
  if (chip_variant_ == ChipVariant::PCAL9555A) {
    // Drive strength = full (all bits 1)
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_0), 0xFF);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_1), 0xFF);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_2), 0xFF);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_3), 0xFF);
    // Latch disabled (default 0x00)
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_0), 0x00);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_1), 0x00);
    // Pull-up/down enabled (1), pull-up selected (1), interrupt masked (1)
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_0), 0xFF);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_1), 0xFF);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_0), 0xFF);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_1), 0xFF);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::INT_MASK_0), 0xFF);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::INT_MASK_1), 0xFF);
    // Output mode = push-pull (0)
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_CONF), 0x00);
  }
}

// Initialize using compile-time configuration
template <typename I2cType>
void pcal95555::PCAL95555<I2cType>::InitFromConfig() noexcept {
  if (!EnsureInitialized()) {
    return;
  }
#if CONFIG_PCAL95555_INIT_FROM_KCONFIG
  // Standard PCA9555 registers (always available)
  writeRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_0), uint8_t(CONFIG_PCAL95555_INIT_OUTPUT & 0xFF));
  writeRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_1), uint8_t((CONFIG_PCAL95555_INIT_OUTPUT >> 8) & 0xFF));

  writeRegister(static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_0), uint8_t(CONFIG_PCAL95555_INIT_DIRECTION & 0xFF));
  writeRegister(static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_1),
                uint8_t((CONFIG_PCAL95555_INIT_DIRECTION >> 8) & 0xFF));

  // PCAL9555A Agile I/O registers (only if chip supports them)
  if (chip_variant_ == ChipVariant::PCAL9555A) {
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_0), uint8_t(CONFIG_PCAL95555_INIT_PULL_ENABLE & 0xFF));
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_1),
                  uint8_t((CONFIG_PCAL95555_INIT_PULL_ENABLE >> 8) & 0xFF));

    writeRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_0), uint8_t(CONFIG_PCAL95555_INIT_PULL_UP & 0xFF));
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_1), uint8_t((CONFIG_PCAL95555_INIT_PULL_UP >> 8) & 0xFF));

    uint8_t open_drain_config =
        (CONFIG_PCAL95555_INIT_OD_PORT1 ? 1 : 0) << 1 | (CONFIG_PCAL95555_INIT_OD_PORT0 ? 1 : 0);
    writeRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_CONF), open_drain_config);
  }
#endif
}

// Set or clear a bit in a register byte
static inline uint8_t updateBit(uint8_t regVal, uint8_t bit, bool set) noexcept {
  if (set) {
    return regVal | (1 << bit);
  }
  return regVal & ~(1 << bit);
}

// ---- Internal R-M-W helpers ----

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::readDualPort(uint8_t reg0, uint8_t reg1,
                                                  uint8_t& val0, uint8_t& val1) noexcept {
  /* Dual-port assemble in TU static storage (not caller stack / external RAM).
   * Keeps both port bytes coherent for R-M-W and avoids single-byte load
   * hazards on MCU maps where task stacks sit outside tightly-coupled SRAM. */
  static uint8_t s_ports[2] = {0, 0};
  if (!readRegister(reg0, s_ports[0])) {
    return false;
  }
  if (!readRegister(reg1, s_ports[1])) {
    return false;
  }
  val0 = s_ports[0];
  val1 = s_ports[1];
  return true;
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::writeDualPort(uint8_t reg0, uint8_t reg1,
                                                   uint8_t val0, uint8_t val1) noexcept {
  /* Two single-register writes. (A 3-byte sequential [cmd][v0][v1] frame was
   * observed to leave CONFIG mismatched on Mid I2C0 while reads are fixed via
   * sequential 2-byte Mem_Read below.) */
  if (!writeRegister(reg0, val0)) {
    return false;
  }
  return writeRegister(reg1, val1);
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::modifySinglePinRegister(uint8_t reg0, uint8_t reg1,
                                                             uint8_t pin, bool bit_value) noexcept {
  uint8_t reg = (pin < 8) ? reg0 : reg1;
  uint8_t bit = pin % 8;
  uint8_t val = 0;
  if (!readRegister(reg, val)) {
    return false;
  }
  val = updateBit(val, bit, bit_value);
  return writeRegister(reg, val);
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::modifyDualPortByMask(uint8_t reg0, uint8_t reg1,
                                                          uint16_t mask, bool bit_value) noexcept {
  uint8_t val0 = 0;
  uint8_t val1 = 0;
  if (!readDualPort(reg0, reg1, val0, val1)) {
    return false;
  }
  for (int bit = 0; bit < 8; ++bit) {
    if ((mask & (1U << bit)) != 0U) {
      val0 = updateBit(val0, bit, bit_value);
    }
    if ((mask & (1U << (bit + 8))) != 0U) {
      val1 = updateBit(val1, bit, bit_value);
    }
  }
  return writeDualPort(reg0, reg1, val0, val1);
}

// ---- Direction configuration ----

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetPinDirection(uint8_t pin, GPIODir dir) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);
  return modifySinglePinRegister(static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_0),
                                  static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_1),
                                  pin, (dir == GPIODir::Input));
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetMultipleDirections(uint16_t mask, GPIODir dir) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  clearError(Error::InvalidMask);
  return modifyDualPortByMask(static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_0),
                               static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_1),
                               mask, (dir == GPIODir::Input));
}

// Configure direction for multiple pins with individual settings
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetDirections(std::initializer_list<std::pair<uint8_t, GPIODir>> configs) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  uint8_t port0 = 0;
  uint8_t port1 = 0;
  if (!readDualPort(static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_0),
                     static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_1), port0, port1)) {
    return false;
  }
  for (const auto& config : configs) {
    if (config.first >= 16) {
      setError(Error::InvalidPin);
      return false;
    }
    uint8_t bit = config.first % 8;
    if (config.first < 8) {
      port0 = updateBit(port0, bit, (config.second == GPIODir::Input));
    } else {
      port1 = updateBit(port1, bit, (config.second == GPIODir::Input));
    }
  }
  clearError(Error::InvalidPin);
  return writeDualPort(static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_0),
                        static_cast<uint8_t>(Pcal95555Reg::CONFIG_PORT_1), port0, port1);
}

// Read input port registers and return bit
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::ReadPin(uint8_t pin) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);
  uint8_t reg = (pin < 8) ? static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_0) : static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_1);
  uint8_t bit = pin % 8;
  /* Read into TU static storage, then mask. Avoids relying on a stack-local
   * receive byte when the I2C adapter may only safely touch internal SRAM. */
  static uint8_t s_val = 0;
  if (!readRegister(reg, s_val)) {
    return false;
  }
  return (s_val & (1 << bit)) != 0;
}

// Write output port registers
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::WritePin(uint8_t pin, bool value) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);
  return modifySinglePinRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_0),
                                  static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_1),
                                  pin, value);
}

// Set multiple outputs via bitmask
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetMultipleOutputs(uint16_t mask, bool value) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  clearError(Error::InvalidMask);
  return modifyDualPortByMask(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_0),
                               static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_1),
                               mask, value);
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::TogglePin(uint8_t pin) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);
  uint8_t reg = (pin < 8) ? static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_0)
                           : static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_1);
  uint8_t bit = pin % 8;
  uint8_t val = 0;
  if (!readRegister(reg, val)) {
    return false;
  }
  val ^= (1 << bit);
  return writeRegister(reg, val);
}

// Write multiple pins
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::WritePins(std::initializer_list<std::pair<uint8_t, bool>> configs) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  uint8_t port0 = 0;
  uint8_t port1 = 0;
  if (!readDualPort(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_0),
                     static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_1), port0, port1)) {
    return false;
  }
  for (const auto& config : configs) {
    if (config.first >= 16) {
      setError(Error::InvalidPin);
      return false;
    }
    uint8_t bit = config.first % 8;
    if (config.first < 8) {
      port0 = updateBit(port0, bit, config.second);
    } else {
      port1 = updateBit(port1, bit, config.second);
    }
  }
  clearError(Error::InvalidPin);
  return writeDualPort(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_0),
                        static_cast<uint8_t>(Pcal95555Reg::OUTPUT_PORT_1), port0, port1);
}

// Read multiple pins (zero heap allocation — uses stack-based PinReadResult)
template <typename I2cType>
pcal95555::PinReadResult pcal95555::PCAL95555<I2cType>::ReadPins(std::initializer_list<uint8_t> pins) noexcept {
  PinReadResult results;
  if (!EnsureInitialized()) {
    return results;  // Return empty results if not initialized
  }

  // Read both input port registers once
  uint8_t port0 = 0;
  uint8_t port1 = 0;
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_0), port0)) {
    // On failure, return empty results
    return results;
  }
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_1), port1)) {
    return results;
  }

  // Extract values for each requested pin (skip duplicates)
  for (uint8_t pin : pins) {
    if (results.contains(pin)) continue;  // Deduplicate repeated pins

    if (pin >= 16) {
      // Invalid pin - add with false value
      results.push_back(pin, false);
      continue;
    }

    uint8_t bit = pin % 8;
    bool value = false;
    if (pin < 8) {
      value = (port0 & (1 << bit)) != 0;
    } else {
      value = (port1 & (1 << bit)) != 0;
    }
    results.push_back(pin, value);
  }

  return results;
}

// Pull-up/down control
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetPullEnable(uint8_t pin, bool enable) noexcept {
  if (!EnsureInitialized() || !requireAgileIO()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);
  return modifySinglePinRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_0),
                                  static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_1),
                                  pin, enable);
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetPullDirection(uint8_t pin, bool pull_up) noexcept {
  if (!EnsureInitialized() || !requireAgileIO()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);
  return modifySinglePinRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_0),
                                  static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_1),
                                  pin, pull_up);
}

// Configure pull enable for multiple pins
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetPullEnables(std::initializer_list<std::pair<uint8_t, bool>> configs) noexcept {
  if (!EnsureInitialized() || !requireAgileIO()) {
    return false;
  }
  uint8_t port0 = 0;
  uint8_t port1 = 0;
  if (!readDualPort(static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_0),
                     static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_1), port0, port1)) {
    return false;
  }
  for (const auto& config : configs) {
    if (config.first >= 16) {
      setError(Error::InvalidPin);
      return false;
    }
    uint8_t bit = config.first % 8;
    if (config.first < 8) {
      port0 = updateBit(port0, bit, config.second);
    } else {
      port1 = updateBit(port1, bit, config.second);
    }
  }
  clearError(Error::InvalidPin);
  return writeDualPort(static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_0),
                        static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_1), port0, port1);
}

// Configure pull direction for multiple pins
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetPullDirections(std::initializer_list<std::pair<uint8_t, bool>> configs) noexcept {
  if (!EnsureInitialized() || !requireAgileIO()) {
    return false;
  }
  uint8_t port0 = 0;
  uint8_t port1 = 0;
  if (!readDualPort(static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_0),
                     static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_1), port0, port1)) {
    return false;
  }
  for (const auto& config : configs) {
    if (config.first >= 16) {
      setError(Error::InvalidPin);
      return false;
    }
    uint8_t bit = config.first % 8;
    if (config.first < 8) {
      port0 = updateBit(port0, bit, config.second);
    } else {
      port1 = updateBit(port1, bit, config.second);
    }
  }
  clearError(Error::InvalidPin);
  return writeDualPort(static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_0),
                        static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_1), port0, port1);
}

// Read pull configuration from hardware
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::GetPullConfiguration(uint16_t& enable_mask,
                                                          uint16_t& direction_mask) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (!requireAgileIO()) {
    return false;
  }

  uint8_t en0 = 0, en1 = 0, sel0 = 0, sel1 = 0;
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_0), en0)) {
    return false;
  }
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_ENABLE_1), en1)) {
    return false;
  }
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_0), sel0)) {
    return false;
  }
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::PULL_SELECT_1), sel1)) {
    return false;
  }

  enable_mask    = static_cast<uint16_t>((en1 << 8) | en0);
  direction_mask = static_cast<uint16_t>((sel1 << 8) | sel0);
  return true;
}

// Drive strength (2 bits per pin)
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetDriveStrength(uint8_t pin, DriveStrength level) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (!requireAgileIO()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);
  uint8_t base = (pin < 8) ? static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_0) : static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_2);
  uint8_t index = pin % 8;
  uint8_t reg = base + ((index >= 4) ? 1 : 0);
  uint8_t bit = (index % 4) * 2; // each pin uses 2 bits
  uint8_t val = 0;
  if (!readRegister(reg, val)) {
    return false;
  }
  // Clear the two bits and set the new level
  val &= ~(0x3 << bit);
  val |= (uint8_t(level) << bit);
  return writeRegister(reg, val);
}

// Configure drive strength for multiple pins
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetDriveStrengths(std::initializer_list<std::pair<uint8_t, DriveStrength>> configs) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (!requireAgileIO()) {
    return false;
  }
  // Read all drive strength registers (4 registers total)
  uint8_t ds0 = 0, ds1 = 0, ds2 = 0, ds3 = 0;
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_0), ds0)) {
    return false;
  }
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_1), ds1)) {
    return false;
  }
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_2), ds2)) {
    return false;
  }
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_3), ds3)) {
    return false;
  }

  // Update values for each pin
  for (const auto& config : configs) {
    uint8_t pin = config.first;
    DriveStrength level = config.second;

    if (pin >= 16) {
      setError(Error::InvalidPin);
      return false;
    }

    uint8_t index = pin % 8;
    uint8_t reg_offset = (index >= 4) ? 1 : 0;
    uint8_t bit = (index % 4) * 2; // each pin uses 2 bits

    uint8_t* reg_ptr = nullptr;
    if (pin < 8) {
      reg_ptr = (reg_offset == 0) ? &ds0 : &ds1;
    } else {
      reg_ptr = (reg_offset == 0) ? &ds2 : &ds3;
    }

    // Clear the two bits and set the new level
    *reg_ptr &= ~(0x3 << bit);
    *reg_ptr |= (uint8_t(level) << bit);
  }

  clearError(Error::InvalidPin);

  // Write back all registers
  if (!writeRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_0), ds0)) {
    return false;
  }
  if (!writeRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_1), ds1)) {
    return false;
  }
  if (!writeRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_2), ds2)) {
    return false;
  }
  return writeRegister(static_cast<uint8_t>(Pcal95555Reg::DRIVE_STRENGTH_3), ds3);
}

// Configure interrupt for a single pin
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::ConfigureInterrupt(uint8_t pin, InterruptState state) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (!requireAgileIO()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);

  // Read current mask
  uint8_t mask0 = 0;
  uint8_t mask1 = 0;
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::INT_MASK_0), mask0)) {
    return false;
  }
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::INT_MASK_1), mask1)) {
    return false;
  }

  // Combine into 16-bit mask
  uint16_t mask = (uint16_t(mask1) << 8) | mask0;

  // Update the bit for this pin (0 = enabled, 1 = disabled)
  if (state == InterruptState::Enabled) {
    mask &= ~(1U << pin);  // Clear bit (enable interrupt)
  } else {
    mask |= (1U << pin);   // Set bit (disable interrupt)
  }

  // Write back
  return ConfigureInterruptMask(mask);
}

// Configure interrupts for multiple pins
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::ConfigureInterrupts(std::initializer_list<std::pair<uint8_t, InterruptState>> configs) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (!requireAgileIO()) {
    return false;
  }
  // Read current mask
  uint8_t mask0 = 0;
  uint8_t mask1 = 0;
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::INT_MASK_0), mask0)) {
    return false;
  }
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::INT_MASK_1), mask1)) {
    return false;
  }

  // Combine into 16-bit mask
  uint16_t mask = (uint16_t(mask1) << 8) | mask0;

  // Update mask for each pin configuration
  for (const auto& config : configs) {
    uint8_t pin = config.first;
    InterruptState state = config.second;

    if (pin >= 16) {
      setError(Error::InvalidPin);
      return false;
    }

    // Update the bit for this pin (0 = enabled, 1 = disabled)
    if (state == InterruptState::Enabled) {
      mask &= ~(1U << pin);  // Clear bit (enable interrupt)
    } else {
      mask |= (1U << pin);   // Set bit (disable interrupt)
    }
  }

  clearError(Error::InvalidPin);

  // Write back once for all pins
  return ConfigureInterruptMask(mask);
}

// Interrupt mask (low-level method)
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::ConfigureInterruptMask(uint16_t mask) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (!requireAgileIO()) {
    return false;
  }
  if (!writeRegister(static_cast<uint8_t>(Pcal95555Reg::INT_MASK_0), uint8_t(mask & 0xFF))) {
    return false;
  }
  return writeRegister(static_cast<uint8_t>(Pcal95555Reg::INT_MASK_1), uint8_t((mask >> 8) & 0xFF));
}

// Read interrupt status (and clear)
template <typename I2cType>
uint16_t pcal95555::PCAL95555<I2cType>::GetInterruptStatus() noexcept {
  if (!EnsureInitialized()) {
    return 0;
  }
  if (!requireAgileIO()) {
    return 0;
  }
  uint8_t low_byte = 0;
  uint8_t high_byte = 0;
  readRegister(static_cast<uint8_t>(Pcal95555Reg::INT_STATUS_0), low_byte);
  readRegister(static_cast<uint8_t>(Pcal95555Reg::INT_STATUS_1), high_byte);
  return uint16_t(high_byte) << 8 | low_byte;
}

// Output mode configuration (ODEN bits)
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetOutputMode(bool port_0_open_drain, bool port_1_open_drain) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (!requireAgileIO()) {
    return false;
  }
  uint8_t val = (port_1_open_drain ? 1 : 0) << 1 | (port_0_open_drain ? 1 : 0);
  return writeRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_CONF), val);
}

// Register pin interrupt callback
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::RegisterPinInterrupt(uint8_t pin, InterruptEdge edge,
                                                         std::function<void(uint8_t, bool)> callback) {
  if (!EnsureInitialized()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);

  if (!callback) {
    return false;  // Invalid callback
  }

  pin_callbacks_[pin].callback = callback;
  pin_callbacks_[pin].edge = edge;
  pin_callbacks_[pin].registered = true;

  // Read current pin state for edge detection
  uint16_t current_states = readPinStates();
  previous_pin_states_ = current_states;

  return true;
}

// Unregister pin interrupt callback
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::UnregisterPinInterrupt(uint8_t pin) noexcept {
  // No I2C communication needed, but check initialization for consistency
  if (!EnsureInitialized()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);

  if (!pin_callbacks_[pin].registered) {
    return false;  // No callback registered
  }

  pin_callbacks_[pin].registered = false;
  pin_callbacks_[pin].callback = nullptr;
  return true;
}

// Set global interrupt callback
template <typename I2cType>
void pcal95555::PCAL95555<I2cType>::SetInterruptCallback(const std::function<void(uint16_t)>& callback) noexcept {
  irq_callback_ = callback;
}

// Register interrupt handler with I2C interface
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::RegisterInterruptHandler() noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  // Register this driver's HandleInterrupt method with the I2C interface
  return i2c_->RegisterInterruptHandler([this]() {
    HandleInterrupt();
  });
}

// Read current pin states (private helper)
template <typename I2cType>
uint16_t pcal95555::PCAL95555<I2cType>::readPinStates() noexcept {
  /* Assemble both INPUT ports in TU static storage (see readDualPort). */
  static uint8_t s_ports[2] = {0, 0};
  readRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_0), s_ports[0]);
  readRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_1), s_ports[1]);
  return (uint16_t(s_ports[1]) << 8) | s_ports[0];
}

// Read all 16 pin input states (public API)
template <typename I2cType>
uint16_t pcal95555::PCAL95555<I2cType>::ReadAllInputs() noexcept {
  if (!EnsureInitialized()) {
    return 0;
  }
  return readPinStates();
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::ReadDualPortRegister(uint8_t reg0, uint8_t reg1,
                                                         uint16_t& value) noexcept {
  value = 0;
  if (!EnsureInitialized()) {
    return false;
  }
  /* Assemble through readDualPort’s static staging buffers. */
  static uint8_t s_ports[2] = {0, 0};
  if (!readDualPort(reg0, reg1, s_ports[0], s_ports[1])) {
    return false;
  }
  value = static_cast<uint16_t>(s_ports[0]) |
          static_cast<uint16_t>(static_cast<uint16_t>(s_ports[1]) << 8);
  return true;
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::WriteDualPortRegister(uint8_t reg0, uint8_t reg1,
                                                          uint16_t value) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  return writeDualPort(reg0, reg1, static_cast<uint8_t>(value & 0xFFU),
                       static_cast<uint8_t>((value >> 8) & 0xFFU));
}

// Handle interrupt - read status, check conditions, call callbacks
template <typename I2cType>
void pcal95555::PCAL95555<I2cType>::HandleInterrupt() noexcept {
  if (!EnsureInitialized()) {
    return;
  }

  uint16_t interrupt_status = 0;

  if (chip_variant_ == ChipVariant::PCAL9555A) {
    // PCAL9555A: read hardware interrupt status register
    interrupt_status = GetInterruptStatus();
  } else {
    // PCA9555: No hardware interrupt status registers.
    // Fall back to change-detection by comparing current vs previous pin states.
    uint16_t current_states = readPinStates();
    interrupt_status = current_states ^ previous_pin_states_;
    // Note: previous_pin_states_ is updated at the end of this method
  }

  // Read current pin states
  uint16_t current_states = readPinStates();

  // Call global callback if registered
  if (irq_callback_) {
    irq_callback_(interrupt_status);
  }

  // Process each pin that triggered an interrupt
  for (uint8_t pin = 0; pin < 16; ++pin) {
    // Check if this pin triggered an interrupt
    if ((interrupt_status & (1U << pin)) == 0) {
      continue;  // Pin didn't trigger interrupt
    }

    // Check if callback is registered for this pin
    if (!pin_callbacks_[pin].registered || !pin_callbacks_[pin].callback) {
      continue;  // No callback registered
    }

    // Determine edge type
    bool previous_state = (previous_pin_states_ & (1U << pin)) != 0;
    bool current_state = (current_states & (1U << pin)) != 0;
    bool rising_edge = !previous_state && current_state;
    bool falling_edge = previous_state && !current_state;

    // Check if edge matches registered condition
    bool should_call = false;
    switch (pin_callbacks_[pin].edge) {
      case InterruptEdge::Rising:
        should_call = rising_edge;
        break;
      case InterruptEdge::Falling:
        should_call = falling_edge;
        break;
      case InterruptEdge::Both:
        should_call = rising_edge || falling_edge;
        break;
    }

    // Call callback if condition matches
    if (should_call) {
      pin_callbacks_[pin].callback(pin, current_state);
    }
  }

  // Update previous states for next interrupt
  previous_pin_states_ = current_states;
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetPinPolarity(uint8_t pin, Polarity polarity) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);
  return modifySinglePinRegister(static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_0),
                                  static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_1),
                                  pin, (polarity == Polarity::Inverted));
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetMultiplePolarities(uint16_t mask, Polarity polarity) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  return modifyDualPortByMask(static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_0),
                               static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_1),
                               mask, (polarity == Polarity::Inverted));
}

// Configure polarity for multiple pins with individual settings
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::SetPolarities(std::initializer_list<std::pair<uint8_t, Polarity>> configs) noexcept {
  if (!EnsureInitialized()) {
    return false;
  }
  uint8_t port0 = 0;
  uint8_t port1 = 0;
  if (!readDualPort(static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_0),
                     static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_1), port0, port1)) {
    return false;
  }
  for (const auto& config : configs) {
    if (config.first >= 16) {
      setError(Error::InvalidPin);
      return false;
    }
    uint8_t bit = config.first % 8;
    if (config.first < 8) {
      port0 = updateBit(port0, bit, (config.second == Polarity::Inverted));
    } else {
      port1 = updateBit(port1, bit, (config.second == Polarity::Inverted));
    }
  }
  clearError(Error::InvalidPin);
  return writeDualPort(static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_0),
                        static_cast<uint8_t>(Pcal95555Reg::POLARITY_INV_1), port0, port1);
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::EnableInputLatch(uint8_t pin, bool enable) noexcept {
  if (!EnsureInitialized() || !requireAgileIO()) {
    return false;
  }
  if (pin >= 16) {
    setError(Error::InvalidPin);
    return false;
  }
  clearError(Error::InvalidPin);
  return modifySinglePinRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_0),
                                  static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_1),
                                  pin, enable);
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::EnableMultipleInputLatches(uint16_t mask, bool enable) noexcept {
  if (!EnsureInitialized() || !requireAgileIO()) {
    return false;
  }
  clearError(Error::InvalidMask);
  return modifyDualPortByMask(static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_0),
                               static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_1),
                               mask, enable);
}

// Configure input latch for multiple pins with individual settings
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::EnableInputLatches(std::initializer_list<std::pair<uint8_t, bool>> configs) noexcept {
  if (!EnsureInitialized() || !requireAgileIO()) {
    return false;
  }
  uint8_t port0 = 0;
  uint8_t port1 = 0;
  if (!readDualPort(static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_0),
                     static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_1), port0, port1)) {
    return false;
  }
  for (const auto& config : configs) {
    if (config.first >= 16) {
      setError(Error::InvalidPin);
      return false;
    }
    uint8_t bit = config.first % 8;
    if (config.first < 8) {
      port0 = updateBit(port0, bit, config.second);
    } else {
      port1 = updateBit(port1, bit, config.second);
    }
  }
  clearError(Error::InvalidPin);
  return writeDualPort(static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_0),
                        static_cast<uint8_t>(Pcal95555Reg::INPUT_LATCH_1), port0, port1);
}

template <typename I2cType>
uint16_t pcal95555::PCAL95555<I2cType>::GetErrorFlags() const noexcept {
  return error_flags_;
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::HasError(Error e) const noexcept {
  return (error_flags_ & static_cast<uint16_t>(e)) != 0;
}

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::HasAnyError() const noexcept {
  return error_flags_ != 0;
}

template <typename I2cType>
void pcal95555::PCAL95555<I2cType>::ClearError(Error e) noexcept {
  error_flags_ &= ~static_cast<uint16_t>(e);
}

template <typename I2cType>
void pcal95555::PCAL95555<I2cType>::ClearErrorFlags(uint16_t mask) noexcept {
  error_flags_ &= ~mask;
}

template <typename I2cType>
void pcal95555::PCAL95555<I2cType>::setError(Error error_code) noexcept {
  error_flags_ |= static_cast<uint16_t>(error_code);
}

template <typename I2cType>
void pcal95555::PCAL95555<I2cType>::clearError(Error error_code) noexcept {
  error_flags_ &= ~static_cast<uint16_t>(error_code);
}

// Check if chip supports Agile I/O (PCAL9555A features)
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::HasAgileIO() const noexcept {
  return chip_variant_ == ChipVariant::PCAL9555A;
}

// Get detected chip variant
template <typename I2cType>
ChipVariant pcal95555::PCAL95555<I2cType>::GetChipVariant() const noexcept {
  return chip_variant_;
}

// Guard helper: require Agile I/O support
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::requireAgileIO() noexcept {
  if (chip_variant_ != ChipVariant::PCAL9555A) {
    setError(Error::UnsupportedFeature);
    return false;
  }
  return true;
}

// Detect chip variant by probing OUTPUT_CONF register (0x4F)
// Uses a 3-step sandwich: standard read -> probe -> standard read
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::detectChipVariant() noexcept {
  // Use single-shot (no retries) for the probe to avoid spamming the bus
  int saved_retries = retries_;
  retries_ = 0;
  chip_variant_ = ChipVariant::Unknown;

  uint8_t dummy = 0;

  // Step 1: Verify basic I2C communication with a standard register
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_0), dummy)) {
    // Bus is not working at all - leave variant as Unknown
    retries_ = saved_retries;
    return false;
  }
  clearError(Error::I2CReadFail);

  // Step 2: Probe an Agile I/O register (OUTPUT_CONF = 0x4F)
  // PCAL9555A will ACK; PCA9555 will NACK (expected — do not treat as hard error)
  uint8_t probe_value = 0;
  if (readRegister(static_cast<uint8_t>(Pcal95555Reg::OUTPUT_CONF), probe_value)) {
    // Probe succeeded - chip has Agile I/O registers

    // Step 3a: Confirm bus is still healthy after successful read
    if (readRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_0), dummy)) {
      chip_variant_ = ChipVariant::PCAL9555A;
      clearError(Error::I2CReadFail);
    } else {
      // Bus broke after successful probe - inconclusive, leave as Unknown
      clearError(Error::I2CReadFail);
    }
  } else {
    // Probe NACKed - expected for standard PCA9555
    clearError(Error::I2CReadFail);

    // Step 3b: Verify the bus recovered after the NACK
    if (readRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_0), dummy)) {
      // Bus healthy after NACK - confidently a PCA9555
      chip_variant_ = ChipVariant::PCA9555;
      clearError(Error::I2CReadFail);
    } else {
      // Bus did not recover - detection inconclusive, leave as Unknown
      clearError(Error::I2CReadFail);
    }
  }

  // Restore retries
  retries_ = saved_retries;
  return chip_variant_ != ChipVariant::Unknown;
}

// Get current I2C address
template <typename I2cType>
uint8_t pcal95555::PCAL95555<I2cType>::GetAddress() const noexcept {
  return dev_addr_;
}

// Get current A2-A0 address bits
template <typename I2cType>
uint8_t pcal95555::PCAL95555<I2cType>::GetAddressBits() const noexcept {
  return address_bits_;
}

// ---- Shared ChangeAddress implementation ----

template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::changeAddressImpl(uint8_t new_bits) noexcept {
  uint8_t new_addr = calculateAddress(new_bits);
  bool a0_level = (new_bits & 0x01) != 0;
  bool a1_level = (new_bits & 0x02) != 0;
  bool a2_level = (new_bits & 0x04) != 0;

  // Try to set GPIO pins via I2C interface (returns false if hardwired)
  bool gpio_set = i2c_->SetAddressPins(a0_level, a1_level, a2_level);

  // Update internal state
  address_bits_ = new_bits;
  dev_addr_ = new_addr;
  a0_level_ = a0_level;
  a1_level_ = a1_level;
  a2_level_ = a2_level;

  // Reset initialization flag since address changed
  initialized_ = false;

  // Verify communication at new address
  uint8_t test_value = 0;
  if (!readRegister(static_cast<uint8_t>(Pcal95555Reg::INPUT_PORT_0), test_value)) {
    setError(Error::I2CReadFail);
    (void)gpio_set; // gpio_set status is irrelevant - communication failed either way
    return false;
  }

  // Communication successful
  clearError(Error::I2CReadFail);
  clearError(Error::InvalidAddress);

  // Re-detect chip variant at new address
  chip_variant_ = ChipVariant::Unknown;
  if (user_variant_ != ChipVariant::Unknown) {
    chip_variant_ = user_variant_;
  } else if (!detectChipVariant()) {
    setError(Error::I2CReadFail);
    return false;
  }

  initialized_ = true;
  return true;
}

// Change address by setting A2-A0 pins via GPIO
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::ChangeAddress(bool a0_level, bool a1_level, bool a2_level) noexcept {
  uint8_t new_bits = (a0_level ? 1U : 0U)
                   | ((a1_level ? 1U : 0U) << 1)
                   | ((a2_level ? 1U : 0U) << 2);
  return changeAddressImpl(static_cast<uint8_t>(new_bits));
}

// Change address using address value directly
template <typename I2cType>
bool pcal95555::PCAL95555<I2cType>::ChangeAddress(uint8_t address) noexcept {
  constexpr uint8_t BASE_ADDRESS = 0x20;
  constexpr uint8_t MAX_ADDRESS = 0x27;

  if (address < BASE_ADDRESS || address > MAX_ADDRESS) {
    setError(Error::InvalidAddress);
    return false;
  }

  uint8_t new_bits = (address - BASE_ADDRESS) & 0x07;
  return changeAddressImpl(new_bits);
}

#endif // PCAL95555_IMPL
