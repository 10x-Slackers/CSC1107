#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODULE_PATH="$ROOT_DIR/kernel/xfermon.ko"
CTL_PATH="$ROOT_DIR/userspace/xfermonctl"
TEST_FILE="/tmp/xfermon-demo-20mb.bin"

if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
  SUDO=""
else
  SUDO="sudo"
fi

usage() {
  cat <<USAGE
Usage:
  bash scripts/demo.sh simulate
  bash scripts/demo.sh usb <mounted-usb-path>

Modes:
  simulate  Build, load in all-device test mode, run simulator, write a test
            file, show /dev stats and kernel logs.
  usb       Build, load in removable-only mode, copy a test file to the USB
            mount path, show /dev stats and kernel logs.
USAGE
}

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "demo: missing required command: $1" >&2
    exit 1
  fi
}

module_loaded() {
  lsmod | awk '{print $1}' | grep -qx "xfermon"
}

unload_module() {
  if module_loaded; then
    $SUDO rmmod xfermon
  fi
}

build_project() {
  require_command make
  make -C "$ROOT_DIR"
}

load_module() {
  unload_module
  $SUDO insmod "$MODULE_PATH" "$@"
}

show_status() {
  echo
  echo "== /dev/xfermon =="
  "$CTL_PATH" stats
  echo
  echo "== Recent kernel logs =="
  $SUDO dmesg | tail -n 30
}

run_simulate_demo() {
  build_project
  load_module include_all_devices=1 alert_threshold_mb=10
  $SUDO "$CTL_PATH" reset

  echo "== Simulating one 1 MiB transfer =="
  $SUDO "$CTL_PATH" simulate 1048576 testdisk

  echo "== Writing 20 MiB test file to exercise the vfs_write hook =="
  dd if=/dev/zero of="$TEST_FILE" bs=1M count=20 status=none
  sync

  show_status
  unload_module
}

run_usb_demo() {
  local usb_path="$1"

  if [[ ! -d "$usb_path" ]]; then
    echo "demo: USB mount path does not exist: $usb_path" >&2
    echo "demo: run lsblk and use a path like /media/$USER/USB_NAME" >&2
    exit 1
  fi

  build_project
  load_module alert_threshold_mb=10
  $SUDO "$CTL_PATH" reset

  echo "== Creating 20 MiB test file =="
  dd if=/dev/zero of="$TEST_FILE" bs=1M count=20 status=none

  echo "== Copying test file to USB mount path =="
  cp "$TEST_FILE" "$usb_path/xfermon-demo-20mb.bin"
  sync

  show_status
  unload_module
}

case "${1:-}" in
  simulate)
    run_simulate_demo
    ;;
  usb)
    if [[ $# -ne 2 ]]; then
      usage
      exit 1
    fi
    run_usb_demo "$2"
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    usage
    exit 1
    ;;
esac
