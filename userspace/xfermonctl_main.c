#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xfermonctl.h"

/* print usage text */
static void print_usage(const char *program) {
  char message[512];
  int length = snprintf(message, sizeof(message),
                        "Usage:\n"
                        "  %s stats\n"
                        "  %s watch <seconds>\n"
                        "  sudo %s reset\n",
                        program, program, program);

  if (length > 0) {
    write_all(STDERR_FILENO, message, (size_t)length);
  }
}

/**
 * program entry point
 */
int main(int argc, char **argv) {
  if (strcmp(argv[1], "stats") == 0) {
    return print_stats();
  }

  if (strcmp(argv[1], "reset") == 0) {
    return send_command("reset");
  }

  if (strcmp(argv[1], "watch") == 0) {
    int seconds;

    if (argc != 3) {
      print_usage(argv[0]);
      return 1;
    }

    seconds = atoi(argv[2]);
    if (seconds <= 0) {
      write_str(STDERR_FILENO,
                "xfermonctl: watch interval must be positive.\n");
      return 1;
    }

    /* poll the device at the requested interval */
    while (1) {
      int ret = print_stats();
      if (ret) {
        return ret;
      }

      write_str(STDOUT_FILENO, "\n");
      sleep((unsigned int)seconds);
    }
  }

  print_usage(argv[0]);
  return 1;
}
