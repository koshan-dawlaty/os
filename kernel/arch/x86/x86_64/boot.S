.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
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


.global stack_top
.section .bootstrap_stack, "aw", @nobits
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:






# PML4 -> 512^4 * 4096   >> 39 bits
# PDPT -> 512^3 * 4096   >> 30 bits
# PD -> 512 * 512 * 4096 >> 21 bits
# PT -> 512 * 4096       >> 12 bits



# got these addresses from https://wiki.osdev.org/Setting_Up_Long_Mode
.set PML4_ADDR_PHYSICAL, 0x1000
.set PDPT_ID, 0x2000
.set PDT_ID,  0x3000
.set PT_ID,   0x4000

.set PAGING_ADDR_MASK, 0xffffffffff000

.set PAGING_PRESENT, 1
.set PAGING_READABLE, 2

.set KERNEL_PHYSADDR, 0x00100000
.extern KERNEL_VMA
# .set KERNEL_PHYSADDR, 0xffffffff80000000


# .extern KERNEL_PHYSADDR

.section .multiboot.text, "a"
.code32
.global _start
.type _start, @function
_start:

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
	# TODO: check if this is the right order for cmp	
	cmp $512, %edx
	jl 1b

	movl $(PML4_ADDR_PHYSICAL | PAGING_PRESENT | PAGING_READABLE), PML4_ADDR_PHYSICAL + (512 * 8) + 4 

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

    


putchar 'd', 100
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


.set PDPT_ADDR 0x5000
.set PDT_ADDR 0x6000
.set PT_ADDR 0x7000



.section .text
.code64
.global entry_64
.type entry_64, @function
entry_64:
	
	movq $(PDPT_ADDR | PAGING_PRESENT | PAGING_READABLE), PML4_ADDR_PHYSICAL

	

	# putchar 'a', 100	

	# mov $stack_top, %rsp
	
	# call main
	
	hlt
