# SUBDIRS := kernel/ lib/ libc/ userspace/
SUBDIRS := lib libc kernel userspace

.PHONY: all clean $(SUBDIRS)

CONFIG_FILE_RAW!=cat config
CONFIG_FLAGS=$(shell echo $(CONFIG_FILE_RAW) | sed "s/^/-DCONFIG_/")

export SYSROOT := $(shell pwd)/build/sysroot
export FS_ROOT := fs-root

export DESTDIR := $(SYSROOT)
export CC := x86_64-elf-gcc --sysroot=$(SYSROOT)
export ARCH := $(shell ./build/default-host.sh)

export CFLAGS :=  -DMYOS1234 -Werror -Wno-unused-but-set-variable -Wno-address-of-packed-member -Wno-unused-function -Wno-unused-parameter -Wno-unused-label -Wno-unused-variable -Wno-error=type-limits -O0 -g -isystem=/usr/include -nostdlib $(shell ./build/arch-flags.sh $(ARCH)) -DCONFIG_$(ARCH) $(CONFIG_FLAGS) 


export QEMU_COMMAND := qemu-system-$(shell ./build/qemu-arch.sh $(ARCH))

export QEMU_FLAGS :=  -cdrom build/os.iso \
-m 512M \
-machine pc \
-d int \
-D build/log.txt \
-boot d  \
-vga std \
-display default,show-cursor=on \
-drive file=build/qemu.img,format=raw,if=ide,index=0 

ISO=build/os.iso


.PHONY: all debug test clean $(SUBDIRS)

all: $(ISO)

$(ISO) : $(SUBDIRS)
	./build/iso.sh

# $(SUBDIRS):
# 	mkdir -p "$(SYSROOT)"
# 	$(MAKE) -C $@ install-headers
# 	$(MAKE) -C $@ install
# 	@echo $@
# 	@exit 1

# Final artifact
build: $(ISO)

$(ISO): kernel userspace
	./build/iso.sh

# Directory rules
lib:
	@echo building lib
	$(MAKE) -C lib install
	# @exit 1

libc:
	$(MAKE) -C libc install

kernel: lib libc
	$(MAKE) -C kernel install

userspace: lib libc
	$(MAKE) -C userspace install

# $(SUBDIRS):
# 	mkdir -p "$(SYSROOT)"
# 	set -e
# 	for dir in $(SUBDIRS); do \
# 		$(MAKE) -C $$dir install-headers; \
# 	done
# 	for dir in $(SUBDIRS); do \
# 		$(MAKE) -C $$dir install; \
# 	done


qemu : $(ISO)
	rm -f build/qemu.img
	./build/get-ext2-image.sh "build/qemu.img" build/fs-root
	$(QEMU_COMMAND) -monitor stdio $(QEMU_FLAGS)

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

debug : $(ISO)
	./build/debug.sh

test : CFLAGS += -DTESTS
test : clean $(ISO) 
	./build/test.sh
