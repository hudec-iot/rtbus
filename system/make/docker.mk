# SPDX-License-Identifier: MPL-2.0
ifeq ($(OS),Windows_NT)
DOCKER_EXE ?=
DEFAULT_DOCKER_EXE := C:/Program Files/Docker/Docker/resources/bin/docker.exe
ifeq ($(strip $(DOCKER_EXE)),)
DETECTED_DOCKER_EXE := $(strip $(shell where docker.exe 2>NUL))
ifeq ($(strip $(DETECTED_DOCKER_EXE)),)
DETECTED_DOCKER_EXE := $(strip $(shell if exist "$(DEFAULT_DOCKER_EXE)" echo $(DEFAULT_DOCKER_EXE)))
endif
else
DETECTED_DOCKER_EXE := $(DOCKER_EXE)
endif
VM = "$(DETECTED_DOCKER_EXE)"
define resolve_container_cli
	@if "$(DETECTED_DOCKER_EXE)"=="" (echo Docker CLI not found. Install Docker Desktop, add docker.exe to PATH, or pass DOCKER_EXE="C:/path/to/docker.exe". & exit /b 1)
	@echo Container CLI: $(VM)
endef
else
VM ?= $(shell command -v podman >/dev/null 2>&1 && echo podman || echo docker)
define resolve_container_cli
	@command -v $(VM) >/dev/null 2>&1 || { echo "Container CLI not found: $(VM)" >&2; exit 1; }
	@echo Container CLI: $(VM)
endef
endif

ZEPHYR_SDK_VERSION ?= 1.0.0
ZEPHYR_VERSION ?= v4.4.0
DOCKER_IMAGE ?= ghcr.io/hudec-iot/rtbus-zephyr:1.0.0
DOCKER_WORK ?= /workdir
DOCKER_HOST_WORKDIR ?= $(CURDIR)
docker_path = $(if $(filter /%,$(1)),$(1),$(DOCKER_WORK)/$(1))
RTBUS_ZEPHYR_WORKSPACE ?= /opt/rtbus/zephyr-workspace
RTBUS_CONTAINER_BUILD_ROOT ?= /tmp/rtbus-build
RTBUS_ZEPHYR_VOLUME ?= rtbus-zephyr-workspace-$(ZEPHYR_VERSION)

EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
ZEPHYR_PROJECT_FILTER_RAW := -loramac-node,
ZEPHYR_PROJECT_FILTER_RAW += -hal_adi,
ZEPHYR_PROJECT_FILTER_RAW += -hal_afbr,
ZEPHYR_PROJECT_FILTER_RAW += -hal_atmel,
ZEPHYR_PROJECT_FILTER_RAW += -hal_bouffalolab,
ZEPHYR_PROJECT_FILTER_RAW += -hal_espressif,
ZEPHYR_PROJECT_FILTER_RAW += -hal_ethos_u,
ZEPHYR_PROJECT_FILTER_RAW += -hal_gigadevice,
ZEPHYR_PROJECT_FILTER_RAW += -hal_infineon,
ZEPHYR_PROJECT_FILTER_RAW += -hal_intel,
ZEPHYR_PROJECT_FILTER_RAW += -hal_microchip,
ZEPHYR_PROJECT_FILTER_RAW += -hal_nuvoton,
ZEPHYR_PROJECT_FILTER_RAW += -hal_nxp,
ZEPHYR_PROJECT_FILTER_RAW += -hal_openisa,
ZEPHYR_PROJECT_FILTER_RAW += -hal_quicklogic,
ZEPHYR_PROJECT_FILTER_RAW += -hal_realtek,
ZEPHYR_PROJECT_FILTER_RAW += -hal_renesas,
ZEPHYR_PROJECT_FILTER_RAW += -hal_rpi_pico,
ZEPHYR_PROJECT_FILTER_RAW += -hal_sifli,
ZEPHYR_PROJECT_FILTER_RAW += -hal_silabs,
ZEPHYR_PROJECT_FILTER_RAW += -hal_st,
ZEPHYR_PROJECT_FILTER_RAW += -hal_tdk,
ZEPHYR_PROJECT_FILTER_RAW += -hal_telink,
ZEPHYR_PROJECT_FILTER_RAW += -hal_ti,
ZEPHYR_PROJECT_FILTER_RAW += -hal_wch,
ZEPHYR_PROJECT_FILTER_RAW += -hal_wurthelektronik,
ZEPHYR_PROJECT_FILTER_RAW += -hal_xtensa
ZEPHYR_PROJECT_FILTER := $(subst $(SPACE),$(EMPTY),$(ZEPHYR_PROJECT_FILTER_RAW))

