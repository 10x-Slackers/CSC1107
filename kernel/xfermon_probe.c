#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>

#include "xfermon.h"

/* runtime breakpoints that trap every write path */
struct kprobe vfs_write_probe;
struct kprobe splice_write_probe;

/**
 * resolve the backing disk for a file and record the transfer if it is
 * removable.
 */
static void xfermon_record_file(struct file *file, u64 bytes) {
  struct gendisk *disk;
  const char *device;
  bool is_removable;

  if (bytes == 0) {
    return;
  }

  /* resolve the backing block device */
  disk = xfermon_file_disk(file);
  if (!disk) {
    return;
  }

  /* only track removable media */
  is_removable = xfermon_disk_is_removable(disk);
  if (!is_removable) {
    return;
  }

  /* record the transfer against the disk name */
  device = disk->disk_name[0] ? disk->disk_name : "unknown";
  xfermon_add_event(bytes, device);
}

/**
 * kprobe pre-handler on vfs_write
 */
int xfermon_vfs_write_pre(struct kprobe *probe, struct pt_regs *regs) {
  struct file *file = (struct file *)PT_REGS_PARM1(regs);
  size_t bytes = (size_t)PT_REGS_PARM3(regs);

  xfermon_record_file(file, (u64)bytes);
  return 0;
}

/**
 * kprobe pre-handler on iter_file_splice_write
 */
int xfermon_splice_write_pre(struct kprobe *probe, struct pt_regs *regs) {
  struct file *file = (struct file *)PT_REGS_PARM2(regs);
  size_t bytes = (size_t)PT_REGS_PARM4(regs);

  xfermon_record_file(file, (u64)bytes);
  return 0;
}

/**
 * register the vfs_write and splice_write kprobes
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

  memset(&splice_write_probe, 0, sizeof(splice_write_probe));
  splice_write_probe.symbol_name = "iter_file_splice_write";
  splice_write_probe.pre_handler = xfermon_splice_write_pre;

  ret = register_kprobe(&splice_write_probe);
  if (ret) {
    printk(KERN_ERR
           "xfermon: failed to register iter_file_splice_write kprobe ret=%d\n",
           ret);
    unregister_kprobe(&vfs_write_probe);
    return ret;
  }

  return 0;
}

/**
 * disarm and unregister both kprobes
 */
void xfermon_probe_exit(void) {
  unregister_kprobe(&splice_write_probe);
  unregister_kprobe(&vfs_write_probe);
}
