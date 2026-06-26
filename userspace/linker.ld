ENTRY (_start)


SECTIONS
{
	.ctors ALIGN (4) : AT(ADDR(.ctors))
	{
		PROVIDE(__ctors_start = .);
		KEEP(*(SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP(*(.ctors))
		PROVIDE(__ctors_end = .);
	}
}

/* INSERT AFTER .text */
