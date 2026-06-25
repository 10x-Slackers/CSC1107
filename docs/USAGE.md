# Usage Guide

## Automated demo

`scripts/demo.sh` automates the whole flow:

```sh
sudo bash scripts/demo.sh /media/<user>/<label>
```

## 1. Install necessary packages

```sh
sudo apt update
sudo apt install -y build-essential make git linux-headers-rpi-v8
```

## 2. Build

```sh
# Run from the project root directory
make
```

Expected build outputs:

- `kernel/xfermon.ko`
  - kernel module
- `userspace/xfermonctl`
  - user-space program

## 3. Load the module

```sh
sudo insmod kernel/xfermon.ko
```

Verify the module is loaded and created the character device:

```sh
lsmod | grep xfermon
ls -l /dev/xfermon
sudo dmesg | tail
```

Expected `dmesg`:

```text
xfermon: module loaded device=/dev/xfermon alert_threshold_mb=50
```

## 4. Run the userspace program

Commands:

- `./userspace/xfermonctl stats`
  - print stats from device
- `./userspace/xfermonctl watch 1`
  - periodically print stats every 1 second
- `sudo ./userspace/xfermonctl reset`
  - reset stats and clear recent events

Trigger a real transfer:

- Find the USB mount path with `lsblk` (e.g. `/media/user/usb_drive`)

```sh
dd if=/dev/urandom of=/tmp/20mb.bin bs=1M count=20
cp /tmp/20mb.bin /media/user/usb_drive
./userspace/xfermonctl stats
```

Example `stats` output:

```text
status: active
transfers: 8
bytes: 2097152
alerts: 0
alert_threshold_mb: 50
device_node: /dev/xfermon
uptime_seconds: 12
recent_events:
  #1 age=0s device=sda bytes=262144
  #2 age=0s device=sda bytes=262144
  #3 age=0s device=sda bytes=262144
```

The transfer count may exceed 1 because Linux can split one file copy into
multiple write operations.

## 5. Tune the alert threshold (optional)

`alert_threshold_mb` raises an alert after this many MB are written within a
60-second window. Set it at load time:

```sh
sudo rmmod xfermon
sudo insmod kernel/xfermon.ko alert_threshold_mb=5
```

Or adjust at runtime:

```sh
echo 5 | sudo tee /sys/module/xfermon/parameters/alert_threshold_mb
```

## 6. Unload the module

```sh
sudo rmmod xfermon
```

`dmesg` shows `xfermon: module unloaded` and `/dev/xfermon` is removed.
