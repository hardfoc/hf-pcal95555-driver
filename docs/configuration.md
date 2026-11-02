# Configuration

This guide covers all configuration options available for the PCAL95555 driver.

## I2C Address Configuration

The PCA9555 / PCAL9555A I2C address is configured via hardware pins A0-A2. Each pin represents one bit of the 3-bit address field. Both chip variants use the same address scheme.

| Pin | Address Bit | Description |
|-----|-------------|-------------|
| A0  | Bit 0       | Least significant address bit |
| A1  | Bit 1       | |
| A2  | Bit 2       | Most significant address bit |

**Default I2C address**: `0x20` (all address pins to GND)

**Address Range**: 0x20 to 0x27 (7-bit I2C addresses)

### Address Pin Configuration Table

| A2 | A1 | A0 | I2C Address (7-bit) | Constructor Call |
|----|----|----|---------------------|------------------|
| LOW | LOW | LOW | 0x20 (default) | `PCAL95555(bus, false, false, false)` |
| LOW | LOW | HIGH | 0x21 | `PCAL95555(bus, true, false, false)` |
| LOW | HIGH | LOW | 0x22 | `PCAL95555(bus, false, true, false)` |
| LOW | HIGH | HIGH | 0x23 | `PCAL95555(bus, true, true, false)` |
| HIGH | LOW | LOW | 0x24 | `PCAL95555(bus, false, false, true)` |
| HIGH | LOW | HIGH | 0x25 | `PCAL95555(bus, true, false, true)` |
| HIGH | HIGH | LOW | 0x26 | `PCAL95555(bus, false, true, true)` |
| HIGH | HIGH | HIGH | 0x27 | `PCAL95555(bus, true, true, true)` |

### Dynamic Address Changes

If your I2C interface supports GPIO control of address pins (via `SetAddressPins()`), you can change the address dynamically:

**Option 1: Using address directly (recommended)**
```cpp
// Change address to 0x21
if (gpio.ChangeAddress(0x21)) {
    printf("Address changed to 0x%02X\n", gpio.GetAddress()); // 0x21
}

// Change address to 0x25
if (gpio.ChangeAddress(0x25)) {
    printf("Address changed to 0x%02X\n", gpio.GetAddress()); // 0x25
}
```

**Option 2: Using pin levels**
```cpp
// Change address to 0x21 (A0=HIGH, A1=LOW, A2=LOW)
if (gpio.ChangeAddress(true, false, false)) {
    printf("Address changed to 0x%02X\n", gpio.GetAddress()); // 0x21
}
```

**Get current address and address bits:**
```cpp
uint8_t addr = gpio.GetAddress();      // Returns current address (e.g., 0x21)
uint8_t bits = gpio.GetAddressBits();  // Returns address bits (e.g., 1 = binary 001)
```

**Note**: Dynamic address changes require hardware support. If address pins are hardwired, `ChangeAddress()` will still update internal state but won't change hardware pins.

## Kconfig Configuration (Optional)

If your project uses Kconfig (e.g., ESP-IDF), the driver supports compile-time configuration.

The Kconfig compile-time macros are defined in a dedicated header: [`inc/pcal95555_kconfig.hpp`](../inc/pcal95555_kconfig.hpp). This file is automatically included by the main `pcal95555.hpp` header -- no additional includes are needed.

**Kconfig file**: [`Kconfig`](../Kconfig)

### Configuration Options

- **Per-pin direction**: Configure each pin as input (1) or output (0)
- **Per-pin pull-up/pull-down**: Enable pull resistors and select direction
- **Per-pin initial output**: Set initial output state
- **Port open-drain**: Configure ports for open-drain or push-pull mode

### Using Kconfig

1. Run `idf.py menuconfig` (or your Kconfig tool)
2. Navigate to component configuration
3. Configure per-pin settings as needed
4. Use `InitFromConfig()` to apply settings:

```cpp
gpio.InitFromConfig(); // Apply Kconfig settings
```

## Runtime Configuration

### Pin Direction

Set individual pin direction:

```cpp
// Set pin 0 as output
gpio.SetPinDirection(0, pcal95555::PCAL95555<MyI2c>::GPIODir::Output);

// Set pin 1 as input
gpio.SetPinDirection(1, pcal95555::PCAL95555<MyI2c>::GPIODir::Input);

// Set multiple pins at once
gpio.SetMultipleDirections(0x000F, pcal95555::PCAL95555<MyI2c>::GPIODir::Output); // Pins 0-3
```

### Pull Resistors (PCAL9555A only)

> **Note**: These features require the PCAL9555A. On a standard PCA9555, these methods
> return `false` and set `Error::UnsupportedFeature`. Check with `HasAgileIO()` first.

```cpp
if (gpio.HasAgileIO()) {
    // Enable pull-up on pin 0
    gpio.SetPullEnable(0, true);
    gpio.SetPullDirection(0, true); // true = pull-up, false = pull-down

    // Enable pull-down on pin 1
    gpio.SetPullEnable(1, true);
    gpio.SetPullDirection(1, false);
}
```

### Drive Strength (PCAL9555A only)

> **Note**: Requires PCAL9555A. Returns `false` with `Error::UnsupportedFeature` on PCA9555.

```cpp
gpio.SetDriveStrength(0, pcal95555::PCAL95555<MyI2c>::DriveStrength::Level3); // Full strength
```

