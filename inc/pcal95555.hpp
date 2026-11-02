/**
 * @file pcal95555.hpp
 * @brief Driver for the PCA9555 and PCAL9555A 16-bit I/O expanders with I²C interface
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <utility>
#include <stdio.h> // NOLINT(modernize-deprecated-headers) - For FILE* used by ESP-IDF headers
#include <string.h> // NOLINT(modernize-deprecated-headers) - For C string functions (must be before namespace)

#include "pcal95555_i2c_interface.hpp"

#include "pcal95555_kconfig.hpp"
#include "pcal95555_version.h"

namespace pcal95555 {

/** PCAL95555 register map (all control registers). */
/**
 * @enum Pcal95555Reg
 * @brief Register addresses for the PCA9555 / PCAL9555A I/O expander chips
 *
 * The register map is split into two banks:
 *
 * **Standard PCA9555 registers (0x00-0x07)** - Supported by both PCA9555 and PCAL9555A:
 * - Input/Output registers (0x00-0x03): Read inputs and control outputs
 * - Configuration registers (0x04-0x07): Configure polarity and port direction
 *
 * **PCAL9555A Agile I/O registers (0x40-0x4F)** - PCAL9555A only:
 * - Drive strength registers (0x40-0x43): Control output drive strength
 * - Input/Pull registers (0x44-0x49): Configure input latching and pull-up/down
 * - Interrupt registers (0x4A-0x4D): Manage interrupt mask and status
 * - Output configuration (0x4F): Push-pull vs open-drain per port
 *
 * Each register controls 8 pins, with PORT_0 handling pins 0-7 and
 * PORT_1 handling pins 8-15.
 *
 * @note Accessing Agile I/O registers on a standard PCA9555 will result in
 *       I2C NACK. The driver auto-detects the chip variant and guards these
 *       registers accordingly.
 */
enum class Pcal95555Reg : uint8_t {
  // === Standard PCA9555 registers (0x00-0x07) — PCA9555 + PCAL9555A ===
  INPUT_PORT_0 = 0x00,     ///< Input port 0 (read-only) [PCA9555 + PCAL9555A]
  INPUT_PORT_1 = 0x01,     ///< Input port 1 (read-only) [PCA9555 + PCAL9555A]
  OUTPUT_PORT_0 = 0x02,    ///< Output port 0 [PCA9555 + PCAL9555A]
  OUTPUT_PORT_1 = 0x03,    ///< Output port 1 [PCA9555 + PCAL9555A]
  POLARITY_INV_0 = 0x04,   ///< Polarity inversion port 0 [PCA9555 + PCAL9555A]
  POLARITY_INV_1 = 0x05,   ///< Polarity inversion port 1 [PCA9555 + PCAL9555A]
  CONFIG_PORT_0 = 0x06,    ///< Configuration (direction) port 0 [PCA9555 + PCAL9555A]
  CONFIG_PORT_1 = 0x07,    ///< Configuration (direction) port 1 [PCA9555 + PCAL9555A]

  // === PCAL9555A Agile I/O registers (0x40-0x4F) — PCAL9555A only ===
  DRIVE_STRENGTH_0 = 0x40, ///< Output drive strength port 0 low nibble [PCAL9555A only]
  DRIVE_STRENGTH_1 = 0x41, ///< Output drive strength port 0 high nibble [PCAL9555A only]
  DRIVE_STRENGTH_2 = 0x42, ///< Output drive strength port 1 low nibble [PCAL9555A only]
  DRIVE_STRENGTH_3 = 0x43, ///< Output drive strength port 1 high nibble [PCAL9555A only]
  INPUT_LATCH_0 = 0x44,    ///< Input latch port 0 [PCAL9555A only]
  INPUT_LATCH_1 = 0x45,    ///< Input latch port 1 [PCAL9555A only]
  PULL_ENABLE_0 = 0x46,    ///< Pull-up/pull-down enable port 0 [PCAL9555A only]
  PULL_ENABLE_1 = 0x47,    ///< Pull-up/pull-down enable port 1 [PCAL9555A only]
  PULL_SELECT_0 = 0x48,    ///< Pull-up/pull-down selection port 0 [PCAL9555A only]
  PULL_SELECT_1 = 0x49,    ///< Pull-up/pull-down selection port 1 [PCAL9555A only]
  INT_MASK_0 = 0x4A,       ///< Interrupt mask port 0 [PCAL9555A only]
  INT_MASK_1 = 0x4B,       ///< Interrupt mask port 1 [PCAL9555A only]
  INT_STATUS_0 = 0x4C,     ///< Interrupt status port 0 (read-only) [PCAL9555A only]
  INT_STATUS_1 = 0x4D,     ///< Interrupt status port 1 (read-only) [PCAL9555A only]
  OUTPUT_CONF = 0x4F       ///< Output port configuration [PCAL9555A only]
};

/** GPIO direction (1=input, 0=output). */
enum class GPIODir : uint8_t { Input = 1, Output = 0 };

/** Input polarity inversion (0=normal, 1=inverted). */
enum class Polarity : uint8_t { Normal = 0, Inverted = 1 };

/** Output drive strength levels (00=¼, 11=full). */
enum class DriveStrength : uint8_t { Level0 = 0, Level1 = 1, Level2 = 2, Level3 = 3 };

/** Output mode (push-pull vs open-drain). */
enum class OutputMode : uint8_t { PushPull = 0, OpenDrain = 1 };

/** Interrupt edge trigger type. */
enum class InterruptEdge : uint8_t {
  Rising = 1,   ///< Trigger on rising edge (LOW to HIGH)
  Falling = 2, ///< Trigger on falling edge (HIGH to LOW)
  Both = 3     ///< Trigger on both edges
};

/** Interrupt enable/disable state. */
enum class InterruptState : uint8_t {
  Enabled = 0,  ///< Interrupt enabled (mask bit = 0)
  Disabled = 1  ///< Interrupt disabled/masked (mask bit = 1)
};

/** Error conditions reported by the driver. */
// NOLINTNEXTLINE(performance-enum-size) - uint16_t allows for future error code expansion
enum class Error : uint16_t {
  None = 0,
  InvalidPin = 1 << 0,          ///< Provided pin index out of range
  InvalidMask = 1 << 1,         ///< A mask-form call got a mask that selects no pin (0)
  I2CReadFail = 1 << 2,         ///< An I2C read operation failed
  I2CWriteFail = 1 << 3,        ///< An I2C write operation failed
  UnsupportedFeature = 1 << 4,  ///< Feature requires PCAL9555A but chip is PCA9555
  InvalidAddress = 1 << 5       ///< Provided I2C address is out of valid range (0x20-0x27)
};

