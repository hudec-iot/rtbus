# RTDuo Platform

Extension source lives in `.vscode/extensions/rtduo-platform/`, alongside the
repository's VS Code settings. This directory does not automatically install
the extension into VS Code.

The workspace recommendation in `.vscode/extensions.json` points to
`rtbus.rtduo-platform`, matching this extension's publisher and name. Installing
from that recommendation requires a Marketplace release under that ID. The
recommendation does not load this local source directory or install host tools.
After the extension is installed, its commands check for Docker/Podman and pull
the builder image when needed. A complete environment installation wizard is
not implemented yet.

This extension uses the published RTDuo builder image from
[GitHub Packages](https://github.com/orgs/hudec-iot/packages/container/package/rtbus-zephyr).

Install Docker or Podman on the machine running the extension, and make sure
`make` is available for application builds. Open the RTDuo repository in VS Code
and run either command:

- `RTDuo: Compile Arduino Example`
- `RTDuo: Open Builder Shell`

Both commands reuse `ghcr.io/hudec-iot/rtbus-zephyr:1.0.0` when it is already
available locally, or pull it automatically when missing. No local image build
or host Zephyr SDK installation is needed. Application builds use the repository
Makefile with the same image and container CLI.

The local Dockerfile and `tools/docker/Makefile` remain available for image
maintenance. GitHub Actions handles image publication.
