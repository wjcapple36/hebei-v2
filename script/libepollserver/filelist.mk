# SRCS += shell/minishell_core.c 

# SRCS-y = src/main.c
# SRCS-(CONFIG_MINISHELL_CORE) += src/shell.c
# SRCS-(CONFIG_MINISHELL_CORE) += shell/minishell_core.c
# SRCS-(CONFIG_MINISHELL_EX) += shell/minishell_core_ex.c
# SRCS-y += shell/minishell_core_ex.c

# SRCS-y = src/main.c
# SRCS-y += src/shell.c
SRCS-y += osnet/bipbuffer.c  osnet/epollserver.c  osnet/ossocket.c
