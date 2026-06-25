# CSC1107 Operating Systems Project 15

Linux kernel module for tracking USB file transfer activity.

---

## Project Scope

- [`kernel/xfermon_main.c`](kernel/xfermon_main.c)
  - module init/exit and the `alert_threshold_mb` runtime parameter
- [`kernel/xfermon_probe.c`](kernel/xfermon_probe.c)
  - kprobes on `vfs_write()`/`splice_write()`
  - records writes to removable disks
- [`kernel/xfermon_detect.c`](kernel/xfermon_detect.c)
  - checks `GENHD_FL_REMOVABLE` flag for removable disks
  - resolves `file -> gendisk`
- [`kernel/xfermon_stats.c`](kernel/xfermon_stats.c)
  - atomic counters
  - event ring buffer
  - 60-second mass-copy alert window
- [`kernel/xfermon_dev.c`](kernel/xfermon_dev.c)
  - `/dev/xfermon` misc char device
  - `read()` for stats
  - `write()` for reset
- [`userspace/xfermonctl.c`](userspace/xfermonctl.c)
  - CLI front end (`stats`, `reset`, `watch`)
  - Uses raw `open()`/`read()`/`write()`/`close()` on `/dev/xfermon`
- [`scripts/demo.sh`](scripts/demo.sh)
  - end-to-end demo walkthrough on a Pi 4
- [`scripts/get-pi-headers.sh`](scripts/get-pi-headers.sh)
  - fetch and prepare RPi kernel headers for cross-compilation

## Usage

For a step-by-step build, test, and demo guide, see
[`docs/PROJECT15_GUIDE.md`](docs/PROJECT15_GUIDE.md).

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
