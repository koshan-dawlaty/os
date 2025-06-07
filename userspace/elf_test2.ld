ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5); /* 5 = R + X */
  data PT_LOAD FLAGS(6); /* 6 = R + W */
}

SECTIONS
{
  . = 0x600000; 

  .text : {
    *(.text)
  } :text

  .rodata : {
    *(.rodata)
  } :text

  .data : {
    *(.data)
  } :data

  .bss : {
    *(.bss)
    *(COMMON)
  } :data
}
