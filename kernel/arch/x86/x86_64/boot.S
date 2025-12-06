.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set VIDEO,    1<<2     # request VESA/VGA mode
.set FLAGS,    ALIGN | MEMINFO | VIDEO # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot


# Declare a multiboot header that marks the program as a kernel.
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM


.macro putchar char, offset
	movb $(\char), 0xb8000  + \offset
	movb $(7), 0xb8001 + \offset
.endm




# PML4 -> 512^4 * 4096   >> 39 bits
# PDPT -> 512^3 * 4096   >> 30 bits
# PD -> 512 * 512 * 4096 >> 21 bits
# PT -> 512 * 4096       >> 12 bits



# got these addresses from https://wiki.osdev.org/Setting_Up_Long_Mode
.set PML4_ADDR_PHYSICAL, 0x1000
.set PDPT_ID, 0x2000
.set PDT_ID,  0x3000
.set PT_ID,   0x4000

.set PAGING_PHYSICAL_MAP_ADDR, 0x5000
.set PAGING_PHYSICAL_MAP_VADDR, 0xffffc00000000000

.set PAGING_ADDR_MASK, 0xffffffffff000

.set PAGING_PRESENT, 1
.set PAGING_READABLE, 2

.set KERNEL_PHYSADDR, 0x00100000
.extern KERNEL_VMA
 .set KERNEL_VADDR, 0xffffffff80100000

# .extern KERNEL_PHYSADDR

# the kernel assumes these are next to eachother
.align 4096
.globl multiboot_struct_physical 
multiboot_struct_physical:
    .space 4 
.globl multiboot_struct_physical 
multiboot_magic_physical:
    .space 4 



.section .multiboot.text, "a"
.code32
.global _start
.type _start, @function
_start:

	mov %ebx, multiboot_struct_physical	
	mov %eax, multiboot_magic_physical	
	

	# TODO: maybe use a macro for things like this?
	movl $(PDPT_ID | PAGING_PRESENT | PAGING_READABLE), PML4_ADDR_PHYSICAL + (((KERNEL_PHYSADDR >> 39) & 0x1FF) * 8)
	movl $0, PML4_ADDR_PHYSICAL + (((KERNEL_PHYSADDR >> 39) & 0x1FF) * 8) + 4
	movl $(PDT_ID | PAGING_PRESENT | PAGING_READABLE), PDPT_ID + (((KERNEL_PHYSADDR >> 30) & 0x1FF) * 8)
	movl $0, PDPT_ID + (((KERNEL_PHYSADDR >> 30) & 0x1FF) * 8) + 4
	movl $(PT_ID | PAGING_PRESENT | PAGING_READABLE), PDT_ID + (((KERNEL_PHYSADDR >> 21) & 0x1FF) * 8)
	movl $0, PDT_ID + (((KERNEL_PHYSADDR >> 21) & 0x1FF) * 8) + 4


	mov $(1024), %ebx
	mov $(0), %edx

	mov $(PT_ID), %ecx

1:
	mov %edx, %eax	
	shl $12, %eax	
	or $3, %eax
	movl %eax, (%ecx, %edx, 8)
	movl $0, 4(%ecx, %edx, 8)
	add $1, %edx
	
	cmp $512, %edx
	jl 1b

	.set MASK_32, 0xFFFFFFFF

	# TODO: only maps 1GB of physical memory
	# map all physical memory
	movl $(((PAGING_PHYSICAL_MAP_ADDR >> 32) & MASK_32)), PML4_ADDR_PHYSICAL + (((PAGING_PHYSICAL_MAP_VADDR >> 39) & 0x1FF) * 8) + 4
	movl $((PAGING_PHYSICAL_MAP_ADDR & MASK_32)	| PAGING_READABLE | PAGING_PRESENT), PML4_ADDR_PHYSICAL + (((PAGING_PHYSICAL_MAP_VADDR >> 39) & 0x1FF) * 8)

	movl $(0 | (1 << 7) | PAGING_PRESENT | PAGING_READABLE), PAGING_PHYSICAL_MAP_ADDR + 0 


	# https://wiki.osdev.org/Setting_Up_Long_Mode
	mov $(PML4_ADDR_PHYSICAL), %eax
	mov %eax, %cr3

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


	# mov $(PDPT_ID & PAGING_ADDR_MASK | PAGING_PRESENT | PAGING_READABLE), PML4_ADDR_PHYSICAL 
	# mov $(PDT_ID & PAGING_ADDR_MASK | PAGING_PRESENT | PAGING_READABLE), PDPT_ID 
