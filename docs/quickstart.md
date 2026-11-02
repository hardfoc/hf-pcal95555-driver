# Quick Start

This guide will get you up and running with the PCAL95555 driver in just a few steps.

## Prerequisites

- [Driver installed](installation.md)
- [Hardware wired](hardware_setup.md)
- [I2C interface implemented](platform_integration.md)

## Minimal Example

Here's a complete working example:

```cpp
#include "pcal95555.hpp"

// 1. Implement the I2C interface
class MyI2c : public pcal95555::I2cInterface<MyI2c> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        // Your I2C write implementation
        return true;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        // Your I2C read implementation
        return true;
    }
};

// 2. Create instances
MyI2c i2c;
// Option 1: Using address directly (recommended)
pcal95555::PCAL95555<MyI2c> gpio(&i2c, 0x20); // Address 0x20 (default)

// Option 2: Using pin levels
// pcal95555::PCAL95555<MyI2c> gpio(&i2c, false, false, false); // A0=LOW, A1=LOW, A2=LOW -> 0x20

// Option 3: Force chip variant (skip auto-detection)
// pcal95555::PCAL95555<MyI2c> gpio(&i2c, 0x20, pcal95555::ChipVariant::PCA9555);

// 3. Initialize (lazy initialization - happens automatically on first use)
// You can also explicitly initialize:
if (!gpio.EnsureInitialized()) {
    // Handle initialization failure
    return;
}

// Reset to default state
gpio.ResetToDefault(); // all pins become inputs with pull-ups

// 4. Check chip variant (auto-detected during init)
if (gpio.HasAgileIO()) {
    // PCAL9555A detected -- pull resistors, drive strength, etc. available
} else {
    // PCA9555 detected -- standard GPIO only
}

// 5. Configure and use
gpio.SetPinDirection(0, pcal95555::PCAL95555<MyI2c>::GPIODir::Output);
gpio.WritePin(0, true);
bool input = gpio.ReadPin(1);
```

## Step-by-Step Explanation

### Step 1: Include the Header

```cpp
#include "pcal95555.hpp"
```

### Step 2: Implement the I2C Interface

You need to implement the `I2cInterface` for your platform. See [Platform Integration](platform_integration.md) for detailed examples for ESP32, STM32, and Arduino.

The interface requires two methods:
- `Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)` - Write data to a register
- `Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)` - Read data from a register

### Step 3: Create Driver Instance

You can create the driver instance in two ways:

**Option 1: Using I2C address directly (recommended)**
```cpp
MyI2c i2c;
pcal95555::PCAL95555<MyI2c> gpio(&i2c, 0x20); // Address 0x20 (default)
```

**Option 2: Using address pin levels**
```cpp
MyI2c i2c;
// Constructor takes A2, A1, A0 pin levels (all LOW = address 0x20, default)
pcal95555::PCAL95555<MyI2c> gpio(&i2c, false, false, false);
```

**Constructor Parameters:**

**Using address (Option 1):**
- Pointer to your I2C interface implementation
- I2C address (0x20 to 0x27). Address bits are calculated automatically.

**Using pin levels (Option 2):**
- Pointer to your I2C interface implementation
- A0 pin level (true = HIGH/VDD, false = LOW/GND)
- A1 pin level (true = HIGH/VDD, false = LOW/GND)
- A2 pin level (true = HIGH/VDD, false = LOW/GND)

The I2C address is calculated automatically: base address 0x20 + (A2<<2 | A1<<1 | A0). For example:
- Address 0x20 → A0=LOW, A1=LOW, A2=LOW (default)
- Address 0x21 → A0=HIGH, A1=LOW, A2=LOW

### Step 4: Initialize

The driver uses **lazy initialization** - it initializes automatically on first use. However, you can explicitly initialize:

```cpp
// Explicit initialization (optional)
if (!gpio.EnsureInitialized()) {
    // Handle initialization failure
    return;
}

// Reset to default state
gpio.ResetToDefault();
```

**Lazy Initialization:**
- The driver initializes automatically when you call any method that requires I2C communication
- Initialization includes setting address pins, verifying I2C communication, **auto-detecting the chip variant** (PCA9555 vs PCAL9555A), and initializing internal state
- If initialization fails, methods return `false` or appropriate error values
- You can call `EnsureInitialized()` explicitly to verify initialization before use

**Chip Variant Detection:**
- The driver automatically probes the Agile I/O register bank to determine if the chip is a PCA9555 or PCAL9555A
- Use `HasAgileIO()` to check if PCAL9555A features are available
- Use `GetChipVariant()` to get the specific variant enum
- PCAL9555A-only methods return `false` and set `Error::UnsupportedFeature` on a PCA9555

`ResetToDefault()` puts the device in a known state (all pins as inputs with pull-ups enabled on PCAL9555A).

### Step 5: Configure and Use Pins

```cpp
// Set pin 0 as output
gpio.SetPinDirection(0, pcal95555::PCAL95555<MyI2c>::GPIODir::Output);

// Write to pin 0
gpio.WritePin(0, true);

// Read from pin 1 (input)
bool value = gpio.ReadPin(1);
```

## Expected Output

When running this example:
- Pin 0 should be set high (if connected to an LED, it should light up)
- Pin 1 value should be read correctly
- No error messages should appear

## Troubleshooting

If you encounter issues:

- **Compilation errors**: Check that you've implemented all required I2C interface methods
- **Initialization fails**: Verify hardware connections and I2C address
- **No response**: Check I2C bus and pull-up resistors
- **See**: [Troubleshooting](troubleshooting.md) for common issues

## Next Steps

- Explore [Examples](examples.md) for more advanced usage
- Review the [API Reference](api_reference.md) for all available methods
- Check [Configuration](configuration.md) for customization options

---

**Navigation**
⬅️ [Installation](installation.md) | [Next: Hardware Setup ➡️](hardware_setup.md) | [Back to Index](index.md)

