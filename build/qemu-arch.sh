#!/bin/sh

if [ $1 = "x86_64"  ] ; then
	echo "x86_64"
elif [ $1 = "x86_32" ] ; then
	echo "i386"
fi
