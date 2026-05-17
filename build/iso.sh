#!/bin/sh
set -e
# . ./build.sh
cd build


mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

SYMBOL_MODULE_LINE=""
if [[ "$CFLAGS" == *"-DCONFIG_SYMBOL_TABLE"* ]]; then

cp sysroot/boot/os.kernel$TEST_SUFFIX isodir/boot/os.kernel$TEST_SUFFIX
KERNEL_SYMBOLS=../kernel/bin/kernel_symbols$TEST_SUFFIX
cp $KERNEL_SYMBOLS isodir/boot/kernel_symbols

SYMBOL_MODULE_LINE="module /boot/kernel_symbols"
fi




echo 

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
    multiboot /boot/os.kernel$TEST_SUFFIX
	$SYMBOL_MODULE_LINE
    boot
}
EOF
# grub-mkrescue -o os.iso isodir
grub-mkrescue --modules="vbe gfxterm multiboot normal iso9660 biosdisk" -o "os.iso" isodir 
