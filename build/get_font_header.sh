#!/usr/bin/env bash
# TODO: this is not called by the build system

FONTNAME=koi8u_8x16.psfu
OUTPUT_NAME=../kernel/kernel/graphics/psu_font.h

xxd -n "_psfu_font" -i $FONTNAME > $OUTPUT_NAME
