#include <minishell_core.h>

#include <stdio.h>

static int do_fpga(void *ptr, int argc, char **argv);
static int do_net(void *ptr, int argc, char **argv);
static int do_quit_system(void *ptr, int argc, char **argv);


struct cmd_prompt boot_root[];
extern struct cmd_prompt boot_fpga_root[];
extern struct cmd_prompt boot_epollserver_root[];


struct cmd_prompt boot_root[] = {
	PROMPT_NODE(boot_fpga_root   ,      do_fpga,
		 (char*)"fpga"  ,
		 (char*)"connect something",
		 (int)  NULL),
	PROMPT_NODE(boot_epollserver_root   ,      do_net,
		 (char*)"net"  ,
		 (char*)"to do ",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      do_quit_system,
		 (char*)"quit"  ,
		 (char*)"Exit from current command view",
		 (int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};



static int do_fpga(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	sh_editpath("fpga");
	sh_down_prompt_level(boot_fpga_root);
	return 0;
}

static int do_net(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	sh_editpath("net");
	sh_down_prompt_level(boot_epollserver_root);
	return 0;
}

static int do_quit_system(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	exit(0);
	return 0;
}

