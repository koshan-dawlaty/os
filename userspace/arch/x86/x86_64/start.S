
.section .text
.global _start
.type _start, @function
_start:
	push %rdi
	push %rsi
	call init_libc
	pop %rsi
	pop %rdi

	call main

	mov %eax, %edi
	call exit
	hlt
