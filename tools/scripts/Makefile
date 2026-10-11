# SPDX-License-Identifier: MPL-2.0
# Release archive/index generation; GitHub Actions owns publication.
.DEFAULT_GOAL := package

PACKAGE_MAKEFILE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
RTBUS_SOURCE_ROOT := $(abspath $(PACKAGE_MAKEFILE_DIR)/../..)
PYTHON ?= python3
ARDUINO_DIST_DIR ?= dist/arduino
ARDUINO_PACKAGE_INDEX ?= package_rtbus_index.json
ARDUINO_RELEASE_REPOSITORY ?= hudec-iot/rtbus
ARDUINO_RELEASE_VERSION ?= $(strip $(shell sed -n '1{s/[[:space:]]//g;p;q;}' "$(RTBUS_SOURCE_ROOT)/.version" 2>/dev/null))
ARDUINO_PACKAGE_BASE_URL ?= https://github.com/$(ARDUINO_RELEASE_REPOSITORY)/releases/download/v$(ARDUINO_RELEASE_VERSION)
ARDUINO_PREVIOUS_INDEX_URL ?=

.PHONY: package
package:
	@test -n "$(ARDUINO_RELEASE_VERSION)" || { echo ".version or ARDUINO_RELEASE_VERSION is required" >&2; exit 1; }
	$(PYTHON) "$(PACKAGE_MAKEFILE_DIR)/package_arduino.py" \
		--output-dir "$(ARDUINO_DIST_DIR)" \
		--index-file "$(ARDUINO_PACKAGE_INDEX)" \
		--base-url "$(ARDUINO_PACKAGE_BASE_URL)" \
		--version "$(ARDUINO_RELEASE_VERSION)" \
		$(if $(strip $(ARDUINO_PREVIOUS_INDEX_URL)),--previous-index-url "$(ARDUINO_PREVIOUS_INDEX_URL)",)
