#include <linux/atomic.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#if defined(CONFIG_ARM64) && !defined(PT_REGS_PARM1)
#define PT_REGS_PARM1(regs) ((regs)->regs[0])
#define PT_REGS_PARM3(regs) ((regs)->regs[2])
#endif

#define XFERMON_DEVICE_NAME "xfermon"
#define XFERMON_INPUT_MAX 96
#define XFERMON_LOG_COUNT 64
#define XFERMON_DEVICE_LEN 32
#define XFERMON_REASON_LEN 32

struct xfermon_event {
  u64 id;
  u64 bytes;
  unsigned long timestamp;
  char device[XFERMON_DEVICE_LEN];
  char reason[XFERMON_REASON_LEN];
};

static struct kprobe vfs_write_probe;
static atomic64_t transfer_count;
static atomic64_t transfer_bytes;
static atomic64_t alert_count;
static atomic64_t event_sequence;
static unsigned long started_at;
static unsigned long window_started_at;
static u64 window_bytes;
static DEFINE_SPINLOCK(event_lock);
static struct xfermon_event events[XFERMON_LOG_COUNT];
static unsigned int event_next;
static unsigned int event_total;

static bool include_all_devices;
module_param(include_all_devices, bool, 0644);
MODULE_PARM_DESC(include_all_devices,
                 "Count writes to all block-backed filesystems for VM testing");

static uint alert_threshold_mb = 100;
module_param(alert_threshold_mb, uint, 0644);
MODULE_PARM_DESC(alert_threshold_mb,
                 "Raise an alert after this many MB are written in 60 seconds");

static bool xfermon_disk_is_removable(struct gendisk *disk) {
#ifdef GENHD_FL_REMOVABLE
  return (disk->flags & GENHD_FL_REMOVABLE) != 0;
#else
  return false;
#endif
}

static struct gendisk *xfermon_file_disk(struct file *file) {
  struct super_block *sb;

  if (!file || !file->f_inode) {
    return NULL;
  }

  sb = file->f_inode->i_sb;
  if (!sb || !sb->s_bdev) {
    return NULL;
  }

  return sb->s_bdev->bd_disk;
}

static void xfermon_add_event(u64 bytes, const char *device,
                              const char *reason) {
  unsigned long flags;
  u64 alert_threshold_bytes = (u64)alert_threshold_mb * 1024 * 1024;
  struct xfermon_event *event;

  atomic64_inc(&transfer_count);
  atomic64_add(bytes, &transfer_bytes);

  spin_lock_irqsave(&event_lock, flags);

  event = &events[event_next];
  event->id = (u64)atomic64_inc_return(&event_sequence);
  event->bytes = bytes;
  event->timestamp = jiffies;
  strscpy(event->device, device, sizeof(event->device));
  strscpy(event->reason, reason, sizeof(event->reason));

  event_next = (event_next + 1) % XFERMON_LOG_COUNT;
  if (event_total < XFERMON_LOG_COUNT) {
    event_total++;
  }

  if (time_after(jiffies, window_started_at + 60 * HZ)) {
    window_started_at = jiffies;
    window_bytes = 0;
  }

  window_bytes += bytes;
  if (window_bytes >= alert_threshold_bytes && alert_threshold_bytes > 0) {
    atomic64_inc(&alert_count);
    printk(KERN_WARNING
           "xfermon: alert possible mass-copy behavior bytes_60s=%llu\n",
           window_bytes);
    window_started_at = jiffies;
    window_bytes = 0;
  }

  spin_unlock_irqrestore(&event_lock, flags);

  printk(KERN_INFO "xfermon: write device=%s bytes=%llu reason=%s\n", device,
         bytes, reason);
}

static int xfermon_vfs_write_pre(struct kprobe *probe, struct pt_regs *regs) {
  struct file *file = (struct file *)PT_REGS_PARM1(regs);
  size_t bytes = (size_t)PT_REGS_PARM3(regs);
  struct gendisk *disk;
  const char *device;
  bool is_removable;

  if (bytes == 0) {
    return 0;
  }

  disk = xfermon_file_disk(file);
  if (!disk) {
    return 0;
  }

  is_removable = xfermon_disk_is_removable(disk);
  if (!include_all_devices && !is_removable) {
    return 0;
  }

  device = disk->disk_name[0] ? disk->disk_name : "unknown";
  xfermon_add_event((u64)bytes, device,
                    is_removable ? "removable-write" : "test-write");
  return 0;
}

static void xfermon_reset(void) {
  unsigned long flags;

  atomic64_set(&transfer_count, 0);
  atomic64_set(&transfer_bytes, 0);
  atomic64_set(&alert_count, 0);
  atomic64_set(&event_sequence, 0);

  spin_lock_irqsave(&event_lock, flags);
  memset(events, 0, sizeof(events));
  event_next = 0;
  event_total = 0;
  window_started_at = jiffies;
  window_bytes = 0;
  spin_unlock_irqrestore(&event_lock, flags);
}

