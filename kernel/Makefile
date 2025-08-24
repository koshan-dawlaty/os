CC?=i686-elf-gcc

DEFAULT_HOST!=../build/default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../build/target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O2 -g 
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra  -Wno-unknown-pragmas -fstack-protector-all
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude 
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS)  

ARCHDIR=arch/$(HOSTARCH)


include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS) # -mgeneral-regs-only
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

TEST_OBJS=\
		  tests/tests_main.o \
		  tests/lib/tests_lib.o \

KERNEL_OBJS= \
$(KERNEL_ARCH_OBJS) \
$(KERNEL_TEST_OBJS) \
$(KERNEL_ARCH_TEST_OBJS) \
kernel/kernel.o \
kernel/utils.o \
kernel/usermode.o \
kernel/panic.o \
kernel/elf/elf.o \
kernel/scheduler/scheduler.o \
kernel/scheduler/semaphore.o \
fs/ext2/dentry.o \
fs/ext2/balloc.o \
fs/ext2/file.o \
fs/ext2/init.o \
fs/ext2/inode.o \
fs/ext2/misc.o \
fs/vfs/file.o \
fs/vfs/vfs.o \

OBJS=\
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(TEST_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

OBJS_BIN = $(foreach obj,$(OBJS),bin/$(obj))


LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(TEST_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

LINK_LIST_BIN = $(foreach obj,$(LINK_LIST),bin/$(obj))

BIN=bin


.PHONY: all clean install install-headers install-kernel

all: $(BIN)/os.kernel


# ../libc/libk.a is bad
$(BIN)/os.kernel: $(OBJS_BIN) $(ARCHDIR)/linker.ld ../libc/libk.a
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST_BIN) -nostdlib -L$(SYSROOT)/usr/lib -lk -lgcc
	grub-file --is-x86-multiboot $(BIN)/os.kernel


$(BIN)/$(ARCHDIR)/crtbegin.o $(BIN)/$(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@


$(BIN)/%.o: %.S
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(BIN)/%.o: %.c
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	$(RM) -r bin/*

install: install-headers install-kernel

install-headers:
	@mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: $(BIN)/os.kernel
	@mkdir -p $(DESTDIR)$(BOOTDIR)
	cp $(BIN)/os.kernel $(DESTDIR)$(BOOTDIR)

-include $(BIN)/$(OBJS:.o=.d)
