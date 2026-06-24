#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "xfermon.h"

/* alert threshold in MB over 60 seconds */
uint alert_threshold_mb = 100;

/* module_param: exposes /sys/module/xfermon/parameters/alert_threshold_mb */
module_param(alert_threshold_mb, uint, 0644);
MODULE_PARM_DESC(alert_threshold_mb,
                 "Raise an alert after this many MB are written in 60 seconds");

/**
 * module entry point
 */
static int __init xfermon_init(void) {
  int ret;

  started_at = jiffies;
  xfermon_reset();

  ret = xfermon_dev_init();
  if (ret) {
    return ret;
  }

  ret = xfermon_probe_init();
  if (ret) {
    xfermon_dev_exit();
    return ret;
  }

  printk(KERN_INFO
         "xfermon: module loaded device=/dev/%s alert_threshold_mb=%u\n",
         XFERMON_DEVICE_NAME, alert_threshold_mb);
  return 0;
}

/**
 * module exit point
 */
static void __exit xfermon_exit(void) {
  xfermon_probe_exit();
  xfermon_dev_exit();
  printk(KERN_INFO "xfermon: module unloaded\n");
}

module_init(xfermon_init);
module_exit(xfermon_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CSC1107");
MODULE_DESCRIPTION("USB File Transfer Activity Monitor");
