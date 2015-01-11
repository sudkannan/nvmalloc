# build modules
obj-m = pmc.o
pcmdisk-objs = pmc.o pcm.o stat.o cr4.o

# use the kernel build system
KERNEL_VERSION := `uname -r`
KERNEL_SOURCE := /lib/modules/$(KERNEL_VERSION)/build

SRCDIR=`pwd`
OBJDIR=`pwd`

all: build_kernelmodule build_userapp

clean: clean_kernelmodule clean_userapp

build_kernelmodule: 
	make -C $(KERNEL_SOURCE)  M=$(OBJDIR) modules

clean_kernelmodule:
	make -C $(KERNEL_SOURCE) M=$(OBJDIR) clean

build_userapp:
	 gcc -Wall -fPIC -c rdpmc.c
	 gcc -shared -Wl,-soname,librdpmc.so -o librdpmc.so   rdpmc.o
clean_userapp:

install:
	cp  librdpmc.so /usr/lib/
	cp  librdpmc.so /usr/lib64/

