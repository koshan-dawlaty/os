#!/bin/sh
set -e
cd build
. ./iso.sh


QEMU_FLAGS=" -cdrom os.iso
-d int,cpu_reset
-D log.txt
-boot d 
-drive file=test.img,format=raw,if=ide,index=0"

# qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -nographic console=ttyS0

qemu-img create -f raw test.img 1M
# dd if=/dev/zero of=test.img bs=512 count=2048
# echo -ne '\x55\xAA' | dd of=test.img bs=1 seek=510 conv=notrunc

    # -device piix3-ide \
if [[  $# -ge 1  ]] && [[ $1 == "terminal"  ]] ; then
qemu-system-$(./target-triplet-to-arch.sh $HOST)  -nographic  -display curses $QEMU_FLAGS
exit 0
fi

qemu-system-$(./target-triplet-to-arch.sh $HOST) -monitor stdio $QEMU_FLAGS
