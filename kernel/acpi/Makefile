ACPICA_CFLAGS=-Iacpica/ -Iacpica/include -I. -DMYOS1234 -g
ACPI_SOURCES := $(shell find acpi/acpica -name '*.c')
ACPI_OBJS := $(patsubst %.c,%.o,$(ACPI_SOURCES))
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))


%.o : %.c
	$(CC) -c $^ -o $@ $(ACPICA_CFLAGS) $(CFLAGS)


$(ACPICA_OBJ) : $(ACPI_OBJS)
	$(LD) -r $^ -o $@ 


acpi-clean : 
	$(RM) $(shell find $(MAKEFILE_DIR) -name '*.o')
	$(RM) $(ACPICA_OBJ)
	$(RM) $(ACPI_OBJS)
