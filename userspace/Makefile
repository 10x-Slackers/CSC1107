CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

.PHONY: all clean

all: xfermonctl

xfermonctl: xfermonctl_main.o xfermonctl_cmd.o xfermonctl_io.o
	$(CC) $(CFLAGS) -o $@ $^

xfermonctl_main.o: xfermonctl_main.c xfermonctl.h
	$(CC) $(CFLAGS) -c -o $@ $<

xfermonctl_cmd.o: xfermonctl_cmd.c xfermonctl.h
	$(CC) $(CFLAGS) -c -o $@ $<

xfermonctl_io.o: xfermonctl_io.c xfermonctl.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f xfermonctl *.o
