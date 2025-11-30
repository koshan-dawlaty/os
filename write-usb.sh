#!/bin/sh

#TODO:(maybe) this does not build the os

DEVICE="/dev/sda"

if findmnt -n -o TARGET "$DEVICE" > /dev/null; then
  echo "$DEVICE is mounted!!!!!!! !!!!!!!!!!!!!!!!!!!!!!!!!!!!1!!!!!!!111!!!1!!!!!!!!!!!!!!!!!"
  echo "cant run script it mighgt corrupt data !!1 this writes the os iso image to the usb device"
  exit 1
else
 	sleep 1 && sudo sync && sleep 0.5 && sudo dd if=build/os.iso of=$DEVICE status=progress && sleep 1 && sudo sync && sleep 1 
	echo "you can remove the device now"
fi
