# Changelog

All notable changes to this driver are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Host unit tests (`-DHF_PCAL95555_BUILD_TESTS=ON`) against a register model of the PCA9555 and PCAL9555A, and a
  CI workflow that runs them with GCC and Clang.

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
