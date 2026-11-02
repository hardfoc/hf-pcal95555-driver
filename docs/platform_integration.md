# Platform Integration Guide

This guide explains how to implement the hardware abstraction interface for the PCAL95555 driver on your platform.

## Understanding CRTP (Curiously Recurring Template Pattern)

The PCAL95555 driver uses **CRTP** (Curiously Recurring Template Pattern) for hardware abstraction. This design choice provides several critical benefits for embedded systems:

### Why CRTP Instead of Virtual Functions?

#### 1. **Zero Runtime Overhead**
- **Virtual functions**: Require a vtable lookup (indirect call) = ~5-10 CPU cycles overhead per call
- **CRTP**: Direct function calls = 0 overhead, compiler can inline
- **Impact**: In time-critical embedded code, this matters significantly

#### 2. **Compile-Time Polymorphism**
- **Virtual functions**: Runtime dispatch - the compiler cannot optimize across the abstraction boundary
- **CRTP**: Compile-time dispatch - full optimization, dead code elimination, constant propagation
- **Impact**: Smaller code size, faster execution

#### 3. **Memory Efficiency**
- **Virtual functions**: Each object needs a vtable pointer (4-8 bytes)
- **CRTP**: No vtable pointer needed
- **Impact**: Critical in memory-constrained systems (many MCUs have <64KB RAM)

#### 4. **Type Safety**
- **Virtual functions**: Runtime errors if method not implemented
- **CRTP**: Compile-time errors if method not implemented
- **Impact**: Catch bugs at compile time, not in the field

### How CRTP Works

```cpp
// Base template class (from pcal95555_i2c_interface.hpp)
template <typename Derived>
class I2cInterface {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        // Cast 'this' to Derived* and call the derived implementation
        return static_cast<Derived*>(this)->Write(addr, reg, data, len);
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        return static_cast<Derived*>(this)->Read(addr, reg, data, len);
    }
};

// Your implementation
class MyI2c : public pcal95555::I2cInterface<MyI2c> {
public:
    // This method is called directly (no virtual overhead)
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        // Your platform-specific I2C code
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        // Your platform-specific I2C code
    }
};
```

The key insight: `static_cast<Derived*>(this)` allows the base class to call methods on the derived class **at compile time**, not runtime.

### Performance Comparison

| Aspect | Virtual Functions | CRTP |
|--------|------------------|------|
| Function call overhead | ~5-10 cycles | 0 cycles (inlined) |
| Code size | Larger (vtables) | Smaller (optimized) |
| Memory per object | +4-8 bytes (vptr) | 0 bytes |
| Compile-time checks | No | Yes |
| Optimization | Limited | Full |

## Interface Definition

The PCAL95555 driver requires you to implement the `I2cInterface` template:

