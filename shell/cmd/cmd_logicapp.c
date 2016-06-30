#include <minishell_core.h>

#include <stdio.h>

static int do_cmd1(void *ptr, int argc, char **argv);
static int do_cmd2(void *ptr, int argc, char **argv);
static int do_cmd3(void *ptr, int argc, char **argv);
static int do_cmd4(void *ptr, int argc, char **argv);
static int do_quit_system(void *ptr, int argc, char **argv);
static int do_val1(void *ptr, int argc, char **argv);



extern struct cmd_prompt boot_logicapp_root[];
extern struct cmd_prompt boot_cmd1[];
extern struct cmd_prompt boot_cmd2[];
extern struct cmd_prompt boot_cmd3[];
extern struct cmd_prompt boot_cmd4[];
extern struct cmd_prompt boot_cmd5[];
extern struct cmd_prompt boot_cmd6[];



struct cmd_prompt boot_logicapp_root[] = {
	PROMPT_NODE(boot_cmd1   ,      do_cmd1,
		 (char*)"cmd1"  ,
		 (char*)"",
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
struct cmd_prompt boot_cmd1[] = {
	PROMPT_NODE(NULL    ,      do_val1,
		 (char*)"val1"  ,
		 (char*)"",
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



static int do_cmd1(void *ptr, int argc, char **argv)
{
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
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_val1(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}




