"""The three layers of the driver: hardware-agnostic core, communication interface, platform port.

Reads: From top to bottom. Your application talks to one PCAL95555<I2c> object per chip. The core owns the
chip's register map (listed from Pcal95555Reg in inc/pcal95555.hpp): the eight standard registers both chips have,
and the Agile I/O bank only the PCAL9555A answers, which the driver guards after it has detected the variant. The
communication interface (pcal95555::I2cInterface in inc/pcal95555_i2c_interface.hpp) is a CRTP base: three methods
are required, the rest have defaults. The platform port is the only code you write. Below it is the chip.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
DRIVER = "inc/pcal95555.hpp"
COMM = "inc/pcal95555_i2c_interface.hpp"


def data():
    drv = (ROOT / DRIVER).read_text()
    enum = re.search(r"enum class Pcal95555Reg : uint8_t \{(.*?)\};", drv, re.S).group(1)
    regs = re.findall(r"^\s*(\w+) = (0x[0-9A-Fa-f]+)", enum, re.M)
    standard = [n for n, v in regs if int(v, 16) < 0x40]
    agile = [n for n, v in regs if int(v, 16) >= 0x40]
    comm = (ROOT / COMM).read_text()
    # A method whose body dispatches to Derived is required; one with a body of its own has a default.
    methods = re.findall(r"^  (?:bool|void) (\w+)\([^{;]*?\) noexcept \{(.*?)^  \}", comm, re.M | re.S)
    required = [n for n, body in methods if "static_cast<Derived*>" in body]
    optional = [n for n, body in methods if "static_cast<Derived*>" not in body and n not in ("GpioSetActive",
                                                                                             "GpioSetInactive")]
    return {
        "standard": standard,
        "agile": agile,
        "required": required,
        "optional": optional,
        "sources": [f"{DRIVER}:Pcal95555Reg", f"{COMM}:I2cInterface"],
    }


def build(d):
    f = Fig(
        "driver-layers",
        "Three layers, one of them yours",
        alt="Layer diagram: the application calls the hardware-agnostic PCAL95555 driver, which owns the standard "
        "register map of both chips and the PCAL9555A-only Agile I/O bank, and uses the CRTP I2C interface, which "
        "calls the platform port you write, which drives the I2C bus and the INT line of the PCA9555 or PCAL9555A.",
        width=1200,
        kicker="Architecture",
        subtitle="The driver and the interface are the same on every MCU. The port is a small class with three "
        "required methods.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "you write"), ("slate", "hardware")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your application", ["buttons, LEDs, enables, chip selects ..."], tone="slate")
    top = app.b + 40
    y = f.region_inner_top(top)
    f.text(16, y + 12, "Both chips (0x00-0x07)", size=f.k["small"], weight=700, color="brand.text", bg="brand.tint")
    h1 = f.chips(16, y + 22, W - 32, d["standard"], tone="brand")
    y2 = y + 22 + h1 + 18
    f.text(16, y2 + 12, "PCAL9555A only: Agile I/O (0x40-0x4F)", size=f.k["small"], weight=700, color="brand.text",
           bg="brand.tint")
    h2 = f.chips(16, y2 + 22, W - 32, d["agile"], tone="brand")
    reg = f.region(0, top, W, y2 + 22 + h2 + 18 - top, "Hardware-agnostic core", tone="brand",
                   sub="PCAL95555<I2c>, inc/pcal95555.hpp")
    comm = f.card(0, reg.b + 40, W, "Communication interface",
                  ["pcal95555::I2cInterface<Self>: CRTP, no virtual calls; variant detection, retries and",
                   "the Agile I/O guard live in the core above, so the interface only moves register bytes"],
                  tone="brand", accent=True)
    port = f.card(0, comm.b + 40, W, "Platform port (your class)",
                  ["required:  " + "  ".join(n + "()" for n in d["required"]),
                   "optional:  " + "  ".join(n + "()" for n in d["optional"])],
                  tone="teal", accent=True, mono=True)
    hw = f.card(0, port.b + 40, W, "PCA9555 / PCAL9555A",
                ["I2C up to 400 kHz, address 0x20-0x27 (A2 A1 A0)  /  INT open drain, active low"], tone="slate")
    for a, b in ((app, reg), (reg, comm), (comm, port), (port, hw)):
        f.connect(a, b, "bottom", "top", tone="rail")
    return f
