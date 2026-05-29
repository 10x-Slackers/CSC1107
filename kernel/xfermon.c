#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static int xfermon_init(void) {
  printk(KERN_INFO "xfermon: module loaded\n");
  return 0;
}

static void xfermon_exit(void) {
  printk(KERN_INFO "xfermon: module unloaded\n");
}

module_init(xfermon_init);
module_exit(xfermon_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CSC1107");
MODULE_DESCRIPTION("USB File Transfer Activity Monitor");
