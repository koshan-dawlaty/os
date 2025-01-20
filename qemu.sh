#!/bin/sh
set -e
cd build
. ./iso.sh

# qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -nographic console=ttyS0

qemu-img create -f raw test.img 1M
# dd if=/dev/zero of=test.img bs=512 count=2048
# echo -ne '\x55\xAA' | dd of=test.img bs=1 seek=510 conv=notrunc

# qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -monitor stdio  -d int,cpu_reset -D log.txt -hda test.img
qemu-system-$(./target-triplet-to-arch.sh $HOST) \
	-cdrom os.iso \
    -drive file=test.img,format=raw,if=ide,index=0 \
    -monitor stdio \
    -boot d \
    # -device piix3-ide \
