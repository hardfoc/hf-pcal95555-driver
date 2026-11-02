# Installation

Add the driver to a CMake or ESP-IDF project. It is a header-only C++20 library with no dependencies.

## Requirements

- A C++20 compiler: GCC 10+, Clang 12+ or a current MSVC.
- No RTOS or exceptions. Every call except `RegisterPinInterrupt()` is `noexcept` and returns `bool`, a value or
  sets an error flag. The driver keeps `std::function` callbacks, so registering a callback may allocate.

## Get the source

A git submodule keeps the driver pinned to a version you chose:

```bash
git submodule add https://github.com/hardfoc/hf-pcal95555-driver.git external/hf-pcal95555-driver
git submodule update --init external/hf-pcal95555-driver
```

Only `inc/`, `src/`, `cmake/` and `CMakeLists.txt` are needed to build. The examples have their own submodule
(`examples/esp32/scripts`); you do not need it to use the driver.

## CMake

```cmake
add_subdirectory(external/hf-pcal95555-driver)
target_link_libraries(my_app PRIVATE hf::pcal95555)
```

`hf::pcal95555` is an alias of the `hf_pcal95555` INTERFACE target. It carries the include directories (`inc/` and
the generated version header) and requires C++20 (`cxx_std_20`), so your target does not need to set either.
`find_package(hf_pcal95555)` works after `cmake --install`.

| Option | Default | Effect |
| --- | --- | --- |
| `HF_PCAL95555_ENABLE_WARNINGS` | `OFF` | `-Wall -Wextra -Wpedantic` on targets that link the driver |
| `HF_PCAL95555_BUILD_TESTS` | `OFF` | Builds the host unit tests in `tests/` ([Testing and CI](testing.md)) |

## ESP-IDF

The ESP-IDF component wrapper is
[`examples/esp32/components/hf_pcal95555/`](../examples/esp32/components/hf_pcal95555/CMakeLists.txt). Copy that
folder into your project's `components/` (or point `EXTRA_COMPONENT_DIRS` at it) and set the `HF_PCAL95555_ROOT`
cache variable to the driver checkout; the wrapper stops with a clear message when the path is wrong. Your
component then declares `REQUIRES hf_pcal95555`. The wrapper requires the IDF `driver` component, which the ESP32
reference port uses.

## How the build is organised

| Layer | File | Role |
| --- | --- | --- |
| Settings | [`cmake/hf_pcal95555_build_settings.cmake`](../cmake/hf_pcal95555_build_settings.cmake) | Target name, include directories, version: the single source of truth |
| Library | [`CMakeLists.txt`](../CMakeLists.txt) | INTERFACE target, install and package export |
| ESP-IDF | [`examples/esp32/components/hf_pcal95555/CMakeLists.txt`](../examples/esp32/components/hf_pcal95555/CMakeLists.txt) | IDF component wrapper over the same settings |

The version is set once in the settings file and written into `pcal95555_version.h` at configure time. Read it at
compile or run time:

```cpp
const char* v = pcal95555::PCAL95555<MyI2c>::GetDriverVersion();   // "1.0.0"
```

## Compile-time configuration

`inc/pcal95555_kconfig.hpp` gives every `CONFIG_PCAL95555_*` macro a default, so the driver builds without
ESP-IDF. The values only matter to `InitFromConfig()`; [Configuration](configuration.md) lists them.

## Next

[Quickstart](quickstart.md) wires the chip and lights an LED; [Porting](porting.md) writes the I2C interface for
your MCU.
