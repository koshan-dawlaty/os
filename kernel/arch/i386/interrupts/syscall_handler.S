.globl syscall_handler_asm
.type syscall_handler_asm, @function
syscall_handler_asm:
	push %gs
	push %fs
	push %es
	push %ds
	push $(-38)
	push %ebp
	push %edi
	push %esi
	push %edx
	push %ecx
	push %ebx
	push %eax
	push %esp

	call syscall_handler

	add $4, %esp
	pop %eax
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp

	add $4, %esp

	pop %ds
	pop %es
	pop %fs
	pop %gs


	iret              
