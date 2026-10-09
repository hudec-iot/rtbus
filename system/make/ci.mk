# SPDX-License-Identifier: MPL-2.0

GITHUB_CI_DOCKER_RUN = $(DOCKER_RUN_BASE) -e GH_TOKEN $(DOCKER_IMAGE)

.PHONY: ci.github.gh
ci.github.gh: docker.image
	@test -n "$(GH_TOKEN)" || { echo "GH_TOKEN is required" >&2; exit 1; }
	$(GITHUB_CI_DOCKER_RUN) gh $(GH_ARGS)
