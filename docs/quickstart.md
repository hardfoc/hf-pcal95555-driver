# Quickstart

Light an LED from a button through a PCA9555 or PCAL9555A: wire five signals, write a small I2C port class, call
four driver functions.

## 1. Wire it

| MCU | PCA9555 / PCAL9555A | Notes |
| --- | --- | --- |
| I2C SCL | SCL | Pull-up to the bus voltage (4.7 kΩ is a common value at 400 kHz) |
| I2C SDA | SDA | Pull-up as for SCL |
| GND | VSS | Common ground |
| 3.3 V | VDD | 1.65 V to 5.5 V (PCAL9555A) or 2.3 V to 5.5 V (PCA9555); 100 nF next to the pin |
| GND | A0, A1, A2 | Address 0x20; tie each pin to VDD or VSS, never leave it open |
| GPIO in (optional) | INT | Open drain, active low: pull it up |

For the example, connect an LED with a series resistor from P0_0 (pin 0) to VDD, so the expander sinks its current,
and a push button from P1_0 (pin 8) to ground. The [Hardware](hardware.md) page has the pin numbers per package,
the address table and the electrical limits.

## 2. Add the driver

Add the repository as a submodule and link the `hf_pcal95555` CMake target (or add `inc/` to your include path and
build with C++20). [Installation](installation.md) covers CMake and ESP-IDF.

```bash
git submodule add https://github.com/hardfoc/hf-pcal95555-driver.git external/hf-pcal95555-driver
```

## 3. Write the port

The port is the only platform code. It brings the bus up, writes one register and reads one register. Replace the
`platform_*` calls with your SDK's.

```cpp title="my_i2c.hpp"
--8<-- "tests/docs/my_i2c.hpp:port"
```

[Porting](porting.md) explains each method, the optional ones and the ESP-IDF reference port.

## 4. Light the LED

```cpp title="main.cpp"
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

What each step does:

1. `EnsureInitialized()` brings the bus up through your port, reads Input Port 0 to prove the chip answers at
   0x20, and probes register 0x4F to tell a PCAL9555A from a PCA9555. Every other call runs it for you on first
   use; calling it yourself gives you one place to handle a missing chip.
2. `SetPinDirection()` writes one bit of the Configuration register: `GPIODir::Output` is 0, `GPIODir::Input` is 1.
   All pins are inputs after power-up.
3. `ReadPin()` reads the Input Port register. The button pulls the pin low, so pressed reads `false`; a PCAL9555A's
   internal pull-up (on by default) or an external one holds it high otherwise.
4. `WritePin()` reads the Output Port register, changes one bit and writes it back. Writing `true` drives the pin
   high, which turns this LED off; `pressed` therefore lights it.

This example is built by the host CI, so it compiles against the current headers.

## If it does not work

| Symptom | First thing to check |
| --- | --- |
| `EnsureInitialized()` returns `false` | SDA / SCL swapped, missing pull-ups, A2-A0 levels against the address you gave |
| `GetChipVariant()` says `PCA9555` on a PCAL9555A | Read the part marking; a bus that does not report NACKs correctly ([Troubleshooting](troubleshooting.md)) |
| The pin reads high with the button pressed | Wrong pin number: pins 8-15 are P1_0 to P1_7 |
| The button reads random values on a PCA9555 | The PCA9555 pull-up is weak (about 100 kΩ); add an external one if the line is long |

## Next

- [API guide](api-guide.md): pull resistors, drive strength, interrupts, errors.
- [Examples](examples.md): the ESP32-S3 test suite and LED animation demo.
- [PCA9555 and PCAL9555A](variants.md): what each part has and how the driver tells them apart.
