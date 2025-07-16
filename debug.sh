#!/bin/sh
set -e
cd build


# CFLAGS="${CFLAGS} -DTESTS"
# ISO="test.iso"

# IMAGE_FILE="test.img"
export CFLAGS
export ISO
. ./iso.sh

# echo $QEMU_FLAGS

# qemu-system-i386 -gdb tcp::1234 -S $QEMU_FLAGS &
# sleep 2

# gdb -ex "set \$flags=$QEMU_FLAGS" -x debug.gdb
#
KERNEL_ADDR=objdump -h isodir/boot/os.kernel | grep \ .text\  | awk '{print $4}'
USERSPACE_ADDR=objdump -h fs-root/elf_test | grep \ .text\  | awk '{print $4}'

gdb -ex " add-symbol-file isodir/boot/os.kernel $KERNEL_ADDR" \
	-ex "add-symbol-file ../userspace/bin/elf_test $USERSPACE_ADDR" \
	-ex "target remote | qemu-system-i386 -S -gdb stdio $(echo $QEMU_FLAGS | tr -d "\n")"
