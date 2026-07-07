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

CFLAGS:=$(CFLAGS) -ffreestanding -Wno-unknown-pragmas  
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude 
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS)  

CFLAGS:=$(CFLAGS) -fno-merge-constants -fno-merge-all-constants -Wno-write-strings 
# this gets rid of moddi3 and simliar functions
CFLAGS:=$(CFLAGS) -fno-semantic-interposition -L$(SYSROOT)/usr/lib -Tlinker.ld


# very bad
# LINK_LIST=-l../libc/k.a

# ARCHDIR_x86=x86_64


ifdef CONFIG_x86_32
OBJS_START=\
$(ARCHDIR)/$(ARCHDIR_x86)/crti.o$(TEST_SUFFIX) \
$(ARCHDIR)/$(ARCHDIR_x86)/crtbegin.o$(TEST_SUFFIX) \

OBJS_END=\
$(ARCHDIR)/$(ARCHDIR_x86)/crtend.o$(TEST_SUFFIX) \
$(ARCHDIR)/$(ARCHDIR_x86)/crtn.o$(TEST_SUFFIX) \

else
OBJS_START=\
	$(ARCHDIR)/$(ARCHDIR_x86)/crti.o$(TEST_SUFFIX) \

OBJS_END=\
	$(ARCHDIR)/$(ARCHDIR_x86)/crtn.o$(TEST_SUFFIX) \

endif


# this is kinda bad
OBJS_START_BIN = $(foreach obj,$(OBJS_START),bin/$(obj))
OBJS_END_BIN = $(foreach obj,$(OBJS_END),bin/$(obj))

START_OBJ=\
		  $(BIN)/start.o$(TEST_SUFFIX)

BINARIES=\
	flashcards$(TEST_SUFFIX) \
	term$(TEST_SUFFIX) \
	edit$(TEST_SUFFIX) \
	elf_test2$(TEST_SUFFIX) \
	overwrite$(TEST_SUFFIX) \
	touch$(TEST_SUFFIX) \
	ksh$(TEST_SUFFIX) \
	cat$(TEST_SUFFIX) \
	imgview$(TEST_SUFFIX) \
	rm$(TEST_SUFFIX) \
	ls$(TEST_SUFFIX) \
	wm$(TEST_SUFFIX) \
	compiler$(TEST_SUFFIX) \
	ui_lib$(TEST_SUFFIX) \
	tron$(TEST_SUFFIX) \
	render_test$(TEST_SUFFIX) \
	explore$(TEST_SUFFIX) \
	run$(TEST_SUFFIX) \
	sokoban$(TEST_SUFFIX) \

BINARIES_BIN=$(foreach obj,$(BINARIES),bin/$(obj))

DEPENDENCIES=../libc/libc$(TEST_SUFFIX).a ../lib/bin/libutil$(TEST_SUFFIX).a

# UTIL_FLAG=-lutil$(TEST_SUFFIX)

.PHONY : all clean install install-binaries install-headers

all : $(BINARIES_BIN)
	$(CC) -o $(BIN)/$

# $(BIN)/term : $(OBJS_START_BIN) $(BIN)/term.o $(BIN)/wm_lib.o  $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
# 	@mkdir -p $(BIN)/$(dir $<)
# 	$(CC) -Ttext=0x600000  -o $@ $^ $(CFLAGS) $(UTIL_FLAG)  -nostdlib -lgcc 
	# $(CC) -o $@ $^ $(CFLAGS)  -nostdlib -lgcc 
	# $(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/elf_test2$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/elf_test2.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x700000 -o  $@ $^ $(CFLAGS)  -nostdlib -lgcc
	# $(CC) -o  $@ $^ $(CFLAGS)  -nostdlib -lgcc
$(BIN)/ksh$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/ksh.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0xb00000  -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/ls$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/ls.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x800000  -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/overwrite$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/overwrite.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/cat$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/cat.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/rm$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/rm.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x800000  -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/touch$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/touch.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) -nostdlib -lgcc

$(BIN)/wm$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/wm.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/imgview$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/imgview.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x300000 -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/tron$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/tron.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/explore$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/explore.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/flashcards$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/flashcards.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x300000 -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/render_test$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/render_test.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x300000 -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/term$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/term.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x300000 -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/run$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/run.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x300000 -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/edit$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/edit.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x300000 -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/ui_lib$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/ui_lib.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -Ttext=0x300000 -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz
	
$(BIN)/sokoban$(TEST_SUFFIX): $(OBJS_START_BIN) $(BIN)/sokoban.o$(TEST_SUFFIX) $(BIN)/wm_lib.o$(TEST_SUFFIX) $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc -Oz

$(BIN)/compiler$(TEST_SUFFIX) : $(OBJS_START_BIN) $(BIN)/c_compiler/c_compiler.o$(TEST_SUFFIX) $(BIN)/c_compiler/interpreter.o$(TEST_SUFFIX) $(BIN)/c_compiler/tests.o$(TEST_SUFFIX)  $(DEPENDENCIES) $(START_OBJ)  $(OBJS_END_BIN)
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -o $@ $^ $(CFLAGS) $(UTIL_FLAG) -nostdlib -lgcc  -Ttext=0x300000  


$(BIN)/%.o$(TEST_SUFFIX): %.S
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(BIN)/%.o$(TEST_SUFFIX): %.c
	@mkdir -p $(BIN)/$(dir $<)
	$(CC) -MD -MF$(BIN)/$*.d -c $< -o $@ $(CFLAGS) $(CPPFLAGS) # -DTESTS #  stupid


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
		cp $$f ../build/$(FS_ROOT)/bin/; \
	done; 

	# # may break with more files in binaries
	# cp $(BIN)/$(BINARIES) ../build/$(FS_ROOT)/$(BINARY_PREFIX)


test : 
	echo $(HOSTARCH)
