obj-m += xfermon.o

xfermon-y := xfermon_main.o xfermon_stats.o xfermon_detect.o xfermon_probe.o xfermon_dev.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) clean