/** @brief Combine two Error flags. */
inline constexpr Error operator|(Error lhs, Error rhs) noexcept {
  return static_cast<Error>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
}
/** @brief Intersect two Error flags. */
inline constexpr Error operator&(Error lhs, Error rhs) noexcept {
  return static_cast<Error>(static_cast<uint16_t>(lhs) & static_cast<uint16_t>(rhs));
}
/** @brief Complement an Error flag (useful with ClearErrorFlags). */
inline constexpr Error operator~(Error e) noexcept {
  return static_cast<Error>(~static_cast<uint16_t>(e));
}

/**
 * @brief Fixed-capacity result container for multi-pin read operations.
 *
 * Stores up to 16 pin/value pairs on the stack, avoiding heap allocation.
 * The PCAL95555 has exactly 16 GPIO pins, so 16 is the hard upper bound.
 *
 * Supports range-based for loops and structured bindings:
 * @code
 *   auto results = driver.ReadPins({0, 5, 10});
 *   for (const auto& [pin, value] : results) {
 *       printf("Pin %d: %s\n", pin, value ? "HIGH" : "LOW");
 *   }
 * @endcode
 */
struct PinReadResult {
  /// Maximum number of entries (constrained by hardware: 16 GPIO pins).
  static constexpr uint8_t kMaxPins = 16;

  /// Pin number / value pairs.
  std::array<std::pair<uint8_t, bool>, kMaxPins> data{};

  /// Number of valid entries in data[].
  uint8_t count = 0;

  // -- Container-like API --------------------------------------------------

  /// Number of populated entries.
  [[nodiscard]] uint8_t size() const noexcept { return count; }

  /// True when no entries have been added.
  [[nodiscard]] bool empty() const noexcept { return count == 0; }

  /// Access entry by index (no bounds check).
  [[nodiscard]] const std::pair<uint8_t, bool>& operator[](uint8_t i) const noexcept {
    return data[i];
  }

  /// Append a pin/value pair.  Silently drops entries beyond kMaxPins.
  void push_back(uint8_t pin, bool value) noexcept {
    if (count < kMaxPins) {
      data[count++] = {pin, value};
    }
  }

  // -- Lookup API ----------------------------------------------------------

  /// Check whether a pin number is already stored in the result set.
  [[nodiscard]] bool contains(uint8_t pin) const noexcept {
    for (uint8_t i = 0; i < count; ++i) {
      if (data[i].first == pin) return true;
    }
    return false;
  }

  /// Look up a pin's read result by pin number.
  /// @return Pointer to the {pin, value} pair, or nullptr if not found.
  [[nodiscard]] const std::pair<uint8_t, bool>* find(uint8_t pin) const noexcept {
    for (uint8_t i = 0; i < count; ++i) {
      if (data[i].first == pin) return &data[i];
    }
    return nullptr;
  }

  // -- Iterator support for range-based for --------------------------------
  using iterator       = typename std::array<std::pair<uint8_t, bool>, kMaxPins>::const_iterator;
  [[nodiscard]] iterator begin() const noexcept { return data.begin(); }
  [[nodiscard]] iterator end()   const noexcept { return data.begin() + count; }
};

/**
 * @enum ChipVariant
 * @brief Identifies the detected or user-specified chip variant.
 *
 * The driver supports both the standard PCA9555 and the enhanced PCAL9555A.
 * The variant is auto-detected during initialization by probing an Agile I/O
 * register. It can also be forced via the constructor to skip detection.
 */
enum class ChipVariant : uint8_t {
  Unknown = 0,    ///< Not yet detected (pre-initialization)
  PCA9555 = 1,    ///< Standard PCA9555 (registers 0x00-0x07 only)
  PCAL9555A = 2   ///< NXP PCAL9555A with Agile I/O (registers 0x00-0x07 + 0x40-0x4F)
};

/**
 * @class PCAL95555
 * @brief Driver for the PCA9555 / PCAL9555A / PCAL95555AHF I²C GPIO expander.
 *
 * This driver supports both the standard PCA9555 and the enhanced PCAL9555A.
 * The chip variant is auto-detected during initialization by probing the
 * Agile I/O register bank (0x40-0x4F). Features requiring the extended
 * register set are gracefully disabled when a standard PCA9555 is detected.
 *
 * **Chip Compatibility:**
 * | Feature                    | PCA9555 | PCAL9555A |
 * |----------------------------|---------|-----------|
 * | GPIO direction             | Yes     | Yes       |
 * | Pin read/write/toggle      | Yes     | Yes       |
 * | Polarity inversion         | Yes     | Yes       |
 * | Pull-up/pull-down config   | No      | Yes       |
 * | Drive strength             | No      | Yes       |
 * | Input latch                | No      | Yes       |
 * | Interrupt mask/status      | No      | Yes       |
 * | Output mode (PP/OD)        | No      | Yes       |
 *
 * @tparam I2cType The I2C interface implementation type that inherits from
 * pcal95555::I2cInterface<I2cType>
 *
 * @note The driver uses CRTP-based I2C interface for zero virtual call overhead.
 * @note Address is calculated internally from A2-A0 pins. The base address is 0x20.
 * @note The driver uses lazy initialization - no initialization happens in the constructor.
 *       Call EnsureInitialized() explicitly or let it initialize automatically on first use.
 *       Initialization includes setting address pins, verifying I2C communication, and
 *       auto-detecting the chip variant.
 * @note Use HasAgileIO() or GetChipVariant() to query which features are available.
 */
template <typename I2cType>
class PCAL95555 {
public:
  /**
   * @brief Construct a new PCAL95555 driver instance using address pin levels.
   *
   * @param bus Pointer to a user-implemented I2C interface (must inherit from
   * pcal95555::I2cInterface<I2cType>).
   * @param a0_level Initial state of A0 pin: true = HIGH (VDD), false = LOW (GND)
   * @param a1_level Initial state of A1 pin: true = HIGH (VDD), false = LOW (GND)
   * @param a2_level Initial state of A2 pin: true = HIGH (VDD), false = LOW (GND)
   * @param variant Optional chip variant override. If ChipVariant::Unknown (default),
   *                the driver auto-detects during initialization. Set to
   *                ChipVariant::PCA9555 or ChipVariant::PCAL9555A to skip detection.
   *
   * The constructor uses lazy initialization:
   * 1. Calculates the I2C address from A2-A0 levels (base 0x20 + bits)
   * 2. Stores pin levels for later initialization
   * 3. No I2C communication happens in constructor
   *
   * Initialization happens automatically on first method call, or explicitly
   * via EnsureInitialized(). Initialization includes setting address pins,
   * verifying I2C communication, and auto-detecting the chip variant.
   *
   * @example
   *   // Auto-detect chip variant (default)
   *   PCAL95555 driver(bus, false, false, false);
   *
   *   // Force PCA9555 mode (skip Agile I/O detection)
   *   PCAL95555 driver(bus, false, false, false, ChipVariant::PCA9555);
   *
   *   // Force PCAL9555A mode
   *   PCAL95555 driver(bus, true, false, false, ChipVariant::PCAL9555A);
   */
  PCAL95555(I2cType* bus, bool a0_level, bool a1_level, bool a2_level,
            ChipVariant variant = ChipVariant::Unknown);

