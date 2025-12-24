#!/usr/bin/env bash


file_name="$1"	
size="32M"
fs_root="$2"

if [[ $# < 2 ]] then 
	echo "not enough arugments"
	exit 1
fi


if [[ $file_name == /* ]]; then

	echo "script get ext2 image cannot run on root dir"
	exit 1
fi

rm -f "$file_name"

mke2fs \
  -L '' \
  -N 0 \
  -d "$fs_root" \
  -m 5 \
  -r 1 \
  -t ext2 \
  "$file_name" \
  "$size" \
;
