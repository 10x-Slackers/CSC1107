#ifndef XFERMONCTL_H
#define XFERMONCTL_H

#include <stddef.h>

#define XFERMON_DEVICE_PATH "/dev/xfermon"

int write_all(int fd, const char *buffer, size_t length);
int write_str(int fd, const char *text);
void print_io_error(const char *operation);
int print_stats(void);
int send_command(const char *command);

#endif /* XFERMONCTL_H */
