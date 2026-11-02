# Consumer of an installed hf-pcal95555-driver: find_package() + one translation unit that
# instantiates the driver. Run by the hf_pcal95555_install test (tests/CMakeLists.txt).
cmake_minimum_required(VERSION 3.16)
project(hf_pcal95555_install_consumer LANGUAGES CXX)

find_package(hf_pcal95555 REQUIRED CONFIG)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE hf::pcal95555)
