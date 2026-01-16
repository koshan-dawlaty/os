ENTRY(_start)
SECTIONS
{


    . = 0x00100000;
	KERNEL_PHYSADDR = .;


	.multiboot.data ALIGN (4K)  : {
		*(.multiboot.data)
	}

	.multiboot.text ALIGN (4K) : {
		*(.multiboot.text)
	}


	KERNEL_VMA = 0xffffffff80000000;
    . += KERNEL_VMA;
	LINKER_KERNEL_START = .;

    .text : AT(ADDR(.text) - KERNEL_VMA)
    {
        _code = .;
        *(.text)
        *(.rodata*)
        . = ALIGN(4096);
    }

   .data : AT(ADDR(.data) - KERNEL_VMA)
   {
        _data = .;
        *(.data)
        . = ALIGN(4096);
   }

   .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA)
   {
       _ehframe = .;
       *(.eh_frame)
        . = ALIGN(4096);
   }

   .bss : AT(ADDR(.bss) - KERNEL_VMA)
   {
       _bss = .;

       *(.bss)
	   *(.stack)
       /*
        * You usually need to include generated COMMON symbols
        * under kernel BSS section or use gcc's -fno-common
        */
        *(COMMON)
       . = ALIGN(4096);
   }


	.ctors ALIGN (4) : AT(ADDR(.ctors) - KERNEL_VMA)
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

   _end = .;


   /DISCARD/ :
   {
        *(.comment)
   }
	LINKER_KERNEL_END = .;
}
