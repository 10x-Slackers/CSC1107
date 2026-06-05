#ifndef XFERMON_H
#define XFERMON_H

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#define XFERMON_NAME "xfermon"
/* 100 MB Threshold */
#define XFERMON_DEFAULT_THRESHOLD (100ULL * 1024 * 1024)

struct xfermon_device {
  /* Device identity */
  char disk_name[32];

  /* Accumulated I/O counters */
  unsigned long long write_bytes;
  unsigned long long read_bytes;
  unsigned long long write_ops;
  unsigned long long read_ops;

  /* Previous raw snapshots for delta computation */
  unsigned long long prev_wr_sectors;
  unsigned long long prev_rd_sectors;
  unsigned long prev_wr_ios;
  unsigned long prev_rd_ios;

  /* Status and timing */
  unsigned long suspicious;
  unsigned long first_seen_jiffies;

  /* List linkage and per-device synchronisation */
  struct list_head list;
  spinlock_t lock;
};

extern struct list_head xfermon_device_list;
extern spinlock_t xfermon_device_list_lock;

/*
 * Threshold is a single scalar with rare writes and frequent reads.
 * Use READ_ONCE()/WRITE_ONCE() for lock-free access.
 */
extern unsigned long long xfermon_threshold;

extern struct delayed_work xfermon_work;

void xfermon_work_handler(struct work_struct *work);
int xfermon_format_stats(char *kbuf, size_t size);

#endif /* XFERMON_H */
