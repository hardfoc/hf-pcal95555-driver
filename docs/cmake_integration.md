---
layout: default
title: "⚙️ CMake Integration"
description: "How to integrate the PCAL95555 driver into your CMake project"
nav_order: 5
parent: "📚 Documentation"
permalink: /docs/cmake_integration/
---

# PCAL95555 — CMake Integration Guide

How to consume the PCAL95555 driver in your CMake or ESP-IDF project.


---

## Quick Start (Generic CMake)

### Option A — Git Submodule (recommended)

```bash
# From your project root
git submodule add https://github.com/hardfoc/hf-pcal95555-driver.git external/hf-pcal95555-driver
git submodule update --init --recursive
```

In your top-level `CMakeLists.txt`:

```cmake
add_subdirectory(external/hf-pcal95555-driver)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE hf::pcal95555)
```

The `hf::pcal95555` alias target provides the header include paths, C++20
compile feature, and generated version header automatically.

### Option B — Cloned / Vendored Copy

Same as above — just point `add_subdirectory()` at wherever you placed the
driver checkout:

```cmake
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/hf-pcal95555-driver)
target_link_libraries(my_app PRIVATE hf::pcal95555)
```

### Option C — Installed Package (`find_package`)

If the driver has been installed to a system or prefix path:

```bash
# Install from source
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/hf-drivers
cmake --build build
cmake --install build
```

Then in the consuming project:

```cmake
find_package(hf_pcal95555 REQUIRED)
target_link_libraries(my_app PRIVATE hf::pcal95555)
```

CMake will locate the package via `hf_pcal95555Config.cmake` installed in
`<prefix>/lib/cmake/hf_pcal95555/`.

---

## ESP-IDF Integration

The driver ships with an ESP-IDF component wrapper in
`examples/esp32/components/hf_pcal95555/`.

### 1. Reference the Component

**Option 1 — Symlink or copy the wrapper:**

```
your-esp-project/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   └── app_main.cpp
└── components/
    └── hf_pcal95555/
        └── CMakeLists.txt      ← copy from driver's examples/esp32/components/
```

**Option 2 — Use `EXTRA_COMPONENT_DIRS`:**

```cmake
# In your project-level CMakeLists.txt, before include($ENV{IDF_PATH}/...)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../path/to/hf-pcal95555-driver/examples/esp32/components")
```

### 2. Override the Driver Root (if needed)

The component wrapper auto-detects the driver root via a relative path.  If
your directory layout differs, override it:

```cmake
set(HF_PCAL95555_ROOT "/absolute/path/to/hf-pcal95555-driver" CACHE PATH "")
```

### 3. Require the Component

In your `main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES hf_pcal95555 driver
)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
```

### 4. Include and Use

```cpp
#include "pcal95555.hpp"
#include "pcal95555_version.h"  // optional — version macros

// Create your platform I2C implementation, then:
pcal95555::PCAL95555<MyI2cBus> driver(&bus, /*a0=*/false, /*a1=*/false, /*a2=*/false);
driver.EnsureInitialized();
```

---

## PCAL95555-Specific Build Variables

These variables are defined in `cmake/hf_pcal95555_build_settings.cmake`:

| Variable | Value |
|----------|-------|
| `HF_PCAL95555_TARGET_NAME` | `hf_pcal95555` |
| `HF_PCAL95555_VERSION` | Current `MAJOR.MINOR.PATCH` |
| `HF_PCAL95555_PUBLIC_INCLUDE_DIRS` | `inc/` + generated header dir |
| `HF_PCAL95555_SOURCE_FILES` | `""` (header-only) |
| `HF_PCAL95555_IDF_REQUIRES` | `driver` |


---

## Version Header

The build system generates `pcal95555_version.h` from `inc/pcal95555_version.h.in`
at configure time.  It provides:

```c
#define HF_PCAL95555_VERSION_MAJOR  1
#define HF_PCAL95555_VERSION_MINOR  0
#define HF_PCAL95555_VERSION_PATCH  0
#define HF_PCAL95555_VERSION_STRING "1.0.0"
```

Include it in your application to verify at runtime which version is compiled in:

```cpp
#include "pcal95555_version.h"
printf("PCAL95555 driver v%s\n", HF_PCAL95555_VERSION_STRING);
```

---

**Navigation**
⬅️ [Back to Documentation Index](index.md)
