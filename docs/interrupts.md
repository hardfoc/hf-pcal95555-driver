# Interrupts

How the chips signal an input change on INT, how the driver turns that into per-pin callbacks, and what you must
do in your port and your application.

## What the chip does

INT is an open-drain, active-low output. It goes active when an input pin differs from the value the Input Port
register held when it was last read. It goes inactive when the host reads the Input Port register of that port, or
when the input returns to its earlier level before the read.

| Behaviour | PCA9555 | PCAL9555A |
| --- | --- | --- |
| Pins that can raise INT | Every pin configured as an input | Inputs whose `INT_MASK` bit is 0 |
| Interrupt mask at power-up | none (all inputs active) | all masked (`0xFF`, `0xFF`) |
| Which pin changed | Compare two Input Port reads | `INT_STATUS_0`, `INT_STATUS_1`: 1 = source |
| Cleared by | Reading the Input Port register | Reading the Input Port register (reading the status does not clear it) |
| Short pulses | Lost if the pin returns before the read | Kept with the input latch (`EnableInputLatch()`) until the read |

## What the driver does

`HandleInterrupt()` is the whole interrupt path:

1. Find the sources: on a PCAL9555A, read `INT_STATUS_0` and `INT_STATUS_1`; on a PCA9555, read both Input Port
   registers and XOR them with the previous snapshot.
2. Read both Input Port registers. This also clears INT.
3. Call the global callback (`SetInterruptCallback()`) with the 16-bit source mask.
4. For every source pin with a callback (`RegisterPinInterrupt()`), compare the previous and current level and call
   the callback if the edge matches `InterruptEdge::Rising`, `Falling` or `Both`.
5. Store the current levels as the new snapshot.

`HandleInterrupt()` does I2C transfers, so it must run in a task, never in an interrupt service routine.

## Set it up

```cpp
--8<-- "tests/docs/api_tasks.cpp:interrupts"
```

`RegisterInterruptHandler()` hands a closure that calls `HandleInterrupt()` to your port's optional
`RegisterInterruptHandler(std::function<void()>)`. A port that supports it attaches a falling-edge interrupt on the
MCU pin wired to INT and runs the closure from a task when it fires; the
[ESP32 port](../examples/esp32/main/esp32_pcal95555_bus.hpp) does this with a GPIO ISR that wakes a FreeRTOS task.
If your port keeps the default, `RegisterInterruptHandler()` returns `false` and you call the handler yourself.

## Poll instead

```cpp
--8<-- "tests/docs/api_tasks.cpp:poll"
```

Without an INT line, call `HandleInterrupt()` from a periodic task. On a PCA9555 that is change detection by
polling; on a PCAL9555A it also picks up latched inputs that changed between polls.

## Pitfalls

| Symptom | Cause | Fix |
| --- | --- | --- |
| No interrupt at all on a PCAL9555A | Every pin is masked at power-up | `ConfigureInterrupt(pin, InterruptState::Enabled)` |
| INT stays low | Nobody reads the Input Port register | Make sure `HandleInterrupt()` runs; any `ReadPin()` of that port also clears it |
| A PCAL9555A callback is missed | Another call read the Input Port between the change and the handler, which cleared the interrupt before the handler saw it | Let the handler be the only reader of interrupt inputs, and use the callback's `level` argument |
| Fast pulses are missed | The input returned before the read | PCAL9555A: `EnableInputLatch(pin, true)` |
| `RegisterPinInterrupt()` returns `false` | Pin above 15, an empty callback, or initialisation failed | Check the pin and `GetErrorFlags()` |

Next: [API guide](api-guide.md).
