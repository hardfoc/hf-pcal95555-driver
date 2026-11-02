# Changelog

All notable changes to this driver are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Breaking

- **BREAKING:** `EnsureInitialized()` and `ChangeAddress()` fail when variant detection is inconclusive (the bus did not
  answer the read after the 0x4F probe). They used to succeed with `ChipVariant::Unknown`. They now return `false` with
  `Error::I2CReadFail`, and the next call probes again. Pass the variant to the constructor to skip the probe.
- **BREAKING:** mask-form calls reject a mask of 0: `SetMultipleDirections()`, `SetMultipleOutputs()`,
  `SetMultiplePolarities()` and `EnableMultipleInputLatches()` return `false` with `Error::InvalidMask` and send
  nothing. They used to read and rewrite both port registers and return `true`.
- **BREAKING:** `InitFromConfig()` under ESP-IDF now applies the menuconfig values. The component sources the root
  `Kconfig` and the driver includes `sdkconfig.h`; projects that relied on the header defaults while Kconfig was ignored
  get their `sdkconfig` values instead (the Kconfig defaults equal the header defaults). A bool set to `n` now reads as
  0 instead of the header's `default y` value.

### Added

- `ReadPin(pin, level)`: returns whether the read worked and writes the level separately, so a failed read no longer
  looks like a low pin. `ReadPin(pin)` is unchanged.
- `examples/esp32/components/hf_pcal95555/Kconfig`, so `idf.py menuconfig` shows the driver options.
- Host unit tests (`-DHF_PCAL95555_BUILD_TESTS=ON`) against a register model of the PCA9555 and PCAL9555A, and a CI
  workflow that runs them with GCC and Clang.
- Host tests for Kconfig handling, inconclusive detection, `ReadPin(pin, level)`, empty masks, independent driver
  objects, and a CTest that installs the driver and builds a `find_package()` consumer.

### Deprecated

- The global-scope names `Pcal95555Reg`, `GPIODir`, `Polarity`, `DriveStrength`, `OutputMode`, `InterruptEdge`,
  `InterruptState` and `Error`: the types are now in namespace `pcal95555` and the global names are aliases, to be
  removed in the next major version. Define `HF_PCAL95555_NO_GLOBAL_ALIASES` to drop them now.
- `CtrlPin::RSTN`: neither the PCA9555 nor the PCAL9555A has a reset pin.

### Fixed

- Driver objects of one port type shared function-local static transfer buffers, so two chips used from two tasks could
  read each other's register values. Each object now has its own transfer byte.
- `cmake --install` did not install `src/pcal95555.ipp`, so an installed `pcal95555.hpp` did not compile, and the
  exported target was `hf::hf_pcal95555` instead of `hf::pcal95555`.


### Changed

- Documentation moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, architecture, PCA9555
  and PCAL9555A, interrupts, API guide, porting, testing), code snippets compiled by the host build from
  `tests/docs/`, Doxygen under `/api/`, versioned with mike. The Jekyll site files and the doxygen-awesome
  submodule are gone.
- `updateBit()` in `src/pcal95555.ipp` is `static inline`, so including the header no longer warns about an unused
  internal function.

## 1.0.0 (version in the build settings; not tagged)

- PCA9555 and PCAL9555A support with variant auto-detection, single-pin, mask and list APIs, Agile I/O, interrupt
  callbacks, address control and the ESP32-S3 examples.