  /**
   * @brief Construct a new PCAL95555 driver instance using I2C address directly.
   *
   * @param bus Pointer to a user-implemented I2C interface (must inherit from
   * pcal95555::I2cInterface<I2cType>).
   * @param address 7-bit I2C address (0x20 to 0x27). Address bits are calculated
   *                automatically: address_bits = address - 0x20
   * @param variant Optional chip variant override. If ChipVariant::Unknown (default),
   *                the driver auto-detects during initialization. Set to
   *                ChipVariant::PCA9555 or ChipVariant::PCAL9555A to skip detection.
   *
   * The constructor uses lazy initialization:
   * 1. Calculates A2-A0 pin levels from the address (address - 0x20)
   * 2. Stores pin levels for later initialization
   * 3. No I2C communication happens in constructor
   * 
   * Initialization happens automatically on first method call, or explicitly via EnsureInitialized().
   * Initialization includes setting address pins, verifying I2C communication, and
   * auto-detecting the chip variant.
   *
   * @note If address is out of range (not 0x20-0x27), the address will be clamped
   *       and an error flag may be set.
   *
   * @example
   *   // Address 0x20, auto-detect variant (default)
   *   PCAL95555 driver(bus, 0x20);
   *
   *   // Address 0x21, force PCA9555 mode
   *   PCAL95555 driver(bus, 0x21, ChipVariant::PCA9555);
   */
  explicit PCAL95555(I2cType* bus, uint8_t address,
                     ChipVariant variant = ChipVariant::Unknown);

  // ---- Version Information (compile-time, static) ----

  /**
   * @brief Get the compiled driver version as a string.
   *
   * Returns the version that was compiled into the binary, in
   * "MAJOR.MINOR.PATCH" format. This is a static method — it can be
   * called without a driver instance.
   *
   * @return Null-terminated version string, e.g. "1.0.0".
   *
   * @example
   *   ESP_LOGI("APP", "PCAL95555 driver v%s", PCAL95555::GetDriverVersion());
   */
  static constexpr const char* GetDriverVersion() noexcept {
    return HF_PCAL95555_VERSION_STRING;
  }

  /**
   * @brief Get the compiled driver major version number.
   * @return Major version (incremented on breaking API changes).
   */
  static constexpr uint8_t GetDriverVersionMajor() noexcept {
    return HF_PCAL95555_VERSION_MAJOR;
  }

  /**
   * @brief Get the compiled driver minor version number.
   * @return Minor version (incremented on new features, backward-compatible).
   */
  static constexpr uint8_t GetDriverVersionMinor() noexcept {
    return HF_PCAL95555_VERSION_MINOR;
  }

  /**
   * @brief Get the compiled driver patch version number.
   * @return Patch version (incremented on bug fixes).
   */
  static constexpr uint8_t GetDriverVersionPatch() noexcept {
    return HF_PCAL95555_VERSION_PATCH;
  }

  /**
   * @brief Configure retry mechanism for I2C transactions.
   *
   * @param retries Maximum number of retry attempts on failure.
   *                Note: setting N will result in N+1 total attempts per
   * transfer.
   */
  void SetRetries(int retries) noexcept;

  /**
   * @brief Retrieve currently latched error flags.
   *
   * @return Bitmask composed of @ref Error values.
   */
  [[nodiscard]] uint16_t GetErrorFlags() const noexcept;

  /**
   * @brief Check if a specific error flag is set.
   *
   * @param e The error flag to test.
   * @return true if the flag is set in the current error bitmask.
   *
   * @code
   *   if (driver.HasError(Error::I2CReadFail)) { ... }
   * @endcode
   */
  [[nodiscard]] bool HasError(Error e) const noexcept;

  /**
   * @brief Check if any error flag is set.
   *
   * @return true if error_flags_ != 0.
   */
  [[nodiscard]] bool HasAnyError() const noexcept;

  /**
   * @brief Clear a single error flag.
   *
   * @param e The error flag to clear.
   */
  void ClearError(Error e) noexcept;

  /**
   * @brief Clear specific error flags by raw bitmask.
   *
   * @param mask Bitmask of errors to clear (default: all).
   */
  void ClearErrorFlags(uint16_t mask = 0xFFFF) noexcept;

  /**
   * @brief Reset all registers to their power-on default state.
   *
   * @details Available registers are set to inputs, pull-ups enabled,
   * interrupts masked, drive strength set to full, and output mode
   * configured as push-pull as specified by the datasheet.
   */
  void ResetToDefault() noexcept;

  /**
   * @brief Initialize the device using values from Kconfig.
   *
   * @details This writes the configuration registers using the
   * CONFIG_PCAL95555_INIT_* options defined at compile time.
   */
  void InitFromConfig() noexcept;

  /**
   * @brief Set the direction of a single GPIO pin.
   *
   * @param pin Zero-based pin index (0-15).
   * @param dir Direction enum: Input or Output.
   * @return true on success; false on I2C failure.
   */
  bool SetPinDirection(uint8_t pin, GPIODir dir) noexcept;

  /**
   * @brief Set the direction for multiple GPIO pins at once using a bitmask.
   *
   * @param mask Bitmask where each set bit indicates a pin to configure.
   * @param dir Common direction for all selected pins.
   * @return true on success; false if any I2C operation fails; false with
   *         Error::InvalidMask (and no bus traffic) when @p mask is 0.
   *
   * @note This is a low-level method. For easier use with individual pin directions,
   *       prefer SetDirections() which works with pin numbers directly.
   */
  bool SetMultipleDirections(uint16_t mask, GPIODir dir) noexcept;

