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
#
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS)  

TEST_SUFFIX?=


CFLAGS:=$(CFLAGS) -Iinclude $(KERNEL_ARCH_CFLAGS) 
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)


OBJS=\
	compiler/ast.o \
	compiler/interpreter.o \
	compiler/codegen_x86_64.o \
	list.o \
	font.o \
	img.o \
	array.o \
	term.o \
	framebuffer.o \
	log.o \
	wm_lib.o \
	rect.o \
	assert.o \
	tests_lib/tap.o \
	tests_lib/tests_lib.o \
	ui/ui.o \
	ui/text_input.o \
	ui/text.o \
	ui/image.o \
	ui/button.o \
	userspace/input/keyboard.o \
	tests/compiler_tests.o \
	tests/interpreter_tests.o \


OBJS_BIN_NO_SUFFIX = $(foreach obj,$(OBJS),bin/$(obj))
OBJS_BIN = $(addsuffix $(TEST_SUFFIX), $(OBJS_BIN_NO_SUFFIX))

$(info $(OBJS_BIN))

LIBK_OBJS=$(OBJS:.o=.libk.o)
LIBK_OBJS_BIN_NO_SUFFIX = $(foreach obj,$(LIBK_OBJS),bin/$(obj))
LIBK_OBJS_BIN = $(addsuffix $(TEST_SUFFIX), $(LIBK_OBJS_BIN_NO_SUFFIX))

$(info $(LIBK_OBJS_BIN))
# LIBK_OBJS_BIN = $(foreach obj,$(LIBK_OBJS),bin/$(obj))

LIB_BINARY=$(BIN)/libutil$(TEST_SUFFIX).a
KERNEL_LIB_BINARY=$(BIN)/libutilkernel$(TEST_SUFFIX).a
.PHONY : all install install-headers install-lib clean

all : $(LIB_BINARY) $(KERNEL_LIB_BINARY)

$(LIB_BINARY) : $(OBJS_BIN)
	@mkdir -p $(BIN)
	$(AR) rcs $@ $(OBJS_BIN)

$(KERNEL_LIB_BINARY) : $(LIBK_OBJS_BIN)
	@mkdir -p $(BIN)
	$(AR) rcs $@ $^ 


$(BIN)/%.libk.o$(TEST_SUFFIX): %.c
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -D__is_libk 

$(BIN)/%.o$(TEST_SUFFIX): %.c
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -D__is_libc

$(BIN)/%.libk.o$(TEST_SUFFIX): %.cpp
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) -D__is_libk 

$(BIN)/%.o$(TEST_SUFFIX): %.cpp
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) -D__is_libc


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
