#!/bin/sh
echo $@
make DP=pi4 ARCH=armv7 $@
ls -l  release-armv7/

