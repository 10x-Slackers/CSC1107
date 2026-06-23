# Project 15 Guide: USB File Transfer Activity Driver

This project implements a Linux kernel module that monitors file write activity
to removable USB storage devices. It creates a character device at
`/dev/xfermon`, provides a user-space tool for reading transfer statistics, logs
kernel events with `printk()`, and raises alerts for large copy activity.

## Project Files

- `kernel/xfermon_*.c` and `kernel/xfermon.h`: kernel module and character device driver (split across xfermon_main.c, xfermon_stats.c, xfermon_detect.c, xfermon_probe.c, xfermon_dev.c).
- `kernel/Makefile`: builds `kernel/xfermon.ko`.
- `userspace/xfermonctl.c`: user-space app that communicates with
  `/dev/xfermon` using `open()`, `read()`, `write()`, and `close()`.
- `userspace/Makefile`: builds `userspace/xfermonctl`.
- `scripts/demo.sh`: Bash script that automates build, load, test, log display,
  and cleanup.
- `Makefile`: builds both kernel and user-space components.

## How It Works

```text
User copies file to USB
        |
Linux vfs_write() runs
        |
xfermon kprobe sees the write
        |
xfermon finds the backing disk
        |
If the disk is removable, xfermon records the event
        |
xfermonctl reads the results from /dev/xfermon
```

The kernel module:

- creates `/dev/xfermon` as a character device;
- hooks `vfs_write()` using a kprobe;
- finds the backing block device for file writes;
- records transfer count, byte count, alerts, and recent events;
- filters to removable block devices in normal mode;

- logs kernel events with `printk()`.

The user-space application supports:

```sh
./userspace/xfermonctl stats
./userspace/xfermonctl watch 1
sudo ./userspace/xfermonctl reset
```

## Setup On Raspberry Pi

Install build tools and matching kernel headers.

For Raspberry Pi OS:

```sh
sudo apt update
sudo apt install -y raspberrypi-kernel-headers build-essential make git
```

For Debian on Raspberry Pi with an `rpt-rpi-v8` kernel:

```sh
sudo apt update
sudo apt install -y build-essential make git linux-headers-rpi-v8
```

Check the current OS if unsure:

```sh
cat /etc/os-release
uname -r
```

## Full USB Demo

Go to the project folder:

```sh
cd ~/CSC1107
```

Build the module and user-space app:

```sh
make clean
make
```

Expected build outputs:

```text
kernel/xfermon.ko
userspace/xfermonctl
```

Plug in the USB drive and find its mount path:

```sh
lsblk
```

Example USB output:

```text
sda      8:0    1 14.9G  0 disk
`-sda1   8:1    1 14.9G  0 part /media/linco/FELICE
```

Load the module:

```sh
sudo insmod kernel/xfermon.ko alert_threshold_mb=10
```

Check that the character device exists:

```sh
ls -l /dev/xfermon
```

Expected output starts with `c`, meaning character device:

```text
crw-rw-rw- ... /dev/xfermon
```

Show initial stats:

```sh
./userspace/xfermonctl stats
```

Create a 20 MiB test file:

```sh
dd if=/dev/zero of=large-file.bin bs=1M count=20
```

Copy the file to the USB drive and flush writes:

```sh
cp large-file.bin /media/linco/FELICE/
sync
```

Show updated stats and kernel logs:

```sh
./userspace/xfermonctl stats
sudo dmesg | tail -30
```

Unload the module when finished:

```sh
sudo rmmod xfermon
```

Expected `xfermonctl stats` output:

```text
status: active
transfers: 1
bytes: 20971520
alerts: 1
alert_threshold_mb: 10
device_node: /dev/xfermon
recent_events:
  #1 age=0s device=sda bytes=20971520 reason=removable-write
```

The transfer count may be more than `1` because Linux can split one file copy
into multiple write operations.

Expected `dmesg` output:

```text
xfermon: module loaded device=/dev/xfermon alert_threshold_mb=10
xfermon: write device=sda bytes=... reason=removable-write
xfermon: alert possible mass-copy behavior bytes_60s=...
xfermon: module unloaded
```

## Demo Script

Use the Bash script for a shorter repeatable demo.

Real USB test:

```sh
bash scripts/demo.sh usb /media/linco/FELICE
```

The script builds the project, loads the module, resets stats, performs the
test, shows `/dev/xfermon` stats, shows recent `dmesg` logs, and unloads the
module.

## Requirement Mapping

- Linux kernel module development: `kernel/xfermon_*.c` and `kernel/xfermon.h` build into
  `kernel/xfermon.ko`.
- Character device driver creation: the module creates `/dev/xfermon`.
- Communication between user-space and kernel-space: `xfermonctl` reads and
  writes `/dev/xfermon`.
- Device node creation under `/dev`: `ls -l /dev/xfermon` proves this.
- Proper use of `printk()` and `dmesg` logging: `sudo dmesg | tail -30` shows
  `xfermon:` kernel messages.
- Makefile-based module compilation: `make` builds the kernel module and
  user-space app.
- Safe loading and unloading: `sudo insmod kernel/xfermon.ko` loads the module,
  and `sudo rmmod xfermon` unloads it.
- Error handling and debugging: user input is validated, kernel copy failures
  return Linux error codes, and module load failures clean up allocated
  resources.
- Demonstration on Raspberry Pi 4: run the USB demo on the Pi with a real USB
  thumb drive.
- Technical documentation and testing procedure: this guide documents the build,
  test, demo, expected output, and known limitations.

## Presentation Script

```text
First, we build the kernel module and user-space app using Makefile.
Then we insert the module using insmod.
The module creates /dev/xfermon, which proves device node creation.
Our user-space app xfermonctl communicates with the driver through /dev/xfermon.
When we copy a file to the USB drive, the driver hooks vfs_write, finds the
backing disk, checks if it is removable, and records the transfer.
The stats command shows transfer count, bytes, alerts, and recent events.
Finally, dmesg shows printk logs from the kernel module.
Then we remove the module using rmmod.
```

## Known Limitations

- The module tracks normal filesystem writes through `vfs_write`.
- It does not monitor raw writes directly to `/dev/sdX`.
- It counts requested write sizes at the VFS layer, not confirmed bytes written
  to the physical storage device.
- Removable-device detection depends on how the Linux kernel marks the disk, so
  real USB detection can vary across adapters, kernels, and mount
  configurations.

- The final acceptance test should be done on Raspberry Pi 4 with a real USB
  thumb drive.
