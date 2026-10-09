# SPDX-License-Identifier: MPL-2.0

.PHONY: jflash_erase
jflash_erase:
	$(JLINK_CMD).device $(JLINK_TARGET)
	@printf 'si 1\nspeed auto\nr\nh\nerase\nr\nq\n' > $(JLINK_ERASE_SCRIPT)
	$(JLINK_EXE) -device $(JLINK_TARGET) -if $(JLINK_IF) -speed $(JLINK_SPEED) -autoconnect 1 -nogui 1 -IP $(JLINK_IP) -CommanderScript $(JLINK_ERASE_SCRIPT)
	@rm -f $(JLINK_ERASE_SCRIPT)
	@echo "current time: $$(date +'%Y-%m-%d %H:%M:%S')"

.PHONY: jlink_rttlog
jlink_rttlog:
	$(JLINK_CMD).device $(JLINK_TARGET)
	myjlink.rttlog

.PHONY: jflash_write.runtime
jflash_write.runtime:
	$(JLINK_CMD).device $(JLINK_TARGET)
	@test -s "$(dir $(RUNTIME_PACKAGE_IMAGE))$(RUNTIME_JFLASH_HEX)" || { echo "Missing runtime HEX. Rebuild runtime and ensure MCUboot image generation is enabled." >&2; exit 1; }
	@test -s "$(dir $(BOOTLOADER_PACKAGE_IMAGE))$(BOOTLOADER_JFLASH_HEX)" || { echo "Missing bootloader HEX. Rebuild bootloader." >&2; exit 1; }
	cd "$(dir $(RUNTIME_PACKAGE_IMAGE))" && $(JLINK_CMD).write "$(RUNTIME_JFLASH_HEX)"
	cd "$(dir $(BOOTLOADER_PACKAGE_IMAGE))" && $(JLINK_CMD).write "$(BOOTLOADER_JFLASH_HEX)"
	@echo "current time: $$(date +'%Y-%m-%d %H:%M:%S')"

.PHONY: jflash_write.application
jflash_write.application:
	@test -s "$(APPLICATION_FLASH_DIR)/$(APPLICATION_FLASH_HEX)" || { echo "Missing $(APPLICATION_FLASH_DIR)/$(APPLICATION_FLASH_HEX). Build application with the same BOARD_PROFILE and ARDUINO_SKETCH, or specify APPLICATION_FLASH_DIR and APPLICATION_FLASH_HEX." >&2; exit 1; }
	$(JLINK_CMD).device $(JLINK_TARGET)
	cd "$(APPLICATION_FLASH_DIR)" && $(JLINK_CMD).write "$(APPLICATION_FLASH_HEX)"
	@echo "current time: $$(date +'%Y-%m-%d %H:%M:%S')"
