#!/usr/bin/env bash

if [[  $# -ge 1  ]] && [[ $1 == "num_failed"  ]] ; then
# exec 3>&1 &>/dev/null
echo 
fi

set -e
./clean.sh
cd build



if [[  $# -ge 1  ]] && [[ $1 == "debug"  ]] ; then
	CFLAGS="${CFLAGS} -DTEST_DEBUG"
fi

CFLAGS="${CFLAGS} -DTESTS"
ISO="test.iso"

IMAGE_FILE="test.img"

export CFLAGS
export ISO

# bad 
. ./iso.sh
./get-ext2-image.sh "$IMAGE_FILE" fs-root 


QEMU_FLAGS=" -cdrom ${ISO}
-machine pc
-d int
-D log.txt
-boot d 
-drive file=${IMAGE_FILE},format=raw,if=ide,index=0"

# qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -nographic console=ttyS0

# qemu-img create -f raw test.img 10M
# dd if=/dev/zero bs=1 bs=512 count=2048 | tr '\0' '\xAB' > test.img
# echo -ne "123456789" | dd conv=notrunc bs=1 count=10 of=test.img
# dd if=/dev/zero bs=1 bs=512 count=2048  > test.img
# echo "\x12" | dd of=test.img bs=512 count=2048
# echo -ne '\x55\xAA' | dd of=test.img bs=1 seek=510 conv=notrunc

    # -device piix3-ide \
if [[  $# -ge 1  ]] && [[ $1 == "terminal"  ]] ; then
qemu-system-$(./target-triplet-to-arch.sh $HOST)  -nographic  -display curses $QEMU_FLAGS
exit 0
fi

if [[  $# -ge 1  ]] && [[ $1 == "num_failed"  ]] ; then
qemu-system-$(./target-triplet-to-arch.sh "$HOST") -serial stdio $QEMU_FLAGS  &

# qemu-system-$(./target-triplet-to-arch.sh $HOST) -serial pty $QEMU_FLAGS &
echo "test"
cat /proc/$!/fd/1  > "test.txt"
exit 1
tail -f /proc/$!/fd/1 | while IFS= read -r -n1 char; do
	echo $char
    if [[ $char == "e" ]]; then
        echo "test"
		exit 1
        break
    fi
done

kill $!

exit 0
fi

SERIAL_PIPE=$(mktemp /tmp/QEMU_SERIAL_PIPE_XXXXXXX)

mkfifo "${SERIAL_PIPE}.in"
mkfifo "${SERIAL_PIPE}.out"



# gdb \
# 	-ex "source pretty-print.py" \
# 	-ex "set print pretty" \
# 	-ex " add-symbol-file isodir/boot/os.kernel $KERNEL_ADDR" \
# 	-ex "add-symbol-file $USER_EXECUTABLE $USERSPACE_ADDR" \
# 	-ex "add-symbol-file $USER_EXECUTABLE2 $USERSPACE_ADDR2" \
# 	-ex "target remote | $QEMU_COMMAND -S -gdb stdio $(echo $QEMU_FLAGS | tr -d "\n")" \
# 	-ex ""
$QEMU_COMMAND -monitor stdio $QEMU_FLAGS -serial pipe:$SERIAL_PIPE &
QEMU_PID=$!

echo $SERIAL_PIPE.out


FILE_NAME="$SERIAL_PIPE.out"


TEST_OUTPUT=""

while IFS= read -r -N1 char; do

	TEST_OUTPUT="${TEST_OUTPUT}$char"

  if [[ $char == $'\x04' ]]; then
		break
  fi
done < $FILE_NAME

# TEST_OUTPUT="${TEST_OUTPUT%?}"
kill $QEMU_PID

echo "\n"
TEST_STRING=$(printf "%s" "$TEST_OUTPUT")


printf "%s" "$TEST_STRING" | tappy -v 


rm $SERIAL_PIPE.out
rm $SERIAL_PIPE.in

cd ..
./clean.sh