  /**
   * @brief Configure direction for multiple pins at once with individual settings.
   *
   * @param configs Initializer list of pin/direction pairs: {{pin1, dir1}, {pin2, dir2}, ...}
   * @return true if all pins configured successfully; false on any failure.
   *
   * @example
   *   // Set pin 0 as output, pin 5 as input, pin 10 as output
   *   driver.SetDirections({
   *       {0, GPIODir::Output},
   *       {5, GPIODir::Input},
   *       {10, GPIODir::Output}
   *   });
   */
  bool SetDirections(std::initializer_list<std::pair<uint8_t, GPIODir>> configs) noexcept;

  /**
   * @brief Read the current logical level of a GPIO pin.
   *
   * @param pin Zero-based pin index (0-15).
   * @return true if pin is high; false if pin is low or on read error.
   *
   * @note A low pin and a failed read both return false. Use
   *       ReadPin(uint8_t, bool&) when the difference matters.
   */
  bool ReadPin(uint8_t pin) noexcept;

  /**
   * @brief Read the logical level of a GPIO pin and report whether the read worked.
   *
   * @param pin   Zero-based pin index (0-15).
   * @param level Receives the level (true = high) on success; unchanged on failure.
   * @return true if the Input Port register was read; false on an invalid pin
   *         (Error::InvalidPin) or an I2C failure (Error::I2CReadFail).
   *
   * @code
   *   bool level = false;
   *   if (!driver.ReadPin(8, level)) { handle_bus_error(); }
   * @endcode
   */
  bool ReadPin(uint8_t pin, bool& level) noexcept;

  /**
   * @brief Write a logical level to a GPIO output pin.
   *
   * @param pin Zero-based pin index (0-15).
   * @param value true to drive high, false to drive low.
   * @return true if write succeeded; false on I2C failure.
   */
  bool WritePin(uint8_t pin, bool value) noexcept;

  /**
   * @brief Set the output level for multiple GPIO pins at once using a bitmask.
   *
   * Reads both output port registers, modifies the bits indicated by @p mask to
   * the specified @p value, and writes both registers back. This is significantly
   * more efficient than calling WritePin() in a loop (2 reads + 2 writes vs.
   * 2 reads + 2 writes per pin).
   *
   * @param mask  Bitmask where each set bit indicates a pin to modify.
   * @param value Common output level for all selected pins (true=HIGH, false=LOW).
   * @return true on success; false if any I2C operation fails; false with
   *         Error::InvalidMask (and no bus traffic) when @p mask is 0.
   *
   * @example
   *   // Set pins 0, 2, 4 HIGH in one call
   *   driver.SetMultipleOutputs(0x0015, true);
   *
   *   // Clear pins 8-15
   *   driver.SetMultipleOutputs(0xFF00, false);
   */
  bool SetMultipleOutputs(uint16_t mask, bool value) noexcept;

  /**
   * @brief Toggle the output state of a GPIO pin.
   *
   * @param pin Zero-based pin index (0-15).
   * @return true on success; false on I2C failure.
   */
  bool TogglePin(uint8_t pin) noexcept;

  /**
   * @brief Write values to multiple GPIO output pins at once.
   *
   * @param configs Initializer list of pin/value pairs: {{pin1, value1}, {pin2, value2}, ...}
   * @return true if all pins written successfully; false on any failure.
   *
   * @example
   *   // Set pin 0 HIGH, pin 5 LOW, pin 10 HIGH
   *   driver.WritePins({
   *       {0, true},
   *       {5, false},
   *       {10, true}
   *   });
   */
  bool WritePins(std::initializer_list<std::pair<uint8_t, bool>> configs) noexcept;

  /**
   * @brief Read values from multiple GPIO input pins at once.
   *
   * Reads both input port registers in a single I2C burst, then extracts
   * the requested pin values.  The result is a stack-allocated
   * @ref PinReadResult (max 16 entries) — no heap allocation occurs.
   *
   * @param pins Initializer list of pin numbers to read: {pin1, pin2, pin3, ...}
   * @return PinReadResult containing pin/value pairs.  Empty on I2C failure.
   *         Pins outside 0-15 are included with value=false.
   *
   * @example
   *   // Read pins 0, 5, and 10
   *   auto results = driver.ReadPins({0, 5, 10});
   *   for (const auto& [pin, value] : results) {
   *       printf("Pin %d: %s\n", pin, value ? "HIGH" : "LOW");
   *   }
   */
  PinReadResult ReadPins(std::initializer_list<uint8_t> pins) noexcept;

  /**
   * @brief Read all 16 pin input states in a single operation.
   *
   * Reads both INPUT_PORT_0 and INPUT_PORT_1 registers in two I2C transactions
   * and returns a 16-bit mask where bit N represents the state of pin N.
   *
   * @return 16-bit mask with current input states (bit N = pin N level).
   *         Returns 0 on I2C failure (check GetErrorFlags() to distinguish
   *         all-low from error).
   *
   * @example
   *   uint16_t all_states = driver.ReadAllInputs();
   *   bool pin5_high = (all_states >> 5) & 1;
   */
  uint16_t ReadAllInputs() noexcept;

  /**
   * @brief Read a dual-port register pair into a 16-bit pin mask.
   *
   * @param reg0 Port-0 register (bits 0..7).
   * @param reg1 Port-1 register (bits 8..15).
   * @param[out] value Bit N corresponds to pin N.
   * @return true on success; false on I2C failure.
   */
  bool ReadDualPortRegister(uint8_t reg0, uint8_t reg1, uint16_t& value) noexcept;

  /**
   * @brief Write a dual-port register pair from a 16-bit pin mask (no readback).
   *
   * Prefer this over read-modify-write when the caller maintains a shadow of
   * the port (OUTPUT / CONFIG / POLARITY). Absolute writes avoid corrupting
   * latches if a prior register read returned the wrong bank.
   *
   * @param reg0 Port-0 register (bits 0..7).
   * @param reg1 Port-1 register (bits 8..15).
   * @param value Bit N corresponds to pin N.
   * @return true on success; false on I2C failure.
   */
  bool WriteDualPortRegister(uint8_t reg0, uint8_t reg1, uint16_t value) noexcept;

