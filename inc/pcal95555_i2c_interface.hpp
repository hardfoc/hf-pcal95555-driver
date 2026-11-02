/**
 * @file pcal95555_i2c_interface.hpp
 * @brief CRTP-based I2C interface for PCAL95555 driver
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace pcal95555 {

// ============================================================================
// GPIO Enums -- Standardised Control Pin Model
// ============================================================================

/**
 * @enum CtrlPin
 * @brief Identifies the hardware control pins of the PCAL95555.
 *
 * Used with `GpioSet()` / `GpioRead()` to control or read the IC's
 * dedicated GPIO pins through the I2cInterface.
 *
 * - **INTN**: Active-low interrupt output (read-only via GpioRead)
 *
 * The PCA9555 and PCAL9555A have no reset input (PCAL9555A data sheet Rev. 2,
 * table 3; PCA9555 Rev. 10, table 3): the only reset is power-on reset, and
 * PCAL95555::ResetToDefault() rewrites the power-up register values instead.
 * RSTN is kept only so existing ports still compile.
 */
enum class CtrlPin : uint8_t {
  INTN = 0, ///< Interrupt output (active-low, open-drain, read-only)
  RSTN [[deprecated("PCA9555/PCAL9555A have no reset pin; use ResetToDefault()")]]
  ///< No such pin on these parts; never driven by the driver
};

/**
 * @enum GpioSignal
 * @brief Abstract signal level for control pins.
 *
 * Decouples the driver's intent from the physical pin polarity.
 */
enum class GpioSignal : uint8_t {
  INACTIVE = 0, ///< Pin function is deasserted
  ACTIVE   = 1  ///< Pin function is asserted
};

/**
 * @brief CRTP-based template interface for I2C bus operations
 *
 * This template class provides a hardware-agnostic interface for I2C communication
 * using the CRTP pattern. Platform-specific implementations should inherit from
 * this template with themselves as the template parameter.
 *
 * Benefits of CRTP:
 * - Compile-time polymorphism (no virtual function overhead)
 * - Static dispatch instead of dynamic dispatch
 * - Better optimization opportunities for the compiler
 *
 * Example usage:
 * @code
 * class MyI2C : public pcal95555::I2cInterface<MyI2C> {
 * public:
 *   bool Write(...) { ... }
 *   bool Read(...) { ... }
 * };
 * @endcode
 *
 * @tparam Derived The derived class type (CRTP pattern)
 */
template <typename Derived>
class I2cInterface {
public:
  /**
   * @brief Write bytes to a device register.
   *
   * @param addr 7-bit I2C address of the target device.
   * @param reg Register address to write to.
   * @param data Pointer to the data buffer containing bytes to send.
   * @param len Number of bytes to write from the buffer.
   * @return true if the device acknowledges the transfer; false on NACK or
   * error.
   */
  bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept {
    return static_cast<Derived*>(this)->Write(addr, reg, data, len);
  }

  /**
   * @brief Read bytes from a device register.
   *
   * @param addr 7-bit I2C address of the target device.
   * @param reg Register address to read from.
   * @param data Pointer to the buffer to store received data.
   * @param len Number of bytes to read into the buffer.
   * @return true if the read succeeds; false on NACK or error.
   */
  bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept {
    return static_cast<Derived*>(this)->Read(addr, reg, data, len);
  }

  /**
   * @brief Set address pin levels (for controlling address pins A2-A0).
   *
   * This method allows the I2C interface implementation to control GPIO pins
   * that are connected to the PCAL95555 address selection pins (A2, A1, A0).
   * The default implementation returns false, indicating that GPIO control is
   * not supported. Derived classes should override this method if they support
   * dynamic address pin control.
   *
   * @param a0_level true to set A0 pin HIGH (VDD), false to set LOW (GND)
   * @param a1_level true to set A1 pin HIGH (VDD), false to set LOW (GND)
   * @param a2_level true to set A2 pin HIGH (VDD), false to set LOW (GND)
   * @return true if GPIO control is supported and pins were set successfully;
   *         false if not supported or on error.
   *
   * @note This is an optional feature. Most implementations will return false
   *       as address pins are typically hardwired. Only override if your
   *       hardware allows software control of A2-A0 pins.
   *
   * @example
   *   // Set A2=HIGH, A1=LOW, A0=HIGH (address bits = 0b101 = 5)
   *   bool success = i2c->SetAddressPins(true, false, true);
   */
  bool SetAddressPins(bool a0_level, bool a1_level, bool a2_level) noexcept {
    // Default implementation: not supported
    (void)a0_level;  // Suppress unused parameter warning
    (void)a1_level;
    (void)a2_level;
    return false;
  }

