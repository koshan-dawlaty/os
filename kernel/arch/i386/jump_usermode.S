.section .text
.global jump_usermode_asm
.type   jump_usermode_asm @function
.extern user_stack_top
jump_usermode_asm:

	mov $((4 * 8) | 3), %ax  # User data segment selector (0x1B)
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	xor %edx, %edx # not necessary; set to 0
	mov $(0x8), %eax # the segments are computed as follows: CS=MSR+0x10 (0x8+0x10=0x18), SS=MSR+0x18 (0x8+0x18=0x20).
	mov $(0x174), %ecx # MSR specifier: IA32_SYSENTER_CS
	wrmsr

	mov $(0x08048000), %edx  
	# mov %esp, %ecx    
	mov $(0xBFFF0000), %ecx    
 	sysexit
 	


.section .text
.global flush_tss
.type flush_tss @function
flush_tss:
	mov $(5 * 8), %ax 
	ltr %ax
	ret
