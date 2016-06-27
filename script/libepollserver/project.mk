
PRJ_VERSION = "1.0.0"
PRJ_NAME = "EpollServer"


# load_lds = y
# CROSS_COMPILE = /opt/EmbedSky/crosstools_3.4.5_softfloat/gcc-3.4.5-glibc-2.3.6/arm-linux/bin/arm-linux-

# default download.elf,download.dis,download.bin
OUTPUT_ELF	=
OUTPUT_DIS	=
OUTPUT_BIN	= 
OUTPUT_SO 	= libepollserver.so
OUTPUT_A	= libepollserver.a

OUTPUT_DIR = lib




INCLUDE_DIR    = -I./usr/include  \
				-I./include \
				-I./shell \
				-I./osnet \
				-I/usr/local/install/include

LFLAGS    = 
LIB_DIR   = -L/usr/local/install/lib
CFLAGS    = -g

