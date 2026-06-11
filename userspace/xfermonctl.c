#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define XFERMON_DEVICE_PATH "/dev/xfermon"

static int write_all(int fd, const char *buffer, size_t length) {
  size_t written = 0;

  while (written < length) {
    ssize_t result = write(fd, buffer + written, length - written);
    if (result < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (result == 0) {
      errno = EIO;
      return -1;
    }
    written += (size_t)result;
  }

  return 0;
}

static int write_cstr(int fd, const char *text) {
  return write_all(fd, text, strlen(text));
}

static void print_open_error(const char *operation) {
  char message[256];
  int length =
      snprintf(message, sizeof(message), "xfermonctl: cannot %s %s: %s\n",
               operation, XFERMON_DEVICE_PATH, strerror(errno));

  if (length > 0) {
    write_all(STDERR_FILENO, message, (size_t)length);
  }
}

static int print_stats(void) {
  int fd = open(XFERMON_DEVICE_PATH, O_RDONLY);
  char buffer[512];

  if (fd < 0) {
    print_open_error("open");
    write_cstr(STDERR_FILENO,
               "xfermonctl: load the kernel module first with insmod.\n");
    return 1;
  }

  for (;;) {
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    if (bytes < 0) {
      if (errno == EINTR) {
        continue;
      }
      print_open_error("read");
      close(fd);
      return 1;
    }

    if (bytes == 0) {
      break;
    }

    if (write_all(STDOUT_FILENO, buffer, (size_t)bytes) < 0) {
      close(fd);
      return 1;
    }
  }

  close(fd);
  return 0;
}

static int send_command(const char *command) {
  int fd = open(XFERMON_DEVICE_PATH, O_WRONLY);
  char buffer[160];
  int length;

  if (fd < 0) {
    print_open_error("write to");
    write_cstr(STDERR_FILENO,
               "xfermonctl: try running this command with sudo.\n");
    return 1;
  }

  length = snprintf(buffer, sizeof(buffer), "%s\n", command);
  if (length < 0 || (size_t)length >= sizeof(buffer)) {
    write_cstr(STDERR_FILENO, "xfermonctl: command is too long.\n");
    close(fd);
    return 1;
  }

  if (write_all(fd, buffer, (size_t)length) < 0) {
    char message[160];
    int message_length = snprintf(message, sizeof(message),
                                  "xfermonctl: failed to send command: %s\n",
                                  strerror(errno));

    if (message_length > 0) {
      write_all(STDERR_FILENO, message, (size_t)message_length);
    }
    close(fd);
    return 1;
  }

  close(fd);
  return 0;
}

static int simulate_transfer(const char *bytes, const char *device) {
  char command[128];
  char *end;
  unsigned long long value;

  errno = 0;
  value = strtoull(bytes, &end, 10);
  if (errno || end == bytes || *end != '\0' || value == 0) {
    write_cstr(STDERR_FILENO,
               "xfermonctl: simulated bytes must be a positive number.\n");
    return 1;
  }

  snprintf(command, sizeof(command), "simulate %llu %s", value, device);
  return send_command(command);
}

static void print_usage(const char *program) {
  char message[512];
  int length =
      snprintf(message, sizeof(message),
               "Usage:\n"
               "  %s stats\n"
               "  %s watch <seconds>\n"
               "  sudo %s simulate <bytes> [device]\n"
               "  sudo %s reset\n",
               program, program, program, program);

  if (length > 0) {
    write_all(STDERR_FILENO, message, (size_t)length);
  }
}

int main(int argc, char **argv) {
  if (argc == 1 || strcmp(argv[1], "stats") == 0) {
    return print_stats();
  }

  if (strcmp(argv[1], "watch") == 0) {
    int seconds;

    if (argc != 3) {
      print_usage(argv[0]);
      return 1;
    }

    seconds = atoi(argv[2]);
    if (seconds <= 0) {
      write_cstr(STDERR_FILENO,
                 "xfermonctl: watch interval must be positive.\n");
      return 1;
    }

    for (;;) {
      int ret = print_stats();
      if (ret) {
        return ret;
      }

      write_cstr(STDOUT_FILENO, "\n");
      sleep((unsigned int)seconds);
    }
  }

  if (strcmp(argv[1], "simulate") == 0) {
    const char *device = argc >= 4 ? argv[3] : "simulated";

    if (argc < 3 || argc > 4) {
      print_usage(argv[0]);
      return 1;
    }

    return simulate_transfer(argv[2], device);
  }

  if (strcmp(argv[1], "reset") == 0) {
    if (argc != 2) {
      print_usage(argv[0]);
      return 1;
    }

    return send_command("reset");
  }

  print_usage(argv[0]);
  return 1;
}
