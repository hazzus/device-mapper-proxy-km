.PHONY: all run clean

KERNEL_V=$(shell uname -r)
SZ=1024

obj-m += dmp.o

all:
	make -C /lib/modules/$(KERNEL_V)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KERNEL_V)/build M=$(PWD) clean

install: all
	sudo insmod dmp.ko
	
check:
	sudo lsmod | grep dmp

setup: check
	sudo dmsetup create dumb  --table "0 $(SZ) zero"
	sudo dmsetup create proxy --table "0 $(SZ) dmp /dev/mapper/dumb 1"
	ls -al /dev/mapper
	sudo cat /sys/module/dmp/stat/op_stat

load: setup
	./load.sh
	sudo cat /sys/module/dmp/stat/op_stat

destroy:
	sudo dmsetup remove proxy
	sudo dmsetup remove dumb
	
	
