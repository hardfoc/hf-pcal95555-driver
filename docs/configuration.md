# Configuration

What you can set before and at start-up: the address and variant, retries, the power-up state and the
compile-time pin configuration that `InitFromConfig()` applies.

## Address and variant

Both constructors take the I2C port by pointer and an optional `pcal95555::ChipVariant`:

| Constructor | Address |
| --- | --- |
| `PCAL95555(bus, a0, a1, a2, variant = Unknown)` | `0x20 + (a2 << 2 \| a1 << 1 \| a0)` |
| `PCAL95555(bus, uint8_t address, variant = Unknown)` | `address`, clamped to 0x20 to 0x27 without an error |

```cpp
--8<-- "tests/docs/configuration.cpp:variant"
```

`Unknown` (the default) runs the detection probe described in [PCA9555 and PCAL9555A](variants.md).

## Start-up sequence

```cpp
--8<-- "tests/docs/configuration.cpp:startup"
```

| Call | Effect |
| --- | --- |
| `SetRetries(n)` | Each register access is tried `n + 1` times before it fails; default `n = 1`. The detection probe always runs without retries |
| `ResetToDefault()` | Writes the datasheet power-up value of every register: outputs high, no inversion, all inputs; on a PCAL9555A also full drive, no latch, pull-ups on, interrupts masked, push-pull |
| `InitFromConfig()` | Writes the compile-time configuration below: Output Port and Configuration registers; on a PCAL9555A also pull enable, pull select and output mode. Does nothing when `CONFIG_PCAL95555_INIT_FROM_KCONFIG` is 0 |

## Compile-time pin configuration

`InitFromConfig()` reads `CONFIG_PCAL95555_*` macros. [`inc/pcal95555_kconfig.hpp`](../inc/pcal95555_kconfig.hpp)
defines every one that is not already defined, and the [`Kconfig`](../Kconfig) file at the repository root
describes the same options as an ESP-IDF menu.

| Macro | Default | Meaning |
| --- | --- | --- |
| `CONFIG_PCAL95555_INIT_FROM_KCONFIG` | `1` | `InitFromConfig()` applies the values below; `0` makes it a no-op |
| `CONFIG_PCAL95555_Pp_n_DIR` | `1` | Pin `Pp_n` is an input (`1`) or output (`0`) |
| `CONFIG_PCAL95555_Pp_n_OUTPUT` | `0` | Initial output level |
| `CONFIG_PCAL95555_Pp_n_PULL` | `0` | Pull resistor enabled (PCAL9555A) |
| `CONFIG_PCAL95555_Pp_n_PULL_UP` | `1` | Pull-up (`1`) or pull-down (`0`) (PCAL9555A) |
| `CONFIG_PCAL95555_PORT0_OD`, `CONFIG_PCAL95555_PORT1_OD` | `0` | Port in open-drain mode (PCAL9555A) |

`p` is the port (0 or 1) and `n` the pin (0 to 7), for example `CONFIG_PCAL95555_P1_3_DIR`. The header combines
them into 16-bit values (`CONFIG_PCAL95555_INIT_DIRECTION`, `_INIT_OUTPUT`, `_INIT_PULL_ENABLE`, `_INIT_PULL_UP`)
that you can also define directly, for example with `target_compile_definitions()`.

!!! warning "The defaults are not the power-up state"
    With every macro at its default, `InitFromConfig()` drives all outputs low in the Output Port register and, on
    a PCAL9555A, **disables every pull resistor** (the chip powers up with pull-ups on). Define the macros you need
    before calling it, or call only `ResetToDefault()`.

!!! note "ESP-IDF menuconfig"
    The ESP-IDF component wrapper in `examples/esp32/components/hf_pcal95555/` sources the root `Kconfig`, so
    `idf.py menuconfig` shows the options under **PCAL95555 GPIO expander** and the driver reads them from
    `sdkconfig.h`. A bool set to `n` is absent from `sdkconfig.h`; while that menu is enabled
    (`CONFIG_PCAL95555` defined) the driver reads an absent option as `0`, not as the default above. If you copy
    the component elsewhere, keep its `Kconfig` pointing at the driver's root `Kconfig`.
    `CONFIG_PCAL95555_DEFAULT_ADDRESS` in `Kconfig` is not read by the driver.

## Run-time settings

Everything else (directions, polarity, pulls, drive strength, latch, output mode, interrupt mask) is a run-time
call; the [API guide](api-guide.md) shows each one. Recommended settings by pin use:

| Pin use | PCA9555 | PCAL9555A |
| --- | --- | --- |
| Push button to ground | Input; external pull-up if the line is long | Input, pull-up (power-up default), latch if presses are short |
| LED to VDD (sinking) | Output, high = off | Output, drive `Level3` (power-up default) |
| Shared or wired-OR line | Not possible: push-pull only | `SetOutputMode()` open drain for that port, then output |
| Interrupt input | Input | Input, then `ConfigureInterrupt(pin, InterruptState::Enabled)` |

Next: [Porting](porting.md).
