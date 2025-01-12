#!/usr/bin/env bash
set -e
./clean.sh
cd build


if [[  $# -ge 1  ]] && [[ $1 == "debug"  ]] ; then
	CFLAGS="${CFLAGS} -DTEST_DEBUG"
fi

CFLAGS="${CFLAGS} -DTESTS"

export CFLAGS
# bad 
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom os.iso -monitor stdio  -d int -D log.txt
