#!/bin/sh
set -e
cd build
. ./iso.sh

gdb -x debug.gdb
