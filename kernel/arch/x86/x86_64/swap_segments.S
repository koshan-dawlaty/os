.globl swap_segments
.type swap_segments, @function
swap_segments:
	mov $0x10, %ax
	# mov $8, %ax
	# mov %ax, %ss
	# mov %ax, %ds
	# mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	ret


.section .text
.global flush_tss
.type flush_tss @function
flush_tss:
	mov $0x28, %ax 
	ltr %ax
	ret
