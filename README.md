# CSC1107 Operating Systems Project

> [!NOTE]
> WIP, remove this note when project is ready.

Linux Kernel Module for tracking USB file transfer activity.

---

## Project Scope

- [link_to_source](link_to_source)
  - scope_description

## Usage

### Cross-compilation

```sh
./get-pi-headers.sh [PI_KERNEL_VERSION] [PI_HOST]

make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KERNELDIR=/workspaces/CSC1107/pi-kernel
```
- `./get-pi-headers.sh`
  - clone rpi-6.18.y, use Pi 4 default config
- `./get-pi-headers.sh 6.18.23`
  - clone specific version branch
- `./get-pi-headers.sh 6.18.23 pi@192.168.1.10`
  - also scp .config from Pi for exact match

### runtime_executable

```sh
./runtime_executable <args> [optional]
```

- runtime_executable_description

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

## Developer Tooling

- Dev Containers
  - Standardised developer environment
- Pre-Commit
  - Run linting and formatting for all files during git commit
