.globl swap_segments
.type swap_segments, @function
swap_segments:
	mov $0x10, %ax
	# mov $8, %ax
	# mov %ax, %ss
	# mov %ax, %ds
	# mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	ret



.globl pit_test_asm
.type pit_test_asm, @function
pit_test_asm:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    call pit_test123
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax
    iretq
