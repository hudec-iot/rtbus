# SPDX-License-Identifier: MPL-2.0
# Arduino platform staging and release package/index generation.
# Included by the root Makefile after the shared container configuration.

ARDUINO_PACKAGE_ROOT ?= .arduino/package
ARDUINO_PACKAGE_DIR ?= $(ARDUINO_PACKAGE_ROOT)/hardware/rtbus/rtduo
ARDUINO_DIST_DIR ?= dist/arduino
ARDUINO_PACKAGE_INDEX ?= package_rtbus_index.json
ARDUINO_RELEASE_REPOSITORY ?= hudec-iot/rtbus
ARDUINO_RELEASE_VERSION ?= $(strip $(shell sed -n '1{s/[[:space:]]//g;p;q;}' .version 2>/dev/null))
ARDUINO_PACKAGE_BASE_URL ?= https://github.com/$(ARDUINO_RELEASE_REPOSITORY)/releases/download/v$(ARDUINO_RELEASE_VERSION)
ARDUINO_PACKAGE_TOOL_ARGS ?=

.PHONY: arduino.package.local
arduino.package.local:
	rm -rf $(ARDUINO_PACKAGE_DIR)
	mkdir -p $(ARDUINO_PACKAGE_DIR)
	tar --exclude='build' --exclude='build.*' -cf - cores variants libraries system boards.txt platform.txt programmers.txt | tar -xf - -C $(ARDUINO_PACKAGE_DIR)

.PHONY: package.rtbus.index
package.rtbus.index: builder.image
	@test -n "$(ARDUINO_RELEASE_VERSION)" || { echo ".version is required" >&2; exit 1; }
	$(DOCKER_RUN) python3 tools/scripts/package_arduino.py \
		--output-dir $(ARDUINO_DIST_DIR) \
		--index-file $(ARDUINO_PACKAGE_INDEX) \
		--base-url "$(ARDUINO_PACKAGE_BASE_URL)" \
		--version "$(ARDUINO_RELEASE_VERSION)"
	cp $(ARDUINO_DIST_DIR)/$(ARDUINO_PACKAGE_INDEX) $(ARDUINO_PACKAGE_INDEX)

.PHONY: arduino.package.index
arduino.package.index: package.rtbus.index
