#ifndef XFERMON_H
#define XFERMON_H

#include <linux/types.h>

#define XFERMON_DEVICE_NAME "xfermon"
#define XFERMON_INPUT_MAX 96
#define XFERMON_LOG_COUNT 64
#define XFERMON_DEVICE_LEN 32

/**
 * entry in the in-memory event ring buffer
 */
struct xfermon_event {
  u64 id;
  u64 bytes;
  unsigned long timestamp;
  char device[XFERMON_DEVICE_LEN];
};

/* Fallback for arm64 trees missing the PT_REGS_PARM* macros */
#if defined(CONFIG_ARM64) && !defined(PT_REGS_PARM1)
#define PT_REGS_PARM1(regs) ((regs)->regs[0])
#define PT_REGS_PARM3(regs) ((regs)->regs[2])
#endif

/* runtime instrumentation hook that traps a kernel function */
extern struct kprobe vfs_write_probe;
/* lock-free 64-bit counter safe on all CPUs */
extern atomic64_t transfer_count;
extern atomic64_t transfer_bytes;
extern atomic64_t alert_count;
extern atomic64_t event_sequence;
extern unsigned long started_at;
extern unsigned long window_started_at;
extern u64 window_bytes;
/* spinlock to protect the event ring and 60s window state */
extern spinlock_t event_lock;
extern struct xfermon_event events[];
extern unsigned int event_next;
extern unsigned int event_total;
extern uint alert_threshold_mb;

void xfermon_add_event(u64 bytes, const char *device);
void xfermon_reset(void);
bool xfermon_disk_is_removable(struct gendisk *disk);
struct gendisk *xfermon_file_disk(struct file *file);
int xfermon_vfs_write_pre(struct kprobe *probe, struct pt_regs *regs);
int xfermon_probe_init(void);
void xfermon_probe_exit(void);
int xfermon_dev_init(void);
void xfermon_dev_exit(void);

#endif /* XFERMON_H */
