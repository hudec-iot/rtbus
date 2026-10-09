# SPDX-License-Identifier: MPL-2.0

ARDUINO_LOCAL_BUILD_PROPERTIES = \
	--build-property compiler.path=$(ARDUINO_LOCAL_COMPILER_PATH) \
	--build-property compiler.host.path=$(ARDUINO_LOCAL_HOST_COMPILER_PATH) \
	--build-property compiler.host.cmd=$(ARDUINO_LOCAL_HOST_COMPILER_CMD) \
	--build-property compiler.host.flags=$(ARDUINO_LOCAL_HOST_COMPILER_FLAGS) \
	--build-property tools.ymodem_upload.cmd.path=$(ARDUINO_LOCAL_HOST_COMPILER_PATH)$(ARDUINO_LOCAL_HOST_COMPILER_CMD) \
	--build-property tools.ymodem_upload.flags=$(ARDUINO_LOCAL_HOST_COMPILER_FLAGS)

.PHONY: arduino.version
arduino.version: builder.image
	$(DOCKER_RUN) arduino-cli version

.PHONY: arduino.boards
arduino.boards: builder.image arduino.package.local
	$(DOCKER_RUN) arduino-cli --config-file $(ARDUINO_CONFIG) board listall rtduo

.PHONY: application.arduino
application.arduino: builder.image arduino.package.local
	@test -n "$(ARDUINO_BOARD_ID)" || { echo "Unsupported BOARD_PROFILE=$(BOARD_PROFILE)" >&2; exit 1; }
	$(DOCKER_RUN) arduino-cli --config-file $(ARDUINO_CONFIG) compile \
		--verbose \
		--fqbn $(ARDUINO_FQBN) \
		--build-path "$(call docker_path,$(ARDUINO_APPLICATION_BUILD_DIR))" \
		$(ARDUINO_LOCAL_BUILD_PROPERTIES) \
		--build-property build.application.pack=true \
		--build-property "build.application.export.path=" \
		"$(call docker_path,$(ARDUINO_SKETCH))"

.PHONY: application.arduino.clean
application.arduino.clean:
	rm -rf "$(ARDUINO_APPLICATION_BUILD_DIR)"

.PHONY: arduino.clean
arduino.clean:
	rm -rf "$(ARDUINO_BUILD_ROOT)" "$(ARDUINO_PACKAGE_DIR)"