.PHONY: docker.image
docker.image:
	$(call resolve_container_cli)
	@$(VM) image inspect $(DOCKER_IMAGE) >/dev/null 2>&1 || { \
		echo "Pulling builder image: $(DOCKER_IMAGE)"; \
		$(VM) pull $(DOCKER_IMAGE); \
	}

.PHONY: zephyr.workspace
zephyr.workspace: docker.image
	$(VM) run --user root --rm \
		-v $(RTBUS_ZEPHYR_VOLUME):$(RTBUS_ZEPHYR_WORKSPACE) \
		-e ZEPHYR_SDK_INSTALL_DIR=/opt/toolchains/zephyr-sdk-$(ZEPHYR_SDK_VERSION) \
		-e ZEPHYR_TOOLCHAIN_VARIANT=zephyr \
		$(DOCKER_IMAGE) \
		sh -ec 'if [ -f "$(RTBUS_ZEPHYR_WORKSPACE)/.west/config" ] \
				&& [ -f "$(RTBUS_ZEPHYR_WORKSPACE)/zephyr/west.yml" ] \
				&& [ -d "$(RTBUS_ZEPHYR_WORKSPACE)/modules/hal/nordic" ] \
				&& [ -d "$(RTBUS_ZEPHYR_WORKSPACE)/modules/hal/stm32" ] \
				&& [ -d "$(RTBUS_ZEPHYR_WORKSPACE)/modules/hal/ambiq" ] \
				&& [ ! -d "$(RTBUS_ZEPHYR_WORKSPACE)/modules/hal/nxp" ]; then \
				exit 0; \
			fi; \
			echo "Zephyr workspace volume is missing or not using the reduced module set; initializing."; \
			git config --global http.version HTTP/1.1; \
			rm -rf "$(RTBUS_ZEPHYR_WORKSPACE)"/.west "$(RTBUS_ZEPHYR_WORKSPACE)"/zephyr "$(RTBUS_ZEPHYR_WORKSPACE)"/modules "$(RTBUS_ZEPHYR_WORKSPACE)"/bootloader "$(RTBUS_ZEPHYR_WORKSPACE)"/tools; \
			cd "$(RTBUS_ZEPHYR_WORKSPACE)"; \
			git clone --depth=1 --single-branch --branch "$(ZEPHYR_VERSION)" https://github.com/zephyrproject-rtos/zephyr zephyr; \
			west init -l zephyr; \
			west config manifest.project-filter -- "$(ZEPHYR_PROJECT_FILTER)"; \
			west -v update --narrow --fetch-opt=--depth=1 || { echo "Shallow west update failed; retrying with full fetch."; west -v update --narrow; }; \
			cd zephyr; \
			west zephyr-export; \
			echo "Zephyr workspace volume init done: $(RTBUS_ZEPHYR_VOLUME)"'

.PHONY: zephyr.workspace.clean
zephyr.workspace.clean:
	$(call resolve_container_cli)
	$(VM) volume rm $(RTBUS_ZEPHYR_VOLUME)

.PHONY: docker.images
docker.images:
	$(call resolve_container_cli)
	$(VM) images | grep -E 'rtbus-zephyr|embeddedcontainers/zephyr' || true

.PHONY: docker.shell
docker.shell: docker.image
	$(DOCKER_RUN_BASE) -it --user root $(DOCKER_IMAGE)

DOCKER_RUN_BASE = $(VM) run --rm \
	-v $(DOCKER_HOST_WORKDIR):$(DOCKER_WORK) \
	-v $(RTBUS_ZEPHYR_VOLUME):$(RTBUS_ZEPHYR_WORKSPACE) \
	--tmpfs $(DOCKER_WORK)/.west \
	-w $(DOCKER_WORK) \
	-e RTBUS_ZEPHYR_WORKSPACE=$(RTBUS_ZEPHYR_WORKSPACE) \
	-e ZEPHYR_BASE=$(RTBUS_ZEPHYR_WORKSPACE)/zephyr \
	-e ZEPHYR_SDK_INSTALL_DIR=/opt/toolchains/zephyr-sdk-$(ZEPHYR_SDK_VERSION) \
	-e ZEPHYR_TOOLCHAIN_VARIANT=zephyr

DOCKER_RUN = $(DOCKER_RUN_BASE) $(DOCKER_IMAGE)

