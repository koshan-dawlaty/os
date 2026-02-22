# SUBDIRS := kernel/ lib/ libc/ userspace/
SUBDIRS := lib libc kernel userspace

.PHONY: all clean $(SUBDIRS)

export SYSROOT := $(shell pwd)/build/sysroot
export FS_ROOT := fs-root

export DESTDIR := $(SYSROOT)
export CC := x86_64-elf-gcc --sysroot=$(SYSROOT)
export ARCH := $(shell ./build/default-host.sh)

export CFLAGS :=  -DMYOS1234 -Werror -Wno-unused-but-set-variable -Wno-address-of-packed-member -Wno-unused-function -Wno-unused-parameter -Wno-unused-label -Wno-unused-variable -Wno-error=type-limits -O0 -g -isystem=/usr/include -nostdlib $(shell ./build/arch-flags.sh $(ARCH)) -DCONFIG_$(ARCH) $(CONFIG_FLAGS) 


QEMU_COMMAND := qemu-system-$(shell ./build/qemu-arch.sh $(ARCH))

QEMU_FLAGS :=  -cdrom build/os.iso \
-m 512M \
-machine pc \
-d int \
-D build/log.txt \
-boot d  \
-vga std \
-display default,show-cursor=on \
-drive file=build/qemu.img,format=raw,if=ide,index=0 

ISO=build/os.iso

all: $(ISO)

$(ISO) : $(SUBDIRS)
	./build/iso.sh

$(SUBDIRS):
	@echo $(ARCH)
	mkdir -p "$(SYSROOT)"
	set -e
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install-headers; \
	done
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install; \
	done


qemu : $(ISO)
	rm -f build/qemu.img
	./build/get-ext2-image.sh "build/qemu.img" build/fs-root
	$(QEMU_COMMAND) -monitor stdio $(QEMU_FLAGS)





clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
