# API Reference

Complete reference documentation for all public methods and types in the PCAL95555 driver.

## Source Code

- **Main Header**: [`inc/pcal95555.hpp`](../inc/pcal95555.hpp)
- **Kconfig Macros**: [`inc/pcal95555_kconfig.hpp`](../inc/pcal95555_kconfig.hpp) (compile-time configuration, included by main header)
- **Implementation**: [`src/pcal95555.ipp`](../src/pcal95555.ipp)

## Core Class

### `PCAL95555<I2cType>`

Main driver class for interfacing with the **PCA9555** and **PCAL9555A** I/O expanders.
The chip variant is auto-detected during initialization. PCAL9555A-specific features
(pull resistors, drive strength, input latch, interrupt mask/status, output mode)
return `false` and set `Error::UnsupportedFeature` when a standard PCA9555 is detected.

**Template Parameter**: `I2cType` - Your I2C interface implementation (must inherit from `pcal95555::I2cInterface<I2cType>`)

**Location**: [`inc/pcal95555.hpp`](../inc/pcal95555.hpp)

**Constructors:**

**Constructor 1: Using address pin levels**
```cpp
PCAL95555(I2cType* bus, bool a0_level, bool a1_level, bool a2_level,
          ChipVariant variant = ChipVariant::Unknown);
```

**Parameters:**
- `bus`: Pointer to your I2C interface implementation
- `a0_level`: Initial state of A0 pin (true = HIGH/VDD, false = LOW/GND)
- `a1_level`: Initial state of A1 pin (true = HIGH/VDD, false = LOW/GND)
- `a2_level`: Initial state of A2 pin (true = HIGH/VDD, false = LOW/GND)
- `variant`: Optional chip variant override. Default `ChipVariant::Unknown` enables auto-detection. Set to `ChipVariant::PCA9555` or `ChipVariant::PCAL9555A` to skip detection.

**Constructor 2: Using I2C address directly**
```cpp
explicit PCAL95555(I2cType* bus, uint8_t address,
                   ChipVariant variant = ChipVariant::Unknown);
```

**Parameters:**
- `bus`: Pointer to your I2C interface implementation
- `address`: 7-bit I2C address (0x20 to 0x27). Address bits are calculated automatically.
- `variant`: Optional chip variant override (same as above).

**Description:**
Both constructors use lazy initialization -- no I2C traffic occurs in the constructor.
Initialization (including chip variant detection) happens on first method call or
explicit `EnsureInitialized()`. The address is calculated from A0-A2 levels or used
directly. `SetAddressPins()` is called if supported by the I2C interface.

**Examples:**
```cpp
// Using pin levels - auto-detect chip variant (default)
PCAL95555 driver1(bus, false, false, false);

// Using address directly - auto-detect
PCAL95555 driver2(bus, 0x20);

// Force PCA9555 mode (skip Agile I/O detection probe)
PCAL95555 driver3(bus, 0x20, ChipVariant::PCA9555);

// Force PCAL9555A mode
PCAL95555 driver4(bus, false, false, false, ChipVariant::PCAL9555A);
```

**Location**: [`inc/pcal95555.hpp`](../inc/pcal95555.hpp)

## Methods

### Initialization

| Method | Signature | Location |
|--------|-----------|----------|
| `EnsureInitialized()` | `bool EnsureInitialized()` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `ResetToDefault()` | `void ResetToDefault()` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `InitFromConfig()` | `void InitFromConfig()` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Pin Direction

| Method | Signature | Location |
|--------|-----------|----------|
| `SetPinDirection()` | `bool SetPinDirection(uint16_t pin, GPIODir dir)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `SetDirections()` | `bool SetDirections(std::initializer_list<std::pair<uint16_t, GPIODir>> configs)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `SetMultipleDirections()` | `bool SetMultipleDirections(uint16_t mask, GPIODir dir)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Pin I/O

| Method | Signature | Location |
|--------|-----------|----------|
| `ReadPin()` | `bool ReadPin(uint16_t pin)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `ReadPins()` | `std::vector<std::pair<uint16_t, bool>> ReadPins(std::initializer_list<uint16_t> pins)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `WritePin()` | `bool WritePin(uint16_t pin, bool value)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `WritePins()` | `bool WritePins(std::initializer_list<std::pair<uint16_t, bool>> configs)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `TogglePin()` | `bool TogglePin(uint16_t pin)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Pull-up/Pull-down (PCAL9555A only)

