add-symbol-file isodir/boot/os.kernel 0xC0000000 
target remote | qemu-system-i386 -S -gdb stdio flags
