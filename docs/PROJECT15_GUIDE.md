# Project 15 Guide: USB File Transfer Activity Driver

This project implements a Linux kernel module and a user-space viewer for
monitoring write activity to removable USB storage devices.

## What To Use

Use these tools:

- VS Code: edit the project files.
- Ubuntu Linux VM: build, load, and test the kernel module without a Raspberry
  Pi.
- Raspberry Pi 4: final hardware demonstration when available.

Do not use normal Windows Command Prompt or PowerShell to load the module.
Windows cannot load Linux kernel modules.

## What The Project Contains

- `kernel/xfermon.c`: kernel module.
- `kernel/Makefile`: builds `xfermon.ko`.
- `userspace/xfermonctl.c`: user-space app for showing logs and stats.
- `userspace/Makefile`: builds `xfermonctl`.
- `Makefile`: builds both kernel and user-space parts.

The kernel module:

- hooks `vfs_write`;
- counts file write requests;
- filters to removable block devices by default;
- exposes stats and logs at `/proc/xfermon`;
- logs events with `printk()`;
- raises an alert when many bytes are written inside a 60-second window;
- supports a local test mode with `include_all_devices=1`.

## Testing Without Raspberry Pi

Use an Ubuntu VM. VirtualBox, VMware, or another VM app is fine. The simplest
path is Ubuntu Desktop in VirtualBox.

Inside the Ubuntu VM, install build tools and matching kernel headers:

```sh
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r) git
```

Clone or copy this repository into the VM, then build it:

```sh
cd CSC1107
make
```

You should see:

- `kernel/xfermon.ko`
- `userspace/xfermonctl`

Load the module in VM test mode:

```sh
sudo insmod kernel/xfermon.ko include_all_devices=1 alert_threshold_mb=10
```

Check that it loaded:

```sh
lsmod | grep xfermon
sudo dmesg | tail
```

Show current stats:

```sh
./userspace/xfermonctl stats
```

Test using the built-in simulator:

```sh
sudo ./userspace/xfermonctl simulate 1048576 testdisk
./userspace/xfermonctl stats
sudo dmesg | tail
```

Test using a real file write inside the VM:

```sh
mkdir -p /tmp/xfermon-test
dd if=/dev/zero of=/tmp/xfermon-test/bigfile.bin bs=1M count=20
./userspace/xfermonctl stats
sudo dmesg | tail
```

Because the module was loaded with `include_all_devices=1`, writes to the VM's
normal disk are counted. This proves the write hook, statistics, user-space app,
and kernel logging work before you have a USB drive or Raspberry Pi.

Reset stats:

```sh
sudo ./userspace/xfermonctl reset
```

Unload the module:

```sh
sudo rmmod xfermon
```

## Testing With A USB Drive On A Normal Linux Machine

Load the module in real removable-device mode:

```sh
sudo insmod kernel/xfermon.ko
```

Plug in and mount a USB thumb drive. Find its mount point:

```sh
lsblk
```

Copy a file to the mounted USB drive:

```sh
cp large-file.bin /media/$USER/YOUR_USB_NAME/
sync
```

Show logs:

```sh
./userspace/xfermonctl stats
sudo dmesg | tail
```

In normal mode, the module only counts writes where the backing disk is marked
as removable by the Linux kernel.

## Raspberry Pi Final Demo

On the Raspberry Pi, install the matching headers if available:

```sh
sudo apt update
sudo apt install -y raspberrypi-kernel-headers build-essential git
```

Build directly on the Pi:

```sh
make
```

Load the module:

```sh
sudo insmod kernel/xfermon.ko alert_threshold_mb=100
```

Plug in a USB thumb drive, copy files to it, then show:

```sh
./userspace/xfermonctl stats
sudo dmesg | tail
```

Unload after the demo:

```sh
sudo rmmod xfermon
```

## Demo Script

For a no-USB VM demo:

```sh
make
sudo insmod kernel/xfermon.ko include_all_devices=1 alert_threshold_mb=10
./userspace/xfermonctl stats
dd if=/dev/zero of=/tmp/xfermon-demo.bin bs=1M count=20
./userspace/xfermonctl stats
sudo dmesg | tail
sudo rmmod xfermon
```

For a real USB demo:

```sh
make
sudo insmod kernel/xfermon.ko
cp large-file.bin /media/$USER/YOUR_USB_NAME/
sync
./userspace/xfermonctl stats
sudo dmesg | tail
sudo rmmod xfermon
```

## Limitations To Mention In The Report

- The module tracks normal filesystem writes through `vfs_write`.
- It does not monitor raw writes directly to `/dev/sdX`.
- It counts requested write sizes at the VFS layer, so it is a lightweight
  auditing monitor rather than a forensic byte-perfect storage logger.
- Removable-device detection depends on how the Linux kernel marks the disk.
- `include_all_devices=1` is for VM testing only.
