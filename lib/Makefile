BIN=bin
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

# CFLAGS:=-ffreestanding -Wall -Wextra  -Wno-unknown-pragmas -fstack-protector-all -D__is_libk \
# 		$(CFLAGS) \
		# -Iacpi/acpica/ # TODO: this is bad 
# CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude 
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS)  


CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)


OBJS=\
	list.o

OBJS_BIN = $(foreach obj,$(OBJS),bin/$(obj))


LIB_BINARY=$(BIN)/libutil.a

all : install

$(LIB_BINARY) : $(OBJS_BIN)
	$(AR) rcs $@ $(OBJS_BIN)


$(BIN)/%.o: %.c
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

.PHONY : install install-headers install-lib clean

install: install-lib

install-lib: $(LIB_BINARY)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(LIB_BINARY) $(DESTDIR)$(LIBDIR)

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

clean:
	$(RM) -r bin/*
	$(RM) $(LIB_BINARY)

-include $(BIN)/$(OBJS:.o=.d)
