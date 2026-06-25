#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "xfermonctl.h"

/**
 * write the full buffer to fd, retrying until every byte is sent.
 */
int write_all(int fd, const char *buffer, size_t length) {
  size_t written = 0;

  while (written < length) {
    ssize_t result = write(fd, buffer + written, length - written);

    /* retry if signal interrupted */
    if (result < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }

    written += (size_t)result;
  }

  return 0;
}

/* write_all wrapper for a NUL-terminated string */
int write_str(int fd, const char *text) {
  return write_all(fd, text, strlen(text));
}

/**
 * print a error message for a failed I/O operation of the device.
 */
void print_io_error(const char *operation) {
  char message[256];
  int length =
      snprintf(message, sizeof(message), "xfermonctl: cannot %s %s: %s\n",
               operation, XFERMON_DEVICE_PATH, strerror(errno));

  if (length > 0) {
    write_all(STDERR_FILENO, message, (size_t)length);
  }
}
