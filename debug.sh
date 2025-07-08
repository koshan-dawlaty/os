#!/bin/sh
set -e
cd build


CFLAGS="${CFLAGS} -DTESTS"
ISO="test.iso"

IMAGE_FILE="test.img"
export CFLAGS
export ISO
. ./iso.sh

# echo $QEMU_FLAGS

# qemu-system-i386 -gdb tcp::1234 -S $QEMU_FLAGS &
# sleep 2

# gdb -ex "set \$flags=$QEMU_FLAGS" -x debug.gdb

gdb -ex " add-symbol-file isodir/boot/os.kernel 0xC0102000" \
	-ex "add-symbol-file ../userspace/bin/elf_test 0x08048090" \
	-ex "target remote | qemu-system-i386 -S -gdb stdio $(echo $QEMU_FLAGS | tr -d "\n")"
