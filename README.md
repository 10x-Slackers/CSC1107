# CSC1107 Operating Systems Project 15

Linux kernel module for tracking USB file transfer activity.

---

## Project Scope

Project 15 asks for a driver that monitors file transfer activity to removable
USB storage devices.

Current milestone:

- Kernel module loads and unloads cleanly.
- Kernel module creates a character device node at `/dev/xfermon`.
- User-space tool reads and writes `/dev/xfermon` using direct `open()`,
  `read()`, `write()`, and `close()` system calls.
- Bash demo automation is provided in `scripts/demo.sh`.

Next milestone:

- Test on Raspberry Pi 4 with a real USB thumb drive.
- Confirm whether the Pi kernel marks the target USB device as removable.

## Usage

For a step-by-step build, test, and demo guide, see
[`docs/PROJECT15_GUIDE.md`](docs/PROJECT15_GUIDE.md).

### Cross-compilation

```sh
./get-pi-headers.sh [PI_KERNEL_VERSION] [PI_HOST]

make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KERNELDIR=/workspaces/CSC1107/pi-kernel
```

- `./get-pi-headers.sh`
  - clone the script's default Raspberry Pi kernel branch and use Pi 4 default config
- `./get-pi-headers.sh 6.18.34`
  - clone specific version branch
- `./get-pi-headers.sh 6.18.34 pi@192.168.1.10`
  - also scp .config from Pi for exact match

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
