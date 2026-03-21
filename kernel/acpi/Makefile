ACPICA_CFLAGS=-Iacpica/ -I. -DMYOS1234 -g
ACPI_SOURCES := $(shell find acpi/acpica -name '*.c')
ACPI_OBJS := $(patsubst %.c,%.o,$(ACPI_SOURCES))

%.o : %.c
	$(CC) -c $^ -o $@ $(ACPICA_CFLAGS) $(CFLAGS)


$(ACPICA_OBJ) : $(ACPI_OBJS)
	$(LD) -r $^ -o $@ 


acpi-clean : 
	$(RM) $(ACPICA_OBJ)
	$(RM) $(ACPI_OBJS)
