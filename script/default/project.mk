
PRJ_NAME       = "[default-project]"
PRJ_VERSION    = 1
PRJ_PATCHLEVEL = 0
PRJ_SUBLEVEL   = 0


# default download.elf,download.dis,download.bin
OUTPUT_ELF	= hebei2.elf
OUTPUT_DIS	= 
OUTPUT_BIN	= 
OUTPUT_SO 	= 
OUTPUT_A	= 
OUTPUT_DIR 	= 



INCLUDE_DIR	+=  -I./include -I./osnet -I./shell -I./src -I/usr/include/readline -I/usr/local/install/include -I./ramlog
LFLAGS		+= -lreadline -lpthread -lhistory   -ltermcap -lminishell-ex -lepollserver
# -lsqlite3
LIB_DIR 	+= 

CFLAGS  += -DHEBEI2_DBG 
CFLAGS	+= -DPRINT_CMD_NAME_DBG 
CFLAGS	+= -DTRACE_DBG 
CFLAGS	+= -DAUTOCONNECT_DBG 


