/**
 * @file ports.cpp
 * @brief Instantiates the driver over the documented port, so every member compiles in full.
 */
#include "my_i2c.hpp"

template class pcal95555::PCAL95555<MyI2c>;
