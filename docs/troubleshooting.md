# Troubleshooting

This guide helps you diagnose and resolve common issues when using the PCAL95555 driver.

## Common Error Messages

### Error: Device Not Detected

**Symptoms:**
- `ResetToDefault()` or other operations fail
- No response from device

**Causes:**
- Wrong I2C address
- Hardware connections incorrect
- Pull-up resistors missing

**Solutions:**
1. **Check I2C address**:
   - Default is 0x20 (all A0-A2 pins to GND)
   - Use I2C scanner to verify device address
   - Update address pin levels in constructor if different:
     ```cpp
     // For address 0x21: A0=HIGH, A1=LOW, A2=LOW
     PCAL95555 driver(bus, true, false, false);
     ```
   - Use `GetAddress()` to verify current address:
     ```cpp
     uint8_t addr = driver.GetAddress();
     printf("Current address: 0x%02X\n", addr);
     ```

2. **Verify hardware connections**:
   - Check SDA/SCL connections
   - Verify 4.7kΩ pull-up resistors on SCL and SDA
   - Ensure power connections (VDD and GND)

3. **Test I2C bus**:
   - Verify I2C bus is properly initialized
   - Check I2C bus speed (try 100 kHz if 400 kHz fails)

---

### Error: Invalid Pin

**Symptoms:**
- Methods return `false`
- Pin operations fail

**Causes:**
- Pin number >= 16
- Invalid pin parameter

**Solutions:**
```cpp
// Valid pins are 0-15
if (pin < 16) {
    gpio.SetPinDirection(pin, dir);
}
```

---

### Error: I2C Communication Failures

**Symptoms:**
- Operations fail intermittently
- Timeout errors

**Solutions:**
1. **Increase retries**:
   ```cpp
   gpio.SetRetries(3); // Default is 0
   ```

2. **Check bus speed**: Reduce I2C speed if using long wires

3. **Verify signal integrity**: Check for noise on I2C lines

---

## Hardware Issues

### Device Not Responding

**Symptoms:**
- No response to I2C commands
- Initialization fails

**Checklist:**
- [ ] Verify power supply voltage (1.65V-5.5V)
- [ ] Check all connections are secure
- [ ] Verify pull-up resistors (4.7kΩ) on SCL and SDA
- [ ] Check I2C address configuration (A0-A2 pins)
- [ ] Use I2C scanner to detect device address
- [ ] Verify ground connection

---

### Incorrect Pin States

**Symptoms:**
- Pins don't respond to writes
- Reads return wrong values

**Checklist:**
- [ ] Verify pin direction is set correctly
- [ ] Check pin is configured as output before writing
- [ ] Verify pull resistors are configured if needed
- [ ] Check for external loads affecting pin state

---

## Software Issues

### Compilation Errors

**Error: "No matching function"**

**Solution:**
- Ensure you've implemented all required `I2cInterface` methods
- Check method signatures match exactly

**Error: "Undefined reference"**

**Solution:**
- Verify you're including the header file
- Check include paths are correct

---

### Runtime Errors

**Initialization Fails**

**Checklist:**
- [ ] I2C interface is properly implemented
- [ ] I2C bus is initialized
- [ ] Hardware connections are correct
- [ ] I2C address matches hardware configuration

**Pin Operations Fail**

**Checklist:**
- [ ] Pin number is valid (0-15)
- [ ] Pin direction is set correctly
- [ ] I2C communication is working
- [ ] Error flags are checked

---

## Debugging Tips

### Enable Debug Output

Add debug prints to your I2C interface:

```cpp
bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
    printf("I2C Write: addr=0x%02X, reg=0x%02X, len=%zu\n", addr, reg, len);
    // ... your implementation
    bool result = /* ... */;
    printf("Result: %s\n", result ? "OK" : "FAIL");
    return result;
}
```

### Check Error Flags

```cpp
gpio.ClearErrorFlags(); // Clear all errors
// Perform operation
// Check if errors occurred
```

### Use I2C Scanner

Scan the I2C bus to verify device detection:

```cpp
void i2c_scanner() {
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        // Try to communicate with address
        if (/* device responds */) {
            printf("Device found at 0x%02X\n", addr);
        }
    }
}
```

