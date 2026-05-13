.section .text
.global jump_usermode_asm
.type   jump_usermode_asm @function
jump_usermode_asm:




	mov 0x4(%esp), %ebx
	mov 0x8(%esp), %edi

	mov 0xC(%esp), %ecx
	mov %ecx, %edx
	# mov $(0xBFFEF000), %ecx
	# mov $(0xBFFEF000), %edx
	# call %eax



jump_usermode_asm_iret_start:


 	cli
 	mov $((4 * 8) | 3), %ax # ring 3 data with bottom 2 bits set for ring 3
 	mov %ax, %ds
 	mov %ax, %es 
 	mov %ax, %fs
 	mov %ax, %gs # SS is handled by iret

 
	push %edi

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

	mov 0x14(%esp), %ecx

	# mov 0x00(%ecx), %edi
	# mov 0x04(%ecx), %esi
	# mov 0x10(%ecx), %ebx
	# mov 0x14(%ecx), %edx
	# mov 0x1C(%ecx), %eax
	# mov 0x18(%ecx), %ecx

	mov 0x10(%ecx), %edi      
	mov 0x14(%ecx), %esi      
	mov 0x18(%ecx), %ebp      
	mov 0x20(%ecx), %ebx      
	mov 0x24(%ecx), %edx      
	mov 0x2C(%ecx), %eax      
	mov 0x28(%ecx), %ecx      
 
 	# sti
 	iret


.extern tss

.section .text
.global switch_tasks_asm
.type   switch_tasks_asm @function
switch_tasks_asm:


	# registers pointer
	mov 0x14(%esp), %edi	
	# user bp
	mov 0x10(%esp), %edx
	# user sp
	mov 0xC(%esp), %ecx
 	# kernel sp	
	mov 0x8(%esp), %eax
	# ip
	mov 0x4(%esp), %ebx

	mov %eax, tss + 4

	jmp jump_usermode_asm_iret_start
	# mov 4(tss), %ecx

	cli
	hlt

	# ret

.section .text
.global flush_tss
.type flush_tss @function
flush_tss:
	mov $(5 * 8), %ax 
	ltr %ax
	ret

.section .text
.global switch_tasks_kernel
.type switch_tasks_kernel @function
switch_tasks_kernel:
	
	mov 0x08(%esp), %ebx
	mov 0x04(%esp), %eax


	mov 0x10(%eax), %edi      
	mov 0x14(%eax), %esi      
	mov 0x18(%eax), %ebp      

	mov 0x1C(%eax), %esp      
	mov %ebx, 0(%esp)

	mov 0x20(%eax), %ebx
	mov 0x24(%eax), %edx
	mov 0x28(%eax), %ecx
	mov 0x2C(%eax), %eax
	
	sti
	ret




# void save_registers(struct registers * regs)
.section .text
.global save_registers
.type save_registers @function
save_registers:

	push %eax
	mov 0x08(%esp), %eax

	mov 0x34(%eax), %edx
	fsave (%edx)
	fwait

	# add $4, %esp		
	push %esp
	add $4, 0(%esp)
	# sub $4, %esp		

	pushf
	push %ecx
	push %edx
	push %ebx
	push %ebp
	push %esi
	push %edi

	mov 0x00(%esp), %edi # esi
	mov %edi, 0x10(%eax)

	mov 0x04(%esp), %edi # edi
	mov %edi, 0x14(%eax)

	mov 0x08(%esp), %edi # ebp
	mov %edi, 0x18(%eax)

	mov 0x0C(%esp), %edi # ebx
	mov %edi, 0x20(%eax)

	mov 0x10(%esp), %edi # edx
	mov %edi, 0x24(%eax)

	mov 0x14(%esp), %edi # ecx
	mov %edi, 0x28(%eax)

	mov 0x18(%esp), %edi # eflags
	mov %edi, 0x30(%eax)

	mov 0x1C(%esp), %edi # esp
	mov %edi, 0x1C(%eax)

	mov 0x20(%esp), %edi # eax
	mov %edi, 0x2C(%eax)

	# lea %eip, 0x34(%eax)

	pop %edi
	add $0x20, %esp

	ret


.section .text
.global load_registers
.type load_registers @function
load_registers:


	mov 0x04(%esp), %ecx
	
	mov 0x34(%ecx), %edx
	frstor (%edx)
	fwait

	mov 0x10(%ecx), %edi
	mov 0x14(%ecx), %esi
	mov 0x18(%ecx), %ebp
	mov 0x1C(%ecx), %esp
	mov 0x20(%ecx), %ebx
	mov 0x24(%ecx), %edx
	mov 0x2C(%ecx), %eax
	mov 0x28(%ecx), %ecx

	# re-enable interrupts, this could be bad
	# pushf
	# or $(1 << 9), 0(%esp)
	# popf

	ret
