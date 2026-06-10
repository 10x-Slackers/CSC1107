#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define XFERMON_PROC_PATH "/proc/xfermon"

static int print_stats(void) {
  FILE *file = fopen(XFERMON_PROC_PATH, "r");
  char line[256];

  if (!file) {
    fprintf(stderr, "xfermonctl: cannot open %s: %s\n", XFERMON_PROC_PATH,
            strerror(errno));
    fprintf(stderr, "xfermonctl: load the kernel module first with insmod.\n");
    return 1;
  }

  while (fgets(line, sizeof(line), file)) {
    fputs(line, stdout);
  }

  fclose(file);
  return 0;
}

static int send_command(const char *command) {
  FILE *file = fopen(XFERMON_PROC_PATH, "w");

  if (!file) {
    fprintf(stderr, "xfermonctl: cannot write to %s: %s\n", XFERMON_PROC_PATH,
            strerror(errno));
    fprintf(stderr, "xfermonctl: try running this command with sudo.\n");
    return 1;
  }

  if (fprintf(file, "%s\n", command) < 0) {
    fprintf(stderr, "xfermonctl: failed to send command: %s\n",
            strerror(errno));
    fclose(file);
    return 1;
  }

  fclose(file);
  return 0;
}

static int simulate_transfer(const char *bytes, const char *device) {
  char command[128];
  char *end;
  unsigned long long value;

  errno = 0;
  value = strtoull(bytes, &end, 10);
  if (errno || end == bytes || *end != '\0' || value == 0) {
    fprintf(stderr, "xfermonctl: simulated bytes must be a positive number.\n");
    return 1;
  }

  snprintf(command, sizeof(command), "simulate %llu %s", value, device);
  return send_command(command);
}

static void print_usage(const char *program) {
  fprintf(stderr,
          "Usage:\n"
          "  %s stats\n"
          "  %s watch <seconds>\n"
          "  sudo %s simulate <bytes> [device]\n"
          "  sudo %s reset\n",
          program, program, program, program);
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
      fprintf(stderr, "xfermonctl: watch interval must be positive.\n");
      return 1;
    }

    for (;;) {
      int ret = print_stats();
      if (ret) {
        return ret;
      }

      puts("");
      fflush(stdout);
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
