.globl swap_segments
.type swap_segments, @function
swap_segments:
	mov $0x10, %ax
	mov %ax, %ss
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	ret