  /**
   * @brief Enable or disable the pull-up/pull-down resistor on a pin.
   *
   * @param pin Zero-based pin index (0-15).
   * @param enable true to enable; false to disable.
   * @return true on success; false on I2C failure.
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool SetPullEnable(uint8_t pin, bool enable) noexcept;

  /**
   * @brief Configure pull resistor enable/disable for multiple pins at once.
   *
   * @param configs Initializer list of pin/enable pairs: {{pin1, enable1}, {pin2, enable2}, ...}
   * @return true if all pins configured successfully; false on any failure.
   *
   * @example
   *   // Enable pull on pins 0 and 5, disable on pin 3
   *   driver.SetPullEnables({
   *       {0, true},
   *       {5, true},
   *       {3, false}
   *   });
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool SetPullEnables(std::initializer_list<std::pair<uint8_t, bool>> configs) noexcept;

  /**
   * @brief Select internal pull-up or pull-down resistor direction.
   *
   * @param pin Zero-based pin index (0-15).
   * @param pull_up true for pull-up; false for pull-down.
   * @return true on success; false on I2C failure.
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool SetPullDirection(uint8_t pin, bool pull_up) noexcept;

  /**
   * @brief Configure pull resistor direction for multiple pins at once.
   *
   * @param configs Initializer list of pin/pull_up pairs: {{pin1, pull_up1}, {pin2, pull_up2}, ...}
   * @return true if all pins configured successfully; false on any failure.
   *
   * @example
   *   // Set pin 0 to pull-up, pin 5 to pull-down, pin 10 to pull-up
   *   driver.SetPullDirections({
   *       {0, true},   // pull-up
   *       {5, false},  // pull-down
   *       {10, true}   // pull-up
   *   });
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool SetPullDirections(std::initializer_list<std::pair<uint8_t, bool>> configs) noexcept;

  /**
   * @brief Read the current pull resistor configuration from hardware registers.
   *
   * Reads the PULL_ENABLE (0x46-0x47) and PULL_SELECT (0x48-0x49) registers
   * and returns them as 16-bit bitmasks (bit N corresponds to pin N).
   *
   * @param[out] enable_mask  Bitmask: bit set = pull resistor enabled on that pin.
   * @param[out] direction_mask Bitmask: bit set = pull-up, bit clear = pull-down.
   *                            Only meaningful for pins where the corresponding
   *                            enable_mask bit is set.
   * @return true on success; false on I2C failure.
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool GetPullConfiguration(uint16_t& enable_mask, uint16_t& direction_mask) noexcept;

  /**
   * @brief Configure the output drive strength for a GPIO pin.
   *
   * @param pin Zero-based pin index (0-15).
   * @param level Drive strength level (Level0..Level3).
   * @return true on success; false on I2C failure.
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool SetDriveStrength(uint8_t pin, DriveStrength level) noexcept;

  /**
   * @brief Configure drive strength for multiple pins at once.
   *
   * @param configs Initializer list of pin/level pairs: {{pin1, level1}, {pin2, level2}, ...}
   * @return true if all pins configured successfully; false on any failure.
   *
   * @example
   *   // Set pin 0 to Level3 (full), pin 5 to Level1 (50%), pin 10 to Level2 (75%)
   *   driver.SetDriveStrengths({
   *       {0, DriveStrength::Level3},
   *       {5, DriveStrength::Level1},
   *       {10, DriveStrength::Level2}
   *   });
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool SetDriveStrengths(std::initializer_list<std::pair<uint8_t, DriveStrength>> configs) noexcept;

  /**
   * @brief Enable or disable interrupt on a single pin.
   *
   * @param pin Zero-based pin index (0-15).
   * @param state InterruptState::Enabled or InterruptState::Disabled.
   * @return true on success; false on I2C failure or invalid pin.
   *
   * @example
   *   // Enable interrupt on pin 5
   *   driver.ConfigureInterrupt(5, InterruptState::Enabled);
   *
   *   // Disable interrupt on pin 3
   *   driver.ConfigureInterrupt(3, InterruptState::Disabled);
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool ConfigureInterrupt(uint8_t pin, InterruptState state) noexcept;

  /**
   * @brief Configure interrupts for multiple pins at once.
   *
   * @param configs Initializer list of pin/state pairs: {{pin1, state1}, {pin2, state2}, ...}
   * @return true if all pins configured successfully; false on any failure.
   *
   * @example
   *   // Enable interrupts on pins 0, 5, and 10
   *   driver.ConfigureInterrupts({
   *       {0, InterruptState::Enabled},
   *       {5, InterruptState::Enabled},
   *       {10, InterruptState::Enabled}
   *   });
   *
   *   // Mixed enable/disable
   *   driver.ConfigureInterrupts({
   *       {0, InterruptState::Enabled},
   *       {1, InterruptState::Disabled},
   *       {2, InterruptState::Enabled}
   *   });
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool ConfigureInterrupts(std::initializer_list<std::pair<uint8_t, InterruptState>> configs) noexcept;

  /**
   * @brief Enable or disable interrupts on multiple pins using a bitmask.
   *
   * @param mask Bitmask where 0 enables interrupt, 1 disables per pin.
   *             Bit N corresponds to pin N (0-15).
   * @return true on success; false on I2C failure.
   *
   * @note This is a low-level method. For easier use, prefer ConfigureInterrupt() or
   *       ConfigureInterrupts() which work with pin numbers directly.
   *
   * @example
   *   // Enable interrupts on pins 0, 2, and 5 (bits 0, 2, 5 = 0)
   *   // Mask: ~(1<<0 | 1<<2 | 1<<5) = 0xFFD9
   *   driver.ConfigureInterruptMask(0xFFD9);
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool ConfigureInterruptMask(uint16_t mask) noexcept;
  /**
   * @brief Retrieve and clear the interrupt status.
   *
   * @return 16-bit mask indicating which pins triggered an interrupt.
   *         Returns 0 on PCA9555 (interrupt status registers not available).
   * @note Requires PCAL9555A. Returns 0 with Error::UnsupportedFeature on PCA9555.
   */
  uint16_t GetInterruptStatus() noexcept;

  /**
   * @brief Configure per-port output mode (push-pull or open-drain).
   *
   * @param port_0_open_drain true for open-drain on port 0.
   * @param port_1_open_drain true for open-drain on port 1.
   * @return true on success; false on I2C failure.
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool SetOutputMode(bool port_0_open_drain, bool port_1_open_drain) noexcept;

  /**
   * @brief Configure input polarity inversion for a single pin.
   *
   * @param pin Zero-based pin index (0-15).
   * @param polarity Polarity::Normal or Polarity::Inverted.
   * @return true on success; false on I2C failure.
   */
  bool SetPinPolarity(uint8_t pin, Polarity polarity) noexcept;

  /**
   * @brief Configure input polarity for multiple pins using a bitmask.
   *
   * @param mask Bitmask selecting pins to configure.
   * @param polarity Normal or Inverted for all selected pins.
   * @return true on success; false on any I2C failure; false with
   *         Error::InvalidMask (and no bus traffic) when @p mask is 0.
   *
   * @note This is a low-level method. For easier use with individual pin polarities,
   *       prefer SetPolarities() which works with pin numbers directly.
   */
  bool SetMultiplePolarities(uint16_t mask, Polarity polarity) noexcept;

