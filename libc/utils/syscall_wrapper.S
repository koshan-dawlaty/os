.globl syscall_wrapper_asm
.type syscall_wrapper_asm, @function
syscall_wrapper_asm:

	push %ebp
	mov %esp, %ebp

	push %ebx
	push %esi
	push %edi


	mov 0x08(%ebp), %eax
	mov 0x0C(%ebp), %ebx
	mov 0x10(%ebp), %ecx
	mov 0x14(%ebp), %edx
	mov 0x18(%ebp), %esi
	mov 0x1C(%ebp), %edi


	

	int $(0x80)

	pop %edi
	pop %esi
	pop %ebx

	pop %ebp
	ret 
