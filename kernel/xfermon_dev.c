#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "xfermon.h"

/**
 * build and return a stats report to userspace.
 */
static ssize_t xfermon_read(struct file *file, char __user *buffer,
                            size_t count, loff_t *pos) {
  char *output;
  unsigned long flags;
  unsigned long uptime_seconds;
  size_t len = 0;
  int i;

  /* allocate a PAGE_SIZE scratch buffer for the output */
  output = kzalloc(PAGE_SIZE, GFP_KERNEL);
  if (!output) {
    return -ENOMEM;
  }

  uptime_seconds = jiffies_to_msecs(jiffies - started_at) / 1000;
  len += scnprintf(output + len, PAGE_SIZE - len,
                   "status: active\n"
                   "transfers: %llu\n"
                   "bytes: %llu\n"
                   "alerts: %llu\n"
                   "alert_threshold_mb: %u\n"
                   "device_node: /dev/%s\n"
                   "uptime_seconds: %lu\n"
                   "recent_events:\n",
                   atomic64_read(&transfer_count),
                   atomic64_read(&transfer_bytes), atomic64_read(&alert_count),
                   alert_threshold_mb, XFERMON_DEVICE_NAME, uptime_seconds);

  spin_lock_irqsave(&event_lock, flags);
  /* walk the event ring oldest-first, stop when tail room is exhausted */
  for (i = 0; i < event_total && len < PAGE_SIZE - 128; i++) {
    /* convert i into actual array index in the circular buffer */
    unsigned int index =
        (event_next + XFERMON_LOG_COUNT - event_total + i) % XFERMON_LOG_COUNT;
    struct xfermon_event *event = &events[index];

    unsigned long age_seconds =
        jiffies_to_msecs(jiffies - event->timestamp) / 1000;

    len += scnprintf(output + len, PAGE_SIZE - len,
                     "  #%llu age=%lus device=%s bytes=%llu\n", event->id,
                     age_seconds, event->device, event->bytes);
  }
  spin_unlock_irqrestore(&event_lock, flags);

  /* empty history placeholder */
  if (event_total == 0 && len < PAGE_SIZE - 32) {
    len += scnprintf(output + len, PAGE_SIZE - len, "  none\n");
  }

  len = simple_read_from_buffer(buffer, count, pos, output, len);
  kfree(output);
  return len;
}

/**
 * handle userspace commands (reset).
 */
static ssize_t xfermon_write(struct file *file, const char __user *buffer,
                             size_t count, loff_t *pos) {
  char input[XFERMON_INPUT_MAX];

  if (count == 0 || count >= XFERMON_INPUT_MAX) {
    return -EINVAL;
  }

  /* copy userspace input */
  if (copy_from_user(input, buffer, count)) {
    return -EFAULT;
  }

  input[count] = '\0';
  strim(input);

  /* compare string, ignores trailing whitespace */
  if (sysfs_streq(input, "reset")) {
    xfermon_reset();
    printk(KERN_INFO "xfermon: statistics reset\n");
    return count;
  }

  return -EINVAL;
}

/* prevent module unload while the device is open */
static const struct file_operations xfermon_fops = {
    .owner = THIS_MODULE,
    .read = xfermon_read,
    .write = xfermon_write,
    .llseek = noop_llseek,
};

/* simplified char device registration */
static struct miscdevice xfermon_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = XFERMON_DEVICE_NAME,
    .fops = &xfermon_fops,
    .mode = 0666,
};

/**
 * register the /dev/xfermon miscdevice.
 */
int xfermon_dev_init(void) {
  int ret;

  ret = misc_register(&xfermon_misc_device);
  if (ret) {
    printk(KERN_ERR "xfermon: failed to create /dev/%s ret=%d\n",
           XFERMON_DEVICE_NAME, ret);
    return ret;
  }

  return 0;
}

/**
 * deregister the /dev/xfermon miscdevice.
 */
void xfermon_dev_exit(void) { misc_deregister(&xfermon_misc_device); }
