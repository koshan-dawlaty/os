CC?=i686-elf-gcc

DEFAULT_HOST!=../build/default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../build/archdir.sh $(HOST)

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


ARCHDIR_x86=$(notdir $(HOSTARCH))
ARCHDIR=arch/$(firstword $(subst /, ,$(HOSTARCH)))


include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS) # -mgeneral-regs-only
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

TEST_OBJS=\
		  tests/tests_main.o \
		  tests/lib/tests_lib.o \
		  tests/lib/tap.o \

KERNEL_OBJS= \
$(KERNEL_ARCH_OBJS) \
$(KERNEL_TEST_OBJS) \
$(KERNEL_ARCH_TEST_OBJS) \
kernel/kernel.o \
kernel/utils.o \
kernel/usermode.o \
kernel/panic.o \
kernel/elf/elf.o \
kernel/elf/elf32.o \
kernel/elf/elf64.o \
kernel/scheduler/scheduler.o \
kernel/scheduler/sleep.o \
kernel/scheduler/semaphore.o \
kernel/memory/get_free_pages.o \
lib/list.o \
lib/rc.o \
fs/ext2/dentry.o \
fs/ext2/balloc.o \
fs/ext2/file.o \
fs/ext2/init.o \
fs/ext2/inode.o \
fs/ext2/misc.o \
fs/vfs/file.o \
fs/vfs/pathname.o \
fs/vfs/vfs.o \
fs/vfs/dcache.o \

ifdef CONFIG_x86_32
ARCH_OBJS2=
	$(ARCHDIR)/$(ARCHDIR_x86)/crtbegin.o \
	$(ARCHDIR)/$(ARCHDIR_x86)/crtend.o \
else 
ARCH_OBJS2=
endif

OBJS=\
$(ARCHDIR)/$(ARCHDIR_x86)/crti.o \
$(ARCH_OBJS2) \
$(KERNEL_OBJS) \
$(TEST_OBJS) \
$(ARCHDIR)/$(ARCHDIR_x86)/crtn.o \
 
OBJS_BIN = $(foreach obj,$(OBJS),bin/$(obj))


LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/$(ARCHDIR_x86)/crti.o \
$(KERNEL_OBJS) \
$(TEST_OBJS) \
$(LIBS) \
$(ARCHDIR)/$(ARCHDIR_x86)/crtn.o \

LINK_LIST_BIN = $(foreach obj,$(LINK_LIST),bin/$(obj))

BIN=bin


.PHONY: all clean install install-headers install-kernel

all: $(BIN)/os.kernel


# ../libc/libk.a is bad
$(BIN)/os.kernel: $(OBJS_BIN) $(ARCHDIR)/$(ARCHDIR_x86)/linker.ld ../libc/libk.a
	$(CC) -T $(ARCHDIR)/$(ARCHDIR_x86)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST_BIN) -nostdlib -L$(SYSROOT)/usr/lib -lk -lgcc
	grub-file --is-x86-multiboot $(BIN)/os.kernel


ifdef CONFIG_x86_32
# ia_32 only
$(BIN)/$(ARCHDIR)/$(ARCHDIR_x86)/crtbegin.o $(BIN)/$(ARCHDIR)/$(ARCHDIR_x86)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@
endif


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