  /**
   * @brief Ensure the I2C bus is initialized and ready for communication.
   *
   * This method performs lazy initialization of the I2C bus. It should initialize
   * the I2C hardware, configure pins, set up the bus, and verify it's ready for
   * communication. On subsequent calls, it should return true immediately if
   * already initialized.
   *
   * @return true if initialization succeeded or was already initialized;
   *         false if initialization failed.
   *
   * @note This is a required method. All I2C interface implementations must
   *       provide this method to ensure the bus is ready before communication.
   *
   * @example
   *   bool EnsureInitialized() {
   *       if (initialized_) {
   *           return true;
   *       }
   *       // Initialize I2C hardware, configure pins, etc.
   *       initialized_ = true;
   *       return true;
   *   }
   */
  bool EnsureInitialized() noexcept {
    return static_cast<Derived*>(this)->EnsureInitialized();
  }

  /**
   * @brief Register an interrupt handler to be called when INT pin fires.
   *
   * This method allows the PCAL95555 driver to register its interrupt handler
   * with the I2C interface. When the INT pin (open-drain interrupt output from
   * PCAL95555) fires, the I2C interface should call this handler.
   *
   * The handler will typically read the interrupt status registers and invoke
   * user-registered callbacks for pins that triggered interrupts.
   *
   * @param handler Function to call when INT pin interrupt occurs.
   *                The handler should be called from interrupt context or a
   *                dedicated interrupt task.
   * @return true if interrupt handler registration is supported and successful;
   *         false if not supported or on error.
   *
   * @note This is an optional feature. The default implementation does nothing.
   *       Platform-specific implementations should override this to set up GPIO
   *       interrupt handling for the INT pin.
   *
   * @note The INT pin is open-drain and requires an external pull-up resistor.
   *       The interrupt is active-low (falling edge).
   *
   * @example
   *   // In PCAL95555 class:
   *   i2c_->RegisterInterruptHandler([this]() {
   *       HandleInterrupt();  // Reads status and calls user callbacks
   *   });
   */
  bool RegisterInterruptHandler(std::function<void()> handler) noexcept {
    // Default implementation: not supported
    (void)handler;  // Suppress unused parameter warning
    return false;
  }

  // --------------------------------------------------------------------------
  /// @name GPIO Pin Control
  ///
  /// Unified interface for controlling PCAL95555 hardware control pins.
  /// @{

  /**
   * @brief Set a control pin to the specified signal state.
   *
   * @param[in] pin     Which control pin to drive. The chips have no output-side
   *                    control pin, so the driver never calls this.
   * @param[in] signal  ACTIVE to assert the pin function, INACTIVE to deassert.
   *
   * @note Default implementation is a no-op.
   */
  void GpioSet(CtrlPin pin, GpioSignal signal) noexcept {
    (void)pin;
    (void)signal;
  }

  /**
   * @brief Read the current state of a control pin.
   *
   * Primarily used for reading the INTN interrupt pin.
   *
   * @param[in]  pin     Which control pin to read.
   * @param[out] signal  Receives the current signal state.
   * @return true if the read was successful, false otherwise.
   *
   * @note Default implementation returns false (unsupported). Override in
   *       derived class if interrupt pin monitoring is needed.
   */
  bool GpioRead(CtrlPin pin, GpioSignal &signal) noexcept {
    (void)pin;
    (void)signal;
    return false;
  }

  /**
   * @brief Assert a control pin (set to ACTIVE).
   * @param[in] pin  Which control pin to assert.
   */
  void GpioSetActive(CtrlPin pin) noexcept { GpioSet(pin, GpioSignal::ACTIVE); }

  /**
   * @brief Deassert a control pin (set to INACTIVE).
   * @param[in] pin  Which control pin to deassert.
   */
  void GpioSetInactive(CtrlPin pin) noexcept { GpioSet(pin, GpioSignal::INACTIVE); }

  /// @}

protected:
  /**
   * @brief Protected constructor to prevent direct instantiation
   */
  I2cInterface() = default;

  // Prevent copying
  I2cInterface(const I2cInterface&) = delete;
  I2cInterface& operator=(const I2cInterface&) = delete;

  // Allow moving
  I2cInterface(I2cInterface&&) = default;
  I2cInterface& operator=(I2cInterface&&) = default;

  /**
   * @brief Protected destructor
   * @note Derived classes can have public destructors
   */
  ~I2cInterface() = default;
};

} // namespace pcal95555
