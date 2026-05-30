#!/bin/sh

# echo $QEMU_FLAGS

# qemu-system-i386 -gdb tcp::1234 -S $QEMU_FLAGS &
# sleep 2

# gdb -ex "set \$flags=$QEMU_FLAGS" -x debug.gdb
#

USER_EXECUTABLE="build/fs-root/bin/compiler"
USER_EXECUTABLE2="build/fs-root/bin/wm"

KERNEL_ADDR=objdump -h isodir/boot/os.kernel | grep \ .text\  | awk '{print $4}'
USERSPACE_ADDR=objdump -h $USER_EXECUTABLE | grep \ .text\  | awk '{print $4}'
USERSPACE_ADDR=objdump -h $USER_EXECUTABLE2 | grep \ .text\  | awk '{print $4}'

echo "$QEMU_COMMAND -S -gdb tcp::1234 $(echo $QEMU_FLAGS | tr -d "\n")"
$QEMU_COMMAND -S -gdb tcp::1234 $(echo $QEMU_FLAGS | tr -d "\n")