static ssize_t xfermon_read(struct file *file, char __user *buffer,
                            size_t count, loff_t *pos) {
  char *output;
  unsigned long flags;
  unsigned long uptime_seconds;
  size_t len = 0;
  int i;

  output = kzalloc(PAGE_SIZE, GFP_KERNEL);
  if (!output) {
    return -ENOMEM;
  }

  uptime_seconds = jiffies_to_msecs(jiffies - started_at) / 1000;

  len += scnprintf(output + len, PAGE_SIZE - len,
                   "status: active\n"
                   "mode: %s\n"
                   "transfers: %llu\n"
                   "bytes: %llu\n"
                   "alerts: %llu\n"
                   "alert_threshold_mb: %u\n"
                   "device_node: /dev/%s\n"
                   "removable_detection: gendisk-removable-flag\n"
                   "byte_accounting: requested_vfs_write_bytes\n"
                   "uptime_seconds: %lu\n"
                   "commands: reset | simulate <bytes> [device]\n"
                   "recent_events:\n",
                   include_all_devices ? "all-devices-test" : "removable-only",
                   atomic64_read(&transfer_count),
                   atomic64_read(&transfer_bytes), atomic64_read(&alert_count),
                   alert_threshold_mb, XFERMON_DEVICE_NAME, uptime_seconds);

  spin_lock_irqsave(&event_lock, flags);
  for (i = 0; i < event_total && len < PAGE_SIZE - 128; i++) {
    unsigned int index =
        (event_next + XFERMON_LOG_COUNT - event_total + i) % XFERMON_LOG_COUNT;
    struct xfermon_event *event = &events[index];
    unsigned long age_seconds =
        jiffies_to_msecs(jiffies - event->timestamp) / 1000;

    len += scnprintf(output + len, PAGE_SIZE - len,
                     "  #%llu age=%lus device=%s bytes=%llu reason=%s\n",
                     event->id, age_seconds, event->device, event->bytes,
                     event->reason);
  }
  spin_unlock_irqrestore(&event_lock, flags);

  if (event_total == 0 && len < PAGE_SIZE - 32) {
    len += scnprintf(output + len, PAGE_SIZE - len, "  none\n");
  }

  len = simple_read_from_buffer(buffer, count, pos, output, len);
  kfree(output);
  return len;
}

static ssize_t xfermon_write(struct file *file, const char __user *buffer,
                             size_t count, loff_t *pos) {
  char input[XFERMON_INPUT_MAX];
  char command[16] = {0};
  char device[XFERMON_DEVICE_LEN] = "simulated";
  u64 bytes;
  int matched;

  if (count == 0 || count >= XFERMON_INPUT_MAX) {
    return -EINVAL;
  }

  if (copy_from_user(input, buffer, count)) {
    return -EFAULT;
  }

  input[count] = '\0';
  strim(input);

  if (sysfs_streq(input, "reset")) {
    xfermon_reset();
    printk(KERN_INFO "xfermon: statistics reset\n");
    return count;
  }

  matched = sscanf(input, "%15s %llu %31s", command, &bytes, device);
  if (matched >= 2 && strcmp(command, "simulate") == 0) {
    xfermon_add_event(bytes, device, "manual-simulate");
    return count;
  }

  return -EINVAL;
}

static const struct file_operations xfermon_fops = {
    .owner = THIS_MODULE,
    .read = xfermon_read,
    .write = xfermon_write,
    .llseek = no_llseek,
};

static struct miscdevice xfermon_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = XFERMON_DEVICE_NAME,
    .fops = &xfermon_fops,
    .mode = 0666,
};

static int __init xfermon_init(void) {
  int ret;

  started_at = jiffies;
  xfermon_reset();

  ret = misc_register(&xfermon_misc_device);
  if (ret) {
    printk(KERN_ERR "xfermon: failed to create /dev/%s ret=%d\n",
           XFERMON_DEVICE_NAME, ret);
    return ret;
  }

  memset(&vfs_write_probe, 0, sizeof(vfs_write_probe));
  vfs_write_probe.symbol_name = "vfs_write";
  vfs_write_probe.pre_handler = xfermon_vfs_write_pre;

  ret = register_kprobe(&vfs_write_probe);
  if (ret) {
    misc_deregister(&xfermon_misc_device);
    printk(KERN_ERR "xfermon: failed to register vfs_write kprobe ret=%d\n",
           ret);
    return ret;
  }

  printk(KERN_INFO
         "xfermon: module loaded device=/dev/%s mode=%s alert_threshold_mb=%u\n",
         XFERMON_DEVICE_NAME,
         include_all_devices ? "all-devices-test" : "removable-only",
         alert_threshold_mb);
  return 0;
}

static void __exit xfermon_exit(void) {
  unregister_kprobe(&vfs_write_probe);
  misc_deregister(&xfermon_misc_device);
  printk(KERN_INFO "xfermon: module unloaded\n");
}

module_init(xfermon_init);
module_exit(xfermon_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CSC1107");
MODULE_DESCRIPTION("USB File Transfer Activity Monitor");
