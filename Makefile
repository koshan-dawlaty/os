# SUBDIRS := kernel/ lib/ libc/ userspace/
SUBDIRS := lib libc kernel userspace

.PHONY: all clean $(SUBDIRS)

CONFIG?=config

# CONFIG_FLAGS=$(shell echo $(CONFIG_FILE_RAW) | sed "s/^/-DCONFIG_/")
CONFIG_FLAGS=$(shell ./build/parse_config.py $(CONFIG))

export SYSROOT := $(shell pwd)/build/sysroot
export FS_ROOT := fs-root

export DESTDIR := $(SYSROOT)
export ARCH := $(shell ./build/default-host.sh)
export CC := $(shell ./build/compiler-arch.sh $(ARCH))-gcc --sysroot=$(SYSROOT)

export CFLAGS :=  -DMYOS1234 -Werror -Wno-unused-but-set-variable -Wno-address-of-packed-member -Wno-unused-function -Wno-unused-parameter -Wno-unused-label -Wno-unused-variable -Wno-error=type-limits -O0 -g -isystem=/usr/include -nostdlib $(shell ./build/arch-flags.sh $(ARCH)) -DCONFIG_$(ARCH) $(CONFIG_FLAGS) -fomit-frame-pointer


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

build: $(ISO)

$(ISO): kernel userspace
	./build/iso.sh

lib: headers
	@echo building lib
	mkdir -p "$(SYSROOT)"
	$(MAKE) -C lib install
	# @exit 1

libc: headers
	mkdir -p "$(SYSROOT)"
	$(MAKE) -C libc install

kernel: headers lib libc
	mkdir -p "$(SYSROOT)"
	$(MAKE) -C kernel install

userspace: headers lib libc
	mkdir -p "$(SYSROOT)"
	$(MAKE) -C userspace install

headers: lib-headers libc-headers kernel-headers userspace-headers
	

lib-headers:
	@echo building lib
	mkdir -p "$(SYSROOT)"
	$(MAKE) -C lib install-headers

libc-headers:
	@echo building libc
	mkdir -p "$(SYSROOT)"
	$(MAKE) -C libc install-headers

kernel-headers:
	@echo building kernel
	mkdir -p "$(SYSROOT)"
	$(MAKE) -C kernel install-headers

userspace-headers:
	@echo building userspace
	mkdir -p "$(SYSROOT)"
	$(MAKE) -C userspace install-headers



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
	$(QEMU_COMMAND) -serial stdio $(QEMU_FLAGS)

clean:
	$(RM) -r $(SYSROOT)
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

debug : $(ISO)
	./build/debug.sh

test : CFLAGS += -DTESTS
test : clean $(ISO) 
	./build/test.sh
