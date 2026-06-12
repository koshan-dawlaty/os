set print pretty
# add-symbol-file build/isodir/boot/os.kernel
file build/isodir/boot/os.kernel
# add-symbol-file userspace/bin/wm
add-symbol-file userspace/bin/compiler

# break kmain
