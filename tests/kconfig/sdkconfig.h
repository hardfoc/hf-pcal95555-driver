/**
 * @file sdkconfig.h
 * @brief Stand-in for the sdkconfig.h that ESP-IDF generates from the component's Kconfig.
 *
 * Only test_kconfig.cpp sees this directory. It mirrors what Kconfig emits for a project that keeps
 * the defaults except: pins P0_0 and P1_7 set to outputs (their "Input direction" bool set to n,
 * which Kconfig writes as an absent macro), P0_1 driven high, and the pull-up choice of P0_2 set to
 * n (pull-down).
 */
#pragma once

#define CONFIG_PCAL95555 1
#define CONFIG_PCAL95555_DEFAULT_ADDRESS 0x20
#define CONFIG_PCAL95555_INIT_FROM_KCONFIG 1

/* Port 0 */
/* CONFIG_PCAL95555_P0_0_DIR is not set */
#define CONFIG_PCAL95555_P0_0_PULL_UP 1
#define CONFIG_PCAL95555_P0_1_DIR 1
#define CONFIG_PCAL95555_P0_1_PULL_UP 1
#define CONFIG_PCAL95555_P0_1_OUTPUT 1
#define CONFIG_PCAL95555_P0_2_DIR 1
/* CONFIG_PCAL95555_P0_2_PULL_UP is not set */
#define CONFIG_PCAL95555_P0_3_DIR 1
#define CONFIG_PCAL95555_P0_3_PULL_UP 1
#define CONFIG_PCAL95555_P0_4_DIR 1
#define CONFIG_PCAL95555_P0_4_PULL_UP 1
#define CONFIG_PCAL95555_P0_5_DIR 1
#define CONFIG_PCAL95555_P0_5_PULL_UP 1
#define CONFIG_PCAL95555_P0_6_DIR 1
#define CONFIG_PCAL95555_P0_6_PULL_UP 1
#define CONFIG_PCAL95555_P0_7_DIR 1
#define CONFIG_PCAL95555_P0_7_PULL_UP 1

/* Port 1 */
#define CONFIG_PCAL95555_P1_0_DIR 1
#define CONFIG_PCAL95555_P1_0_PULL_UP 1
#define CONFIG_PCAL95555_P1_1_DIR 1
#define CONFIG_PCAL95555_P1_1_PULL_UP 1
#define CONFIG_PCAL95555_P1_2_DIR 1
#define CONFIG_PCAL95555_P1_2_PULL_UP 1
#define CONFIG_PCAL95555_P1_3_DIR 1
#define CONFIG_PCAL95555_P1_3_PULL_UP 1
#define CONFIG_PCAL95555_P1_4_DIR 1
#define CONFIG_PCAL95555_P1_4_PULL_UP 1
#define CONFIG_PCAL95555_P1_5_DIR 1
#define CONFIG_PCAL95555_P1_5_PULL_UP 1
#define CONFIG_PCAL95555_P1_6_DIR 1
#define CONFIG_PCAL95555_P1_6_PULL_UP 1
/* CONFIG_PCAL95555_P1_7_DIR is not set */
#define CONFIG_PCAL95555_P1_7_PULL_UP 1
