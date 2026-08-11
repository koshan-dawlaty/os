
.section .text
.global _start
.type _start, @function
_start:
	# push %edi
	# push %esi
	# call init_libc
	# pop %esi
	# pop %edi
	call main
