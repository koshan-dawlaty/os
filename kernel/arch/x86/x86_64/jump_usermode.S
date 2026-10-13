.section .text
.global jump_usermode_asm
.type   jump_usermode_asm @function
jump_usermode_asm:
	# rdx contains the address of the user stack pointer, as it is the third argument


	# movq $(0x00000000BFFEF000), %rdx
	movq %rdi, %rbx

 	cli
 	mov $((4 * 8) | 3), %ax # ring 3 data with bottom 2 bits set for ring 3
 	mov %ax, %ds
 	mov %ax, %es 
 	mov %ax, %fs
 	mov %ax, %gs # SS is handled by iret

	push %rsi

 	# mov %esp, %eax
 	
 	movq %rdx, %rax
	mov %rdx, %rbp
	# sub $4, %ebp
 	# mov %rcx, %rax
 	pushq $((4 * 8) | 3)
 	pushq %rax # user esp
 	pushf
 	orq $(1 << 9), 0(%rsp)
 	pushq $((3 * 8) | 3)
 	
 	pushq %rbx 

	mov %rsi, %rcx

	mov 0x20(%rcx), %rdi
	mov 0x28(%rcx), %rsi
	mov 0x30(%rcx), %rbp
	mov 0x40(%rcx), %rbx
	mov 0x48(%rcx), %rdx
	mov 0x50(%rcx), %rax
	# mov 0x48(%rcx), %rcx

 	iretq
	

.section .text
.global save_registers
.type save_registers @function
save_registers:

	# fsave 0xA8(%rsp)
	# fwait

	push %rax # 80

	mov 0xA8(%rdi), %rax
	fsave (%rax)
	fwait


	mov %rdi, %rax
	

	# add $4, %esp		
	push %rsp # 78
	add $8, 0(%rsp)
	# sub $4, %esp		


	push %r15 # 70
	push %r14 # 68
	push %r13 # 60
	push %r12 # 58
	push %r11 # 50
	push %r10 # 48
	push %r9 # 40
	push %r8 # 38
	pushfq # 30
	push %rcx # 28
	push %rdx # 20
	push %rbx # 18
	push %rbp # 10
	push %rsi # 8
	push %rdi # 0


	mov 0x00(%rsp), %rdi
	mov %rdi, 0x20(%rax) # edi

	mov 0x08(%rsp), %rdi
	mov %rdi, 0x28(%rax) # esi

	mov 0x10(%rsp), %rdi
	mov %rdi, 0x30(%rax) # ebp

	mov 0x78(%rsp), %rdi
	mov %rdi, 0x38(%rax) # esp

	mov 0x18(%rsp), %rdi
	mov %rdi, 0x40(%rax) # ebx

	mov 0x20(%rsp), %rdi
	mov %rdi, 0x48(%rax) # edx

	mov 0x28(%rsp), %rdi
	mov %rdi, 0x50(%rax) # ecx

	mov 0x80(%rsp), %rdi
	mov %rdi, 0x58(%rax) # eax

	mov 0x30(%rsp), %rdi
	mov %rdi, 0x60(%rax)

	mov 0x38(%rsp), %rdi
	mov %rdi, 0x68(%rax)

	mov 0x40(%rsp), %rdi
	mov %rdi, 0x70(%rax)

	mov 0x48(%rsp), %rdi
	mov %rdi, 0x78(%rax)

	mov 0x50(%rsp), %rdi
	mov %rdi, 0x80(%rax)

	mov 0x58(%rsp), %rdi
	mov %rdi, 0x88(%rax)
	
	mov 0x60(%rsp), %rdi
	mov %rdi, 0x90(%rax)

	mov 0x68(%rsp), %rdi
	mov %rdi, 0x98(%rax)

	mov 0x70(%rsp), %rdi
	mov %rdi, 0xA0(%rax)


	# lea %eip, 0x34(%eax)

	pop %rdi
	add $128, %rsp
	# add $0x40, %rsp


	ret


.section .text
.global load_registers
.type load_registers @function
load_registers:
	cli

	mov %rdi, %rcx
	mov 0xa8(%rdi), %rax
	frstor (%rax) # todo: 32 bit version

	mov 0x60(%rcx), %rax 


	push %rax
	# mov %rax, 0(%esp)
	popfq


	mov 0x20(%rcx), %rdi
	mov 0x28(%rcx), %rsi
	mov 0x30(%rcx), %rbp
	mov 0x38(%rcx), %rsp
	mov 0x40(%rcx), %rbx
	mov 0x48(%rcx), %rdx
	mov 0x58(%rcx), %rax
	mov 0x68(%rcx), %r8
	mov 0x70(%rcx), %r9
	mov 0x78(%rcx), %r10
	mov 0x80(%rcx), %r11
	mov 0x88(%rcx), %r12
	mov 0x90(%rcx), %r13
	mov 0x98(%rcx), %r14
	mov 0xa0(%rcx), %r15
	mov 0x50(%rcx), %rcx

	# pushf
	# or $(1 << 9), 0(%rsp)
	# popf



	ret


# void switch_to(struct registers * prev, struct registers * current)
.section .text
.global switch_to_asm
.type switch_to_asm @function
switch_to_asm:

    mov %rsp, 0(%rdi)
    mov %rbx, 8(%rdi)
    mov %rbp,16(%rdi)
    mov %r12,24(%rdi)
    mov %r13,32(%rdi)
    mov %r14,40(%rdi)
    mov %r15,48(%rdi)

    mov 0(%rsi), %rsp
    mov 8(%rsi), %rbx
    mov 16(%rsi),%rbp
    mov 24(%rsi),%r12
    mov 32(%rsi),%r13
    mov 40(%rsi),%r14
    mov 48(%rsi),%r15

	ret

# void switch_to_arg(struct registers * prev, struct registers * current, ptr_t arg)
.section .text
.global switch_to_asm_arg
.type switch_to_asm_arg @function
switch_to_asm_arg:

    mov %rsp, 0(%rdi)
    mov %rbx, 8(%rdi)
    mov %rbp,16(%rdi)
    mov %r12,24(%rdi)
    mov %r13,32(%rdi)
    mov %r14,40(%rdi)
    mov %r15,48(%rdi)

    mov 0(%rsi), %rsp
    mov 8(%rsi), %rbx
    mov 16(%rsi),%rbp
    mov 24(%rsi),%r12
    mov 32(%rsi),%r13
    mov 40(%rsi),%r14
    mov 48(%rsi),%r15

	# setup rdi for the function that this will return to
	mov %rdx, %rdi 

	ret

