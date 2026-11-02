# PCAL95555 Comprehensive Test Suite Documentation

## Overview

The comprehensive test suite (`pcal95555_comprehensive_test`) validates **all 43 public
API methods** of the PCAL95555 driver across **20 test sections**. It supports both the
standard **PCA9555** and the enhanced **PCAL9555A** -- the chip variant is auto-detected
during initialization, and PCAL9555A-only tests are automatically **skipped** (not failed)
when a standard PCA9555 is detected.

## Test Configuration

### Hardware Target
- **MCU**: ESP32-S3
- **I2C Frequency**: 400 kHz
- **Default I2C Address**: 0x20 (A2=LOW, A1=LOW, A0=LOW)

### Pin Configuration

| Signal | ESP32-S3 GPIO | Chip Pin | Function |
|--------|---------------|----------|----------|
| SDA | GPIO4 | SDA | I2C Data Line |
| SCL | GPIO5 | SCL | I2C Clock Line |
| INT | GPIO7 | INT | Interrupt Output (open-drain) |
| A0 | GPIO45 | A0 | Address Bit 0 (controlled by MCU) |
| A1 | GPIO48 | A1 | Address Bit 1 (controlled by MCU) |
| A2 | GPIO47 | A2 | Address Bit 2 (controlled by MCU) |
| Test Indicator | GPIO14 | - | Visual test progress indicator |

### Address Pin Control

The test suite uses GPIO-controlled address pins, allowing dynamic address configuration:
- **A0**: Controlled via GPIO45
- **A1**: Controlled via GPIO48
- **A2**: Controlled via GPIO47

The driver automatically sets these pins during initialization.

---

## Chip Variant Handling

During the "Driver Init" test, the driver performs a 3-step sandwich detection:

1. Read `INPUT_PORT_0` (0x00) to verify bus health
2. Probe `OUTPUT_CONF` (0x4F) for Agile I/O support
3. Read `INPUT_PORT_0` (0x00) again to confirm bus recovery

The detected variant is logged:
```
Detected chip variant: PCA9555 (standard)
Agile I/O support: NO
```
or
```
Detected chip variant: PCAL9555A (Agile I/O)
Agile I/O support: YES
```

Tests requiring PCAL9555A log a skip message:
```
Skipping: Pull resistor config requires PCAL9555A (detected PCA9555)
```

---

## Test Sections

### 1. Initialization Tests

#### `test_i2c_bus_initialization()`
**Purpose**: Validates I2C bus setup and configuration.

**What it tests**:
- I2C master bus creation
- GPIO pin configuration for SDA/SCL
- Address pin GPIO configuration (A0, A1, A2)

**Expected Result**: I2C bus successfully initialized with all pins configured.

#### `test_driver_initialization()`
**Purpose**: Validates driver initialization, chip variant detection, and default state.

**What it tests**:
- Driver instance creation (address pin constructor)
- I2C communication verification
- Chip variant auto-detection (PCA9555 vs PCAL9555A)
- `HasAgileIO()` and `GetChipVariant()` queries
- Reset to default state
- Error flag checking and clearing

**Expected Result**: Driver initialized, chip variant detected and logged.

---

### 2. GPIO Direction Tests

#### `test_single_pin_direction()`
**Tests**: `SetPinDirection()` for each pin (0-15), input and output modes.

#### `test_multiple_pin_direction()`
**Tests**: `SetMultipleDirections()` using 16-bit mask for port-level direction control.

---

### 3. GPIO Read/Write Tests

#### `test_pin_write()`
**Tests**: `WritePin()` -- writing HIGH and LOW to individual pins.

#### `test_pin_read()`
**Tests**: `ReadPin()` -- reading all 16 pins sequentially.

#### `test_pin_toggle()`
**Tests**: `TogglePin()` -- toggling pin state multiple times with verification.

---

### 4. Pull Resistor Tests (PCAL9555A only)

#### `test_pull_resistor_config()`
**Tests**: `SetPullEnable()`, `SetPullDirection()` -- per-pin pull-up/pull-down configuration.

**Auto-skip**: Returns `true` with skip message on PCA9555.

---

### 5. Drive Strength Tests (PCAL9555A only)

#### `test_drive_strength()`
**Tests**: `SetDriveStrength()` -- all 4 levels (Level0-Level3) on individual pins.

**Auto-skip**: Returns `true` with skip message on PCA9555.

---

### 6. Output Mode Tests (PCAL9555A only)

#### `test_output_mode()`
**Tests**: `SetOutputMode()` -- push-pull and open-drain per port and both ports.

**Auto-skip**: Returns `true` with skip message on PCA9555.

---

