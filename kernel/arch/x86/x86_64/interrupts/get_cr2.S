.globl get_cr2
.type get_cr2, @function
get_cr2:
	mov %cr2, %rax
	ret
