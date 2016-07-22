#################################################################
# list all souce file while be compiled
# file select or not please edit script/config.mk


################################
SRCS-y += src/main_wjc.c \
	src/ep_app.c 
SRCS-$(CONFIG_APP_HEBEI2) += src/interface_wjc.c 	

ifeq ("$(CONFIG_USE_MINISHELL_EX)", "y")
	SRCS-$(CONFIG_CMD_BOOT) += shell/cmd/cmd_root.c
	SRCS-$(CONFIG_CMD_SERVER) += shell/cmd/cmd_server.c 
	SRCS-$(CONFIG_CMD_FPGA) += shell/cmd/cmd_fpga.c
endif
SRCS-y += shell/cmd/cmd_logicapp.c
SRCS-y += protocol/glink.c
SRCS-$(CONFIG_PROC_HEBEI2) += protocol/tmsxx.c 

SRCS-y += protocol/SPICommand.c

SRCS-y += schedule/common/schedule_fun.c \
	schedule/common/program_run_log.c \
	schedule/common/hb_app.c \
	schedule/otdr_ch/hb_spi.c \
	schedule/serri/schedule_tsk.c 

SRCS-y += \
	algorithm/OtdrEdma.c  \
	algorithm/Utility.c \
	algorithm/OtdrTable.c \
	algorithm/Event.c \
	algorithm/Filter.c  \
     	algorithm/OtdrMain.c   \
	algorithm/Otdr.c   \
	algorithm/EventParam.c \
	algorithm/OtdrAlgo.c    \
	algorithm/NetworkSocket.c  \
	algorithm/OtdrTouch.c \
	algorithm/tsk_otdr_wjc.c \

