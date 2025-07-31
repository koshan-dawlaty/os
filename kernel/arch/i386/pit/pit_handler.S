.extern pit_handler_c

.section .text
.global pit_handler_asm
.type   pit_handler_asm @function
pit_handler_asm:
	cli
	pusha

	
	push %ds
	push %es
	push %fs
	push %gs

	mov $(0x10), %ax

	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	cld
	push %esp
	call pit_handler_c
	add $4, %esp
	
	pop %gs
    pop %fs
    pop %es
    pop %ds

	popa

	# or $(1 << 9), 0xC(%esp)

	iret
