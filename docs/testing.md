# Testing and CI

How the driver is checked: host unit tests against a register model of both chips, compiled documentation, ESP32
builds and static analysis, on every pull request.

## Host unit tests

The tests build the real driver against a model of the chip behind the I2C interface. The model holds every
register with its datasheet power-up value, NACKs the Agile I/O bank when it plays a PCA9555, applies polarity
inversion to the Input Port registers, raises interrupts for unmasked inputs and clears them when the Input Port is
read. It records every transfer, so tests check exactly what went over the bus.

```bash
cmake -S . -B build-host -DHF_PCAL95555_BUILD_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

| File | Covers |
| --- | --- |
| [`tests/test_driver.cpp`](../tests/test_driver.cpp) | Variant detection, address mapping and clamping, port-register bit placement, drive-strength encoding, pull / latch / output-mode registers, `ResetToDefault()`, interrupt callbacks on both parts, multi-pin calls, error flags |
| [`tests/mock_pcal95555.hpp`](../tests/mock_pcal95555.hpp) | The PCA9555 / PCAL9555A register model |
| [`tests/test_harness.hpp`](../tests/test_harness.hpp), [`tests/test_main.cpp`](../tests/test_main.cpp) | A dependency-free test registry and runner |

## Documentation snippets

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/my_i2c.hpp), which the host build
compiles (target `hf_pcal95555_doc_snippets`, `-Wall -Wextra`) with the driver instantiated over the documented
port. A change to the API that breaks a page breaks the build first. The README's example is a copy of the
Quickstart snippet that `hardfoc-docs check` keeps in step.

## CI workflows

| Workflow | What it checks |
| --- | --- |
| Host unit tests | GCC and Clang build and run the tests and the doc snippets |
| ESP32 examples build | Every app in `examples/esp32/app_config.yml`, for each listed ESP-IDF version and build type |
| C++ lint, C++ analysis | clang-format, clang-tidy and cppcheck |
| Docs | Strict site build, figure and generated-table drift, Doxygen, link check, Lighthouse budget |
| Markdown lint, YAML lint, link check | Repository text files |

The workflows call the shared, versioned workflows of
[hf-general-ci-tools](https://github.com/hardfoc/hf-general-ci-tools) and
[hf-espidf-ci-tools](https://github.com/hardfoc/hf-espidf-ci-tools).

## On hardware

The [ESP32 examples](examples.md) are the hardware test suite: `pcal95555_comprehensive_test` runs 17 sections
and prints a pass / fail summary over the serial monitor, skipping the PCAL9555A-only sections on a PCA9555.
