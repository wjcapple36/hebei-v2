#include <minishell_core.h>

#include <stdio.h>
//一级函数
static int do_get_fpga_info(void *ptr, int argc, char **argv);
static int do_cmd2(void *ptr, int argc, char **argv);
static int do_cmd3(void *ptr, int argc, char **argv);
static int do_cmd4(void *ptr, int argc, char **argv);
static int do_quit_system(void *ptr, int argc, char **argv);
static int do_val1(void *ptr, int argc, char **argv);

//二级子函数
static int get_fpga_ch_num(void *ptr, int argc, char **argv);
static int get_fpga_slot(void *ptr, int argc, char **argv);
static int get_fpga_net_flags(void *ptr, int argc, char **argv);

extern struct cmd_prompt boot_logicapp_root[];
extern struct cmd_prompt boot_fpga_info[];
extern struct cmd_prompt boot_cmd2[];
extern struct cmd_prompt boot_cmd3[];
extern struct cmd_prompt boot_cmd4[];
extern struct cmd_prompt boot_cmd5[];
extern struct cmd_prompt boot_cmd6[];



struct cmd_prompt boot_logicapp_root[] = {
	PROMPT_NODE(boot_fpga_info   ,      do_get_fpga_info,
		 (char*)"get"  ,
		 (char*)"fpga info: ch, net_flag, slot and so on ",
		 (int)  NULL),
	PROMPT_NODE(boot_cmd2   ,      do_cmd2,
		 (char*)"cmd2"  ,
		 (char*)"",
		 (int)  NULL),
	PROMPT_NODE(boot_cmd3   ,      do_cmd3,
		 (char*)"cmd3"  ,
		 (char*)"",
		 (int)  NULL),
	PROMPT_NODE(boot_cmd4   ,      do_cmd4,
		 (char*)"cmd4"  ,
		 (char*)"",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      do_quit_system,
		 (char*)"quit"  ,
		 (char*)"Exit from current command view",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_fpga_info[] = {
	PROMPT_NODE(NULL    ,      get_fpga_ch_num,
		 (char*)"ch"  ,
		 (char*)"get ch num",
		 (int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_cmd2[] = {
	PROMPT_NODE(NULL    ,      do_val1,
		 (char*)"val2"  ,
		 (char*)"",
		 (int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_cmd3[] = {
	PROMPT_NODE(NULL    ,      do_val1,
		 (char*)"val3"  ,
		 (char*)"",
		 (int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_cmd4[] = {
	PROMPT_NODE(NULL    ,      do_val1,
		 (char*)"val3"  ,
		 (char*)"",
		 (int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_cmd5[] = {
	PROMPT_NODE(NULL    ,      do_val1,
		 (char*)"val3"  ,
		 (char*)"",
		 (int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_cmd6[] = {
	PROMPT_NODE(NULL    ,      do_val1,
		 (char*)"val3"  ,
		 (char*)"",
		 (int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};


#include "../../schedule/common/global.h"
#include "../../schedule/common/hb_app.h"
#include "../../schedule/otdr_ch/hb_spi.h"
static int do_get_fpga_info(void *ptr, int argc, char **argv)
{
	if(argc >= 2 && strcmp(argv[1], "ch") == 0)
		get_fpga_ch_num(ptr, argc, argv);
	else
		printf("%s\n", __FUNCTION__);

	return 0;
}

static int do_cmd2(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_cmd3(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_cmd4(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_quit_system(void *ptr, int argc, char **argv)
{
	sh_editpath("");
	sh_up_prompt_level();
	return 0;
}

static int do_val1(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}
static int get_fpga_ch_num(void *ptr, int argc, char **argv)
{
	int32_t  ret, ch;
	ret = get_ch_num(&spiDev, &ch);
	if(ret == 0)
		printf("%s() %d: ch num %d \n", __FUNCTION__, __LINE__, ch);
	else
		printf("%s() %d: fail ret  %d \n", __FUNCTION__, __LINE__, ret);
		
	return 0;
}



