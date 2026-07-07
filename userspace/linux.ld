ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5); /* R + X */
  data PT_LOAD FLAGS(6); /* R + W */
}

SECTIONS
{
  . = 0x400000;

  .text : ALIGN(0x1000)
  {
    *(.text .text.*)
    *(.init)
    *(.fini)
  } :text

  .rodata : ALIGN(0x1000)
  {
    *(.rodata .rodata.*)
  } :text

  .eh_frame_hdr : { *(.eh_frame_hdr) } :text
  .eh_frame     : { *(.eh_frame) } :text

  .data : ALIGN(0x1000)
  {
    *(.data .data.*)
  } :data

  .bss (NOLOAD) : ALIGN(0x1000)
  {
    *(.bss .bss.*)
    *(COMMON)
  } :data
}
