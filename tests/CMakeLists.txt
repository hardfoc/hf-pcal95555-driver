# =============================================================================
# hf-pcal95555-driver — host unit tests
# =============================================================================
# Builds the driver headers against a register model of the PCA9555 / PCAL9555A behind the I2C
# interface and runs the tests with CTest. No hardware and no ESP-IDF needed:
#
#   cmake -S . -B build-host -DHF_PCAL95555_BUILD_TESTS=ON
#   cmake --build build-host
#   ctest --test-dir build-host --output-on-failure
# =============================================================================

add_executable(hf_pcal95555_host_tests
  test_main.cpp
  test_driver.cpp
  test_kconfig.cpp
)

# Only test_kconfig.cpp sees the stand-in sdkconfig.h.
set_source_files_properties(test_kconfig.cpp PROPERTIES
  INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/kconfig)

target_link_libraries(hf_pcal95555_host_tests PRIVATE ${HF_PCAL95555_TARGET_NAME})
target_include_directories(hf_pcal95555_host_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(hf_pcal95555_host_tests PRIVATE cxx_std_20)
target_compile_options(hf_pcal95555_host_tests PRIVATE -Wall -Wextra)

add_test(NAME hf_pcal95555_host_tests COMMAND hf_pcal95555_host_tests)

# The code shown on the documentation site (docs/ includes these files with pymdownx.snippets), built so a
# page cannot show code that does not compile against the current headers. Compiled, never linked or run.
add_library(hf_pcal95555_doc_snippets OBJECT
  docs/quickstart.cpp
  docs/api_tasks.cpp
  docs/ports.cpp
  docs/configuration.cpp
)
target_link_libraries(hf_pcal95555_doc_snippets PRIVATE ${HF_PCAL95555_TARGET_NAME})
target_include_directories(hf_pcal95555_doc_snippets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/docs)
target_compile_features(hf_pcal95555_doc_snippets PRIVATE cxx_std_20)
target_compile_options(hf_pcal95555_doc_snippets PRIVATE -Wall -Wextra)
target_compile_definitions(hf_pcal95555_doc_snippets PRIVATE HF_PCAL95555_NO_GLOBAL_ALIASES)
