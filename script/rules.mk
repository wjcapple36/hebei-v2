# include $(OUTPUT)

CROSS_COMPILE	=

NOWTIME=$(shell date "+%Y-%m-%d_%H:%M:%S")
 


#################################################################
# INCLUDE_DIR	- Where will be search *.h file
# LFLAGS		- Linking option
# LIB_DIR		- Where will be search *.so/*.a file
#-Wl,-rpath=./:./lib/

#when app.elf run will select *.so/a from $(PATH) -> ./ -> ./lib/
INCLUDE_DIR	= -I../include -I../osnet -I../shell -I../ -I../src -I./usr/include/readline -I./
LFLAGS		= -lreadline -lpthread -lhistory -lncurses -lsqlite3 -lmd5
LIB_DIR 	= 
CFLAGS     += -DBUILD_DATE=\"$(NOWTIME)\"
ifeq ("$(ARCH)", "x86")
	INCLUDE_DIR	+= 
	LFLAGS		+= -ltermcap  
	# -lefence 
	LIB_DIR 	+= -L/usr/local/install/lib
	CFLAGS		+= -DTARGET_X86
endif

ifeq ("$(ARCH)", "armv7")
	INCLUDE_DIR	+= -I/usr/4412/install/include
	LFLAGS		+= 
	LIB_DIR 	+= -L/usr/4412/install/lib
	CFLAGS		+= -DTARGET_ARMV7
endif


ifeq ("$(ARCH)", "arm920t")
	INCLUDE_DIR	+= -I/usr/arm920t/install/include
	LFLAGS		+= 
	LIB_DIR 	+= -L/usr/arm920t/install/lib
	CFLAGS		+= -DTARGET_ARM920T
endif


ifeq ("$(ARCH)", "i586")
	INCLUDE_DIR	+= -I/usr/win32/install/include
	LFLAGS		+= 
	LIB_DIR 	+= -L/usr/win32/install/lib
	CFLAGS		+= 
endif
#################################################################



# ifeq ("$(OUTPUT_DIR)", "")
# 	OUTPUT_DIR=debug
# endif

ifeq ("$(CROSS_COMPILE)", "")
else
endif


GCC_G++ = g++
CC 	= $(CROSS_COMPILE)$(GCC_G++)
LD 	= $(CROSS_COMPILE)ld
OBJDUMP = $(CROSS_COMPILE)objdump
OBJCOPY = $(CROSS_COMPILE)objcopy

#################################################################
# CFLAGS		- Compile general option
# CC_FLAGS		- Compile only for *.c file option
# CS_FLAGS		- Compile only for *.S file option
CFLAGS		+= -g  	 -Wall -static -rdynamic -D_UNUSE_QT_ -fshort-wchar 
ifeq ("$(GCC_G++)","gcc") # 只有gcc编译器才使用该选项，g++无此选项
	CC_FLAGS    = -std=gnu99
else
	CC_FLAGS    = 
endif
CS_FLAGS    = 


CC_FLAGS   += $(CFLAGS)
CS_FLAGS   += $(CFLAGS)





%.o:%.c
	echo ooooo
	@echo compile $^
	@$(CC) -o $@ -c $^ $(CC_FLAGS) $(INCLUDE_DIR) 
%.o:%.cpp
	echo pppppp
	@echo compile $^
	@$(CC) -o $@ -c $^ $(CC_FLAGS) $(INCLUDE_DIR) 
%.o:%.S
	echo oooooffff
	@echo compile $^
	@$(CC) -o $@ -c $^ $(CS_FLAGS) $(INCLUDE_DIR)


x86:$(OUTPUT_DIR) $(OBJS)
	echo $(OBJS)
	echo $(OUTPUT_DIR)
	@echo create $(OUTPUT_DIR)/$(NAME_ELF)
	@$(CC) -o $(OUTPUT_DIR)/$(NAME_ELF) $(OBJS) $(LIB_DIR) $(LFLAGS)

# all:
# clean: 
# 	rm 	$(OBJS) \
# 		$(OUTPUT_DIR)/$(NAME_DIS) \
# 		$(OUTPUT_DIR)/$(NAME_ELF) \
# 		core