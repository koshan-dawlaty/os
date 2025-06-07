.section .text
.global jump_usermode_asm
.type   jump_usermode_asm @function
.extern user_stack_top
jump_usermode_asm:

	# mov 0x4(%esp), %eax
	# mov $(0xBFFEF000), %esp
	# call %eax

	# cli
	# mov $((4 * 8) | 3), %ax # ring 3 data with bottom 2 bits set for ring 3
	# mov %ax, %ds
	# mov %ax, %es 
	# mov %ax, %fs
	# mov %ax, %gs # SS is handled by iret

	# # set up the stack frame iret expects
	# mov $(0xBFFEF000), %eax
	# push $((4 * 8) | 3) # data selector
	# push %eax # current esp
	# pushf # eflags
	# push $((3 * 8) | 3) # code selector (ring 3 code with bottom 2 bits set for ring 3)
	# # push 0x04(%esp) # instruction address to return to
	# push 0x04(%esp) # instruction address to return to

	# sti
	# iret


	cli
	mov $((4 * 8) | 3), %ax  # User data segment selector (0x1B)
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	xor %edx, %edx # not necessary; set to 0
	mov $(0x8), %eax # the segments are computed as follows: CS=MSR+0x10 (0x8+0x10=0x18), SS=MSR+0x18 (0x8+0x18=0x20).
	mov $(0x174), %ecx # MSR specifier: IA32_SYSENTER_CS
	wrmsr

	mov 0x04(%esp), %edx  
	mov $(0xBFFEF000), %ecx

	sti
 	sysexit
 	


.section .text
.global flush_tss
.type flush_tss @function
flush_tss:
	mov $(5 * 8), %ax 
	ltr %ax
	ret
