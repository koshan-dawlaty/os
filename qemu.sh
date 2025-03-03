#!/bin/sh
set -e
cd build

. ./iso.sh


QEMU_FLAGS=" -cdrom os.iso
-machine pc
-d int,cpu_reset
-D log.txt
-boot d 
-drive file=qemu.img,format=raw,if=ide,index=0"

# qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -nographic console=ttyS0

# dd if=/dev/zero bs=1 bs=512 count=2048 | tr '\0' '\xAB' > qemu.img
# dd if=/dev/zero bs=1 bs=512 count=10000  > qemu.img
# echo "\x12" | dd of=test.img bs=512 count=2048
# echo -ne '\x55\xAA' | dd of=test.img bs=1 seek=510 conv=notrunc

    # -device piix3-ide \
if [[  $# -ge 1  ]] && [[ $1 == "terminal"  ]] ; then
qemu-system-$(./target-triplet-to-arch.sh $HOST)  -nographic  -display curses $QEMU_FLAGS
exit 0
fi


qemu-system-$(./target-triplet-to-arch.sh $HOST) -monitor stdio $QEMU_FLAGS 
