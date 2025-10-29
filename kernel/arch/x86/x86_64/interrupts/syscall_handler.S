.globl syscall_handler_asm
.type syscall_handler_asm, @function
syscall_handler_asm:

	push %rax
	push %gs
	push %fs
	push $(-38)
	push %rbp
	push %rdi
	push %rsi
	push %rdx
	push %rcx
	push %rbx
	mov %rsp, %rdi

	# this will allow the kernel to be preempted, however it may result in alot of race conditions
	sti
	call syscall_handler
	# add $8, %rsp

	pop %rbx
	pop %rcx
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rbp


	add $8, %rsp
	

	pop %fs
	pop %gs

	# skip this because it would need to pop ax which is the return value
	add $8, %rsp

	iretq