### 7. Polarity Tests

#### `test_polarity_inversion()`
**Tests**: `SetPinPolarity()`, `SetMultiplePolarities()` -- normal and inverted polarity, single pin and mask-based.

---

### 8. Input Latch Tests (PCAL9555A only)

#### `test_input_latch()`
**Tests**: `EnableInputLatch()`, `EnableMultipleInputLatches()` -- per-pin and mask-based latch enable/disable.

**Auto-skip**: Returns `true` with skip message on PCA9555.

---

### 9. Interrupt Tests (partially PCAL9555A only)

#### `test_interrupt_mask_config()` (PCAL9555A)
**Tests**: `ConfigureInterruptMask()` -- 16-bit interrupt mask register.

#### `test_interrupt_status()` (PCAL9555A)
**Tests**: `GetInterruptStatus()` -- reading and clearing interrupt status.

#### `test_pin_interrupt_callbacks()`
**Tests**: `RegisterPinInterrupt()`, `SetInterruptCallback()` -- per-pin callbacks with edge detection
(Rising, Falling, Both).

#### `test_interrupt_handler_registration()`
**Tests**: `RegisterInterruptHandler()` -- hardware INT pin setup. Passes even without INT pin connected.

#### `test_interrupt_callback_unregistration()`
**Tests**: `UnregisterPinInterrupt()` -- callback removal and double-unregister safety.

#### `test_interrupt_config()` (PCAL9555A)
**Tests**: Combined interrupt mask + status reading.

---

### 10. Port Operation Tests

#### `test_port_operations()`
**Tests**: `SetMultipleDirections()`, `WritePin()`, `ReadPin()` -- port 0 as output, port 1 as input, mixed operations.

---

### 11. Multi-Pin API Tests

#### `test_write_pins_multi()`
**Tests**: `WritePins()` -- writing multiple pins at once via initializer list.

#### `test_read_pins_multi()`
**Tests**: `ReadPins()` -- reading multiple pins at once, verifying result vector.

#### `test_set_directions_multi()`
**Tests**: `SetDirections()` -- mixed input/output per pin via initializer list.

#### `test_set_polarities_multi()`
**Tests**: `SetPolarities()` -- mixed polarity per pin via initializer list.

---

### 12. Address Management Tests

#### `test_address_management()`
**Tests**:
- `GetAddress()`, `GetAddressBits()` -- reading current address
- `ChangeAddress(bool, bool, bool)` -- changing address via pin levels
- `ChangeAddress(uint8_t)` -- changing address via direct value
- Address-based constructor `PCAL95555(bus, address)` -- creating a temporary driver

**Note**: Address change to a non-existent device handles NACK gracefully. Original address is always restored.

---

### 13. Configuration Tests

#### `test_config_and_init()`
**Tests**:
- `SetRetries()` -- setting 0, 3, and restoring to 1
- `EnsureInitialized()` -- on already-initialized and fresh driver instances

---

### 14. Multi-Pin PCAL9555A API Tests (PCAL9555A only)

#### `test_multi_pin_pcal_apis()`
**Tests**:
- `SetPullEnables()` -- multi-pin pull enable via initializer list
- `SetPullDirections()` -- multi-pin pull direction via initializer list
- `SetDriveStrengths()` -- multi-pin drive strength via initializer list
- `ConfigureInterrupt()` -- single-pin interrupt enable/disable
- `ConfigureInterrupts()` -- multi-pin interrupt config via initializer list
- `EnableInputLatches()` -- multi-pin input latch via initializer list

**Auto-skip**: Returns `true` with skip message on PCA9555.

---

### 15. Interactive Input Tests (disabled by default)

#### `test_interactive_input()`

**Requires**: Momentary push-button between PCA9555 pin 0 and GND.

**Tests**:
- `ReadPin()` -- detecting physical button press
- Pull-up configuration (internal on PCAL9555A, external on PCA9555)
- `HandleInterrupt()` -- explicit manual call

**Behavior**:
1. Displays a banner explaining hardware requirements
2. Configures pin 0 as input with pull-up
3. Waits 10 seconds for button press with countdown
4. If button detected: verifies press and release
5. If no button: logs warning and continues (does not fail)
6. Calls `HandleInterrupt()` explicitly to verify no crash

**To enable**: Set `ENABLE_INTERACTIVE_INPUT_TESTS = true` in the test file.

---

### 16. Error Handling Tests

