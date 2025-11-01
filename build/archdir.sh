#!/bin/sh


if [ $1 = "x86_64"  ] ; then
	echo "x86/x86_64"
elif [ $1 = "x86_32"  ] ; then 
	echo "x86/ia_32"	
fi
