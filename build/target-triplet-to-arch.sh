#!/bin/sh

if [ $1 = "x86_64-elf"  ] || [ $1 = "i686-elf"; then
	echo "x86"

fi


# if echo "$1" | grep -Eq 'i[[:digit:]]86-'; then
# 	echo i386
# else
# 	echo "$1" | grep -Eo '^[[:alnum:]_]*'
# fi

