---
layout: default
title: "HardFOC PCAL95555 Driver"
description: "Hardware-agnostic C++ driver for the NXP PCA9555 and PCAL9555A 16-bit I/O expanders"
nav_order: 1
permalink: /
---

# HF-PCAL95555 Driver
**Hardware-agnostic C++ driver for the NXP PCA9555 and PCAL9555A 16-bit I/O expanders**

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![CI](https://github.com/hardfoc/hf-pcal95555-driver/actions/workflows/esp32-examples-build-ci.yml/badge.svg?branch=main)](https://github.com/hardfoc/hf-pcal95555-driver/actions/workflows/esp32-examples-build-ci.yml)
[![Docs](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://hardfoc.github.io/hf-pcal95555-driver/)

---

## 📚 Table of Contents

1. [Overview](#-overview)
2. [Chip Compatibility](#-chip-compatibility)
3. [Features](#-features)
4. [Project Structure](#-project-structure)
5. [Quick Start](#-quick-start)
6. [Installation](#-installation)
7. [Building and Flashing (ESP32)](#-building-and-flashing-esp32)
8. [API Reference](#-api-reference)
9. [Examples](#-examples)
10. [Testing](#-testing)
11. [Hardware Setup](#-hardware-setup)
12. [Troubleshooting](#-troubleshooting)
13. [References](#-references)
14. [Contributing](#-contributing)
15. [License](#-license)

---

## 📦 Overview
> **📖 [📚🌐 Live Complete Documentation](https://hardfoc.github.io/hf-pcal95555-driver/)** —
> Interactive guides, examples, and step-by-step tutorials

The **PCA9555** and **PCAL9555A** are 16-bit I/O expanders from NXP Semiconductors
that communicate via I2C. They provide 16 GPIO pins organized into two 8-bit
ports (PORT_0: pins 0-7, PORT_1: pins 8-15), expanding your microcontroller's I/O
capabilities over a simple two-wire bus.

This driver provides a **single, unified C++ class** that supports **both** chip
variants. The chip type is **auto-detected** during initialization by probing the
extended register bank. Features exclusive to the PCAL9555A degrade gracefully
when a standard PCA9555 is detected.

The driver is **hardware-agnostic** -- it uses the CRTP (Curiously Recurring Template
Pattern) for zero-overhead I2C abstraction. You only need to implement a small
`I2cInterface` class for your platform.

![PCA9555 / PCAL9555A 16-bit I2C GPIO expander topology](docs/assets/pcal95555-topology.svg)

---

## 🔀 Chip Compatibility

The PCAL9555A is a pin-compatible superset of the PCA9555. The driver handles both
transparently:

| Feature                          | PCA9555 | PCAL9555A |
|----------------------------------|---------|-----------|
| 16 GPIO pins (2 x 8-bit ports)  | Yes     | Yes       |
| I2C addresses 0x20-0x27         | Yes     | Yes       |
| Pin direction (input/output)     | Yes     | Yes       |
| Pin read / write / toggle        | Yes     | Yes       |
| Polarity inversion               | Yes     | Yes       |
| Pull-up / pull-down resistors    | --      | Yes       |
| Drive strength (4 levels)        | --      | Yes       |
| Input latch                      | --      | Yes       |
| Interrupt mask / status          | --      | Yes       |
| Output mode (push-pull / OD)     | --      | Yes       |

**How detection works:** During initialization, the driver reads a standard register
(INPUT_PORT_0, 0x00) to verify the bus is alive, then probes the Agile I/O register
(OUTPUT_CONF, 0x4F). If the chip ACKs, it's a PCAL9555A. If it NACKs, it's a PCA9555.
A follow-up read of INPUT_PORT_0 confirms the bus recovered. This 3-step sandwich
ensures reliable detection even on noisy buses.

Methods requiring PCAL9555A features return `false` and set `Error::UnsupportedFeature`
when called on a PCA9555. You can check at runtime with:

```cpp
if (driver.HasAgileIO()) {
    // PCAL9555A features available
    driver.SetDriveStrength(0, DriveStrength::Level2);
}
```

---

## ✨ Features

- **Dual-chip support**: Auto-detects PCA9555 vs PCAL9555A at runtime
- **16 GPIO Pins**: Two 8-bit ports (PORT_0: pins 0-7, PORT_1: pins 8-15)
- **Per-pin configuration**: Direction, pull-up/pull-down, drive strength, polarity
- **Interrupt support**: Per-pin interrupt masking with edge-detection callbacks
- **Hardware agnostic**: CRTP-based I2C interface for platform independence
- **Modern C++17**: Template-based design with `std::initializer_list` multi-pin APIs
- **Zero overhead**: CRTP for compile-time polymorphism -- no virtual calls
- **Lazy initialization**: No I2C traffic in the constructor; init on first use
- **Kconfig integration**: Optional compile-time configuration via ESP-IDF Kconfig
- **Robust error handling**: Bitmask error flags, configurable I2C retries
- **Graceful degradation**: PCAL9555A features fail cleanly on PCA9555

---

## 🗂️ Project Structure

```
hf-pcal95555-driver/
├── inc/
│   ├── pcal95555.hpp              # Main driver header (public API)
│   ├── pcal95555_kconfig.hpp      # Kconfig compile-time configuration macros
│   └── pcal95555_i2c_interface.hpp # CRTP I2C interface base class
├── src/
│   └── pcal95555.ipp              # Template implementation (included by header)
├── examples/
│   └── esp32/
│       ├── main/
│       │   ├── esp32_pcal95555_bus.hpp             # ESP32 I2C implementation
│       │   ├── pcal95555_comprehensive_test.cpp    # Full API test suite
│       │   ├── pcal95555_led_animation.cpp         # LED animation demo
│       │   ├── TestFramework.h                     # Test harness macros
│       │   └── CMakeLists.txt                      # Build configuration
│       ├── scripts/
│       │   ├── build_app.sh           # Build script
│       │   ├── flash_app.sh           # Flash + monitor script
│       │   └── config_loader.sh       # Build config parser
│       ├── app_config.yml             # App definitions for build system
│       └── sdkconfig                  # ESP-IDF configuration
├── docs/datasheet/
│   └── PCAL9555A.pdf              # NXP datasheet
├── _config/                       # Doxygen configuration
├── docs/                          # Additional documentation
├── README.md                      # This file
└── LICENSE                        # GPLv3
```

---

## 🚀 Quick Start

### 1. Implement the I2C interface for your platform

```cpp
#include "pcal95555.hpp"

class MyI2c : public pcal95555::I2cInterface<MyI2c> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
        // Your platform I2C write implementation
    }
    bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) {
        // Your platform I2C read implementation
    }
    bool SetAddressPins(bool a0, bool a1, bool a2) {
        return false; // Return false if address pins are hardwired
    }
};
```

### 2. Create the driver and use it

{% raw %}
```cpp
MyI2c i2c;

// Option A: Address pin levels (A0=LOW, A1=LOW, A2=LOW -> 0x20)
pcal95555::PCAL95555<MyI2c> gpio(&i2c, false, false, false);

// Option B: Direct I2C address
pcal95555::PCAL95555<MyI2c> gpio(&i2c, 0x20);

// Option C: Force chip variant (skip auto-detection)
pcal95555::PCAL95555<MyI2c> gpio(&i2c, 0x20, pcal95555::ChipVariant::PCA9555);

// Use the driver
gpio.SetPinDirection(0, GPIODir::Output);
gpio.WritePin(0, true);                  // Turn on pin 0
bool state = gpio.ReadPin(1);            // Read pin 1

// Check chip variant
if (gpio.HasAgileIO()) {
    gpio.SetDriveStrength(0, DriveStrength::Level3);
    gpio.SetPullEnable(1, true);
    gpio.SetPullDirection(1, true); // pull-up
}

// Multi-pin operations
gpio.WritePins({{0, true}, {1, false}, {2, true}});
gpio.SetDirections({{0, GPIODir::Output}, {1, GPIODir::Input}});
```
{% endraw %}

---

## 🔧 Installation

This is a **header-only template library** with implementation included via `#include`.

### Method 1: Copy into your project
```bash
cp -r inc/ /path/to/your/project/drivers/pcal95555/
cp -r src/ /path/to/your/project/drivers/pcal95555/
```

### Method 2: Git submodule
```bash
git submodule add https://github.com/hardfoc/hf-pcal95555-driver.git lib/pcal95555
```

### Method 3: ESP-IDF component (recommended for ESP32)
Add as a managed component or place in your project's `components/` directory.

### Requirements
- **C++17** or newer compiler
- An I2C peripheral driver for your platform

---

## 🔨 Building and Flashing (ESP32)

### Prerequisites
- [ESP-IDF v5.5+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- ESP32-S3 development board with PCA9555 or PCAL9555A connected via I2C

### Build

```bash
cd examples/esp32

# List available apps
./scripts/build_app.sh list

# Build the comprehensive test suite
./scripts/build_app.sh pcal95555_comprehensive_test Debug

# Build the LED animation demo
./scripts/build_app.sh pcal95555_led_animation Debug
```

### Flash and Monitor

```bash
# Flash and open serial monitor
./scripts/flash_app.sh flash_monitor pcal95555_comprehensive_test Debug

# Flash only
./scripts/flash_app.sh flash pcal95555_comprehensive_test Debug

# Monitor only (if already flashed)
./scripts/flash_app.sh monitor
```

> **Note (ESP32-S3 native USB):** If using the USB-SERIAL-JTAG port (`/dev/ttyACM0`),
> you may need to manually enter download mode: Hold **BOOT**, press **RESET**,
> release **BOOT**, then immediately run the flash command.

---

## 📖 API Reference

### Initialization

| Method | Description |
|--------|-------------|
| `PCAL95555(bus, a0, a1, a2, variant)` | Construct from address pin levels |
| `PCAL95555(bus, address, variant)` | Construct from I2C address (0x20-0x27) |
| `EnsureInitialized()` | Explicitly trigger lazy initialization |
| `ResetToDefault()` | Reset all registers to power-on defaults |
| `InitFromConfig()` | Initialize from Kconfig compile-time values |

### GPIO Direction

| Method | Description |
|--------|-------------|
| `SetPinDirection(pin, dir)` | Set single pin input/output |
| `SetMultipleDirections(mask, dir)` | Set direction by 16-bit mask |
| `SetDirections({...})` | Set mixed directions via initializer list |

### Pin Read / Write

| Method | Description |
|--------|-------------|
| `ReadPin(pin)` | Read single pin state |
| `WritePin(pin, value)` | Write single pin HIGH/LOW |
| `TogglePin(pin)` | Toggle single pin |
| `ReadPins({...})` | Read multiple pins at once |
| `WritePins({...})` | Write multiple pins at once |

### Pull Resistors (PCAL9555A only)

| Method | Description |
|--------|-------------|
| `SetPullEnable(pin, enable)` | Enable/disable pull resistor |
| `SetPullEnables({...})` | Multi-pin pull enable |
| `SetPullDirection(pin, pull_up)` | Select pull-up or pull-down |
| `SetPullDirections({...})` | Multi-pin pull direction |

### Drive Strength (PCAL9555A only)

| Method | Description |
|--------|-------------|
| `SetDriveStrength(pin, level)` | Set drive strength (Level0-Level3) |
| `SetDriveStrengths({...})` | Multi-pin drive strength |

### Interrupts

| Method | Description | PCAL9555A? |
|--------|-------------|------------|
| `ConfigureInterrupt(pin, state)` | Enable/disable per-pin interrupt | Yes |
| `ConfigureInterrupts({...})` | Multi-pin interrupt config | Yes |
| `ConfigureInterruptMask(mask)` | Set 16-bit interrupt mask | Yes |
| `GetInterruptStatus()` | Read and clear interrupt status | Yes |
| `RegisterPinInterrupt(pin, edge, cb)` | Register edge-triggered callback | No |
| `UnregisterPinInterrupt(pin)` | Remove pin callback | No |
| `SetInterruptCallback(cb)` | Register global interrupt callback | No |
| `RegisterInterruptHandler()` | Bind to hardware INT pin | No |
| `HandleInterrupt()` | Process interrupt (auto or manual) | No |

### Polarity

| Method | Description |
|--------|-------------|
| `SetPinPolarity(pin, polarity)` | Set input polarity (normal/inverted) |
| `SetMultiplePolarities(mask, pol)` | Set polarity by mask |
| `SetPolarities({...})` | Multi-pin polarity via initializer list |

### Input Latch (PCAL9555A only)

| Method | Description |
|--------|-------------|
| `EnableInputLatch(pin, enable)` | Enable/disable input latch |
| `EnableMultipleInputLatches(mask, en)` | Mask-based latch control |
| `EnableInputLatches({...})` | Multi-pin latch via initializer list |

### Output Mode (PCAL9555A only)

| Method | Description |
|--------|-------------|
| `SetOutputMode(p0_od, p1_od)` | Set push-pull or open-drain per port |

### Address Management

| Method | Description |
|--------|-------------|
| `GetAddress()` | Get current 7-bit I2C address |
| `GetAddressBits()` | Get A2-A0 bits (0-7) |
| `ChangeAddress(a0, a1, a2)` | Change address via pin levels |
| `ChangeAddress(address)` | Change address directly (0x20-0x27) |

### Error Handling

| Method | Description |
|--------|-------------|
| `GetErrorFlags()` | Get bitmask of active errors |
| `ClearErrorFlags(mask)` | Clear specific or all error flags |
| `SetRetries(n)` | Set I2C retry count |
| `HasAgileIO()` | Check if PCAL9555A features available |
| `GetChipVariant()` | Get detected variant enum |

### Error Flags

| Flag | Value | Meaning |
|------|-------|---------|
| `InvalidPin` | 0x0001 | Pin index >= 16 |
| `InvalidMask` | 0x0002 | Mask bits outside 0-15 |
| `I2CReadFail` | 0x0004 | I2C read transaction failed |
| `I2CWriteFail` | 0x0008 | I2C write transaction failed |
| `UnsupportedFeature` | 0x0010 | PCAL9555A feature called on PCA9555 |
| `InvalidAddress` | 0x0020 | I2C address outside valid 0x20-0x27 range |

---

## 📊 Examples

### Comprehensive Test Suite (`pcal95555_comprehensive_test`)

A thorough test of every public API method, organized into 17 test sections:

- Initialization (I2C bus, driver, chip variant detection)
- GPIO direction (single pin, multi-pin mask, initializer list)
- GPIO read/write (ReadPin, WritePin, TogglePin, WritePins, ReadPins)
- Pull resistor configuration (single + multi-pin, PCAL9555A)
- Drive strength (single + multi-pin, PCAL9555A)
- Output mode (push-pull / open-drain, PCAL9555A)
- Polarity inversion (single, mask, initializer list)
- Input latch (single, mask, initializer list, PCAL9555A)
- Interrupt configuration (mask, status, callbacks, handler, PCAL9555A)
- Port-level operations
- Multi-pin API tests (WritePins, ReadPins, SetDirections, SetPolarities)
- Address management (ChangeAddress, address-based constructor)
- Configuration (SetRetries, EnsureInitialized)
- Multi-pin PCAL9555A APIs (SetPullEnables, SetDriveStrengths, ConfigureInterrupts, etc.)
- Interactive input (button press test, disabled by default)
- Error handling (invalid pins, UnsupportedFeature, selective flag clearing)
- Stress tests (rapid pin toggling)

PCAL9555A-only tests are automatically **skipped** (not failed) on a standard PCA9555.

To enable the **interactive input test** (requires a physical button on pin 0):
```cpp
static constexpr bool ENABLE_INTERACTIVE_INPUT_TESTS = true;
```

### LED Animation Demo (`pcal95555_led_animation`)

A visual demo driving 16 LEDs through 10 animation patterns:

1. **Sequential Chase** -- single LED scans left/right
2. **Bounce** -- LED bounces between endpoints
3. **Binary Counter** -- counts 0-65535 in binary on LEDs
4. **Breathing (PWM)** -- software PWM fade-in/fade-out
5. **Wave / Comet Tail** -- 4-LED comet sweeps back and forth
6. **Random Sparkle** -- random LED patterns
7. **Build-up / Teardown** -- LEDs light up then extinguish one by one
8. **Accelerating Scan** -- speed ramps from 120ms to 1ms and back
9. **Center Expand / Contract** -- symmetric outward/inward animation
10. **Alternating Flash** -- port-vs-port and even-vs-odd flashing

Configure `LEDS_ACTIVE_LOW` for your wiring (common-anode vs common-cathode).

---

## 🧪 Testing

### Running on hardware

```bash
# Build and flash the test suite
cd examples/esp32
./scripts/build_app.sh pcal95555_comprehensive_test Debug
./scripts/flash_app.sh flash_monitor pcal95555_comprehensive_test Debug
```

### Expected output (PCA9555)

```
Detected chip variant: PCA9555 (standard)
Agile I/O support: NO

Total: 31, Passed: 31, Failed: 0, Success: 100.00%
ALL PCAL9555 TESTS PASSED!
```

PCAL9555A-only tests will show:
```
Skipping: Pull resistor config requires PCAL9555A (detected PCA9555)
```

### Expected output (PCAL9555A)

```
Detected chip variant: PCAL9555A (Agile I/O)
Agile I/O support: YES

Total: 31, Passed: 31, Failed: 0, Success: 100.00%
ALL PCAL9555 TESTS PASSED!
```

### Test configuration

Each test section can be individually enabled/disabled at the top of the test file:

```cpp
static constexpr bool ENABLE_INITIALIZATION_TESTS = true;
static constexpr bool ENABLE_GPIO_DIRECTION_TESTS = true;
// ... etc
static constexpr bool ENABLE_INTERACTIVE_INPUT_TESTS = false; // Requires button
```

---

## 🔌 Hardware Setup

### Minimal wiring (ESP32-S3 + PCA9555/PCAL9555A)

```
ESP32-S3              PCA9555 / PCAL9555A
─────────             ──────────────────
GPIO4 (SDA) ────────── SDA
GPIO5 (SCL) ────────── SCL
3.3V ───────────────── VDD
GND ────────────────── VSS (GND)
                       A0 ─── GND (or GPIO45)
                       A1 ─── GND (or GPIO48)
                       A2 ─── GND (or GPIO47)
```

**Pull-up resistors:** 4.7kΩ on SDA and SCL to 3.3V (or enable internal pull-ups).

**Address:** The I2C address is `0x20 + (A2<<2 | A1<<1 | A0)`. Default with all
address pins LOW is **0x20**.

### LED animation demo wiring

Connect LEDs with current-limiting resistors (220Ω-1kΩ) to each I/O pin.
For active-HIGH: LED anode to IO pin, cathode to GND.
For active-LOW: LED cathode to IO pin, anode to VDD through resistor.

### Interactive test wiring

Connect a momentary push-button between PCA9555 pin IO0_0 and GND.
For PCAL9555A, the internal pull-up is enabled automatically.
For PCA9555, add an external 10kΩ pull-up resistor to VDD.

---

## 🛠️ Troubleshooting

### "I2C transaction unexpected nack detected"

| Symptom | Cause | Fix |
|---------|-------|-----|
| NACK on all registers | Wrong I2C address or chip not powered | Verify wiring, address pins, power |
| NACK on 0x40-0x4F only | Standard PCA9555 (no Agile I/O) | Expected behavior -- driver auto-detects |
| Intermittent NACKs | Bus noise, missing pull-ups | Add 4.7kΩ pull-ups, reduce bus speed |

### Stack overflow on ESP32

The default `main_task` stack may be too small for the test suite (extensive logging).
Set in `sdkconfig`:
```
CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384
```

### ESP32-S3 flashing fails

If using native USB (`/dev/ttyACM0`), the auto-reset circuit may not work reliably.
Manually enter download mode: Hold **BOOT** → Press **RESET** → Release **BOOT** →
Run flash command immediately.

### ChipVariant shows "Unknown"

This means the 3-step detection could not confirm the chip type (bus error during
probe). Check wiring, pull-ups, and power supply.

---

## 🔗 References

| Resource | Link |
|----------|------|
| NXP PCA9555 product page | <https://www.nxp.com/products/PCA9555> |
| NXP PCAL9555A product page | <https://www.nxp.com/products/PCAL9555A> |
| PCA9555 datasheet (NXP) | <https://www.nxp.com/docs/en/data-sheet/PCA9555.pdf> |
| PCAL9555A datasheet (NXP) | <https://www.nxp.com/docs/en/data-sheet/PCAL9555A.pdf> |
| ESP-IDF I²C master | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html> |
| C++17 language reference | <https://en.cppreference.com/w/cpp/17> |

## 🤝 Contributing

Pull requests and suggestions are welcome! Please:

1. Follow the existing code style (clang-format, Doxygen comments)
2. Add tests for new features in the comprehensive test suite
3. Update documentation for any API changes
4. Ensure the build passes for both `Debug` and `Release` configurations

---

## 📄 License

This project is licensed under the **GNU General Public License v3.0**.
See the [LICENSE](LICENSE) file for details.
