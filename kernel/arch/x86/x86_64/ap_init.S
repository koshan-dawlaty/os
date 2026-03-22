
/* TODO: TODO: TODO: these are also a fixed constants in x86_64 boot.S, make these global constants */
.set PML4_ADDR_PHYSICAL, 0x1000
.set PDPT_ID, 0x2000
.set PDT_ID,  0x3000
.set PT_ID,   0x4000
.set KERNEL_PHYSADDR, 0x00100000

.set PAGING_PRESENT, 1
.set PAGING_READABLE, 2

# .section .ap_trampoline
# .align 16
# ap_stack_bottom:
# .skip 16384
# ap_stack_top:

.set PRESENT,  1 << 7
.set NOT_SYS,  1 << 4
.set EXEC,     1 << 3
.set DC,       1 << 2
.set RW,       1 << 1
.set ACCESSED, 1 << 0

# Flags bits
.set GRAN_4K, 1 << 7
.set SZ_32,         1 << 6
.set LONG_MODE,     1 << 5


.extern bspdone
.extern aprunning


.section .text
.global ap_trampoline
.code16
ap_trampoline:
    cli
    cld
    ljmp    $0, $0x8040
    .align 16
_L8010_GDT_table:
    .long 0, 0
    .long 0x0000FFFF, 0x00CF9A00    # flat code
    .long 0x0000FFFF, 0x008F9200    # flat data
    .long 0x00000068, 0x00CF8900    # tss
_L8030_GDT_value:
    .word _L8030_GDT_value - _L8010_GDT_table - 1
    .long 0x8010
    .long 0, 0
    .align 64
_L8040:
    xorw    %ax, %ax
    movw    %ax, %ds


    lgdtl   0x8030
    movl    %cr0, %eax
    orl     $1, %eax
    movl    %eax, %cr0
    ljmp    $8, $0x8060

    .align 32
    .code32
_L8060:
    movw    $16, %ax
    movw    %ax, %ds
    movw    %ax, %ss
    # get our Local APIC ID
    mov     $1, %eax
    cpuid
    shrl    $24, %ebx
    movl    %ebx, %edi
    # shll    $15, %ebx
    # movl    ap_stack_top, %esp
    # subl    %ebx, %esp
    # pushl   %edi


	mov $(PML4_ADDR_PHYSICAL), %eax
	mov %eax, %cr3


	# movl $(PDPT_ID | PAGING_PRESENT | PAGING_READABLE), PML4_ADDR_PHYSICAL + (((KERNEL_PHYSADDR >> 39) & 0x1FF) * 8)
	# movl $0, PML4_ADDR_PHYSICAL + (((KERNEL_PHYSADDR >> 39) & 0x1FF) * 8) + 4
	# movl $(PDT_ID | PAGING_PRESENT | PAGING_READABLE), PDPT_ID + (((KERNEL_PHYSADDR >> 30) & 0x1FF) * 8)
	# movl $0, PDPT_ID + (((KERNEL_PHYSADDR >> 30) & 0x1FF) * 8) + 4
	# movl $(PT_ID | PAGING_PRESENT | PAGING_READABLE), PDT_ID + (((KERNEL_PHYSADDR >> 21) & 0x1FF) * 8)
	# movl $0, PDT_ID + (((KERNEL_PHYSADDR >> 21) & 0x1FF) * 8) + 4

	mov %cr4, %eax
	or $(1 << 5), %eax
	mov %eax, %cr4


	mov $(0xC0000080), %ecx
    rdmsr
    or  $(1 << 8), %eax
    wrmsr

	mov %cr0, %eax
	or $(1 << 0 | 1 << 31), %eax
	mov %eax, %cr0

	jmp GDT_END
#  GDT:
#      .Null: 
#          .quad 0
#      .Code: 
#          .Code.limit_lo: .short 0xffff
#          .Code.base_lo: .short 0
#          .Code.base_mid: .byte 0
#          .Code.access: .byte PRESENT | NOT_SYS | EXEC | RW
#          .Code.flags: .byte GRAN_4K | LONG_MODE | 0xF   # Flags & Limit (high, bits 16-19)
#          .Code.base_hi: .byte 0
#      .Data: 
#          .Data.limit_lo: .short 0xffff
#          .Data.base_lo: .short 0
#          .Data.base_mid: .byte 0
#          .Data.access: .byte PRESENT | NOT_SYS | RW
#          .Data.Flags: .byte GRAN_4K | SZ_32 | 0xF       # Flags & Limit (high, bits 16-19)
#          .Data.base_hi: .byte 0
	GDT:
    .quad 0x0000000000000000   # null
    .quad 0x00209A0000000000   # 64-bit code
    .quad 0x0000920000000000   # data
     .Pointer:
 		# .word 24
 		.word 23
         .quad ((GDT - ap_trampoline) + 0x8000 )
         # .quad GDT
GDT_END:
	# jmp ap_64
	# hlt

	lea ((.Pointer - ap_trampoline) + 0x8000), %ebx
	lea (GDT - ap_trampoline), %eax
	add $0x8000, %eax

	mov %eax, 2(%ebx)   # overwrite base at runtime

	lgdt (%ebx)
	# lgdt gdt_ptr

	# lea (.Pointer - ap_trampoline + 0x8000), %eax

	# mov $ap_stack_top, %esp
	
	# sub $10, %esp
	# sgdt (%esp)
	# hlt


	
	# ljmp $0x08, $ap_64
	ljmp $0x08, $0x9000
# 	ljmp $0x08, $ap_64

	# mov $0x94, %ax
	# mov %ax, 0x9000
	# mov %ax, 0x9001
	# ljmp $0x08, $0x9000
	

	# should not get here
	cli
	hlt




.code64
.section .text
.global ap_64
ap_64:
	mov $ap_64_2, %rax
	jmp %rax

.global stack_top
.section .stack
.align 16
ap_stack_bottom:
.skip 16384
ap_stack_top:

.section .text
.code64
.global ap_64_2
.type ap_64_2, @function
ap_64_2:

	mov $ap_stack_top, %rsp

	# call hi

# spinlock, wait for the BSP to finish
1:  pause
	cmpb    $0, bspdone
	jz      1b
	lock    incb aprunning

	5:
	jmp 5b

	