---

### Error: Invalid Address (InvalidAddress)

**Symptoms:**
- Constructor or `ChangeAddress()` sets `Error::InvalidAddress` (flag `0x0020`)
- Methods fail after providing an out-of-range address

**Cause:**
The I2C address provided is outside the valid range **0x20-0x27**. The PCA9555/PCAL9555A
only responds to addresses in this range (determined by the A0-A2 hardware pins).

**Solutions:**
1. **Check address parameter**: Ensure the address is between `0x20` and `0x27`:
   ```cpp
   // Valid: address in 0x20-0x27 range
   PCAL95555 driver(bus, 0x20);

   // Invalid: will set Error::InvalidAddress
   PCAL95555 driver(bus, 0x50);
   ```
2. **Use pin-level constructor**: The pin-level constructor always computes a valid address:
   ```cpp
   PCAL95555 driver(bus, false, false, false); // Always valid (0x20)
   ```
3. **Check error flags**: After construction, verify no errors:
   ```cpp
   if (driver.GetErrorFlags() & static_cast<uint16_t>(Error::InvalidAddress)) {
       printf("Invalid I2C address!\n");
   }
   ```

---

### Error: PCAL9555A Features Fail (UnsupportedFeature)

**Symptoms:**
- `SetPullEnable()`, `SetDriveStrength()`, `SetOutputMode()`, `EnableInputLatch()`, `ConfigureInterrupt()`, or `GetInterruptStatus()` return `false`
- Error flags show `0x0010` (`UnsupportedFeature`)

**Cause:**
The connected chip is a **PCA9555** (standard variant), not a PCAL9555A. These methods
require the Agile I/O register bank (0x40-0x4F) that only exists on the PCAL9555A.

**Solutions:**
1. **Check chip variant**: Use `HasAgileIO()` before calling PCAL9555A features:
   ```cpp
   if (gpio.HasAgileIO()) {
       gpio.SetDriveStrength(0, DriveStrength::Level2);
   } else {
       // Use external components or skip this feature
   }
   ```
2. **Verify hardware**: Confirm your chip is actually a PCAL9555A (check part number marking)
3. **Use external components**: For PCA9555, use external pull-up resistors instead of internal ones

---

### Error: ChipVariant Shows "Unknown"

**Symptoms:**
- `GetChipVariant()` returns `ChipVariant::Unknown`
- `HasAgileIO()` returns `false`

**Cause:**
The 3-step chip detection probe could not confirm the chip type. This usually means
the bus was unhealthy during initialization.

**Solutions:**
1. Check I2C pull-up resistors (4.7kΩ on SDA and SCL)
2. Verify power supply is stable
3. Ensure no other I2C master is on the bus
4. Force the variant in the constructor to skip detection:
   ```cpp
   PCAL95555 driver(bus, 0x20, pcal95555::ChipVariant::PCA9555);
   ```

---

### Error: I2C NACKs on Registers 0x40-0x4F During Init

**Symptoms:**
- Log shows "I2C transaction unexpected nack detected" for registers 0x40-0x4F
- Driver still initializes successfully

**Cause:**
This is **expected behavior** when a PCA9555 is connected. The auto-detection probe
tries to access an Agile I/O register (0x4F). A NACK means the chip is a standard PCA9555.
The driver detects this and sets `ChipVariant::PCA9555`.

**Resolution:** No action needed. This is normal. The NACK errors come from the ESP-IDF
I2C driver and can be safely ignored.

---

## FAQ

### Q: Why do my pin writes not work?

**A:** Common causes:
1. Pin not configured as output: Call `SetPinDirection(pin, GPIODir::Output)` first
2. Wrong pin number: Valid pins are 0-15
3. I2C communication failure: Check I2C bus and connections

### Q: How do I use interrupts?

**A:** There are two ways to handle interrupts:

