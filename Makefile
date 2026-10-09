# SPDX-License-Identifier: MPL-2.0

.DEFAULT_GOAL := arduino.boards

include system/make/config.mk
include tools/docker/docker.mk
include system/make/boards.mk

include zephyr/runtime/runtime.mk
include zephyr/bootloader.mk
include tools/scripts/package_arduino.mk
include system/make/arduino.mk
include system/make/flash.mk
include system/make/ci.mk

.PHONY: application
application: application.arduino

.PHONY: application.clean
application.clean: application.arduino.clean

