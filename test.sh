#!/usr/bin/env bash
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
-d int,cpu_reset
-D log.txt
-boot d 
-drive file=${IMAGE_FILE},format=raw,if=ide,index=0"

# qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -nographic console=ttyS0

# qemu-img create -f raw test.img 10M
dd if=/dev/zero bs=1 bs=512 count=2048 | tr '\0' '\xAB' > test.img
echo -ne "123456789" | dd conv=notrunc bs=1 count=10 of=test.img
# dd if=/dev/zero bs=1 bs=512 count=2048  > test.img
# echo "\x12" | dd of=test.img bs=512 count=2048
# echo -ne '\x55\xAA' | dd of=test.img bs=1 seek=510 conv=notrunc

    # -device piix3-ide \
if [[  $# -ge 1  ]] && [[ $1 == "terminal"  ]] ; then
qemu-system-$(./target-triplet-to-arch.sh $HOST)  -nographic  -display curses $QEMU_FLAGS
exit 0
fi

qemu-system-$(./target-triplet-to-arch.sh $HOST) -monitor stdio $QEMU_FLAGS


cd ..
./clean.sh