> **Note**: These methods return `false` and set `Error::UnsupportedFeature` on PCA9555.

| Method | Signature | Location |
|--------|-----------|----------|
| `SetPullEnable()` | `bool SetPullEnable(uint16_t pin, bool enable)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `SetPullEnables()` | `bool SetPullEnables(std::initializer_list<std::pair<uint16_t, bool>> configs)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `SetPullDirection()` | `bool SetPullDirection(uint16_t pin, bool pull_up)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `SetPullDirections()` | `bool SetPullDirections(std::initializer_list<std::pair<uint16_t, bool>> configs)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Drive Strength (PCAL9555A only)

> **Note**: These methods return `false` and set `Error::UnsupportedFeature` on PCA9555.

| Method | Signature | Location |
|--------|-----------|----------|
| `SetDriveStrength()` | `bool SetDriveStrength(uint16_t pin, DriveStrength level)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `SetDriveStrengths()` | `bool SetDriveStrengths(std::initializer_list<std::pair<uint16_t, DriveStrength>> configs)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Interrupts

> **Note**: `ConfigureInterrupt`, `ConfigureInterrupts`, `ConfigureInterruptMask`, and `GetInterruptStatus` require PCAL9555A and return `false`/`0` with `Error::UnsupportedFeature` on PCA9555. The remaining interrupt methods (callbacks, handler) work on both variants -- on PCA9555, `HandleInterrupt()` uses change-detection (comparing current vs. previous pin states) instead of the hardware interrupt status register.

| Method | Signature | PCAL9555A? | Location |
|--------|-----------|------------|----------|
| `ConfigureInterrupt()` | `bool ConfigureInterrupt(uint16_t pin, InterruptState state)` | Yes | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `ConfigureInterrupts()` | `bool ConfigureInterrupts(std::initializer_list<std::pair<uint16_t, InterruptState>> configs)` | Yes | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `ConfigureInterruptMask()` | `bool ConfigureInterruptMask(uint16_t mask)` | Yes | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `GetInterruptStatus()` | `uint16_t GetInterruptStatus()` | Yes | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `RegisterPinInterrupt()` | `bool RegisterPinInterrupt(uint16_t pin, InterruptEdge edge, std::function<void(uint16_t, bool)> callback)` | No | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `UnregisterPinInterrupt()` | `bool UnregisterPinInterrupt(uint16_t pin)` | No | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `SetInterruptCallback()` | `void SetInterruptCallback(const std::function<void(uint16_t)>& callback)` | No | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `RegisterInterruptHandler()` | `bool RegisterInterruptHandler()` | No | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `HandleInterrupt()` | `void HandleInterrupt()` | No | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Output Mode (PCAL9555A only)

> **Note**: Returns `false` and sets `Error::UnsupportedFeature` on PCA9555.

| Method | Signature | Location |
|--------|-----------|----------|
| `SetOutputMode()` | `bool SetOutputMode(bool port_0_open_drain, bool port_1_open_drain)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Polarity

| Method | Signature | Location |
|--------|-----------|----------|
| `SetPinPolarity()` | `bool SetPinPolarity(uint16_t pin, Polarity polarity)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `SetPolarities()` | `bool SetPolarities(std::initializer_list<std::pair<uint16_t, Polarity>> configs)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `SetMultiplePolarities()` | `bool SetMultiplePolarities(uint16_t mask, Polarity polarity)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Input Latch (PCAL9555A only)

> **Note**: These methods return `false` and set `Error::UnsupportedFeature` on PCA9555.

