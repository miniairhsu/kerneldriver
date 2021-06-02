obj-m := pcd_sysfs.o 
#dependencies 
pcd_sysfs-objs += pcd_platform_driver_dt_sysfs.o pcd_syscalls.o 
ARCH=arm
#KERNELDIR ?= /home/arm/bbb-debian2/linux
KERNELDIR ?= /home/arm/bbb-debian3/bb-kernel/KERNEL
COMPILER=arm-linux-gnueabihf-
PWD := $(shell pwd)
HOST_KERNEL = /lib/modules/$(shell uname -r)/build/
all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) -C $(KERNELDIR) M=$(PWD) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) -C $(KERNELDIR) M=$(PWD) clean 

help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) -C $(KERNELDIR) M=$(PWD) help

host:
	make -C $(HOST_KERNEL) M=$(PWD) modules