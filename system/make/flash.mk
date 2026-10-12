# SPDX-License-Identifier: MPL-2.0

JLINK_DOCKER_NETWORK ?= host
JLINK_RTT_EXE ?= /opt/segger/jlink/JLinkRTTLoggerExe

define jlink_run
	$(DOCKER_RUN_BASE) --network=$(JLINK_DOCKER_NETWORK) $(DOCKER_IMAGE) \
		sh -ec 'test -x "$(JLINK_EXE)"; script=$$(mktemp /tmp/rtbus-jlink.XXXXXX); trap '\''rm -f "$$script"'\'' EXIT HUP INT TERM; printf "%s\n" "ExitOnError 1" $(1) "Exit" > "$$script"; "$(JLINK_EXE)" -device "$(JLINK_TARGET)" -if "$(JLINK_IF)" -speed "$(JLINK_SPEED)" -IP "$(JLINK_IP)" -AutoConnect 1 -NoGui 1 -ExitOnError 1 -CommanderScript "$$script"'
endef

# Connect to the remote probe only; do not connect to or reset the MCU.
.PHONY: jlink.connect
jlink.connect: docker.image
	$(DOCKER_RUN_BASE) --network=$(JLINK_DOCKER_NETWORK) $(DOCKER_IMAGE) \
		sh -ec 'test -x "$(JLINK_EXE)"; script=$$(mktemp /tmp/rtbus-jlink-connect.XXXXXX); trap '\''rm -f "$$script"'\'' EXIT HUP INT TERM; printf "ExitOnError 1\nIP %s\nExit\n" "$(JLINK_IP)" > "$$script"; "$(JLINK_EXE)" -AutoConnect 0 -NoGui 1 -ExitOnError 1 -CommanderScript "$$script"'

.PHONY: jlink.erase
jlink.erase: docker.image
	$(call jlink_run,"r" "h" "erase" "r" "g")
	@echo "current time: $$(date +'%Y-%m-%d %H:%M:%S')"

.PHONY: jlink.rttlog
jlink.rttlog: docker.image
	$(DOCKER_RUN_BASE) --network=$(JLINK_DOCKER_NETWORK) $(DOCKER_IMAGE) \
		"$(JLINK_RTT_EXE)" -Device "$(JLINK_TARGET)" -If "$(JLINK_IF)" -Speed "$(JLINK_SPEED)" -IP "$(JLINK_IP)" -RTTSearchRanges "20000000 10000" -RTTChannel 0 /dev/stdout

.PHONY: jlink.download.runtime
jlink.download.runtime: docker.image
	@test -s "$(dir $(RUNTIME_PACKAGE_IMAGE))$(RUNTIME_JFLASH_HEX)" || { echo "Missing runtime HEX. Rebuild runtime and ensure MCUboot image generation is enabled." >&2; exit 1; }
	@test -s "$(dir $(BOOTLOADER_PACKAGE_IMAGE))$(BOOTLOADER_JFLASH_HEX)" || { echo "Missing bootloader HEX. Rebuild bootloader." >&2; exit 1; }
	$(call jlink_run,"r" "h" "loadfile \"$(call docker_path,$(dir $(RUNTIME_PACKAGE_IMAGE))$(RUNTIME_JFLASH_HEX))\"" "loadfile \"$(call docker_path,$(dir $(BOOTLOADER_PACKAGE_IMAGE))$(BOOTLOADER_JFLASH_HEX))\"" "r" "g")
	@echo "current time: $$(date +'%Y-%m-%d %H:%M:%S')"

.PHONY: jlink.download.application
jlink.download.application: docker.image
	@test -s "$(APPLICATION_FLASH_DIR)/$(APPLICATION_FLASH_HEX)" || { echo "Missing $(APPLICATION_FLASH_DIR)/$(APPLICATION_FLASH_HEX). Build application with the same BOARD_PROFILE and ARDUINO_SKETCH, or specify APPLICATION_FLASH_DIR and APPLICATION_FLASH_HEX." >&2; exit 1; }
	$(call jlink_run,"r" "h" "loadfile \"$(call docker_path,$(APPLICATION_FLASH_DIR)/$(APPLICATION_FLASH_HEX))\"" "r" "g")
	@echo "current time: $$(date +'%Y-%m-%d %H:%M:%S')"
