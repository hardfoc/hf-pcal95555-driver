# API guide

How to do the common jobs with the driver: bring it up, configure pins, write, read, use Agile I/O, handle
interrupts and errors.

Every snippet on this page is compiled by the host CI from
[`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp), with `Gpio` standing for
`pcal95555::PCAL95555<MyI2c>`. The [API reference](api-reference.md) lists every member.

## The shape of the API

One `PCAL95555<I2c>` object per chip. Pins are numbered 0 to 15: 0-7 are P0_0 to P0_7 (port 0), 8-15 are P1_0 to
P1_7 (port 1). Most settings come in three forms:

| Form | Example | Use it for |
| --- | --- | --- |
| One pin | `SetPinDirection(3, dir)` | One change: one register read and one write |
| Mask | `SetMultipleDirections(0x00FF, dir)` | Many pins, one value: each port read and written once |
| List | `SetDirections({{0, d0}, {8, d1}})` | Many pins, different values, one pass |

| Group | Members |
| --- | --- |
| Set-up | `EnsureInitialized()`, `GetChipVariant()`, `HasAgileIO()`, `SetRetries()`, `ResetToDefault()`, `InitFromConfig()` |
| Direction and polarity | `SetPinDirection()`, `SetMultipleDirections()`, `SetDirections()`, `SetPinPolarity()`, `SetMultiplePolarities()`, `SetPolarities()` |
| Outputs | `WritePin()`, `WritePins()`, `SetMultipleOutputs()`, `TogglePin()` |
| Inputs | `ReadPin()`, `ReadPins()`, `ReadAllInputs()` |
| Agile I/O (PCAL9555A) | pull, drive strength, input latch, interrupt mask, `SetOutputMode()` |
| Interrupts | `RegisterPinInterrupt()`, `UnregisterPinInterrupt()`, `SetInterruptCallback()`, `RegisterInterruptHandler()`, `HandleInterrupt()` |
| Address | `GetAddress()`, `GetAddressBits()`, `ChangeAddress()` |
| Raw registers | `ReadDualPortRegister()`, `WriteDualPortRegister()` |
| Errors | `GetErrorFlags()`, `HasError()`, `HasAnyError()`, `ClearError()`, `ClearErrorFlags()` |

Every type is in namespace `pcal95555`: `PCAL95555`, `ChipVariant`, `PinReadResult`, `I2cInterface`, the pin enums
(`GPIODir`, `Polarity`, `DriveStrength`, `InterruptState`, `InterruptEdge`), `Error` and the register map
`Pcal95555Reg`; the snippets bring them in with `using pcal95555::GPIODir;` and so on. Up to 1.0.0 the pin enums,
`Error` and `Pcal95555Reg` were global. Deprecated global aliases still accept the old spelling; define
`HF_PCAL95555_NO_GLOBAL_ALIASES` to drop them (they go in the next major version).

## Bring the chip up

```cpp
--8<-- "tests/docs/api_tasks.cpp:init"
```

Every call runs `EnsureInitialized()` on first use, so an explicit call is optional. It is the one place where a
missing chip shows up as a single `false` instead of a failure in whichever call came first.
[Architecture](architecture.md#what-happens-at-initialisation) lists what it does.

## Configure pins

```cpp
--8<-- "tests/docs/api_tasks.cpp:configure"
```

All pins are inputs after power-up. Polarity inversion applies to the Input Port register only: an inverted pin
reads `true` when it is low. It does not change what an output drives.

## Write outputs

```cpp
--8<-- "tests/docs/api_tasks.cpp:write"
```

`WritePin()` writes the Output Port register, which holds its value while the pin is an input: set the level first
and then switch the direction to output to avoid a glitch. `TogglePin()` toggles the Output Port bit, not the
measured level.

## Read inputs

```cpp
--8<-- "tests/docs/api_tasks.cpp:read"
```

`ReadPin(pin, level)` returns whether the read worked and puts the level in `level`. The one-argument `ReadPin(pin)`
returns the level alone, so a failed read looks like a low pin; `ReadAllInputs()` returns 0 on failure. After those,
check `HasError(Error::I2CReadFail)` when it matters. `ReadPins()` returns a `PinReadResult`: up to 16 `(pin, level)`
pairs in the order you asked for them, iterable, with `contains()` and `find()`. Reading an output pin returns the level
on the pin.

## Use Agile I/O (PCAL9555A)

```cpp
--8<-- "tests/docs/api_tasks.cpp:agile"
```

| Feature | Call | Register | Notes |
| --- | --- | --- | --- |
| Pull enable | `SetPullEnable()` | `0x46`, `0x47` | On at power-up; off while the port is open drain |
| Pull direction | `SetPullDirection()` | `0x48`, `0x49` | `true` = 100 kΩ pull-up, `false` = pull-down |
| Drive strength | `SetDriveStrength()` | `0x40` to `0x43` | `Level0` to `Level3` = 0.25 to 1 × full drive |
| Input latch | `EnableInputLatch()` | `0x44`, `0x45` | Holds a change (and its interrupt) until the read |
| Output stage | `SetOutputMode()` | `0x4F` | Per port; write it before making pins outputs |
| Interrupt mask | `ConfigureInterrupt()` | `0x4A`, `0x4B` | All masked at power-up |

Check `HasAgileIO()` once after initialisation: on a PCA9555 these calls return `false` and set
`Error::UnsupportedFeature`. [PCA9555 and PCAL9555A](variants.md) has the full comparison.

## Handle interrupts

```cpp
--8<-- "tests/docs/api_tasks.cpp:interrupts"
```

The callbacks run in whatever context calls `HandleInterrupt()`: your port's interrupt task, or your polling loop.
[Interrupts](interrupts.md) explains the chip behaviour, the edge decoding and the pitfalls.

## Use several chips

```cpp
--8<-- "tests/docs/api_tasks.cpp:address"
```

One driver object per chip, all on the same port object. Mixed PCA9555 and PCAL9555A parts on one bus are fine:
each object detects its own variant.

## Access registers directly

```cpp
--8<-- "tests/docs/api_tasks.cpp:raw"
```

`ReadDualPortRegister()` and `WriteDualPortRegister()` take the port-0 and port-1 command bytes and move a 16-bit
value with bit N = pin N. `WriteDualPortRegister()` writes without reading first, which suits code that keeps its
own shadow of the Output or Configuration registers. They do not check the Agile I/O guard: on a PCA9555 a command
from 0x40 up NACKs and sets the I2C error flag.

## Handle errors

```cpp
--8<-- "tests/docs/api_tasks.cpp:errors"
```

| Flag | Set when | Cleared by |
| --- | --- | --- |
| `Error::I2CReadFail` | A register read failed after all retries, or initialisation found no chip | The next successful read |
| `Error::I2CWriteFail` | A register write failed after all retries | The next successful write |
| `Error::InvalidPin` | A pin above 15 | The next call with a valid pin |
| `Error::UnsupportedFeature` | An Agile I/O call on a PCA9555 (or an undetected variant) | `ClearError()` / `ClearErrorFlags()` |
| `Error::InvalidAddress` | `ChangeAddress(address)` outside 0x20 to 0x27 | A successful `ChangeAddress()` |
| `Error::InvalidMask` | A mask-form call (`SetMultipleDirections()`, `SetMultipleOutputs()`, `SetMultiplePolarities()`, `EnableMultipleInputLatches()`) got a mask of 0; nothing is sent | The next of those calls with a non-zero mask |

`GetErrorFlags()` returns the flags as a `uint16_t`; `HasError()` tests one, `ClearErrorFlags(mask)` clears the
bits in `mask` (all by default).

## Restore defaults

`ResetToDefault()` writes the power-up values of every register (the Agile I/O bank too on a PCAL9555A), since the
chips have no reset pin. [Configuration](configuration.md) covers `InitFromConfig()` and the retry count.
