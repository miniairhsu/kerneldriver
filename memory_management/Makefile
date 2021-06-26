obj-m := main.o
ARCH=arm
KERNELDIR ?= /home/arm/bbb-debian2/linux
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

AP: ioperm.o 
	gcc ioperm.o -o ioperm.exe

ioperm.o:ioperm.c 
	gcc -c ioperm.c -o ioperm.o  