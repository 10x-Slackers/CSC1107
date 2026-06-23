#include <linux/atomic.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/spinlock.h>
#include <linux/string.h>

#include "xfermon.h"

/* lock-free 64-bit counter */
atomic64_t transfer_count;
atomic64_t transfer_bytes;
atomic64_t alert_count;
atomic64_t event_sequence;
unsigned long started_at;
unsigned long window_started_at;
u64 window_bytes;
DEFINE_SPINLOCK(event_lock);
struct xfermon_event events[XFERMON_LOG_COUNT];
unsigned int event_next;
unsigned int event_total;

/**
 * record a write event and check the 60s alert window.
 */
void xfermon_add_event(u64 bytes, const char *device, const char *reason) {
  unsigned long flags;
  u64 alert_threshold_bytes = (u64)alert_threshold_mb * 1024 * 1024;
  struct xfermon_event *event;

  atomic64_inc(&transfer_count);
  atomic64_add(bytes, &transfer_bytes);

  /* disable local IRQs to protect shared state from interrupt handlers and
   * other CPUs */
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

  /* 60s sliding window, reset on alert */
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

/**
 * zero all counters, clear the event ring, reset the window.
 */
void xfermon_reset(void) {
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
