# SPDX-License-Identifier: MPL-2.0
# Board-local entry point: run the native compiler inside the builder image.

.DEFAULT_GOAL := all
BOARD_PROFILE ?= rak3172t
CORE_MAKEFILE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
REPO_ROOT := $(abspath $(CORE_MAKEFILE_DIR)/../..)
DOCKER_HOST_WORKDIR := $(REPO_ROOT)
include $(REPO_ROOT)/tools/docker/docker.mk

BUILD_DIR ?= $(CORE_MAKEFILE_DIR)/build
SRC ?= $(REPO_ROOT)/cores/$(BOARD_PROFILE)/main.c
GCC_PATH ?= /opt/toolchains/zephyr-sdk-$(ZEPHYR_SDK_VERSION)/gnu/arm-zephyr-eabi/bin
HOST_CC ?= cc
TARGET ?= application
MODULE ?= application
MODULE_NAME ?= application
APPLICATION_VERSION ?= 0.1.0
APPLICATION_BUILD ?= 0

core_container_path = $(if $(filter $(REPO_ROOT)/%,$(abspath $(1))),$(DOCKER_WORK)/$(patsubst $(REPO_ROOT)/%,%,$(abspath $(1))),$(abspath $(1)))

.PHONY: all compile clean
all: compile

compile: docker.image
	$(DOCKER_RUN) $(MAKE) -f "$(DOCKER_WORK)/system/make/core.mk" \
		BOARD_PROFILE="$(BOARD_PROFILE)" \
		BUILD_DIR="$(call core_container_path,$(BUILD_DIR))" \
		SRC="$(call core_container_path,$(SRC))" \
		TARGET="$(TARGET)" MODULE="$(MODULE)" MODULE_NAME="$(MODULE_NAME)" \
		APPLICATION_VERSION="$(APPLICATION_VERSION)" APPLICATION_BUILD="$(APPLICATION_BUILD)" \
		ZEPHYR_SDK_VERSION="$(ZEPHYR_SDK_VERSION)" GCC_PATH="$(GCC_PATH)" \
		HOST_CC="$(HOST_CC)"

clean:
	$(MAKE) -f "$(REPO_ROOT)/system/make/core.mk" \
		BOARD_PROFILE="$(BOARD_PROFILE)" BUILD_DIR="$(abspath $(BUILD_DIR))" clean
