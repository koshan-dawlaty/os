#!/usr/bin/env bash
set -e

if [[  $# -ge 1  ]] && [[ $1 == "num_failed"  ]] ; then
# exec 3>&1 &>/dev/null
echo 
fi




if [[  $# -ge 1  ]] && [[ $1 == "debug"  ]] ; then
	CFLAGS="${CFLAGS} -DTEST_DEBUG"
fi

if [[  $# -ge 1  ]] && [[ $1 == "only"  ]] ; then
	

CFLAGS="${CFLAGS} -DTESTS_ONLY"


fi

args_list=( $@ )
TEST_EXTRA_TESTS=( ${args_list[@]:1} )
TESTS_EXCLUDE=true
TESTS_EXTRA_TESTS_CFLAGS="-DTESTS_EXTRA_TESTS=\""

for item in "${TEST_EXTRA_TESTS[@]}"; do
	echo "Processing: $item"
	TESTS_EXTRA_TESTS_CFLAGS+="\\\"$item\\\", "

done
TESTS_EXTRA_TESTS_CFLAGS+="\""

echo $TESTS_EXTRA_TESTS_CFLAGS


CFLAGS="${CFLAGS} -DTESTS $TESTS_EXTRA_TESTS_CFLAGS "

ISO="build/os.iso"

IMAGE_FILE="build/test.img"

export CFLAGS
export ISO


./build/get-ext2-image.sh "$IMAGE_FILE" build/fs-root 


# qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -nographic console=ttyS0

# qemu-img create -f raw test.img 10M
# dd if=/dev/zero bs=1 bs=512 count=2048 | tr '\0' '\xAB' > test.img
# echo -ne "123456789" | dd conv=notrunc bs=1 count=10 of=test.img
# dd if=/dev/zero bs=1 bs=512 count=2048  > test.img
# echo "\x12" | dd of=test.img bs=512 count=2048
# echo -ne '\x55\xAA' | dd of=test.img bs=1 seek=510 conv=notrunc


SERIAL_PIPE=$(mktemp /tmp/QEMU_SERIAL_PIPE_XXXXXXX)

mkfifo "${SERIAL_PIPE}.in"
mkfifo "${SERIAL_PIPE}.out"



gdb \
	-ex "source pretty-print.py" \
	-ex "set print pretty" \
	-ex "add-symbol-file build/isodir/boot/os.kernel.test $KERNEL_ADDR" \
	-ex "add-symbol-file $USER_EXECUTABLE $USERSPACE_ADDR" \
	-ex "add-symbol-file $USER_EXECUTABLE2 $USERSPACE_ADDR2" \
	-ex "target remote | $QEMU_COMMAND -S -gdb stdio $(echo $QEMU_FLAGS | tr -d "\n")" \
	-ex ""

# redirect stderr to stop message when qemu is killed
# $QEMU_COMMAND -monitor stdio $QEMU_FLAGS -display none -serial pipe:$SERIAL_PIPE  2>/dev/null &  
$QEMU_COMMAND -monitor stdio $QEMU_FLAGS -serial pipe:$SERIAL_PIPE  2>/dev/null &  
QEMU_PID=$!


# tmux split-window -v -b -d -t "$TMUX_PANE" "cat $SERIAL_PIPE.out"


# echo $SERIAL_PIPE.out

FILE_NAME="$SERIAL_PIPE.out"
while [[ ! -p "$FILE_NAME" ]]; do
    sleep 0.05
done
cat "$FILE_NAME" 2>/dev/null | python3 build/tap.py -v

kill $QEMU_PID 2>/dev/null

rm $SERIAL_PIPE.out
rm $SERIAL_PIPE.in

# make clean >/dev/null 2>&1

