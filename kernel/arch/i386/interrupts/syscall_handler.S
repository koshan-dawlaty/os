# .globl syscall_handler_asm
# .type syscall_handler_asm, @function
# syscall_handler_asm:
# 	pushl %eax            

# 	movl $(0x10), %eax

# 	pushl %gs             
# 	pushl %fs             
# 	pushl %es             
# 	pushl %ds             

# 	movl %eax, %ds
# 	movl %eax, %es
# 	movl %eax, %ss

# 	pushl $-38        # Push default error code (-ENOSYS)
# 	pushl %ebp            
# 	pushl %edi            
# 	pushl %esi            
# 	pushl %edx            
# 	pushl %ecx            
# 	pushl %ebx            
# 	pushl %esp            # Pass the stack pointer as an argument to do_syscall_in_C
# 	# call syscall_handler
# 	addl $4, %esp         # clean up the stack (remove %esp argument)
# 	popl %ebx             
# 	popl %ecx             
# 	popl %edx             
# 	popl %esi             
# 	popl %edi             
# 	popl %ebp             
# 	popl %ds              
# 	popl %es              
# 	popl %fs              
# 	popl %gs              
# 	addl $4, %esp         # Remove the error code from the stack
# 	iret                 

.globl syscall_handler_asm
.type syscall_handler_asm, @function
syscall_handler_asm:
	push %eax
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
	push %esp
	call syscall_handler
	add $4, %esp

	add $4, %esp
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp
	pop %ds
	pop %es
	pop %fs
	pop %gs
	add $4, %esp


	iret                 
