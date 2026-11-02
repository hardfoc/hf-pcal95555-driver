# Troubleshooting

Symptoms you may see while bringing up a PCA9555 or PCAL9555A, their causes and the fix for each.

Start with `GetErrorFlags()` after the failing call and with an I2C scan of 0x20 to 0x27: they tell bus problems
(nothing answers) apart from configuration problems (the chip answers, the pins do not do what you expect).

## The chip does not answer

| Symptom | Cause | Fix |
| --- | --- | --- |
| `EnsureInitialized()` returns `false`, `I2CReadFail` set | Nothing ACKs at the address | Scan the bus; check SDA / SCL, a common ground and VDD |
| The scan finds the chip at another address | A2-A0 strapped differently from the constructor | Use the scanned address, or fix the straps; never leave A0-A2 open |
| Works at 100 kHz, fails at 400 kHz | Pull-ups too weak for the bus capacitance | Lower the pull-up value (for example 2.2 kΩ at 3.3 V) or the clock |
| Intermittent `I2CReadFail` / `I2CWriteFail` | Noise, long wires, another master | Shorter wiring, `SetRetries(2)`, one master per bus |
| Fails after a NACK on some MCUs | The controller stays stuck after the detection probe's NACK | Recover the bus inside your port's `Read()`, or pass the variant to the constructor to skip the probe |

## The variant is wrong

| Symptom | Cause | Fix |
| --- | --- | --- |
| `EnsureInitialized()` fails although the chip answers | The read after the detection probe failed: the bus did not recover from the NACK | Fix bus recovery in the port, or pass the variant ([PCA9555 and PCAL9555A](variants.md)) |
| A PCA9555 is reported as `PCAL9555A` | The port returns `true` for a NACK | Return `false` on every NACK ([Porting](porting.md)) |
| Agile I/O calls fail with `UnsupportedFeature` | The part is a PCA9555, or detection failed | Read the marking (PCAL9555A vs PCA9555); check `GetChipVariant()` |

## Pins do not behave

| Symptom | Cause | Fix |
| --- | --- | --- |
| An output does not change | The pin is still an input (power-up default) | `SetPinDirection(pin, GPIODir::Output)` |
| Wrong pin moves | Port mix-up: pins 8-15 are P1_0 to P1_7 | Use the pin table on [Hardware](hardware.md#pins) |
| An input reads inverted | Polarity inversion set on that pin | `SetPinPolarity(pin, Polarity::Normal)` |
| A floating input reads high on a PCAL9555A | The 100 kΩ pull-up is on at power-up | `SetPullEnable(pin, false)` or `SetPullDirection(pin, false)` |
| An open-drain output never goes high | No pull-up on the line; internal pulls are disconnected in open-drain mode | Add an external pull-up |
| `ReadPin()` returns `false` for a high pin | The read failed | Use `ReadPin(pin, level)`, which returns `false` only on a failed read, or check `HasError(Error::I2CReadFail)` |
| Pin settings lost after `InitFromConfig()` | Its compile-time defaults (all pulls off, all outputs low) were applied | Define the `CONFIG_PCAL95555_*` macros ([Configuration](configuration.md)) |

## Interrupts do not arrive

| Symptom | Cause | Fix |
| --- | --- | --- |
| INT never goes low on a PCAL9555A | All pins are masked at power-up | `ConfigureInterrupt(pin, InterruptState::Enabled)` |
| INT stays low | The Input Port register is never read | Make sure `HandleInterrupt()` runs |
| `RegisterInterruptHandler()` returns `false` | Your port keeps the default | Implement it, or poll `HandleInterrupt()` |
| Callbacks miss short pulses | The input returned before the read | `EnableInputLatch(pin, true)` (PCAL9555A) |

[Interrupts](interrupts.md) explains the mechanism.

## Build problems

| Symptom | Cause | Fix |
| --- | --- | --- |
| `pcal95555_version.h` not found | The include path lacks the generated directory | Link the `hf::pcal95555` target instead of adding `inc/` by hand |
| `GPIODir` or `Error` not found in `pcal95555::` | They are declared at global scope | Write `GPIODir::Output`, not `pcal95555::GPIODir::Output` |
| Errors about C++20 features | The target compiles with an older standard | `hf::pcal95555` requires `cxx_std_20`; check the toolchain supports it |
