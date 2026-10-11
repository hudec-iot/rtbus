# SPDX-License-Identifier: MPL-2.0
# Builder image build entry; GitHub Actions owns publication.
.DEFAULT_GOAL := build

DOCKER_MAKEFILE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
RTBUS_SOURCE_ROOT := $(abspath $(DOCKER_MAKEFILE_DIR)/../..)
include $(RTBUS_SOURCE_ROOT)/system/make/docker.mk

ARDUINO_CLI_VERSION ?= 1.5.1
ZEPHYR_BASE_IMAGE ?= ghcr.io/embeddedcontainers/zephyr:arm-$(ZEPHYR_SDK_VERSION)SDK
RTBUS_BUILDER_DOCKERFILE ?= $(DOCKER_MAKEFILE_DIR)/Dockerfile.embedded-arm
RTBUS_BUILDER_IMAGE ?= $(DOCKER_IMAGE)

.PHONY: build
build:
	$(call resolve_container_cli)
	$(VM) build \
		--build-arg EMBEDDED_ZEPHYR_IMAGE=$(ZEPHYR_BASE_IMAGE) \
		--build-arg ZEPHYR_SDK_VERSION=$(ZEPHYR_SDK_VERSION) \
		--build-arg RTBUS_ZEPHYR_WORKSPACE=$(RTBUS_ZEPHYR_WORKSPACE) \
		--build-arg ARDUINO_CLI_VERSION=$(ARDUINO_CLI_VERSION) \
		-t $(RTBUS_BUILDER_IMAGE) \
		-f "$(RTBUS_BUILDER_DOCKERFILE)" "$(RTBUS_SOURCE_ROOT)"
