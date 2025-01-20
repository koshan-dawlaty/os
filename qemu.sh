#!/bin/sh
set -e
cd build
. ./iso.sh


QEMU_FLAGS=" -cdrom os.iso -d int,cpu_reset -D log.txt"

# qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -nographic console=ttyS0

if [[  $# -ge 1  ]] && [[ $1 == "terminal"  ]] ; then
qemu-system-$(./target-triplet-to-arch.sh $HOST)  -nographic  -display curses $QEMU_FLAGS
exit 0
fi


qemu-system-$(./target-triplet-to-arch.sh $HOST) -monitor stdio $QEMU_FLAGS



