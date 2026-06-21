#!/bin/sh

# echo $QEMU_FLAGS

# qemu-system-i386 -gdb tcp::1234 -S $QEMU_FLAGS &
# sleep 2

# gdb -ex "set \$flags=$QEMU_FLAGS" -x debug.gdb
#

USER_EXECUTABLE="build/fs-root/bin/wm"
USER_EXECUTABLE2="build/fs-root/bin/compiler"

KERNEL_ADDR=objdump -h isodir/boot/os.kernel | grep \ .text\  | awk '{print $4}'
USERSPACE_ADDR=objdump -h $USER_EXECUTABLE | grep \ .text\  | awk '{print $4}'
USERSPACE_ADDR=objdump -h $USER_EXECUTABLE2 | grep \ .text\  | awk '{print $4}'

SERIAL_PIPE=$(mktemp /tmp/QEMU_SERIAL_PIPE_XXXXXXX)

mkfifo "${SERIAL_PIPE}.in"
mkfifo "${SERIAL_PIPE}.out"

# -v is for vertial -b is for above current, -d is to keep old focus
tmux split-window -h -d -t "$TMUX_PANE" "cat $SERIAL_PIPE.out"

gdb \
	-ex "source pretty-print.py" \
	-ex "set print pretty" \
	-ex "add-symbol-file build/isodir/boot/os.kernel $KERNEL_ADDR" \
	-ex "add-symbol-file $USER_EXECUTABLE $USERSPACE_ADDR" \
	-ex "add-symbol-file $USER_EXECUTABLE2 $USERSPACE_ADDR2" \
	-ex "target remote | $QEMU_COMMAND -S -gdb stdio -serial pipe:$SERIAL_PIPE $(echo $QEMU_FLAGS | tr -d "\n")" \
	-ex ""

# cat $SERIAL_PIPE.out


rm $SERIAL_PIPE.in
rm $SERIAL_PIPE.out
