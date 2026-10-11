# SPDX-License-Identifier: MPL-2.0

.DEFAULT_GOAL := arduino.boards

include system/make/docker.mk

include system/make/config.mk
include system/make/boards.mk

include system/make/arduino.mk
include system/make/flash.mk
include system/make/ci.mk


include zephyr/runtime/runtime.mk
include zephyr/bootloader.mk

