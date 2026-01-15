
ENTRY (_start)

SECTIONS
{
        . = 0x00100000;
	/* The kernel will live at 3GB + 1MB in the virtual address space, */
	/* which will be mapped to 1MB in the physical address space. */
	/* Note that we page-align the sections. */

	_kernel_start = .;
        .multiboot.data ALIGN (4K)  : {
            *(.multiboot.data)
        }

       .multiboot.text ALIGN (4K) : {
           *(.multiboot.text)
       }

	. += 0xC0000000;
	KERNEL_START = .;
	/* Add a symbol that indicates the start address of the kernel. */
	.text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000)
	{
		*(.text)
	}
	.rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000)
	{
		*(.rodata)
	}
	.data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000)
	{
		*(.data)
	}
	.bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}


	.ctors ALIGN (4) : AT(ADDR(.ctors) - 0xC0000000)
	{
		PROVIDE(__ctors_start = .);
		KEEP(*(SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP(*(.ctors))
		PROVIDE(__ctors_end = .);
	}


	/* .fini_array : ALIGN(8) { */
	/* 	PROVIDE_HIDDEN(__fini_array_start = .); */
	/* 	KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*))) */
	/* 	KEEP(*(.fini_array)) */
	/* 	PROVIDE_HIDDEN(__fini_array_end = .); */
	/* } */
	/* Add a symbol that indicates the end address of the kernel. */
	_kernel_end = .;
	KERNEL_START = .;
}