#### `test_error_handling()`
**Tests**:
- Invalid pin (16, 17, 18) for `SetPinDirection()`, `ReadPin()`, `WritePin()`, `TogglePin()`
- `GetErrorFlags()` -- verifying error flags are set
- `ClearErrorFlags(mask)` -- selective flag clearing (specific mask vs all)
- `Error::UnsupportedFeature` -- calling `SetDriveStrength()` on PCA9555
- `HandleInterrupt()` -- explicit call on clean state (no crash)

---

### 17. Stress Tests

#### `test_rapid_operations()`
**Tests**: Rapid `WritePin()` toggle (100 cycles at 1ms intervals) to verify I2C stability under load.

---

## Test Execution Flow

1. **Initialization Phase**: I2C bus setup, driver creation, chip variant detection
2. **Core GPIO Tests**: Direction, read/write, toggle
3. **PCAL9555A Feature Tests**: Pull resistors, drive strength, output mode, input latch, interrupts (auto-skip on PCA9555)
4. **Advanced API Tests**: Multi-pin operations, address management, configuration
5. **Robustness Tests**: Error handling, stress testing
6. **Cleanup Phase**: Resource cleanup, test summary reporting

---

## Enabling/Disabling Test Sections

```cpp
static constexpr bool ENABLE_INITIALIZATION_TESTS    = true;
static constexpr bool ENABLE_GPIO_DIRECTION_TESTS    = true;
static constexpr bool ENABLE_GPIO_READ_WRITE_TESTS   = true;
static constexpr bool ENABLE_PULL_RESISTOR_TESTS     = true;
static constexpr bool ENABLE_DRIVE_STRENGTH_TESTS    = true;
static constexpr bool ENABLE_OUTPUT_MODE_TESTS       = true;
static constexpr bool ENABLE_POLARITY_TESTS          = true;
static constexpr bool ENABLE_INPUT_LATCH_TESTS       = true;
static constexpr bool ENABLE_INTERRUPT_TESTS         = true;
static constexpr bool ENABLE_PORT_OPERATION_TESTS    = true;
static constexpr bool ENABLE_MULTI_PIN_TESTS         = true;
static constexpr bool ENABLE_MULTI_PIN_API_TESTS     = true;
static constexpr bool ENABLE_ADDRESS_TESTS           = true;
static constexpr bool ENABLE_CONFIG_TESTS            = true;
static constexpr bool ENABLE_MULTI_PIN_PCAL_TESTS    = true;
static constexpr bool ENABLE_INTERACTIVE_INPUT_TESTS = false; // Requires button on pin 0
static constexpr bool ENABLE_ERROR_HANDLING_TESTS    = true;
static constexpr bool ENABLE_STRESS_TESTS            = true;
```

---

## Test Results Interpretation

### Success Indicators
- `[SUCCESS] PASSED` -- test passed
- `Skipping: ... requires PCAL9555A (detected PCA9555)` -- expected skip, counts as pass

### Warning Indicators
- `Driver has error flags: 0xNNNN` -- I2C errors during init (may be expected on PCA9555)
- `No button press detected within 10 seconds` -- interactive test timeout (OK)

### Failure Indicators
- `[FAILED] FAILED` -- test failed, check error messages above it
- `Error flags: 0xNNNN` -- see Error Flags table below

### Error Flags Reference

| Flag | Value | Meaning |
|------|-------|---------|
| `InvalidPin` | 0x0001 | Pin index >= 16 |
| `InvalidMask` | 0x0002 | Mask bits outside valid range |
| `I2CReadFail` | 0x0004 | I2C read transaction failed |
| `I2CWriteFail` | 0x0008 | I2C write transaction failed |
| `UnsupportedFeature` | 0x0010 | PCAL9555A feature called on PCA9555 |

---

## Hardware Requirements

### Minimum
- PCA9555 or PCAL9555A I/O expander
- ESP32-S3 development board
- I2C connections (SDA, SCL) with 4.7k pull-ups
- Power connections (3.3V, GND)

### For Full Test Coverage
- Address pins connected to GPIOs (A0=GPIO45, A1=GPIO48, A2=GPIO47)
- INT pin connected to GPIO7 (for interrupt handler test)
- LED on GPIO14 (test progress indicator)

### For Interactive Input Test
- Momentary push-button between pin 0 and GND
- 10k pull-up to VDD (PCA9555) or use internal pull-up (PCAL9555A)

---

## Expected Test Count

| Chip Variant | Enabled Sections | Expected Tests | Expected Pass |
|-------------|-----------------|----------------|---------------|
| PCA9555 | All (default) | ~31 | 31 (PCAL tests skip as pass) |
| PCAL9555A | All (default) | ~31 | 31 |
| Either + Interactive | All + Interactive | ~32 | 32 |

---

**Navigation**
[Back to Examples README](../README.md) | [Main README](../../../README.md)