**Method 1: Per-pin callbacks (recommended)**
```cpp
// Configure pin as input
gpio.SetPinDirection(pin, GPIODir::Input);
gpio.SetPullEnable(pin, true);
gpio.EnableInputLatch(pin, true);

// Enable interrupt (easy-to-use method)
gpio.ConfigureInterrupt(pin, InterruptState::Enabled);

// Register per-pin callback
gpio.RegisterPinInterrupt(pin, InterruptEdge::Rising, [](uint16_t p, bool state) {
    printf("Pin %d interrupt!\n", p);
});

// Register interrupt handler with I2C interface (for hardware INT pin)
gpio.RegisterInterruptHandler();

// Handle interrupt (called automatically if INT pin configured, or call manually)
gpio.HandleInterrupt();
```

**Method 2: Global callback**
```cpp
gpio.SetInterruptCallback([](uint16_t status) {
    printf("Interrupt on pins: 0x%04X\n", status);
});
```

### Q: Can I use multiple devices?

**A:** Yes! Configure different I2C addresses via A0-A2 pins, then create separate driver instances. You can even mix PCA9555 and PCAL9555A on the same bus:
```cpp
// First device: PCA9555 at 0x20
pcal95555::PCAL95555<MyI2c> gpio1(&i2c, false, false, false);

// Second device: PCAL9555A at 0x21
pcal95555::PCAL95555<MyI2c> gpio2(&i2c, true, false, false);

// Each auto-detects its own chip variant
```

You can also change the address dynamically if your I2C interface supports GPIO control:
```cpp
if (gpio1.ChangeAddress(true, false, false)) {
    printf("Address changed to 0x%02X\n", gpio1.GetAddress()); // Now 0x21
}
```

### Q: How do I know which chip I have (PCA9555 vs PCAL9555A)?

**A:** The driver auto-detects during initialization:
```cpp
gpio.EnsureInitialized();
if (gpio.HasAgileIO()) {
    printf("PCAL9555A (extended features available)\n");
} else {
    printf("PCA9555 (standard GPIO only)\n");
}
```
You can also check the chip marking. PCA9555 has part number "PCA9555"; PCAL9555A has "PCAL9555A".

### Q: What's the difference between pull-up and pull-down?

**A:**
- **Pull-up**: Resistor connects pin to VDD (default high when floating)
- **Pull-down**: Resistor connects pin to GND (default low when floating)

Choose based on your circuit requirements.

### Q: What drive strength levels are available?

**A:** The driver supports four drive strength levels:
- `Level0`: 25% drive strength (¼)
- `Level1`: 50% drive strength (½)
- `Level2`: 75% drive strength (¾)
- `Level3`: 100% drive strength (full)

Example:
```cpp
gpio.SetDriveStrength(pin, DriveStrength::Level1); // 50% strength
```

### Q: How do I check for errors?

**A:** Use error flags to check for errors:
```cpp
uint16_t errors = gpio.GetErrorFlags();
if (errors & static_cast<uint16_t>(Error::I2CReadFail)) {
    // I2C read failed (also set by ReadPins() on failure)
}
if (errors & static_cast<uint16_t>(Error::I2CWriteFail)) {
    // I2C write failed
}
if (errors & static_cast<uint16_t>(Error::InvalidPin)) {
    // Invalid pin number (>= 16)
}
if (errors & static_cast<uint16_t>(Error::UnsupportedFeature)) {
    // PCAL9555A feature called on PCA9555
}
if (errors & static_cast<uint16_t>(Error::InvalidAddress)) {
    // I2C address outside valid 0x20-0x27 range
}
gpio.ClearErrorFlags(); // Clear all errors
// Or clear selectively: gpio.ClearErrorFlags(0x0001); // Clear only InvalidPin
```

> **Note:** `ReadPins()` now properly reports `Error::I2CReadFail` when the underlying
> I2C read fails, instead of silently returning empty results.

---

## Getting More Help

If you're still experiencing issues:

1. Check the [API Reference](api_reference.md) for method details
2. Review [Examples](examples.md) for working code
3. Search existing issues on GitHub
4. Open a new issue with:
   - Description of the problem
   - Steps to reproduce
   - Hardware setup details
   - Error messages/logs
   - I2C bus analyzer output (if available)

---

**Navigation**
⬅️ [Examples](examples.md) | [Back to Index](index.md)

