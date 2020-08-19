# Makefile for wiegand driver

ARCH				?= arm
CROSS_COMPILE		?= arm-himix200-linux-
CC					:= $(CROSS_COMPILE)gcc
LD					:= $(CROSS_COMPILE)ld
obj-m				:= wiegand.o

PWD					:= $(shell pwd)
TOP_DIR				:= $(shell pwd)/../../../../..
KERNEL_DIR			:= $(TOP_DIR)/osdrv/opensource/kernel/linux-4.9.y-smp

# add include
LINUX_INC :=	-Iinclude/linux
INCS :=	$(LINUX_INC) 
EXTRA_CFLAGS += $(INCS)

# Fix build for GCC 4.7
EXTRA_CFLAGS += -Wno-maybe-uninitialized -Wno-unused-function

modules:
	@echo "make modules"
	@echo "KERNEL_DIR=$(KERNEL_DIR)"
	@echo "TOP_DIR=$(TOP_DIR)"
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) $(EXTRA_CFLAGS) ARCH=$(ARCH) CC=$(CC) LD=$(LD) modules

modules_install:
	@echo "make modules_install"
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) $(EXTRA_CFLAGS) ARCH=$(ARCH) CC=$(CC) LD=$(LD) modules_install

clean:
	rm -f *.o *.ko *.mod.c *.mod.o *~ .*.cmd Module.symvers modules.order
	rm -rf .tmp_versions