  /**
   * @brief Configure polarity for multiple pins at once with individual settings.
   *
   * @param configs Initializer list of pin/polarity pairs: {{pin1, pol1}, {pin2, pol2}, ...}
   * @return true if all pins configured successfully; false on any failure.
   *
   * @example
   *   // Set pin 0 to normal, pin 5 to inverted, pin 10 to normal
   *   driver.SetPolarities({
   *       {0, Polarity::Normal},
   *       {5, Polarity::Inverted},
   *       {10, Polarity::Normal}
   *   });
   */
  bool SetPolarities(std::initializer_list<std::pair<uint8_t, Polarity>> configs) noexcept;

  /**
   * @brief Enable or disable the input latch for a single pin.
   *
   * @param pin Zero-based pin index (0-15).
   * @param enable true to enable latch; false to disable.
   * @return true on success; false on I2C failure.
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool EnableInputLatch(uint8_t pin, bool enable) noexcept;

  /**
   * @brief Enable or disable input latch for multiple pins using a bitmask.
   *
   * @param mask Bitmask selecting pins.
   * @param enable true to enable; false to disable.
   * @return true on success; false on any I2C failure; false with
   *         Error::InvalidMask (and no bus traffic) when @p mask is 0.
   *
   * @note This is a low-level method. For easier use with individual pin settings,
   *       prefer EnableInputLatches() which works with pin numbers directly.
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool EnableMultipleInputLatches(uint16_t mask, bool enable) noexcept;

  /**
   * @brief Configure input latch for multiple pins at once with individual settings.
   *
   * @param configs Initializer list of pin/enable pairs: {{pin1, enable1}, {pin2, enable2}, ...}
   * @return true if all pins configured successfully; false on any failure.
   *
   * @example
   *   // Enable latch on pins 0 and 5, disable on pin 3
   *   driver.EnableInputLatches({
   *       {0, true},
   *       {5, true},
   *       {3, false}
   *   });
   * @note Requires PCAL9555A. Returns false with Error::UnsupportedFeature on PCA9555.
   */
  bool EnableInputLatches(std::initializer_list<std::pair<uint8_t, bool>> configs) noexcept;

  /**
   * @brief Register a callback for a specific pin interrupt.
   *
   * This method registers a callback function that will be called when the specified
   * pin triggers an interrupt matching the specified edge condition. The callback
   * receives the pin number and current pin state.
   *
   * @param pin Pin number (0-15) to register callback for.
   * @param edge Edge type to trigger on (Rising, Falling, or Both).
   * @param callback Function to call when interrupt occurs. Receives pin number and current state.
   * @return true if callback was registered successfully; false on error.
   *
   * @note The pin must be configured as an input for interrupts to work.
   * @note Interrupts must be enabled for the pin via ConfigureInterrupt() or ConfigureInterruptMask().
   * @note Only one callback per pin is supported. Registering a new callback replaces the old one.
   *
   * @example
   *   // Register callback for pin 5 on rising edge
   *   driver.RegisterPinInterrupt(5, InterruptEdge::Rising, [](uint8_t pin, bool state) {
   *       ESP_LOGI("APP", "Pin %d went HIGH", pin);
   *   });
   *
   *   // Register callback for pin 3 on both edges
   *   driver.RegisterPinInterrupt(3, InterruptEdge::Both, [](uint8_t pin, bool state) {
   *       ESP_LOGI("APP", "Pin %d changed to %s", pin, state ? "HIGH" : "LOW");
   *   });
   */
  bool RegisterPinInterrupt(uint8_t pin, InterruptEdge edge,
                            std::function<void(uint8_t pin, bool state)> callback);

  /**
   * @brief Unregister callback for a specific pin interrupt.
   *
   * @param pin Pin number (0-15) to unregister callback for.
   * @return true if callback was unregistered; false if pin was invalid or had no callback.
   */
  bool UnregisterPinInterrupt(uint8_t pin) noexcept;

  /**
   * @brief Register a global callback to be invoked on any GPIO interrupt.
   *
   * This is a convenience method that provides a single callback for all interrupts.
   * The callback receives a 16-bit mask indicating which pins triggered interrupts.
   *
   * @param callback Function taking a 16-bit status mask for active interrupts.
   *                 Bit N set indicates pin N triggered an interrupt.
   *
   * @note This works alongside per-pin callbacks. Both will be called.
   * @note Only one global callback is supported. Registering a new one replaces the old.
   *
   * @example
   *   driver.SetInterruptCallback([](uint16_t status) {
   *       ESP_LOGI("APP", "Interrupt! Pins: 0x%04X", status);
   *   });
   */
  void SetInterruptCallback(const std::function<void(uint16_t)>& callback) noexcept;

  /**
   * @brief Register this driver's interrupt handler with the I2C interface.
   *
   * This method registers the driver's HandleInterrupt() method with the I2C
   * interface so that it will be called when the INT pin fires. The I2C interface
   * is responsible for setting up GPIO interrupt handling for the INT pin.
   *
   * @return true if interrupt handler was registered successfully; false if
   *         the I2C interface doesn't support interrupt handling or on error.
   *
   * @note This should be called after creating the driver instance and configuring
   *       interrupt masks. The I2C interface implementation will handle GPIO setup.
   *
   * @note If the I2C interface doesn't support interrupt handling (returns false),
   *       you can still poll GetInterruptStatus() manually.
   *
   * @example
   *   auto driver = std::make_unique<PCAL95555Driver>(bus.get(), false, false, false);
   *   driver->SetPinDirection(5, GPIODir::Input);
   *   driver->ConfigureInterrupt(5, InterruptState::Enabled);  // Enable interrupt on pin 5
   *   driver->RegisterPinInterrupt(5, InterruptEdge::Rising, [](uint8_t pin, bool state) {
   *       ESP_LOGI("APP", "Pin %d interrupt!", pin);
   *   });
   *   driver->RegisterInterruptHandler();  // Register with I2C interface
   */
  bool RegisterInterruptHandler() noexcept;

  /**
   * @brief Internal handler to process an interrupt event.
   *
   * @details This method is called by the I2C interface when the INT pin fires.
   * It reads the interrupt status registers, determines which pins triggered
   * interrupts, checks edge conditions, and invokes registered callbacks.
   *
   * Reading the interrupt status registers clears the interrupt condition.
   */
  void HandleInterrupt() noexcept;

  /**
   * @brief Get the current I2C address of the device.
   *
   * @return 7-bit I2C address (0x20 to 0x27).
   */
  [[nodiscard]] uint8_t GetAddress() const noexcept;

