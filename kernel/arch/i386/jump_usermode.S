.section .text
.global jump_usermode_asm
.type   jump_usermode_asm @function
jump_usermode_asm:




	mov 0x4(%esp), %ebx
	mov $(0xBFFEF000), %ecx
	mov $(0xBFFEF000), %edx
	# call %eax


jump_usermode_asm_iret_start:

 	cli
 	mov $((4 * 8) | 3), %ax # ring 3 data with bottom 2 bits set for ring 3
 	mov %ax, %ds
 	mov %ax, %es 
 	mov %ax, %fs
 	mov %ax, %gs # SS is handled by iret
 
 
 	# mov %esp, %eax
 	# set up the stack frame iret expects
 	# mov $(0xBFFEF000), %eax
	mov %edx, %ebp
	# sub $4, %ebp
 	mov %ecx, %eax
 	push $((4 * 8) | 3) # data selector
 	push %eax # user esp
 	pushf # eflags
 	or $(1 << 9), 0(%esp)
 	push $((3 * 8) | 3) # code selector (ring 3 code with bottom 2 bits set for ring 3)
 	# push 0x04(%esp) # instruction address to return to
 	push %ebx
 
 	# mov $(0x20), %edx
	# outb $(0x20), %edx
 	# sti
 	iret


.extern tss

.section .text
.global switch_tasks_asm
.type   switch_tasks_asm @function
switch_tasks_asm:
	# user bp
	mov 0x10(%esp), %edx
	# user sp
	mov 0xC(%esp), %ecx
 	# kernel sp	
	mov 0x8(%esp), %eax
	# ip
	mov 0x4(%esp), %ebx

	# mov tss + 4, %eax 
	mov %eax, tss + 4

	jmp jump_usermode_asm_iret_start
	# mov 4(tss), %ecx


	ret

	



.section .text
.global flush_tss
.type flush_tss @function
flush_tss:
	mov $(5 * 8), %ax 
	ltr %ax
	ret
