# PCA9555 / PCAL9555A ESP32-S3 Examples

This directory contains example applications for the HF-PCAL95555 driver on ESP32-S3,
covering both the standard **PCA9555** and extended **PCAL9555A** 16-bit I/O expanders.

---

## Table of Contents

- [Hardware Overview](#hardware-overview)
- [Pin Connections](#pin-connections)
- [Available Applications](#available-applications)
- [Building](#building)
- [Flashing and Monitoring](#flashing-and-monitoring)
- [Comprehensive Test Suite](#comprehensive-test-suite)
- [LED Animation Demo](#led-animation-demo)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Hardware Overview

### ESP32-S3

The ESP32-S3 serves as the host controller for communicating with the PCA9555 /
PCAL9555A GPIO expander via I2C.

```
ESP32-S3 Development Board
├── I2C:      SDA (GPIO4), SCL (GPIO5)
├── INT:      GPIO7 (open-drain, needs pull-up)
├── Address:  A0 (GPIO45), A1 (GPIO48), A2 (GPIO47)
└── Indicator: GPIO14 (test progress LED)
```

### PCA9555 / PCAL9555A

Both chips provide 16 GPIO pins in two 8-bit ports over I2C. The driver auto-detects
which variant is connected and enables features accordingly:

| Feature | PCA9555 | PCAL9555A |
|---------|---------|-----------|
| 16 GPIO, I2C, polarity inversion | Yes | Yes |
| Pull-up / pull-down resistors | -- | Yes |
| Drive strength, input latch | -- | Yes |
| Interrupt mask / status | -- | Yes |
| Output mode (push-pull / OD) | -- | Yes |

---

## Pin Connections

### I2C Bus

| Chip Pin | ESP32-S3 GPIO | Notes |
|----------|---------------|-------|
| SDA | GPIO4 | 4.7k pull-up to 3.3V |
| SCL | GPIO5 | 4.7k pull-up to 3.3V |
| VDD | 3.3V | |
| GND | GND | |
| INT | GPIO7 | Open-drain, 4.7k pull-up (optional) |

### Address Pins (GPIO-controlled)

| Chip Pin | ESP32-S3 GPIO | Default |
|----------|---------------|---------|
| A0 | GPIO45 | LOW |
| A1 | GPIO48 | LOW |
| A2 | GPIO47 | LOW |

Default address: **0x20** (all LOW)

### I2C Address Table

| A2 | A1 | A0 | Address |
|----|----|----|---------|
| L | L | L | 0x20 |
| L | L | H | 0x21 |
| L | H | L | 0x22 |
| L | H | H | 0x23 |
| H | L | L | 0x24 |
| H | L | H | 0x25 |
| H | H | L | 0x26 |
| H | H | H | 0x27 |

---

## Available Applications

| Application | Description |
|-------------|-------------|
| `pcal95555_comprehensive_test` | Full API test suite (17 sections, 43 methods) |
| `pcal95555_led_animation` | 16-LED animation demo (10 patterns) |

```bash
# List all available apps
./scripts/build_app.sh list
```

---

## Building

### Prerequisites

- [ESP-IDF v5.5+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- ESP32-S3 devkit
- PCA9555 or PCAL9555A board connected via I2C

### Build Commands

```bash
cd examples/esp32

# Build comprehensive test suite
./scripts/build_app.sh pcal95555_comprehensive_test Debug

# Build LED animation demo
./scripts/build_app.sh pcal95555_led_animation Debug
```

---

## Flashing and Monitoring

```bash
# Flash and open serial monitor
./scripts/flash_app.sh flash_monitor pcal95555_comprehensive_test Debug

# Flash only
./scripts/flash_app.sh flash pcal95555_comprehensive_test Debug

# Monitor only (if already flashed)
./scripts/flash_app.sh monitor
```

> **ESP32-S3 native USB note:** If flashing fails on `/dev/ttyACM0`, manually enter
> download mode: Hold **BOOT** -> Press **RESET** -> Release **BOOT** -> Run flash.

---

## Comprehensive Test Suite

**File:** `main/pcal95555_comprehensive_test.cpp`

Tests every public API method of the PCAL95555 driver class across 17 sections.
PCAL9555A-specific tests are **automatically skipped** (not failed) on a standard PCA9555.

### Test Sections

| # | Section | Methods Tested | PCAL9555A? |
|---|---------|----------------|------------|
| 1 | Initialization | Constructor, ResetToDefault, auto-detection | No |
| 2 | GPIO Direction (single) | SetPinDirection | No |
| 3 | GPIO Direction (multi) | SetMultipleDirections | No |
| 4 | Pin Write | WritePin | No |
| 5 | Pin Read | ReadPin | No |
| 6 | Pin Toggle | TogglePin | No |
| 7 | Pull Resistor | SetPullEnable, SetPullDirection | Yes |
| 8 | Drive Strength | SetDriveStrength | Yes |
| 9 | Output Mode | SetOutputMode | Yes |
| 10 | Polarity | SetPinPolarity, SetMultiplePolarities | No |
| 11 | Input Latch | EnableInputLatch, EnableMultipleInputLatches | Yes |
| 12 | Interrupt Config | ConfigureInterruptMask, GetInterruptStatus, callbacks | Yes |
| 13 | Port Operations | Mixed port direction + read/write | No |
| 14 | Multi-Pin APIs | WritePins, ReadPins, SetDirections, SetPolarities | No |
| 15 | Address Management | ChangeAddress, address-based constructor | No |
| 16 | Configuration | SetRetries, EnsureInitialized | No |
| 17 | Multi-Pin PCAL | SetPullEnables, SetDriveStrengths, ConfigureInterrupt/s, EnableInputLatches | Yes |
| 18 | Interactive Input | Button press, HandleInterrupt (disabled by default) | No |
| 19 | Error Handling | Invalid pins, UnsupportedFeature, flag clearing | No |
| 20 | Stress Tests | Rapid pin toggling | No |

### Enable/Disable Sections

```cpp
// At top of pcal95555_comprehensive_test.cpp
static constexpr bool ENABLE_INITIALIZATION_TESTS = true;
static constexpr bool ENABLE_GPIO_DIRECTION_TESTS = true;
static constexpr bool ENABLE_INTERACTIVE_INPUT_TESTS = false; // Requires button on pin 0
// ... etc
```

### Interactive Input Test

Disabled by default. Requires a momentary push-button between PCA9555 pin 0 and GND:

- **PCAL9555A:** Internal pull-up is enabled automatically
- **PCA9555:** Add an external 10k pull-up resistor to VDD

The test waits 10 seconds for a button press, logging a countdown. If no button is
connected, it times out gracefully without failing.

### Expected Output (PCA9555)

```
Detected chip variant: PCA9555 (standard)
Agile I/O support: NO
...
Skipping: Pull resistor config requires PCAL9555A (detected PCA9555)
...
Total: 31, Passed: 31, Failed: 0, Success: 100.00%
```

---

## LED Animation Demo

**File:** `main/pcal95555_led_animation.cpp`

Drives 16 LEDs through 10 animation patterns to visually test the driver's output
capabilities and I2C throughput. Uses only PCA9555-compatible registers, so it works
with both chip variants.

### Animation Patterns

1. **Sequential Chase** -- single LED scans left to right and back
2. **Bounce** -- LED bounces between endpoints with acceleration
3. **Binary Counter** -- counts 0-65535 showing binary on 16 LEDs
4. **Breathing (PWM)** -- software PWM fade-in/fade-out of all LEDs
5. **Wave / Comet Tail** -- 4-LED comet sweeps back and forth
6. **Random Sparkle** -- random LED patterns at fast rate
7. **Build-up / Teardown** -- LEDs light sequentially then extinguish
8. **Accelerating Scan** -- speed ramps from 120ms down to 1ms and back
9. **Center Expand** -- symmetric outward/inward animation from center
10. **Alternating Flash** -- port-vs-port and even-vs-odd flashing

### LED Wiring

Connect LEDs with 220-1k current-limiting resistors to each I/O pin.

- **Active-HIGH (default):** LED anode to IO pin, cathode to GND
- **Active-LOW:** Set `LEDS_ACTIVE_LOW = true` in the source, LED cathode to IO pin, anode to VDD

### Configuration

Edit at top of `pcal95555_led_animation.cpp`:

```cpp
static constexpr uint16_t NUM_PINS = 16;
static constexpr bool LEDS_ACTIVE_LOW = false;
static constexpr int NUM_CYCLES = 0; // 0 = infinite
```

---

## Configuration

### I2C Bus

Default configuration in the bus header (`esp32_pcal95555_bus.hpp`):

| Parameter | Default | Notes |
|-----------|---------|-------|
| I2C Port | I2C_NUM_0 | |
| SDA | GPIO4 | |
| SCL | GPIO5 | |
| Frequency | 400 kHz | Reduce to 100k for long wires |
| Internal Pull-ups | Enabled | External 4.7k recommended |

### Stack Size

The test suite requires a larger-than-default main task stack due to extensive logging.
Set in `sdkconfig`:
```
CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384
```

---

## Troubleshooting

### I2C NACK Errors

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| NACK on all registers | Wrong address / no power | Check wiring, verify address |
| NACK on 0x40-0x4F only | PCA9555 (not PCAL9555A) | Expected -- driver auto-detects |
| Intermittent NACKs | Bus noise, missing pull-ups | Add 4.7k pull-ups |

### Stack Overflow / Assert

```
assert failed: xTaskRemoveFromEventList tasks.c
```
Increase stack size: `CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384` in sdkconfig.

### ESP32-S3 Flash Failure

```
Failed to connect to ESP32-S3: Invalid head of packet (0x1B)
```
Manually enter download mode (BOOT + RESET) before flashing.

### ChipVariant "Unknown"

The 3-step detection could not confirm chip type. Check:
- I2C pull-ups present
- Chip powered and wired correctly
- No other I2C master on bus

---

## Additional Resources

- [Main README](../../README.md) -- Full project documentation
- [PCAL9555A datasheet](../../docs/datasheet/pcal9555a.pdf), [PCA9555 datasheet](../../docs/datasheet/pca9555.pdf)
- [API guide](../../docs/api-guide.md)
- [Porting](../../docs/porting.md)
- [Build Scripts README](scripts/README.md)