  /**
   * @brief Get the current A2-A0 address bits.
   *
   * @return 3-bit value (0-7) representing A2, A1, A0 pin configuration.
   */
  [[nodiscard]] uint8_t GetAddressBits() const noexcept;

  /**
   * @brief Check if the detected chip supports Agile I/O (PCAL9555A features).
   *
   * @return true if chip is PCAL9555A with extended register support;
   *         false if chip is standard PCA9555 or variant is not yet detected.
   *
   * @note Call after EnsureInitialized() for accurate results.
   *
   * @example
   *   if (driver.HasAgileIO()) {
   *       driver.SetDriveStrength(0, DriveStrength::Level2);
   *       driver.SetPullEnable(5, true);
   *   }
   */
  [[nodiscard]] bool HasAgileIO() const noexcept;

  /**
   * @brief Get the detected chip variant.
   *
   * @return ChipVariant::PCA9555, ChipVariant::PCAL9555A, or
   *         ChipVariant::Unknown if not yet initialized.
   *
   * @note Call after EnsureInitialized() for accurate results.
   */
  [[nodiscard]] ChipVariant GetChipVariant() const noexcept;

  /**
   * @brief Change the I2C address by setting A2-A0 pins.
   *
   * This method attempts to change the device address by controlling the A2-A0
   * address pins via the I2C interface's SetAddressPins() method. If the I2C interface
   * doesn't support GPIO control (SetAddressPins() returns false), this method will
   * still update the internal address state but won't change hardware pins.
   * Communication is verified after setting pins.
   *
   * @param a0_level Desired state of A0 pin: true = HIGH (VDD), false = LOW (GND)
   * @param a1_level Desired state of A1 pin: true = HIGH (VDD), false = LOW (GND)
   * @param a2_level Desired state of A2 pin: true = HIGH (VDD), false = LOW (GND)
   * @return true if address was changed successfully and communication verified;
   *         false if GPIO control failed or communication verification failed.
   *
   * @note This requires hardware support for controlling A2-A0 pins via GPIO.
   *       If pins are hardwired, SetAddressPins() returns false but the method
   *       will still verify communication at the new address.
   *
   * @example
   *   // Change address to 0x21 (A2=LOW, A1=LOW, A0=HIGH)
   *   if (driver.ChangeAddress(true, false, false)) {
   *       ESP_LOGI("APP", "Address changed to 0x%02X", driver.GetAddress());
   *   }
   */
  bool ChangeAddress(bool a0_level, bool a1_level, bool a2_level) noexcept;

  /**
   * @brief Change the I2C address using address value directly.
   *
   * This method attempts to change the device address by controlling the A2-A0
   * address pins via the I2C interface's SetAddressPins() method. The address bits
   * are calculated automatically from the address: address_bits = address - 0x20.
   *
   * @param address 7-bit I2C address (0x20 to 0x27). Address bits are calculated
   *                automatically: address_bits = address - 0x20
   * @return true if address was changed successfully and communication verified;
   *         false if address is out of range, GPIO control failed, or communication
   *         verification failed.
   *
   * @note This requires hardware support for controlling A2-A0 pins via GPIO.
   *       If pins are hardwired, SetAddressPins() returns false but the method
   *       will still verify communication at the new address.
   * @note If address is out of range (not 0x20-0x27), the method will return false
   *       and set an error flag.
   *
   * @example
   *   // Change address to 0x21
   *   if (driver.ChangeAddress(0x21)) {
   *       ESP_LOGI("APP", "Address changed to 0x%02X", driver.GetAddress());
   *   }
   *
   *   // Change address to 0x25
   *   if (driver.ChangeAddress(0x25)) {
   *       ESP_LOGI("APP", "Address changed to 0x%02X", driver.GetAddress());
   *   }
   */
  bool ChangeAddress(uint8_t address) noexcept;

  /**
   * @brief Ensure the driver is initialized before use.
   *
   * This method performs lazy initialization - it initializes the driver on first
   * call and returns true on subsequent calls if already initialized. Initialization
   * includes setting address pins, verifying I2C communication, and initializing
   * internal state.
   *
   * @return true if initialization succeeded or was already initialized;
   *         false if initialization failed: the device did not answer, or
   *         auto-detection could not tell a PCA9555 from a PCAL9555A (the bus
   *         did not answer the read after the probe). Both set
   *         Error::I2CReadFail; the next call tries again.
   *
   * @note This method is automatically called by methods that require initialization.
   *       You can also call it explicitly to verify initialization before use.
   *
   * @example
   *   PCAL95555 driver(bus, 0x20);
   *   if (!driver.EnsureInitialized()) {
   *       ESP_LOGE("APP", "Failed to initialize driver");
   *       return;
   *   }
   *   // Driver is now ready to use
   */
  bool EnsureInitialized() noexcept;

  /**
   * @brief Mark the driver initialized without the probe read.
   *
   * Use only when the bus/adapter path has already proven INPUT_PORT access
   * (e.g. bring-up) and @c EnsureInitialized()'s first @c readRegister is known
   * to misbehave on the target master. Sets @p variant as both user and
   * detected chip type and seeds previous pin state to 0.
   *
   * @note On some STM32H7 masters, repeating the driver probe (W 0x35) after
   *       INPUT_PORT has already succeeded can leave the controller sticky and
   *       block CONFIG/OUTPUT programming. Callers that already proved the bus
   *       may use this path, then program ports via absolute writes + shadows.
   *       Prefer fixing the bus path long-term over expanding callers.
   *
   * @param variant PCA9555 or PCAL9555A (not Unknown).
   * @return true if @p variant is usable; false if Unknown.
   */
  bool ForceMarkInitialized(ChipVariant variant) noexcept;

protected:
  /**
   * @brief Read a device register with retry logic.
   *
   * @param reg Register address to read.
   * @param value Reference to store the read byte.
   * @return true if read succeeds; false on failure.
   */
  bool readRegister(uint8_t reg, uint8_t& value) noexcept;
  /**
   * @brief Write a single byte to a device register with retry logic.
   *
   * @param reg Register address to write.
   * @param value Byte value to write.
   * @return true if write succeeds; false on failure.
   */
  bool writeRegister(uint8_t reg, uint8_t value) noexcept;

private:
  /**
   * @brief Structure to store pin interrupt callback information.
   */
  struct PinInterruptCallback {
    std::function<void(uint8_t pin, bool state)> callback;
    InterruptEdge edge;
    bool registered;

    PinInterruptCallback() : edge(InterruptEdge::Both), registered(false) {}
  };

