DEFAULT_HOST!=../build/default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../build/archdir.sh $(HOST)

CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/
EXEC_PREFIX?=$(PREFIX)
INCLUDEDIR?=$(PREFIX)/include
LIBDIR?=$(EXEC_PREFIX)/lib

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra 
CPPFLAGS:=$(CPPFLAGS) -D__is_libc -Iinclude -I$(DESTDIR)$(INCLUDEDIR)
LIBK_CFLAGS:=$(CFLAGS)
LIBK_CPPFLAGS:=$(CPPFLAGS) -D__is_libk


ARCHDIR_x86=$(notdir $(HOSTARCH))
ARCHDIR=arch/$(firstword $(subst /, ,$(HOSTARCH)))

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(ARCH_CPPFLAGS)
LIBK_CFLAGS:=$(LIBK_CFLAGS) $(KERNEL_ARCH_CFLAGS)
LIBK_CPPFLAGS:=$(LIBK_CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)

TEST_SUFFIX?=

FREEOBJS=\
$(ARCH_FREEOBJS) \
string/strlen.o \
string/strchr.o \
string/strcat.o \
string/strcmp.o \
string/strncmp.o \
stdlib/atoi.o \
stdlib/exit.o \
stdlib/malloc.o \
string/memcpy.o \
string/memset.o \
string/strdup.o \
string/strtok.o \
stdio/putchar.o \
stdio/printf.o \
stdio/vsprintf.o \
stdio/puts.o \
stdio/getchar.o \
stdio/stream.o \
stdio/vprintf.o \
file/open.o \
file/creat.o \
file/lseek.o \
file/unlink.o \
file/read.o \
file/write.o \
file/pread.o \
sys/mman/mmap.o \
sys/stat/mkdir.o \
unistd/execv.o \
unistd/sleep.o \
unistd/close.o \
unistd/chdir.o \
sched/sched_yield.o \
dirent/dirent.o \
init.o \
errno.o \
ctype.o \


HOSTEDOBJS=\
$(ARCH_HOSTEDOBJS) \

OBJS=\
$(FREEOBJS) \
$(HOSTEDOBJS) \

OBJS_BIN_NO_SUFFIX = $(foreach obj,$(OBJS),bin/$(obj))
OBJS_BIN= $(addsuffix $(TEST_SUFFIX), $(OBJS_BIN_NO_SUFFIX))

LIBK_OBJS=$(FREEOBJS:.o=.libk.o)
LIBK_OBJS_BIN_NO_SUFFIX = $(foreach obj,$(LIBK_OBJS),bin/$(obj))
LIBK_OBJS_BIN= $(addsuffix $(TEST_SUFFIX), $(LIBK_OBJS_BIN_NO_SUFFIX))


BINARIES=libc$(TEST_SUFFIX).a libk$(TEST_SUFFIX).a 
# BINARIES=libk.a

.PHONY: all clean install install-headers install-libs
# .SUFFIXES: .o .libk.o .c .S

BIN=bin



# all: $(BINARIES)
all: install-libs

libc$(TEST_SUFFIX).a: $(OBJS_BIN)
	$(AR) rcs $@ $(OBJS_BIN)

libk$(TEST_SUFFIX).a: $(LIBK_OBJS_BIN)
	$(AR) rcs $@ $(LIBK_OBJS_BIN)

$(BIN)/%.o$(TEST_SUFFIX): %.c
	mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(BIN)/%.o$(TEST_SUFFIX): %.S
	mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BIN)/%.libk.o$(TEST_SUFFIX): %.c
	mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.libk.d -c $< -o $@ -std=gnu11 $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

$(BIN)/%.libk.o$(TEST_SUFFIX): %.S
	mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

clean:
	$(RM) -r bin/*
	$(RM) $(BINARIES)

install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-libs: $(BINARIES)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(BINARIES) $(DESTDIR)$(LIBDIR)
	# @echo $(DESTDIR)$(LIBDIR)
	# @exit 1

-include $(BIN)/$(OBJS:.o=.d)
-include $(BIN)/$(LIBK_OBJS:.o=.libk.d)
