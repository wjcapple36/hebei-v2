
PRJ_VERSION = "1.0.0"
PRJ_NAME = "ramlog"

ARCH=x86

ifeq ("$(ARCH)", "x86")
CROSS_COMPILE = 
endif
ifeq ("$(ARCH)", "armv7")
CROSS_COMPILE	=/opt/iTop-4412/4.3.2/bin/arm-linux-
endif
# default download.elf,download.dis,download.bin
OUTPUT_ELF	= ramlog.elf
OUTPUT_DIS	=
OUTPUT_BIN	= 
OUTPUT_SO 	= libramlog.so
OUTPUT_A	= libramlog.a


ifeq ("$(pi9_arg)", "elf") 
OUTPUT_DIR = release
else
OUTPUT_DIR = lib
endif


# 


INCLUDE_DIR	+= -I./ -I./include -I./ramlog
LFLAGS		+= 
LIB_DIR 	+= 
CFLAGS      += -std=gnu99  	 -Wall  -O3
#-D_DEBUG
