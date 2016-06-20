#include <minishell_core.h>

#include <stdio.h>

static int do_alarm(void *ptr, int argc, char **argv);
static int do_test(void *ptr, int argc, char **argv);
static int do_request(void *ptr, int argc, char **argv);
static int do_fpga_get(void *ptr, int argc, char **argv);
static int do_quit_system(void *ptr, int argc, char **argv);
static int do_disappear(void *ptr, int argc, char **argv);
static int do_appear(void *ptr, int argc, char **argv);
static int do_channal(void *ptr, int argc, char **argv);
static int do_slot(void *ptr, int argc, char **argv);
static int do_net(void *ptr, int argc, char **argv);



struct cmd_prompt boot_fpga_root[];
struct cmd_prompt boot_alarm[];
struct cmd_prompt boot_fpga_get[];



struct cmd_prompt boot_fpga_root[] = {
	PROMPT_NODE(boot_alarm   ,      do_alarm,
		 (char*)"alarm"  ,
		 (char*)"connect something",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      do_test,
		 (char*)"test"  ,
		 (char*)"to do ",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      do_request,
		 (char*)"request"  ,
		 (char*)"close connect",
		 (int)  NULL),
	PROMPT_NODE(boot_fpga_get   ,      do_fpga_get,
		 (char*)"get"  ,
		 (char*)"Config Hostname",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      do_quit_system,
		 (char*)"quit"  ,
		 (char*)"Exit from current command view",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_alarm[] = {
	PROMPT_NODE(NULL    ,      do_disappear,
		 (char*)"disappear"  ,
		 (char*)"todo",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      do_appear,
		 (char*)"appear"  ,
		 (char*)"todo",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_fpga_get[] = {
	PROMPT_NODE(NULL    ,      do_channal,
		 (char*)"ch"  ,
		 (char*)"all channal",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      do_slot,
		 (char*)"slot"  ,
		 (char*)"get slot",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      do_net,
		 (char*)"net"  ,
		 (char*)"get net",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};



static int do_alarm(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_test(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_request(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_fpga_get(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_quit_system(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	sh_editpath("");
	sh_up_prompt_level();

	return 0;
}

static int do_disappear(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_appear(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_channal(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_slot(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_net(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}