  I2cType* i2c_;
  uint8_t dev_addr_;
  uint8_t address_bits_;  // A2-A0 bits (0-7)
  int retries_{1};
  uint16_t error_flags_{0};
  std::function<void(uint16_t)> irq_callback_;  // Global callback for all interrupts
  PinInterruptCallback pin_callbacks_[16];      // Per-pin callbacks
  uint16_t previous_pin_states_{0};            // Previous pin states for edge detection
  bool initialized_{false};                    // Lazy initialization flag
  bool a0_level_;                              // Stored pin levels for lazy init
  bool a1_level_;
  bool a2_level_;
  ChipVariant chip_variant_{ChipVariant::Unknown};  // Detected or user-specified chip variant
  ChipVariant user_variant_{ChipVariant::Unknown};  // User-requested variant (for skipping detection)
  uint8_t xfer_byte_{0};  // Bus transfer byte: lives in the driver object, one per chip

  /**
   * @brief Calculate I2C address from A2-A0 bits.
   *
   * @param bits 3-bit value (0-7) representing A2, A1, A0 pins.
   * @return 7-bit I2C address (0x20 to 0x27).
   */
  static constexpr uint8_t calculateAddress(uint8_t bits) {
    constexpr uint8_t BASE_ADDRESS = 0x20;
    constexpr uint8_t MAX_BITS = 0x07;
    return BASE_ADDRESS + (bits & MAX_BITS);
  }

  /**
   * @brief Read current pin states from input port registers.
   *
   * @return 16-bit mask with current pin states (bit N = pin N state).
   */
  uint16_t readPinStates() noexcept;

  /**
   * @brief Perform actual initialization of the driver.
   *
   * This private method contains all initialization logic including:
   * - Setting address pins via I2C interface
   * - Verifying I2C bus is ready (calls i2c_->EnsureInitialized() if available)
   * - Verifying communication with device
   * - Initializing internal state
   *
   * @return true if initialization succeeded; false on failure.
   */
  bool initialize() noexcept;

  void setError(Error error_code) noexcept;
  void clearError(Error error_code) noexcept;

  // ---- Internal R-M-W helpers (reduce code duplication) ----

  /**
   * @brief Read both port registers (port0 and port1) in one call.
   */
  bool readDualPort(uint8_t reg0, uint8_t reg1, uint8_t& val0, uint8_t& val1) noexcept;

  /**
   * @brief Write both port registers (port0 and port1) in one call.
   */
  bool writeDualPort(uint8_t reg0, uint8_t reg1, uint8_t val0, uint8_t val1) noexcept;

  /**
   * @brief Single-pin read-modify-write on a dual-port register pair.
   *
   * Selects the correct port register based on pin number, reads, modifies the bit,
   * and writes back. Does NOT validate pin or call EnsureInitialized().
   */
  bool modifySinglePinRegister(uint8_t reg0, uint8_t reg1, uint8_t pin, bool bit_value) noexcept;

  /**
   * @brief Batch mask read-modify-write on a dual-port register pair.
   *
   * For each bit set in mask (0-15), sets or clears the corresponding bit in the
   * appropriate port register. Does NOT call EnsureInitialized().
   */
  bool modifyDualPortByMask(uint8_t reg0, uint8_t reg1, uint16_t mask, bool bit_value) noexcept;

  /**
   * @brief Check that the chip supports Agile I/O, setting error if not.
   *
   * @return true if chip is PCAL9555A; false if PCA9555 (sets Error::UnsupportedFeature).
   */
  bool requireAgileIO() noexcept;

  /**
   * @brief Shared implementation for ChangeAddress overloads.
   *
   * Sets GPIO address pins, updates internal state, verifies communication,
   * and re-detects chip variant at the new address.
   *
   * @param new_bits  Address bits (0-7) to set.
   * @return true if communication at the new address succeeded.
   */
  bool changeAddressImpl(uint8_t new_bits) noexcept;

  /**
   * @brief Detect the chip variant by probing an Agile I/O register.
   *
   * Detection sequence (3-step sandwich):
   *   1. Read INPUT_PORT_0 (0x00) — verify basic I2C communication
   *   2. Read OUTPUT_CONF (0x4F) — probe Agile I/O register
   *   3. Read INPUT_PORT_0 (0x00) — verify bus recovered after potential NACK
   *
   * If step 1 fails the bus is broken and variant is left as Unknown.
   * If step 2 succeeds the chip is PCAL9555A.
   * If step 2 NACKs but step 3 succeeds the chip is a standard PCA9555.
   * If step 3 also fails the detection is inconclusive (bus error).
   *
   * @return true when a variant was identified; false when it is left Unknown
   *         (initialisation and ChangeAddress() then fail).
   */
  bool detectChipVariant() noexcept;
};

// Include template implementation
#define PCAL95555_HEADER_INCLUDED
// NOLINTNEXTLINE(bugprone-suspicious-include) - Intentional: template implementation file
#include "../src/pcal95555.ipp"
#undef PCAL95555_HEADER_INCLUDED


// Public API: Get driver version string
inline const char* GetDriverVersion() noexcept {
  return HF_PCAL95555_VERSION_STRING;
}

} // namespace pcal95555

// ---------------------------------------------------------------------------
// Deprecated global-scope names
// ---------------------------------------------------------------------------
// Up to 1.0.0 these types were declared in the global namespace, where names
// such as Error and Polarity collide with other libraries. They now live in
// namespace pcal95555; the aliases below keep old code compiling and will be
// removed in the next major version. Define HF_PCAL95555_NO_GLOBAL_ALIASES to
// drop them now.
#ifndef HF_PCAL95555_NO_GLOBAL_ALIASES
using Pcal95555Reg = pcal95555::Pcal95555Reg;      ///< @deprecated Use pcal95555::Pcal95555Reg
using GPIODir = pcal95555::GPIODir;                ///< @deprecated Use pcal95555::GPIODir
using Polarity = pcal95555::Polarity;              ///< @deprecated Use pcal95555::Polarity
using DriveStrength = pcal95555::DriveStrength;    ///< @deprecated Use pcal95555::DriveStrength
using OutputMode = pcal95555::OutputMode;          ///< @deprecated Use pcal95555::OutputMode
using InterruptEdge = pcal95555::InterruptEdge;    ///< @deprecated Use pcal95555::InterruptEdge
using InterruptState = pcal95555::InterruptState;  ///< @deprecated Use pcal95555::InterruptState
using Error = pcal95555::Error;                    ///< @deprecated Use pcal95555::Error
using pcal95555::operator|;
using pcal95555::operator&;
using pcal95555::operator~;
#endif
