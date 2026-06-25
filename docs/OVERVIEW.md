# USB File Transfer Activity Driver

This project implements a Linux kernel module that monitors file write activity to removable USB storage devices. It creates a character device at `/dev/xfermon`, provides a user-space tool for reading transfer statistics, logs kernel events with `printk()`, and raises alerts for large copy activity.

## How It Works

1. User copies file to USB
2. Linux vfs_write() runs
3. xfermon kprobe sees the write
4. xfermon finds the backing disk
5. If the disk is removable, xfermon records the event
6. xfermonctl reads the results from /dev/xfermon

## Kernel Module and User-Space Application

- The kernel module:
  - creates `/dev/xfermon` as a character device
  - hooks `vfs_write()` using a kprobe
  - finds the backing block device for file writes
  - records transfer count, byte count, alerts, and recent events
  - logs kernel events with `printk()`
- The user-space application:
  - reads transfer statistics from the character device
  - writes commands to reset statistics and clear recent events

## Implementation Highlights

- Linux kernel module development
  - `kernel/*.c` implement the module and its functionality
  - All share `kernel/xfermon.h` and build into `kernel/xfermon.ko`
- Character device driver creation
  - `xfermon_dev.c` registers a misc device named `xfermon`
- Communication between user-space and kernel-space
  - User-space program uses `read()` for stats and `write()` for reset
  - Kernel handles it via `xfermon_read` / `xfermon_write` in `xfermon_dev.c`
- `printk()` and `dmesg` logging
  - e.g. load/unload messages, per-write logs, alert messages
- Safe loading and unloading
  - `insmod` sets up device and probes, unwinds on failure
  - `rmmod` tears down in reverse order
- Error handling and debugging
  - `xfermonctl` validates input and reports I/O operation errors with advice
    - e.g. `insmod` if missing, `sudo` for `reset`
  - Kernel returns standard Linux error codes
    - Resources are freed on failure
- Makefile-based module compilation
  - Top-level `Makefile` dispatches to `kernel/Makefile` and `userspace/Makefile`
  - `make` builds both `xfermon.ko` and `xfermonctl`
- Demonstration and testing procedure
  - Runs on Raspberry Pi 4 with a real USB thumb drive
  - Tested to improve user experience and robustness
- Technical documentation
  - Comments throughout the code explaining the implementation and functions
  - This guide, `USAGE.md`, `README.md`, and `demo.sh`