**Location**: [`inc/pcal95555.hpp#L437`](../inc/pcal95555.hpp#L437)

```cpp
template <typename Derived>
class I2cInterface {
public:
    // Required methods (implement both)
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
};
```

**Required Methods** (must be implemented):
- `Write()`: Write `len` bytes from `data` to register `reg` at I2C address `addr` (7-bit address)
- `Read()`: Read `len` bytes into `data` from register `reg` at I2C address `addr` (7-bit address)
- `EnsureInitialized()`: Ensure I2C bus is initialized and ready for communication
- All return `true` on success, `false` on failure (NACK, timeout, etc.)

**Optional Methods** (can be overridden for additional functionality):
- `SetAddressPins()`: Control A2-A0 address pins via GPIO (returns `false` by default if not supported)
- `RegisterInterruptHandler()`: Register interrupt handler for INT pin (returns `false` by default if not supported)

## Implementation Steps

### Step 1: Create Your Implementation Class

```cpp
#include "pcal95555.hpp"

class MyPlatformI2c : public pcal95555::I2cInterface<MyPlatformI2c> {
private:
    // Your platform-specific members
    i2c_handle_t i2c_handle_;
    
public:
    // Constructor
    MyPlatformI2c(i2c_handle_t handle) : i2c_handle_(handle) {}
    
    // Implement required methods (NO virtual keyword!)
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        // Your I2C write implementation
        return true;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        // Your I2C read implementation
        return true;
    }
    
    // Required: Ensure I2C bus is initialized and ready
    bool EnsureInitialized() {
        if (initialized_) {
            return true;  // Already initialized
        }
        // Initialize I2C hardware, configure pins, set up bus, etc.
        // ...
        initialized_ = true;
        return true;
    }
    
    // Optional: Override to support dynamic address pin control
    bool SetAddressPins(bool a0_level, bool a1_level, bool a2_level) {
        // Set GPIO pins connected to A2-A0 address pins
        // Return true if supported, false if not supported (hardwired)
        return false; // Default: not supported
    }
    
    // Optional: Override to support hardware interrupt handling
    bool RegisterInterruptHandler(std::function<void()> handler) {
        // Set up GPIO interrupt for INT pin
        // Call handler() when INT pin fires
        // Return true if supported, false if not supported
        return false; // Default: not supported
    }
    
private:
    bool initialized_ = false;  // Track initialization state
};
```

### Step 2: Platform-Specific Examples

#### ESP32 (ESP-IDF)

```cpp
#include "driver/i2c.h"
#include "pcal95555.hpp"

class Esp32I2cBus : public pcal95555::I2cInterface<Esp32I2cBus> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_write(cmd, (uint8_t*)data, len, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        return ret == ESP_OK;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        return ret == ESP_OK;
    }
};
```

#### STM32 (HAL)

```cpp
#include "stm32f4xx_hal.h"
#include "pcal95555.hpp"

extern I2C_HandleTypeDef hi2c1;

class STM32I2cBus : public pcal95555::I2cInterface<STM32I2cBus> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        // STM32 HAL uses 8-bit address (7-bit << 1)
        return HAL_I2C_Mem_Write(&hi2c1, addr << 1, reg, 
                                 I2C_MEMADD_SIZE_8BIT, 
                                 (uint8_t*)data, len, 
                                 HAL_MAX_DELAY) == HAL_OK;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        return HAL_I2C_Mem_Read(&hi2c1, addr << 1, reg,
                                I2C_MEMADD_SIZE_8BIT,
                                data, len,
                                HAL_MAX_DELAY) == HAL_OK;
    }
};
```

#### Arduino

```cpp
#include <Wire.h>
#include "pcal95555.hpp"

class ArduinoI2cBus : public pcal95555::I2cInterface<ArduinoI2cBus> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.write(data, len);
        return Wire.endTransmission() == 0;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) return false;
        
        Wire.requestFrom(addr, len);
        for (size_t i = 0; i < len && Wire.available(); i++) {
            data[i] = Wire.read();
        }
        return true;
    }
};
```

## Common Pitfalls

### Don't Use Virtual Functions

```cpp
// WRONG - defeats the purpose of CRTP
class MyI2c : public pcal95555::I2cInterface<MyI2c> {
public:
    virtual bool Write(...) override {  // ❌ Virtual keyword not needed
        // ...
    }
};
```

### Correct CRTP Implementation

```cpp
// CORRECT - no virtual keyword
class MyI2c : public pcal95555::I2cInterface<MyI2c> {
public:
    bool Write(...) {  // ✅ Direct implementation
        // ...
    }
};
```

### Don't Forget the Template Parameter

```cpp
// WRONG - missing template parameter
class MyI2c : public pcal95555::I2cInterface {  // ❌ Compiler error
    // ...
};
```

### Correct Template Parameter

```cpp
// CORRECT - pass your class as template parameter
class MyI2c : public pcal95555::I2cInterface<MyI2c> {  // ✅
    // ...
};
```

## Testing Your Implementation

After implementing the interface, test it:

```cpp
MyPlatformI2c i2c;
// Constructor takes A0, A1, A2 pin levels (all LOW = address 0x20, default)
// Optional: pass ChipVariant to skip auto-detection
pcal95555::PCAL95555<MyPlatformI2c> gpio(&i2c, false, false, false);

gpio.ResetToDefault();

// Check which chip variant was detected
if (gpio.HasAgileIO()) {
    printf("PCAL9555A detected - full feature set\n");
} else {
    printf("PCA9555 detected - standard GPIO only\n");
}

gpio.SetPinDirection(0, pcal95555::PCAL95555<MyPlatformI2c>::GPIODir::Output);
gpio.WritePin(0, true);
bool value = gpio.ReadPin(1);
// Interface works!
```

## Next Steps

- See [Configuration](configuration.md) for driver configuration options
- Check [Examples](examples.md) for complete usage examples
- Review [API Reference](api_reference.md) for all available methods

---

**Navigation**
⬅️ [Hardware Setup](hardware_setup.md) | [Next: Configuration ➡️](configuration.md) | [Back to Index](index.md)

