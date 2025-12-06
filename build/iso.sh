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
	insmod part_msdos
	  insmod vbe          # for BIOS/VESA (on UEFI GRUB will use GOP)
	  set gfxmode=1024x768x32
	  set gfxpayload=keep
	multiboot /boot/os.kernel


	boot
}
EOF
# grub-mkrescue -o os.iso isodir
grub-mkrescue --modules="multiboot normal iso9660 biosdisk" -o "$ISO" isodir 