lgdt .Pointer

ljmp $0x08, $entry_64

    
	# mov $'a', 0

	cli	
	hlt





# Access bits
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
GDT:
    .Null: 
        .quad 0
    .Code: 
        .Code.limit_lo: .short 0xffff
        .Code.base_lo: .short 0
        .Code.base_mid: .byte 0
        .Code.access: .byte PRESENT | NOT_SYS | EXEC | RW
        .Code.flags: .byte GRAN_4K | LONG_MODE | 0xF   # Flags & Limit (high, bits 16-19)
        .Code.base_hi: .byte 0
    .Data: 
        .Data.limit_lo: .short 0xffff
        .Data.base_lo: .short 0
        .Data.base_mid: .byte 0
        .Data.access: .byte PRESENT | NOT_SYS | RW
        .Data.Flags: .byte GRAN_4K | SZ_32 | 0xF       # Flags & Limit (high, bits 16-19)
        .Data.base_hi: .byte 0
    .Pointer:
		.word 24
        .quad GDT


.set PDPT_ADDR, 0x5000
.set PDT_ADDR, 0x6000
.set PT_ADDR, 0x7000



.code64
.global entry_64
.type entry_64, @function
entry_64:

	mov $0x10, %ax
	# mov $8, %ax
	mov %ax, %ss
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# mov $'a', %rax


	mov $(PDPT_ADDR | PAGING_PRESENT | PAGING_READABLE), %rax
	movq %rax, ((PML4_ADDR_PHYSICAL + PAGING_PHYSICAL_MAP_VADDR) + (((KERNEL_VADDR >> 39) & 0x1FF) * 8))

	mov $(PDT_ADDR | PAGING_PRESENT | PAGING_READABLE), %rax
	movq %rax, ((PDPT_ADDR + PAGING_PHYSICAL_MAP_VADDR) + (((KERNEL_VADDR >> 30) & 0x1FF) * 8))

	mov $(PT_ADDR | PAGING_PRESENT | PAGING_READABLE), %rax
	movq %rax, ((PDT_ADDR + PAGING_PHYSICAL_MAP_VADDR) + (((KERNEL_VADDR >> 21) & 0x1FF) * 8))
	

	


	# where it should be mapped
	movq $(PAGING_PHYSICAL_MAP_VADDR + PT_ADDR ), %rcx 
	mov $0, %rdx

	1:
		mov %rdx, %rax
		shl $12, %rax
		
		# mov $KERNEL_PHYSADDR, %rdi
		addq %rdi, %rax
		or $(3), %rax

		movq %rax, (%rcx, %rdx, 8)
		

		add $1, %rdx
		cmp $512, %rdx
		jl 1b

	# mov $'a', 0xffffffff80000000
	# mov $'a', 0xffffffff80101180
	# mov $'a', 0xffffffff80201180

	jmp entry_64_2	



	# putchar 'a', 100	

	# mov $stack_top, %rsp

	# call main

	hlt



.global stack_top
.section .stack
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .text
.global entry_64_2
.type entry_64_2, @function
entry_64_2:
	# mov $0x10, %ax
	# mov %ax, %ss
	# mov %ax, %ds
	# mov %ax, %es
	# mov %ax, %fs
	# mov %ax, %gs

	# mov $0, PML4_ADDR_PHYSICAL + (((KERNEL_PHYSADDR >> 39) & 0x1FF) * 8)	
	mov $0, PML4_ADDR_PHYSICAL + (((KERNEL_PHYSADDR >> 39) & 0x1FF) * 8)

	mov %cr3, %rax
	mov %rax, %cr3
	
	mov $stack_top, %rsp
	
	call kernel_init
	call _init
	call main

1:
	hlt
	jmp 1b
