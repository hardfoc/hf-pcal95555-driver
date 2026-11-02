---
hide:
  - navigation
  - toc
---

<div class="hf-hero" markdown>
<span class="hf-kicker">HardFOC driver</span>

# PCA9555 / PCAL9555A Driver

<p>A C++20 driver for the NXP PCA9555 and PCAL9555A 16-bit I2C I/O expanders. It detects which chip is on the
bus, drives and reads all sixteen pins, and uses the PCAL9555A's Agile I/O features (pull resistors, drive
strength, input latch, interrupt mask and status, open-drain outputs) when they are there.</p>

[Quickstart](quickstart.md){ .md-button .md-button--primary }
[API guide](api-guide.md){ .md-button }
[Source on GitHub](https://github.com/hardfoc/hf-pcal95555-driver){ .md-button }
</div>

The driver has no platform code. You give it a small class that writes and reads one register over I2C; it does
the rest: variant detection, read-modify-write of the port registers, retries, interrupt decoding and error flags.

<div class="hf-chip" markdown>
<div class="hf-chip__head">
<span class="hf-chip__kicker">16-bit I2C GPIO expander</span>
<span class="hf-chip__name">PCA9555 / PCAL9555A</span>
<span class="hf-chip__vendor">NXP Semiconductors</span>
</div>

I/O
:   16 pins in two 8-bit ports, 5 V tolerant, 25 mA sink per pin (recommended maximum)

Host interface
:   I2C / SMBus, 0 to 400 kHz, address 0x20 to 0x27 (A2, A1, A0)

Supply
:   PCAL9555A 1.65 V to 5.5 V; PCA9555 2.3 V to 5.5 V

Interrupt
:   INT, open drain, active low; on the PCAL9555A maskable per pin, with a source register

Agile I/O (PCAL9555A)
:   Pull-up / pull-down (100 kΩ typ.), four drive levels, input latch, push-pull or open drain per port

Package
:   PCAL9555A: TSSOP24, HWQFN24; PCA9555 also SO24, SSOP24, HVQFN24. Pin-compatible in the same package

[PCAL9555A datasheet](datasheet/pcal9555a.pdf) ·
[PCA9555 datasheet](datasheet/pca9555.pdf)
</div>

## What you get

- **One driver for both parts.** `PCAL95555<YourI2c>` probes the Agile I/O bank at start-up and reports
  `ChipVariant::PCA9555` or `ChipVariant::PCAL9555A`. See [PCA9555 and PCAL9555A](variants.md).
- **Pins by number.** Pins 0-7 are port 0, 8-15 are port 1; single-pin, mask and list forms for direction,
  output, polarity and reads. See the [API guide](api-guide.md).
- **Agile I/O guarded.** Pull, drive-strength, latch, interrupt-mask and output-mode calls return `false` with
  `Error::UnsupportedFeature` on a PCA9555 instead of sending a write the chip would NACK.
- **Per-pin edge callbacks.** Rising, falling or both, decoded from the PCAL9555A's interrupt status register or
  from input snapshots on a PCA9555. See [Interrupts](interrupts.md).
- **Zero-overhead portability.** The I2C interface is a CRTP base: no virtual calls, three required methods.
  See [Porting](porting.md).
- **Tested without hardware.** Host unit tests run the driver against a register model of both chips. See
  [Testing and CI](testing.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **[Quickstart](quickstart.md)**

    ---

    Wire the chip, write the port, light an LED from a button in about 15 lines.

-   :material-chip: **[Hardware](hardware.md)**

    ---

    Pins, addresses, bus timing, power and the differences between the parts.

-   :material-layers-outline: **[Architecture](architecture.md)**

    ---

    The three layers and what each one owns.

-   :material-code-braces: **[API guide](api-guide.md)**

    ---

    Configure, write, read, interrupts and errors, task by task.

-   :material-swap-horizontal: **[Porting](porting.md)**

    ---

    Implement the I2C interface for your MCU.

-   :material-book-open-variant: **[API reference](api-reference.md)**

    ---

    Every class and member, generated from the headers.

</div>
