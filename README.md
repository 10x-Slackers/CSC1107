# CSC1107 Operating Systems Project 15

Linux kernel module for tracking USB file transfer activity.

---

## Project Scope

Project 15 asks for a driver that monitors file transfer activity to removable
USB storage devices.

Current milestone:

- Kernel module loads and unloads cleanly.
- Kernel module exposes transfer statistics at `/proc/xfermon`.
- User-space tool reads `/proc/xfermon`.
- Local simulator hook lets you test the driver interface before USB monitoring
  is implemented.

Next milestone:

- Detect writes to removable USB block devices.
- Record real transfer counts and byte totals.
- Add suspicious mass-copy alerts.

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
- `./get-pi-headers.sh 6.12.75`
  - clone specific version branch
- `./get-pi-headers.sh 6.12.75 pi@192.168.1.10`
  - also scp .config from Pi for exact match

### Local Linux build

```sh
make
```

This builds:

- `kernel/xfermon.ko`
- `userspace/xfermonctl`

### Local Linux test without Raspberry Pi

You cannot load a Linux kernel module directly on Windows. Use a Linux VM,
bare-metal Linux machine, or a privileged Linux lab machine with matching kernel
headers installed.

Build and load the module:

```sh
make
sudo insmod kernel/xfermon.ko
```

Read the driver statistics:

```sh
./userspace/xfermonctl
```

Simulate a file transfer without a USB drive:

```sh
echo 1048576 | sudo tee /proc/xfermon
./userspace/xfermonctl
```

Watch kernel logs:

```sh
sudo dmesg | tail
```

Unload the module:

```sh
sudo rmmod xfermon
```

The simulator proves that the module, `/proc` interface, user-space app, and
kernel logging work. It does not yet prove real USB transfer detection.

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
