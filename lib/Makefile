BIN=bin
DEFAULT_HOST!=../build/default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../build/archdir.sh $(HOST)

CFLAGS?=-O2 -g 
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include
LIBDIR?=$(PREFIX)/lib

# CFLAGS:=-ffreestanding -Wall -Wextra  -Wno-unknown-pragmas -fstack-protector-all -D__is_libk \
# 		$(CFLAGS) \
		# -Iacpi/acpica/ # TODO: this is bad 
# CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude 
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS)  


CFLAGS:=$(CFLAGS) -Iinclude $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)


OBJS=\
	list.o \
	array.o \
	framebuffer.o \
	log.o \
	rect.o \
	assert.o \

OBJS_BIN = $(foreach obj,$(OBJS),bin/$(obj))

LIBK_OBJS=$(OBJS:.o=.libk.o)
LIBK_OBJS_BIN = $(foreach obj,$(LIBK_OBJS),bin/$(obj))



LIB_BINARY=$(BIN)/libutil.a
KERNEL_LIB_BINARY=$(BIN)/libutilkernel.a
.PHONY : all install install-headers install-lib clean

all : $(LIB_BINARY) $(KERNEL_LIB_BINARY)

$(LIB_BINARY) : $(OBJS_BIN)
	@mkdir -p $(BIN)
	$(AR) rcs $@ $(OBJS_BIN)

$(KERNEL_LIB_BINARY) : $(LIBK_OBJS_BIN)
	@mkdir -p $(BIN)
	$(AR) rcs $@ $^


$(BIN)/%.libk.o: %.c
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -D__is_libk

$(BIN)/%.o: %.c
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -D__is_libc


install: install-lib

install-lib: $(LIB_BINARY) $(KERNEL_LIB_BINARY)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(LIB_BINARY) $(DESTDIR)$(LIBDIR)
	cp $(KERNEL_LIB_BINARY) $(DESTDIR)$(LIBDIR)

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

clean:
	$(RM) -r bin/*
	$(RM) $(LIB_BINARY)

-include $(BIN)/$(OBJS:.o=.d)
