#!/bin/sh

if [ $1 = "x86_64"  ] ; then
	echo "-mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse2"
fi
