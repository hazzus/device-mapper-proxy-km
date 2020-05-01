.PHONY: all run clean

KERNEL_V=$(shell uname -r)

obj-m += dmp.o

all:
	make -C /lib/modules/$(KERNEL_V)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KERNEL_V)/build M=$(PWD) clean
