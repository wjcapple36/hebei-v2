
PRJ_VERSION = "1.0.0"
PRJ_NAME = "EpollServer"


# load_lds = y
# CROSS_COMPILE = /opt/EmbedSky/crosstools_3.4.5_softfloat/gcc-3.4.5-glibc-2.3.6/arm-linux/bin/arm-linux-

# default download.elf,download.dis,download.bin
OUTPUT_ELF	= test.elf
OUTPUT_DIS	=
OUTPUT_BIN	= 
OUTPUT_SO 	= 
OUTPUT_A	= 

OUTPUT_DIR = release




INCLUDE_DIR    = -I./usr/include  \
				-I./include \
				-I./shell \
				-I./osnet \
				-I/usr/local/install/include

LFLAGS    = -lminishell-ex  
# -lpthread -lepollserver 

LFLAGS    += -lreadline -lhistory   -ltermcap
LIB_DIR   = -L/usr/local/install/lib -L./lib-arm920t -L/usr/arm920t/install/lib
CFLAGS    = -g

