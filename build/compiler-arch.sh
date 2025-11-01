#!/bin/sh

if [ "$1" = "x86_64" ] ; then
	echo "x86_64-elf"
elif [ "$1" = "x86_32"  ] ; then 
	echo "i686-elf"
fi
