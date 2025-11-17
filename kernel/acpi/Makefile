ACPICA_CFLAGS=-Iacpica/ -I. -DMYOS1234 -DCONFIG_x86_32
SOURCES := $(shell find acpica -name '*.c')
OBJS := $(patsubst %.c,%.o,$(SOURCES))

%.o : %.c
	$(CC) -c $^ -o $@ $(ACPICA_CFLAGS)

acpica.o : $(OBJS)
	$(LD) -r $^ -o $@ 

clean : 
	$(RM) acpica.o
	$(RM) $(OBJS)
