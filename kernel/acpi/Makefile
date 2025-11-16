ACPICA_CFLAGS=-Iacpica/
SOURCES := $(shell find . -name '*.c')
OBJS := $(patsubst %.c,%.o,$(SOURCES)) os_layer/os_layer.o

%.o : %.c
	$(CC) -c $^ -o $@ $(ACPICA_CFLAGS)

acpica.o : $(OBJS)
	$(LD) -r $^ -o $@ $(ACPICA_CFLAGS)

clean : 
	rm $(OBJS)
