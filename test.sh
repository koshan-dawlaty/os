#!/bin/sh
set -e
./clean.sh
cd build
CFLAGS="${CFLAGS} -DTESTS"
export CFLAGS
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -monitor stdio  -d int -D log.txt
