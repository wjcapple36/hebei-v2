# # 最小系统代码
SRCS-y += ramlog/ramlog.c
# SRCS-y += ramlog/cache.c

# ifeq ("$(pi9_arg)", "elf") 
SRCS-y += ramlog/testramlog.c
# endif