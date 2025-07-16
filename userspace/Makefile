CC?=i686-elf-gcc
DEFAULT_HOST!=../build/default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../build/target-triplet-to-arch.sh $(HOST)
ARCHDIR=arch/$(HOSTARCH)

BIN=bin

DEFAULT_HOST!=../build/default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../build/target-triplet-to-arch.sh $(HOST)

FS_ROOT?=
BINARY_PREFIX?=bin

CFLAGS?=-O0 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra  -Wno-unknown-pragmas
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude 
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS)  

CFLAGS:=$(CFLAGS) -fno-merge-constants -fno-merge-all-constants -Wno-write-strings -fomit-frame-pointer

# very bad
LINK_LIST=-l../libc/k.a

OBJS_START=\
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \

OBJS_END=\
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

# this is kinda bad
OBJS_START_BIN = $(foreach obj,$(OBJS_START),bin/$(obj))
OBJS_END_BIN = $(foreach obj,$(OBJS_END),bin/$(obj))

START_OBJ=\
		  $(BIN)/start.o

BINARIES=\
	elf_test \
	elf_test2 \

BINARIES_BIN=$(foreach obj,$(BINARIES),bin/$(obj))

.PHONY : all clean install install-binaries install-headers

$(BIN)/elf_test : $(OBJS_START_BIN) $(BIN)/elf_test.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/elf_test2 : $(OBJS_START_BIN) $(BIN)/elf_test2.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	# $(CC) -Ttext=0x600000 -o  $@ $^ $(CFLAGS)  -nostdlib -lgcc
	$(CC) -o  $@ $^ $(CFLAGS)  -nostdlib -lgcc

all : $(BINARIES_BIN)
	$(CC) -o $(BIN)/$

$(BIN)/%.o: %.S
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(BIN)/%.o: %.c
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS) 


$(BIN)/$(ARCHDIR)/crtbegin.o $(BIN)/$(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

clean:
	$(RM) -r bin/*

install : install-binaries install-headers 


install-headers:


install-binaries: $(BINARIES_BIN)
	# ../build/ is bad
	mkdir -p ../build/$(FS_ROOT)/$(BINARY_PREFIX)
	# $(foreach binary, $@, $(cp $(binary) ../build/$(FS_ROOT)/$(BINARY_PREFIX)))
	for f in $(BINARIES_BIN); do \
		cp $$f ../build/$(FS_ROOT); \
	done; 

	# # may break with more files in binaries
	# cp $(BIN)/$(BINARIES) ../build/$(FS_ROOT)/$(BINARY_PREFIX)


