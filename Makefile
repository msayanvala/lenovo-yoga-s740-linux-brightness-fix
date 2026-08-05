obj-m += yoga_s740_vpc_poll.o

KERNELRELEASE ?= $(shell uname -r)

.PHONY: all clean

all:
	$(MAKE) -C /lib/modules/$(KERNELRELEASE)/build M=$(CURDIR) modules

clean:
	$(MAKE) -C /lib/modules/$(KERNELRELEASE)/build M=$(CURDIR) clean
