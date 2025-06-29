.globl syscall_wrapper_asm
.type syscall_wrapper_asm, @function
syscall_wrapper_asm:

	# push %ebp
	# mov %esp, %ebp

	#mov $(1), %eax
	#mov $(2), %ebx
	#mov $(3), %ecx
	#mov $(4), %edx
	#mov $(5), %esi
	#mov $(6), %edi

	mov 0x04(%esp), %eax
	mov 0x08(%esp), %ebx
	mov 0x0C(%esp), %ecx
	mov 0x10(%esp), %edx
	mov 0x14(%esp), %esi
	mov 0x18(%esp), %edi
	
	#mov 0x04(%esp), %eax
	#mov 0x08(%esp), %ebx
	#mov 0x0C(%esp), %ecx
	#mov 0x10(%esp), %edx
	#mov 0x14(%esp), %esi
	#mov 0x18(%esp), %edi

	#mov 0x04(%esp), %eax
	#mov 0x08(%esp), %ebx
	#mov 0x10(%esp), %ecx
	#mov 0x18(%esp), %edx
	#mov 0x20(%esp), %esi
	#mov 0x28(%esp), %edi

	#mov 0x02(%esp), %eax
	#mov 0x04(%esp), %ebx
	#mov 0x06(%esp), %ecx
	#mov 0x08(%esp), %edx
	#mov 0x0A(%esp), %esi
	#mov 0x0C(%esp), %edi


	int $(0x80)
	ret
