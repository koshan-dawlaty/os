CC?=i686-elf-gcc

DEFAULT_HOST!=../build/default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../build/archdir.sh $(HOST)


ARCHDIR_x86=$(notdir $(HOSTARCH))
ARCHDIR=arch/$(firstword $(subst /, ,$(HOSTARCH)))


# ARCHDIR=arch/$(HOSTARCH)

BIN=bin


FS_ROOT?=
BINARY_PREFIX?=bin

CFLAGS?= -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra  -Wno-unknown-pragmas 
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude 
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS)  

CFLAGS:=$(CFLAGS) -fno-merge-constants -fno-merge-all-constants -Wno-write-strings -fomit-frame-pointer
# this gets rid of moddi3 and simliar functions
CFLAGS:=$(CFLAGS) -fno-semantic-interposition


# very bad
LINK_LIST=-l../libc/k.a

# ARCHDIR_x86=x86_64


ifdef CONFIG_x86_32
OBJS_START=\
$(ARCHDIR)/$(ARCHDIR_x86)/crti.o \
$(ARCHDIR)/$(ARCHDIR_x86)/crtbegin.o \

OBJS_END=\
$(ARCHDIR)/$(ARCHDIR_x86)/crtend.o \
$(ARCHDIR)/$(ARCHDIR_x86)/crtn.o \

else
OBJS_START=\
	$(ARCHDIR)/$(ARCHDIR_x86)/crti.o \

OBJS_END=\
	$(ARCHDIR)/$(ARCHDIR_x86)/crtn.o \

endif


# this is kinda bad
OBJS_START_BIN = $(foreach obj,$(OBJS_START),bin/$(obj))
OBJS_END_BIN = $(foreach obj,$(OBJS_END),bin/$(obj))

START_OBJ=\
		  $(BIN)/start.o

BINARIES=\
	elf_test \
	elf_test2 \
	overwrite \
	touch \
	ksh \
	cat \
	rm \
	ls \
	wm \

BINARIES_BIN=$(foreach obj,$(BINARIES),bin/$(obj))

.PHONY : all clean install install-binaries install-headers

$(BIN)/elf_test : $(OBJS_START_BIN) $(BIN)/elf_test.o $(BIN)/wm_lib.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/elf_test2 : $(OBJS_START_BIN) $(BIN)/elf_test2.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x600000 -o  $@ $^ $(CFLAGS)  -nostdlib -lgcc
	# $(CC) -o  $@ $^ $(CFLAGS)  -nostdlib -lgcc
$(BIN)/ksh : $(OBJS_START_BIN) $(BIN)/ksh.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/ls : $(OBJS_START_BIN) $(BIN)/ls.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/overwrite : $(OBJS_START_BIN) $(BIN)/overwrite.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/cat : $(OBJS_START_BIN) $(BIN)/cat.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/rm : $(OBJS_START_BIN) $(BIN)/rm.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/touch : $(OBJS_START_BIN) $(BIN)/touch.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/wm : $(OBJS_START_BIN) $(BIN)/wm.o $(BIN)/wm_lib.o ../libc/libc.a $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc



all : $(BINARIES_BIN)
	$(CC) -o $(BIN)/$

$(BIN)/%.o: %.S
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(BIN)/%.o: %.c
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS)


ifdef CONFIG_x86_32
$(BIN)/$(ARCHDIR)/$(ARCHDIR_x86)/crtbegin.o $(BIN)/$(ARCHDIR)/$(ARCHDIR_x86)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@
endif

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


test : 
	echo $(HOSTARCH)
