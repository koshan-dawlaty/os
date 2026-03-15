ACPICA_CFLAGS=-Iacpica/ -I. -DMYOS1234 -DCONFIG_x86_64 -g
ACPI_SOURCES := $(shell find acpi/acpica -name '*.c')
ACPI_OBJS := $(patsubst %.c,%.o,$(ACPI_SOURCES))

%.o : %.c
	$(CC) -c $^ -o $@ $(ACPICA_CFLAGS) $(CFLAGS)

$(ACPICA_OBJ) : acpi/acpica.o

$(ACPICA_OBJ) : $(ACPI_OBJS)
	$(LD) -r $^ -o $@ 


acpi-clean : 
	$(RM) $(ACPICA_OBJ)
	$(RM) $(ACPI_OBJS)
