.globl syscall_wrapper_asm
.type syscall_wrapper_asm, @function
syscall_wrapper_asm:

	push %rbp
	mov %rsp, %rbp

	push %rbx
	push %rsi
	push %rdi


	mov %rdi, %rax
	mov %rsi, %rbx
	mov %rdx, %rcx
	mov %rcx, %rdx
	mov %r8, %rsi
	mov %r9, %rdi

	int $(0x80)

	pop %rdi
	pop %rsi
	pop %rbx

	pop %rbp
	ret 
