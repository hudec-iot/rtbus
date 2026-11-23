# SPDX-License-Identifier: MPL-2.0

PROFILE_NAME := stm32wba62
APPLICATION_TARGET_MCU ?= cortex-m33
# Zephyr v4.4.0 bring-up uses the upstream WBA65 board and SoC.
ZEPHYR_BOARD ?= nucleo_wba65ri/stm32wba65xx
ZEPHYR_BOARD_SOURCE ?= upstream
ZEPHYR_SOC ?= stm32wba65xx
ZEPHYR_BLOB_MODULES ?= hal_stm32
JLINK_TARGET ?= STM32WBA62PI
RUNTIME_BOARD_DIR ?= cores/stm32wba62/zephyr
