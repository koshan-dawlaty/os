.globl syscall_wrapper_asm
.type syscall_wrapper_asm, @function
syscall_wrapper_asm:

	push %ebp
	mov %esp, %ebp
	
	#mov 0x04(%esp), %eax
	#mov 0x08(%esp), %ebx
	#mov 0x10(%esp), %ecx
	#mov 0x18(%esp), %edx
	#mov 0x20(%esp), %esi
	#mov 0x28(%esp), %edi

	mov 4(%esp), %eax    
    mov 8(%esp), %ebx    
    mov 12(%esp), %ecx   
    mov 16(%esp), %edx   
    mov 20(%esp), %esi   
    mov 24(%esp), %edi   
	

	int $(0x80)
	ret
