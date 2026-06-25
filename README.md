# CSC1107 Operating Systems Project 15

Linux kernel module for tracking USB file transfer activity.

## Project Scope

Refer to [`docs/OVERVIEW.md`](docs/OVERVIEW.md) for an in-depth overview.

- [`kernel/`](kernel/)
  - Linux kernel module source code
- [`userspace/`](userspace/)
  - User-space program source code
  - CLI program to interface with the kernel module
- [`scripts/demo.sh`](scripts/demo.sh)
  - End-to-end automated demo on a Pi 4 with a USB drive
- [`scripts/get-pi-headers.sh`](scripts/get-pi-headers.sh)
  - Fetch and prepare RPi kernel headers for cross-compilation

## Usage

For end-user usage on a Raspberry Pi, refer to [`docs/USAGE.md`](docs/USAGE.md).

## Getting Started

### Prerequisites

- [Git](https://github.com/git-guides/install-git) (fully set-up)
- [Docker/Podman](https://docs.docker.com/engine/install/)
- [VS Code](https://code.visualstudio.com/download)
  - [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension

> [!WARNING]
> Do not use GitHub Desktop! All interactions (files, git, runtime, etc.) should be done through the Dev Container within VS Code.

### Installation

1. Clone the repo

   ```sh
   git clone git@github.com:commit2main/CSC1107.git
   ```

2. Open the repository in VS Code

   ```sh
   code CSC1107/
   ```

3. Click on the "Re-open in Dev Container" prompt
4. Start working!

### Cross-compilation

```sh
./scripts/get-pi-headers.sh [PI_KERNEL_VERSION] [PI_HOST]

make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KERNELDIR=/workspaces/CSC1107/pi-kernel
```

- `./scripts/get-pi-headers.sh`
  - clone the script's default Raspberry Pi kernel branch and use Pi 4 default config
- `./scripts/get-pi-headers.sh 6.18.34`
  - clone specific version branch
- `./scripts/get-pi-headers.sh 6.18.34 pi@192.168.1.10`
  - copy the config from RPi for exact match

## Developer Tooling

- Dev Containers
  - Standardised developer environment
- Pre-Commit
  - Run linting and formatting for all files during git commit
