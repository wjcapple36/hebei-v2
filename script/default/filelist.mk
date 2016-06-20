#################################################################
# list all souce file while be compiled
# file select or not please edit script/config.mk

# SRCS-y += shell/minishell_core.c 

SRCS-y += src/main.c \
	src/ep_app.c \
	src/tms_app.c 
	
# osnet/bipbuffer.c 
# osnet/epollserver.c 
# osnet/ossocket.c 
		
		# cmd_server.c
		
		# src/tmsxxdb.c 
		
		
		

SRCS-y += shell/cmd/cmd_root.c \
	shell/cmd/cmd_server.c \
	shell/cmd/cmd_fpga.c
# \
		# shell/cmd/cmd_tr485.c 
		

SRCS-y += protocol/glink.c \
		protocol/tmsxx.c 
		# \
		# shell/cmd/cmd_tmsxx.c 
		 # \
		# protocol/md5.cpp
# SRCS-y +=protocol/tr485.c \
# 		protocol/tr485_hw.c 

# 最小系统代码
# SRCS-y +=  \
# 		osnet/bipbuffer.c \
# 		osnet/TMSepollserver.c \
# 		osnet/ossocket.c \
		# src/ep_app.c
