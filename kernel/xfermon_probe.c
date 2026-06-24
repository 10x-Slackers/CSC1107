#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>

#include "xfermon.h"

/* runtime breakpoint that traps every call to vfs_write */
struct kprobe vfs_write_probe;

/**
 * kprobe pre-handler on vfs_write
 */
int xfermon_vfs_write_pre(struct kprobe *probe, struct pt_regs *regs) {
  /* pull the file, buffer, and byte count from syscall registers */
  struct file *file = (struct file *)PT_REGS_PARM1(regs);
  size_t bytes = (size_t)PT_REGS_PARM3(regs);
  struct gendisk *disk;
  const char *device;
  bool is_removable;

  /* ignore zero-length writes */
  if (bytes == 0) {
    return 0;
  }

  /* resolve the backing block device */
  disk = xfermon_file_disk(file);
  if (!disk) {
    return 0;
  }

  /* only track removable media */
  is_removable = xfermon_disk_is_removable(disk);
  if (!is_removable) {
    return 0;
  }

  /* record the transfer against the disk name */
  device = disk->disk_name[0] ? disk->disk_name : "unknown";
  xfermon_add_event((u64)bytes, device);
  return 0;
}

/**
 * register the vfs_write kprobe
 */
int xfermon_probe_init(void) {
  int ret;

  memset(&vfs_write_probe, 0, sizeof(vfs_write_probe));
  vfs_write_probe.symbol_name = "vfs_write";
  vfs_write_probe.pre_handler = xfermon_vfs_write_pre;

  ret = register_kprobe(&vfs_write_probe);
  if (ret) {
    printk(KERN_ERR "xfermon: failed to register vfs_write kprobe ret=%d\n",
           ret);
    return ret;
  }

  return 0;
}

/**
 * disarm and unregister the vfs_write kprobe
 */
void xfermon_probe_exit(void) { unregister_kprobe(&vfs_write_probe); }
