#!/bin/sh

set -e
cd build


# CFLAGS="${CFLAGS} -DTESTS"
# ISO="test.iso"

# IMAGE_FILE="test.img"
export CFLAGS
export ISO
. ./iso.sh

rm -f qemu.img
./get-ext2-image.sh "qemu.img" fs-root

# echo $QEMU_FLAGS

# qemu-system-i386 -gdb tcp::1234 -S $QEMU_FLAGS &
# sleep 2

# gdb -ex "set \$flags=$QEMU_FLAGS" -x debug.gdb
#

USER_EXECUTABLE="fs-root/wm"
USER_EXECUTABLE2="fs-root/elf_test2"

KERNEL_ADDR=objdump -h isodir/boot/os.kernel | grep \ .text\  | awk '{print $4}'
USERSPACE_ADDR=objdump -h $USER_EXECUTABLE | grep \ .text\  | awk '{print $4}'
USERSPACE_ADDR=objdump -h $USER_EXECUTABLE2 | grep \ .text\  | awk '{print $4}'

gdb \
	-ex "source pretty-print.py" \
	-ex "set print pretty" \
	-ex " add-symbol-file isodir/boot/os.kernel $KERNEL_ADDR" \
	-ex "add-symbol-file $USER_EXECUTABLE $USERSPACE_ADDR" \
	-ex "add-symbol-file $USER_EXECUTABLE2 $USERSPACE_ADDR2" \
	-ex "target remote | $QEMU_COMMAND -S -gdb stdio $(echo $QEMU_FLAGS | tr -d "\n")" \
	-ex ""