**Drive Strength Levels**:
- `Level0`: 25% drive strength (¼)
- `Level1`: 50% drive strength (½)
- `Level2`: 75% drive strength (¾)
- `Level3`: 100% drive strength (full)

### Output Mode (PCAL9555A only)

> **Note**: Requires PCAL9555A. Returns `false` with `Error::UnsupportedFeature` on PCA9555.

```cpp
// Set PORT_0 to open-drain, PORT_1 to push-pull
gpio.SetOutputMode(true, false);
```

### Polarity Inversion

Invert input polarity:

```cpp
// Invert pin 0 polarity
gpio.SetPinPolarity(0, pcal95555::PCAL95555<MyI2c>::Polarity::Inverted);

// Invert multiple pins
gpio.SetMultiplePolarities(0x00FF, pcal95555::PCAL95555<MyI2c>::Polarity::Inverted);
```

## Default Configuration

### ResetToDefault()

Resets all registers to power-on defaults:

```cpp
gpio.ResetToDefault();
```

**Default State**:
- All pins: Inputs with pull-ups enabled
- Drive strength: Full
- Output mode: Push-pull
- Interrupts: Masked (disabled)
- Polarity: Normal (not inverted)

**Location**: [`src/pcal95555.ipp`](../src/pcal95555.ipp)

## Chip Variant Configuration

The driver auto-detects the chip variant (PCA9555 vs PCAL9555A) during initialization.
You can also force a specific variant via the constructor to skip the detection probe:

```cpp
// Auto-detect (default)
pcal95555::PCAL95555<MyI2c> gpio(&i2c, 0x20);

// Force PCA9555 mode (skips Agile I/O probe -- avoids NACK on known PCA9555)
pcal95555::PCAL95555<MyI2c> gpio(&i2c, 0x20, pcal95555::ChipVariant::PCA9555);

// Force PCAL9555A mode
pcal95555::PCAL95555<MyI2c> gpio(&i2c, 0x20, pcal95555::ChipVariant::PCAL9555A);
```

Query the detected variant at runtime:
```cpp
if (gpio.HasAgileIO()) {
    // PCAL9555A features available
}
auto variant = gpio.GetChipVariant();
// ChipVariant::Unknown, ChipVariant::PCA9555, or ChipVariant::PCAL9555A
```

## Interrupt Configuration

### Basic Interrupt Setup

```cpp
// Enable interrupt on pin 5 (recommended method)
gpio.ConfigureInterrupt(5, InterruptState::Enabled);

// Disable interrupt on pin 3
gpio.ConfigureInterrupt(3, InterruptState::Disabled);

// Configure multiple pins at once
gpio.ConfigureInterrupts({
    {0, InterruptState::Enabled},
    {5, InterruptState::Enabled},
    {10, InterruptState::Enabled},
    {3, InterruptState::Disabled}
});

// Set global interrupt callback (optional)
gpio.SetInterruptCallback([](uint16_t status) {
    printf("Interrupt on pins: 0x%04X\n", status);
});

// Handle interrupt (call from ISR or polling)
gpio.HandleInterrupt();
```

### Per-Pin Interrupt Callbacks

```cpp
// Register callback for specific pin with edge detection
gpio.RegisterPinInterrupt(5, InterruptEdge::Rising, [](uint16_t pin, bool state) {
    printf("Pin %d went HIGH\n", pin);
});

// Register callback for falling edge
gpio.RegisterPinInterrupt(3, InterruptEdge::Falling, [](uint16_t pin, bool state) {
    printf("Pin %d went LOW\n", pin);
});

// Register callback for both edges
gpio.RegisterPinInterrupt(7, InterruptEdge::Both, [](uint16_t pin, bool state) {
    printf("Pin %d changed to %s\n", pin, state ? "HIGH" : "LOW");
});

// Register interrupt handler with I2C interface (for hardware interrupts)
gpio.RegisterInterruptHandler(); // Sets up INT pin handling
```

### Get Interrupt Status

```cpp
uint16_t status = gpio.GetInterruptStatus();
// Check which pins triggered interrupt
// Reading this register clears the interrupt condition
```

## Recommended Settings

### For Input Pins

```cpp
gpio.SetPinDirection(pin, pcal95555::PCAL95555<MyI2c>::GPIODir::Input);
gpio.SetPullEnable(pin, true);
gpio.SetPullDirection(pin, true); // Pull-up
```

### For Output Pins

```cpp
gpio.SetPinDirection(pin, pcal95555::PCAL95555<MyI2c>::GPIODir::Output);
gpio.SetDriveStrength(pin, pcal95555::PCAL95555<MyI2c>::DriveStrength::Level3); // Full strength
gpio.WritePin(pin, false); // Set initial state
```

### For Interrupt-Driven Inputs

```cpp
gpio.SetPinDirection(pin, pcal95555::PCAL95555<MyI2c>::GPIODir::Input);
gpio.SetPullEnable(pin, true);
gpio.SetPullDirection(pin, true);
gpio.EnableInputLatch(pin, true); // Latch input changes
gpio.ConfigureInterrupt(pin, InterruptState::Enabled); // Enable interrupt for this pin
```

## Next Steps

- See [Examples](examples.md) for configuration examples
- Review [API Reference](api_reference.md) for all configuration methods

---

**Navigation**
⬅️ [Platform Integration](platform_integration.md) | [Next: API Reference ➡️](api_reference.md) | [Back to Index](index.md)

