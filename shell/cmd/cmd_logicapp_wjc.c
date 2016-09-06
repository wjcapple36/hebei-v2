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
		 (char*)"cmd: ch net slot alarm0 alarm1 commu_0 commu_1",
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
	int32_t  ret,op;
	ret = 0;
	op = 0;
	if(argc >= 2 && strcmp(argv[1], "ch") == 0)
		ret = get_ch_num(&spiDev, &op);
	else if(argc >= 2 && strcmp(argv[1], "net") == 0)
		ret = get_net_flag(&spiDev, &op);
	else if(argc >= 2 && strcmp(argv[1], "slot") == 0)
		ret = get_dev_slot(&spiDev, &op);
	else if(argc >= 2 && strcmp(argv[1], "alarm1") == 0){
		if(argc == 3)
			op = atoi(argv[2]);
		else
			op = 0;
		printf("%s %d alarm occur ch %d \n", __FUNCTION__, __LINE__, op);
		ret = alarm_find(&spiDev, op);
	}
	else if(argc >= 2 && strcmp(argv[1], "alarm0") == 0){
		if(argc == 3)
			op = atoi(argv[2]);
		else
			op = 0;
		printf("%s %d alarm fade ch %d \n", __FUNCTION__, __LINE__, op);
		ret = alarm_disappear(&spiDev, op);
	}
	else if(argc >= 2 && strcmp(argv[1], "commu_0") == 0)
		ret = set_card_commu_state(&spiDev, 0);
	else if(argc >= 2 && strcmp(argv[1], "commu_1") == 0)
		ret = set_card_commu_state(&spiDev, 1);

	else{
		printf("%s\n", __FUNCTION__);
		return 0;
	}

	if(ret == 0)
		printf("%s() %d: %s result %d \n", __FUNCTION__, __LINE__,argv[1], op);
	else
		printf("%s() %d: %s fail ret  %d \n", __FUNCTION__, __LINE__,argv[1], ret);
		
	return 0;

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



