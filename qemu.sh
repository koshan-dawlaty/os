#!/bin/sh
set -e
cd build
. ./iso.sh

# qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -nographic console=ttyS0

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -monitor stdio  -d int,cpu_reset -D log.txt
