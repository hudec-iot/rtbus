# RTDuo Platform

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
