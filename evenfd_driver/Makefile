obj-m := cdata.o
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

incs=-I/home/swlib/gsl/include -I.	
libs=-L/home/swlib/gsl/lib -L.
ldflags=-lgsl -lgslcblas -lm -lrt
deps := $(wildcard *.h) 
	
AP: main-loop.o 
	gcc main-loop.o $(ldflags) $(libs) $(incs) -o main-loop.exe

main-loop.o:main-loop.c $(deps)
	gcc -c main-loop.c $(ldflags) $(libs) $(incs) -o main-loop.o  

CTLAP: control.o
	gcc control.o $(ldflags) $(libs) $(incs) -o control.exe

contro.o: control.c
	gcc -c control.c $(ldflags) $(libs) $(incs) -o control.o 

.PHONY: clean 
clean:
	rm -f *.o .logibone_fifo.ko.cmd .logibone_fifo.mod.o.cmd .logibone_fifo.o.cmd
	rm -f *.ko .tmp_versions/*
	rm -f logibone_fifo.mod.*
	rm -f [mM]odule*
	rm -f *.exe
	rmdir .tmp_versions/