#################################################################
# list all souce file while be compiled
# file select or not please edit script/config.mk


SRCS-y += src/main.c \
	src/ep_app.c 
SRCS-$(CONFIG_APP_HEBEI2) += src/tms_app.c 	

ifeq ("$(CONFIG_USE_MINISHELL_EX)", "y")
	SRCS-$(CONFIG_CMD_BOOT) += shell/cmd/cmd_root.c
	SRCS-$(CONFIG_CMD_SERVER) += shell/cmd/cmd_server.c 
	SRCS-$(CONFIG_CMD_FPGA) += shell/cmd/cmd_fpga.c
endif
SRCS-y += shell/cmd/cmd_logicapp.c
SRCS-y += protocol/glink.c
SRCS-$(CONFIG_PROC_HEBEI2) += protocol/tmsxx.c 

SRCS-y += protocol/SPICommand.c
	
# SRCS-y += shell/minishell_core.c
# SRCS-y += shell/minishell_core_ex.c