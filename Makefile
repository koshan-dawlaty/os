# SUBDIRS := kernel/ lib/ libc/ userspace/
SUBDIRS := lib libc kernel userspace

.PHONY: all clean $(SUBDIRS)

CONFIG?=config
# CONFIG?=sony_vaio.conf

# CONFIG_FLAGS=$(shell echo $(CONFIG_FILE_RAW) | sed "s/^/-DCONFIG_/")
CONFIG_FLAGS=$(shell ./build/parse_config.py $(CONFIG))

export SYSROOT := $(shell pwd)/build/sysroot
export FS_ROOT := fs-root

export DESTDIR := $(SYSROOT)
export ARCH := $(shell ./build/default-host.sh)
export CC := $(shell ./build/compiler-arch.sh $(ARCH))-gcc --sysroot=$(SYSROOT)
export AS := $(shell ./build/compiler-arch.sh $(ARCH))-gcc --sysroot=$(SYSROOT)

export CXX := $(shell ./build/compiler-arch.sh $(ARCH))-g++ --sysroot=$(SYSROOT)

# -fno-optimize-sibling-calls will not generate jmp instructions for save/load registers which will make them not break
# once printf supports the actual standard format specifiers, -Wno-format can be removed
export CFLAGS :=  -DMYOS1234 -Werror -O0 \
-Wno-format	\
-fno-optimize-sibling-calls \
-Wno-unused-but-set-variable -Wno-address-of-packed-member -Wno-unused-function -Wno-unused-parameter -Wno-unused-label -Wno-unused-variable -Wno-error=type-limits -g -g3 -isystem=/usr/include -nostdlib $(shell ./build/arch-flags.sh $(ARCH)) -DCONFIG_$(ARCH) $(CONFIG_FLAGS) -fno-omit-frame-pointer -Wno-unused-parameter

export CXXFLAGS := \
-Wno-pointer-arith -fpermissive -std=gnu++17 -Wno-pointer-arith -fno-rtti  -Wno-narrowing -Wno-invalid-offsetof -fno-exceptions

export QEMU_COMMAND := qemu-system-$(shell ./build/qemu-arch.sh $(ARCH))

export QEMU_FLAGS :=  -cdrom build/os.iso \
-m 256M \
-machine pc \
-D build/log.txt \
-boot d  \
-vga std \
-display default,show-cursor=on \
# --enable-kvm \

# -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
# -device e1000,netdev=net0
# -netdev user,id=net0 \
# -device e1000,netdev=net0 \
# -smp 1 \




export QEMU_DRIVE=\
-drive file=build/qemu.img,format=raw,if=ide,index=0 


ifdef NODISPLAY
	QEMU_FLAGS += -display none
endif

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
	# $(QEMU_COMMAND) -monitor stdio $(QEMU_FLAGS)
	$(QEMU_COMMAND) -serial mon:stdio $(QEMU_FLAGS) $(QEMU_DRIVE)

clean:
	$(RM) -r $(SYSROOT)
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

# debug : QEMU_FLAGS += -display none
debug : $(ISO)
	rm -f build/qemu.img
	./build/get-ext2-image.sh "build/qemu.img" build/fs-root
	./build/debug.sh

codegen-debug : $(ISO)
	rm -f build/qemu.img
	./build/get-ext2-image.sh "build/qemu.img" build/fs-root
	./build/debug.sh "-x build/codegen_debug.gdb"

test : CFLAGS += -DTESTS
test : export TEST_SUFFIX=.test
test : $(ISO) 
	./build/test.sh

nvim-debug: $(ISO)
	./nvim-debug.sh
