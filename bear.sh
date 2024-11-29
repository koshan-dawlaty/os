#!/bin/sh
set -e

. ./clean.sh
. ./headers.sh

for PROJECT in $PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" bear -- $MAKE)
done
