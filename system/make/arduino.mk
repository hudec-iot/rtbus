# SPDX-License-Identifier: MPL-2.0

ARDUINO_PACKAGE_ROOT ?= .arduino/package
ARDUINO_PACKAGE_DIR ?= $(ARDUINO_PACKAGE_ROOT)/hardware/rtbus/rtduo

ARDUINO_LOCAL_BUILD_PROPERTIES = \
	--build-property compiler.path=$(ARDUINO_LOCAL_COMPILER_PATH) \
	--build-property compiler.host.path=$(ARDUINO_LOCAL_HOST_COMPILER_PATH) \
	--build-property compiler.host.cmd=$(ARDUINO_LOCAL_HOST_COMPILER_CMD) \
	--build-property compiler.host.flags=$(ARDUINO_LOCAL_HOST_COMPILER_FLAGS) \
	--build-property tools.ymodem_upload.cmd.path=$(ARDUINO_LOCAL_HOST_COMPILER_PATH)$(ARDUINO_LOCAL_HOST_COMPILER_CMD) \
	--build-property tools.ymodem_upload.flags=$(ARDUINO_LOCAL_HOST_COMPILER_FLAGS)

.PHONY: arduino.version
arduino.version: docker.image
	$(DOCKER_RUN) arduino-cli version

.PHONY: arduino.boards
arduino.boards: docker.image arduino.package.local
	$(DOCKER_RUN) arduino-cli --config-file $(ARDUINO_CONFIG) board listall rtduo

.PHONY: application
application: docker.image arduino.package.local
	@test -n "$(ARDUINO_BOARD_ID)" || { echo "Unsupported BOARD_PROFILE=$(BOARD_PROFILE)" >&2; exit 1; }
	@$(DOCKER_RUN) arduino-cli --config-file $(ARDUINO_CONFIG) compile \
		--verbose \
		--fqbn $(ARDUINO_FQBN) \
		--build-path "$(call docker_path,$(ARDUINO_APPLICATION_BUILD_DIR))" \
		$(ARDUINO_LOCAL_BUILD_PROPERTIES) \
		--build-property build.application.pack=true \
		--build-property "build.application.export.path=" \
		"$(call docker_path,$(ARDUINO_SKETCH))"

.PHONY: application.clean
application.clean:
	rm -rf "$(ARDUINO_APPLICATION_BUILD_DIR)"

.PHONY: arduino.clean
arduino.clean:
	rm -rf "$(ARDUINO_BUILD_ROOT)" "$(ARDUINO_PACKAGE_DIR)"

.PHONY: arduino.package.local
arduino.package.local:
	rm -rf $(ARDUINO_PACKAGE_DIR)
	mkdir -p $(ARDUINO_PACKAGE_DIR)
	tar --exclude='build' --exclude='build.*' --exclude='.key-backup*' --exclude='bootloader.hex' --exclude='runtime.signed.hex' --exclude='libraries/RTDuo/development' -cf - cores variants libraries system boards.txt platform.txt programmers.txt LICENSE NOTICE SOURCE_INFO.md THIRD_PARTY_NOTICES.md | tar -xf - -C $(ARDUINO_PACKAGE_DIR)
	cp zephyr/application-signing.md "$(ARDUINO_PACKAGE_DIR)/APPLICATION_SIGNING.md"
