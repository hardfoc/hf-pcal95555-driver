
#===============================================================================
# PCAL95555 Driver - Root CMake
# Defines the main build target for the driver. Minimal, platform-agnostic.
#===============================================================================

cmake_minimum_required(VERSION 3.16)

project(hf_pcal95555_driver LANGUAGES CXX)


#===============================================================================
# Load shared build settings (target name, includes, sources, dependencies)
#===============================================================================
include(${CMAKE_CURRENT_LIST_DIR}/cmake/hf_pcal95555_build_settings.cmake)

#===============================================================================
# Target type selection
# INTERFACE for header-only, STATIC if sources present
#===============================================================================
# This pattern keeps the target name stable for consumers while allowing
# implementation mode changes without API churn.
if(HF_PCAL95555_SOURCE_FILES)
    add_library(${HF_PCAL95555_TARGET_NAME} STATIC ${HF_PCAL95555_SOURCE_FILES})
    target_include_directories(${HF_PCAL95555_TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
            $<BUILD_INTERFACE:${HF_PCAL95555_VERSION_HEADER_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    target_compile_features(${HF_PCAL95555_TARGET_NAME} PUBLIC cxx_std_20)
else()
    add_library(${HF_PCAL95555_TARGET_NAME} INTERFACE)
    target_include_directories(${HF_PCAL95555_TARGET_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
            $<BUILD_INTERFACE:${HF_PCAL95555_VERSION_HEADER_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    target_compile_features(${HF_PCAL95555_TARGET_NAME} INTERFACE cxx_std_20)
endif()

# =============================================================================
# Namespaced Consumer Alias
# =============================================================================
# Recommended consumption style in external projects:
#   target_link_libraries(my_app PRIVATE hf::pcal95555)
#
# This alias remains stable even if the underlying concrete target changes.
add_library(hf::pcal95555 ALIAS ${HF_PCAL95555_TARGET_NAME})

#===============================================================================
# Versioning
# Version variables and header generation are defined in:
#   cmake/hf_pcal95555_build_settings.cmake (single source of truth)
#===============================================================================
set_target_properties(${HF_PCAL95555_TARGET_NAME} PROPERTIES VERSION ${HF_PCAL95555_VERSION} SOVERSION ${HF_PCAL95555_VERSION_MAJOR})

#===============================================================================
# Optional: Enable extra warnings (can be toggled by user)
#===============================================================================
# Default OFF: INTERFACE warnings propagate to every consumer that links this
# library, which can generate noise in downstream builds.  Enable explicitly
# with -D HF_PCAL95555_ENABLE_WARNINGS=ON when developing the driver itself.
# Example projects already set their own warning flags in main/CMakeLists.txt.
option(HF_PCAL95555_ENABLE_WARNINGS "Enable extra warnings for PCAL95555 driver" OFF)
if(HF_PCAL95555_ENABLE_WARNINGS)
    if(TARGET ${HF_PCAL95555_TARGET_NAME})
        target_compile_options(${HF_PCAL95555_TARGET_NAME} INTERFACE -Wall -Wextra -Wpedantic)
    endif()
endif()

#===============================================================================
# Host unit tests (register model of the chip; no hardware)
#===============================================================================
option(HF_PCAL95555_BUILD_TESTS "Build the host unit tests in tests/" OFF)
if(HF_PCAL95555_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#===============================================================================
# Install and export support (for find_package usage)
#===============================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${HF_PCAL95555_TARGET_NAME} EXPORT hf_pcal95555Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install hand-written headers + generated version header
install(DIRECTORY inc/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.h.in" EXCLUDE)
install(FILES "${HF_PCAL95555_VERSION_HEADER}" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT hf_pcal95555Targets
    FILE hf_pcal95555Targets.cmake
    NAMESPACE hf::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_pcal95555
)

# Generate and install package config so find_package(hf_pcal95555) works
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/hf_pcal95555Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hf_pcal95555Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_pcal95555
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hf_pcal95555ConfigVersion.cmake"
    VERSION ${HF_PCAL95555_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hf_pcal95555Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hf_pcal95555ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hf_pcal95555
)
