# SPDX-License-Identifier: MPL-2.0

BOARD_PROFILE_ROOT := cores
BOARD_PROFILE_ORDER := rak4631 rak3172p rak3172t rak4200
BOARD_PROFILE_CHOICES := $(strip $(foreach profile,$(BOARD_PROFILE_ORDER),$(if $(wildcard $(BOARD_PROFILE_ROOT)/$(profile)/profile.mk),$(profile))))

BOARD_PROFILE ?=
ARDUINO_BOARD_ID_rak4631 := RAK4631
ARDUINO_BOARD_ID_rak3172p := RAK3172P
ARDUINO_BOARD_ID_rak3172t := RAK3172T
ARDUINO_BOARD_ID_rak4200 := RAK4200
ARDUINO_BOARD_ID ?= $(ARDUINO_BOARD_ID_$(BOARD_PROFILE))
ARDUINO_FQBN ?= rtbus:rtduo:$(ARDUINO_BOARD_ID)
ARDUINO_APPLICATION_BUILD_DIR ?= $(ARDUINO_BUILD_ROOT)/$(ARDUINO_BOARD_ID)
ARDUINO_SKETCH_NAME ?= $(notdir $(ARDUINO_SKETCH))
ARDUINO_APPLICATION_JFLASH_HEX ?= $(ARDUINO_SKETCH_NAME).ino.signed.hex

BOARD_PROFILE_PROMPT_GOALS := \
	board.profile \
	runtime \
	bootloader \
	application \
	application.clean \
	jflash_erase \
	jlink_rttlog \
	jflash_write.runtime \
	jflash_write.application

BOARD_PROFILE_REQUESTED := $(filter $(BOARD_PROFILE_PROMPT_GOALS),$(MAKECMDGOALS))

define select_board_profile
$(strip $(shell \
	tty_path=`tty 2>/dev/null`; \
	if [ -z "$$tty_path" ] || [ "$$tty_path" = "not a tty" ]; then \
		exit 0; \
	fi; \
	printf 'Select BOARD_PROFILE:\n' > "$$tty_path"; \
	index=1; \
	for board in $(BOARD_PROFILE_CHOICES); do \
		printf '  %s. %s\n' "$$index" "$$board" > "$$tty_path"; \
		index=$$(($$index + 1)); \
	done; \
	printf 'BOARD_PROFILE [1]: ' > "$$tty_path"; \
	read answer < "$$tty_path"; \
	index=1; \
	for board in $(BOARD_PROFILE_CHOICES); do \
		if [ -z "$$answer" ] && [ "$$index" = "1" ]; then \
			printf '%s\n' "$$board"; \
			exit 0; \
		fi; \
		if [ "$$answer" = "$$index" ] || [ "$$answer" = "$$board" ]; then \
			printf '%s\n' "$$board"; \
			exit 0; \
		fi; \
		index=$$(($$index + 1)); \
	done; \
	printf '__invalid__%s\n' "$$answer"))
endef

ifeq ($(strip $(BOARD_PROFILE)),)
ifneq ($(BOARD_PROFILE_REQUESTED),)
BOARD_PROFILE := $(call select_board_profile)
ifeq ($(strip $(BOARD_PROFILE)),)
$(error BOARD_PROFILE is required for $(BOARD_PROFILE_REQUESTED). Use one of: $(BOARD_PROFILE_CHOICES))
endif
ifneq ($(filter __invalid__%,$(BOARD_PROFILE)),)
$(error Invalid BOARD_PROFILE selection: $(patsubst __invalid__%,%,$(BOARD_PROFILE)))
endif
endif
endif

ifneq ($(strip $(BOARD_PROFILE)),)
BOARD_PROFILE_DIR := $(BOARD_PROFILE_ROOT)/$(BOARD_PROFILE)
BOARD_PROFILE_MK := $(BOARD_PROFILE_DIR)/profile.mk
ifeq ($(wildcard $(BOARD_PROFILE_MK)),)
$(error Unsupported BOARD_PROFILE=$(BOARD_PROFILE). Use one of: $(BOARD_PROFILE_CHOICES))
endif
include $(BOARD_PROFILE_MK)
endif
BOARDS ?= $(ZEPHYR_BOARD)
BOARD_ROOTS_DOCKER = $(foreach root,$(BOARD_ROOTS),$(DOCKER_WORK)/$(root))
BOARD_ROOT_CMAKE = $(subst $(SPACE),;,$(strip $(BOARD_ROOTS_DOCKER)))
BOARD_ROOT_ARG = $(if $(strip $(BOARD_ROOTS)),-DBOARD_ROOT="$(BOARD_ROOT_CMAKE)",)

.PHONY: $(BOARD_PROFILE_CHOICES)
$(BOARD_PROFILE_CHOICES):
	$(MAKE) runtime BOARD_PROFILE=$@

.PHONY: board.profile
board.profile:
	@printf 'BOARD_PROFILE=%s\n' '$(BOARD_PROFILE)'
	@printf 'BOARD_PROFILE_DIR=%s\n' '$(BOARD_PROFILE_DIR)'
	@printf 'PROFILE_NAME=%s\n' '$(PROFILE_NAME)'
	@printf 'BOARDS=%s\n' '$(BOARDS)'
	@printf 'ZEPHYR_BOARD=%s\n' '$(ZEPHYR_BOARD)'
	@printf 'ZEPHYR_BOARD_SOURCE=%s\n' '$(ZEPHYR_BOARD_SOURCE)'
	@printf 'ZEPHYR_SOC=%s\n' '$(ZEPHYR_SOC)'
	@printf 'BOARD_ROOTS=%s\n' '$(BOARD_ROOTS)'
	@printf 'BOARD_ROOT_CMAKE=%s\n' '$(BOARD_ROOT_CMAKE)'
	@printf 'BOARD_ROOT_ARG=%s\n' '$(BOARD_ROOT_ARG)'
	@printf 'JLINK_TARGET=%s\n' '$(JLINK_TARGET)'
	@printf 'JLINK_EXE=%s\n' '$(JLINK_EXE)'
	@printf 'JLINK_IP=%s\n' '$(JLINK_IP)'
	@printf 'RUNTIME_BOARD_DIR=%s\n' '$(RUNTIME_BOARD_DIR)'
	@printf 'BOARD_RUNTIME_CONF=%s\n' '$(BOARD_RUNTIME_CONF)'
	@printf 'BOARD_RUNTIME_OVERLAY=%s\n' '$(BOARD_RUNTIME_OVERLAY)'
	@printf 'BOOTLOADER_BOARD_DIR=%s\n' '$(BOOTLOADER_BOARD_DIR)'
	@printf 'BOARD_BOOTLOADER_CONF=%s\n' '$(BOARD_BOOTLOADER_CONF)'
	@printf 'BOARD_BOOTLOADER_OVERLAY=%s\n' '$(BOARD_BOOTLOADER_OVERLAY)'
	@printf 'ARDUINO_APPLICATION_BUILD_DIR=%s\n' '$(ARDUINO_APPLICATION_BUILD_DIR)'
