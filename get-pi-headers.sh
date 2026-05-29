#!/usr/bin/env bash
# Fetch and prepare RPi kernel headers for cross-compilation

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/pi-kernel"
PI_KERNEL_VERSION="${1:-}"
PI_HOST="${2:-}"

# Figure out which branch to clone
if [ -n "$PI_KERNEL_VERSION" ]; then
    MAJOR_MINOR=$(echo "$PI_KERNEL_VERSION" | sed 's/\([0-9]*\.[0-9]*\).*/\1/')
    BRANCH="rpi-${MAJOR_MINOR}.y"
else
    BRANCH="rpi-6.12.y"
fi

echo "=== RPi Kernel Headers for Cross-Compilation ==="
echo "Branch: $BRANCH"
echo "Output: $OUTPUT_DIR"

# Clone shallow on the target branch
if [ -d "${OUTPUT_DIR}/.git" ]; then
    echo "Kernel repo already exists at ${OUTPUT_DIR}, fetching latest..."
    git -C "$OUTPUT_DIR" fetch origin "$BRANCH"
    git -C "$OUTPUT_DIR" reset --hard "origin/$BRANCH"
else
    echo "Cloning raspberrypi/linux (branch: ${BRANCH})..."
    git clone --depth=1 --branch="$BRANCH" \
        https://github.com/raspberrypi/linux.git "$OUTPUT_DIR"
fi
cd "$OUTPUT_DIR"

# Get .config either from the Pi or use default
if [ -n "$PI_HOST" ]; then
    echo "Fetching .config from Pi (${PI_HOST})..."
    scp "${PI_HOST}:/boot/config-$(ssh "$PI_HOST" uname -r)" .config \
        || scp "${PI_HOST}:/proc/config.gz" - | gunzip > .config
else
    echo "No Pi host specified, using bcm2711_defconfig (Pi 4 default)..."
    make ARCH=arm64 bcm2711_defconfig
fi

# Prepare the build tree for out-of-tree module compilation
echo "Running modules_prepare..."
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules_prepare -j"$(nproc)"

# Build vmlinux and modules to generate Module.symvers
echo "Building vmlinux..."
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- vmlinux -j"$(nproc)"
echo "Building kernel modules..."
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules -j"$(nproc)"

echo ""
echo "=== Done ==="
echo "Set KERNELDIR in your Makefile or build command:"
echo "  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KERNELDIR=${OUTPUT_DIR}"
