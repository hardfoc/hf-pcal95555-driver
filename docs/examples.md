# Examples

Complete ESP32-S3 applications that exercise the driver on a PCA9555 or PCAL9555A, one test suite and one demo.

They run on an ESP32-S3 board wired as on the [Hardware](hardware.md#evaluation-boards) page; the pins are in
[`esp32_pcal95555_test_config.hpp`](../examples/esp32/main/esp32_pcal95555_test_config.hpp) and the port in
[`esp32_pcal95555_bus.hpp`](../examples/esp32/main/esp32_pcal95555_bus.hpp). CI builds every app below on each pull
request.

## The apps

<!-- generated: examples -->
| App | What it shows | Source | CI |
| --- | --- | --- | --- |
| `pcal95555_comprehensive_test` :material-star:{ title="featured" } | Comprehensive PCAL95555 GPIO expander test suite covering all functionality | [`pcal95555_comprehensive_test.cpp`](../examples/esp32/main/pcal95555_comprehensive_test.cpp) | yes |
| `pcal95555_led_animation` :material-star:{ title="featured" } | LED animation demo driving 16 LEDs through PCA9555/PCAL9555A with visual patterns | [`pcal95555_led_animation.cpp`](../examples/esp32/main/pcal95555_led_animation.cpp) | yes |

Build and flash one (from `examples/esp32`, ESP-IDF environment loaded):

```bash
./scripts/build_app.sh pcal95555_comprehensive_test Release
./scripts/flash_app.sh flash_monitor pcal95555_comprehensive_test Release
```

Source of this table: [`examples/esp32/app_config.yml`](../examples/esp32/app_config.yml).
<!-- /generated -->

## Set up once

```bash
git submodule update --init examples/esp32/scripts   # the HardFOC ESP-IDF build scripts
cd examples/esp32
. $IDF_PATH/export.sh                                 # ESP-IDF v5.5
./scripts/build_app.sh list                           # every app and build type
```

The scripts read [`app_config.yml`](../examples/esp32/app_config.yml) (the same file this page's table comes
from) and build each app as its own firmware image.

## What to read in an example

| You want to | Read |
| --- | --- |
| See every driver call checked against the hardware, with PCAL9555A-only sections skipped on a PCA9555 | `pcal95555_comprehensive_test.cpp`, one function per test section |
| Drive 16 LEDs with only the standard registers (works on both parts) | `pcal95555_led_animation.cpp` |
| Wire up interrupts with a GPIO ISR and a FreeRTOS task | `esp32_pcal95555_bus.hpp`, `RegisterInterruptHandler()` |
| Change the address at run time through GPIO-driven A0-A2 | `esp32_pcal95555_bus.hpp`, `SetAddressPins()`; the test's address section |

The [example folder's README](../examples/esp32/README.md) has the wiring, the expected serial output and the
per-section notes of the [comprehensive test](../examples/esp32/docs/comprehensive_test.md).
