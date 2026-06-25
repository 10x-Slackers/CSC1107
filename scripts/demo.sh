#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODULE="$ROOT_DIR/kernel/xfermon.ko"
CTL="$ROOT_DIR/userspace/xfermonctl"
TEST_FILE="/tmp/20mb.bin"
USB_PATH="${1:-}"

if [[ -z "$USB_PATH" || ! -d "$USB_PATH" ]]; then
  echo "usage: bash scripts/demo.sh <usb-mount-path>"
  exit 1
fi

if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
  echo "demo: must be run as root"
  exit 1
fi

step() { echo; echo "[STAGE] $*"; }

# Build
step "build"
make -C "$ROOT_DIR" clean
make -C "$ROOT_DIR"

# Load the module
step "load module"
rmmod xfermon 2>/dev/null || true
insmod "$MODULE"

# Verify loaded module
step "module status"
lsmod | grep xfermon
dmesg | tail -n 20
ls -l /dev/xfermon

# USB drive verification
step "USB drive"
lsblk

# Read from the driver
step "print stats (read from driver)"
"$CTL" stats

# File transfer test
step "copy file to USB"
dd if=/dev/zero of="$TEST_FILE" bs=1M count=20 status=none
cp "$TEST_FILE" "$USB_PATH/20mb.bin"
sync
"$CTL" stats
dmesg | tail -n 10

# Write to the driver
step "reset stats (write to driver)"
"$CTL" reset
"$CTL" stats

# Live monitoring
step "live monitoring function"
timeout 4 "$CTL" watch 1 || true

# Error handling
step "error handling (reading before load)"
rmmod xfermon
"$CTL" stats || true

step "error handling (non-root write)"
insmod "$MODULE"
"$CTL" reset || true

# Mass-copy alert
step "mass-copy alert (advanced feature)"
"$CTL" reset
cp -r /usr/include "$USB_PATH/"
sync
"$CTL" stats
dmesg | tail -n 30

# Threshold tuning
step "alert threshold tuning"
rmmod xfermon 2>/dev/null || true
insmod "$MODULE" alert_threshold_mb=5
"$CTL" stats

# Teardown
rmmod xfermon 2>/dev/null || true
lsmod | grep xfermon || true
ls -l /dev/xfermon 2>&1 || true
dmesg | tail
