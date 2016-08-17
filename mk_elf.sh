#!/bin/sh
echo $@
make DP=pi4 ARCH=armv7 CFLAGS+=-g $@
ls -l  release-armv7/

