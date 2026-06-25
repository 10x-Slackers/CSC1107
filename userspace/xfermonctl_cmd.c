#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "xfermonctl.h"

/**
 * read and print the stats report from the device.
 */
int print_stats(void) {
  int fd = open(XFERMON_DEVICE_PATH, O_RDONLY);
  char buffer[512];

  if (fd < 0) {
    print_io_error("open");
    write_str(STDERR_FILENO,
              "xfermonctl: load the kernel module first with insmod.\n");
    return 1;
  }

  /* stream the report until the device closes */
  while (1) {
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    if (bytes < 0) {
      if (errno == EINTR) {
        continue;
      }
      print_io_error("read");
      close(fd);
      return 1;
    }

    /* EOF */
    if (bytes == 0) {
      break;
    }

    /* forward the chunk to stdout, exit on error */
    if (write_all(STDOUT_FILENO, buffer, (size_t)bytes) < 0) {
      close(fd);
      return 1;
    }
  }

  close(fd);
  return 0;
}

/**
 * send a single command line to the device.
 */
int send_command(const char *command) {
  int fd = open(XFERMON_DEVICE_PATH, O_WRONLY);
  char buffer[160];
  int length;

  if (fd < 0) {
    print_io_error("write to");
    write_str(STDERR_FILENO,
              "xfermonctl: try running this command with sudo.\n");
    return 1;
  }

  length = snprintf(buffer, sizeof(buffer), "%s\n", command);
  if (length < 0 || (size_t)length >= sizeof(buffer)) {
    write_str(STDERR_FILENO, "xfermonctl: command is too long.\n");
    close(fd);
    return 1;
  }

  /* send the command to the device, exit on error */
  if (write_all(fd, buffer, (size_t)length) < 0) {
    char message[160];
    int message_length =
        snprintf(message, sizeof(message),
                 "xfermonctl: failed to send command: %s\n", strerror(errno));

    if (message_length > 0) {
      write_all(STDERR_FILENO, message, (size_t)message_length);
    }
    close(fd);
    return 1;
  }

  close(fd);
  return 0;
}
