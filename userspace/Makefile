CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

.PHONY: all clean

all: xfermonctl

xfermonctl: xfermonctl.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f xfermonctl
