#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/os.kernel isodir/boot/os.kernel
cat > isodir/boot/grub/grub.cfg << EOF
set timeout=0

menuentry "os" {
    insmod vbe
    insmod gfxterm

    # Set graphics mode for GRUB menu
    set gfxmode=1024x768x24

    # Pass graphics mode to the kernel
    set gfxpayload=1024x768x24

	set gfxpayload=keep

    # This must come after gfxpayload
    terminal_output gfxterm

    # Boot the Multiboot1 kernel
    multiboot /boot/os.kernel
    boot
}
EOF
# grub-mkrescue -o os.iso isodir
grub-mkrescue --modules="vbe gfxterm multiboot normal iso9660 biosdisk" -o "$ISO" isodir 
