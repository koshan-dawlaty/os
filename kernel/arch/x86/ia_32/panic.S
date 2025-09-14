.section .text
.global panic_asm
.type   panic_asm @function
panic_asm:
	cli
	# TODO: this should disable paging, however it would unmap the stack
	ret