| Method | Signature | Location |
|--------|-----------|----------|
| `EnableInputLatch()` | `bool EnableInputLatch(uint16_t pin, bool enable)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `EnableInputLatches()` | `bool EnableInputLatches(std::initializer_list<std::pair<uint16_t, bool>> configs)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `EnableMultipleInputLatches()` | `bool EnableMultipleInputLatches(uint16_t mask, bool enable)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Address Management

| Method | Signature | Location |
|--------|-----------|----------|
| `GetAddress()` | `uint8_t GetAddress() const` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `GetAddressBits()` | `uint8_t GetAddressBits() const` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `ChangeAddress()` | `bool ChangeAddress(bool a0_level, bool a1_level, bool a2_level)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `ChangeAddress()` | `bool ChangeAddress(uint8_t address)` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Error Handling

| Method | Signature | Location |
|--------|-----------|----------|
| `SetRetries()` | `void SetRetries(int retries) noexcept` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `GetErrorFlags()` | `[[nodiscard]] uint16_t GetErrorFlags() const noexcept` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `ClearErrorFlags()` | `void ClearErrorFlags(uint16_t mask = 0xFFFF) noexcept` | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

### Chip Variant Detection

| Method | Signature | Description | Location |
|--------|-----------|-------------|----------|
| `HasAgileIO()` | `[[nodiscard]] bool HasAgileIO() const noexcept` | Returns `true` if PCAL9555A detected (Agile I/O available) | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |
| `GetChipVariant()` | `[[nodiscard]] ChipVariant GetChipVariant() const noexcept` | Returns the detected `ChipVariant` enum value | [`src/pcal95555.ipp`](../src/pcal95555.ipp) |

**Usage:**
```cpp
if (driver.HasAgileIO()) {
    // PCAL9555A features available
    driver.SetDriveStrength(0, DriveStrength::Level2);
} else {
    // PCA9555 -- only standard registers
}

auto variant = driver.GetChipVariant();
// ChipVariant::Unknown, ChipVariant::PCA9555, or ChipVariant::PCAL9555A
```

## Types

### Enumerations

| Type | Values | Description | Location |
|------|--------|-------------|----------|
| `GPIODir` | `Input`, `Output` | GPIO pin direction | [`inc/pcal95555.hpp`](../inc/pcal95555.hpp) |
| `DriveStrength` | `Level0`, `Level1`, `Level2`, `Level3` | Output drive strength (Level0=25%, Level1=50%, Level2=75%, Level3=100%). **PCAL9555A only.** | [`inc/pcal95555.hpp`](../inc/pcal95555.hpp) |
| `Polarity` | `Normal`, `Inverted` | Input polarity inversion | [`inc/pcal95555.hpp`](../inc/pcal95555.hpp) |
| `OutputMode` | `PushPull`, `OpenDrain` | Output mode (per port). **PCAL9555A only.** | [`inc/pcal95555.hpp`](../inc/pcal95555.hpp) |
| `InterruptState` | `Enabled`, `Disabled` | Interrupt enable/disable state. **PCAL9555A only.** | [`inc/pcal95555.hpp`](../inc/pcal95555.hpp) |
| `InterruptEdge` | `Rising`, `Falling`, `Both` | Interrupt edge trigger type (works on both variants via software) | [`inc/pcal95555.hpp`](../inc/pcal95555.hpp) |
| `ChipVariant` | `Unknown`, `PCA9555`, `PCAL9555A` | Detected or user-specified chip variant | [`inc/pcal95555.hpp`](../inc/pcal95555.hpp) |
| `Error` | `None`, `InvalidPin`, `InvalidMask`, `I2CReadFail`, `I2CWriteFail`, `UnsupportedFeature`, `InvalidAddress` | Error conditions (bitmask). `UnsupportedFeature` (0x0010) is set when a PCAL9555A-only method is called on a PCA9555. `InvalidAddress` (0x0020) is set when an I2C address outside the valid 0x20-0x27 range is provided. | [`inc/pcal95555.hpp`](../inc/pcal95555.hpp) |

---

**Navigation**
⬅️ [Configuration](configuration.md) | [Next: Examples ➡️](examples.md) | [Back to Index](index.md)
