#!/bin/sh
magick $1 -alpha set -compress None tga:$2
