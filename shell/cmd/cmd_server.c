#ifdef __cplusplus
extern "C" {
#endif

#include "minishell_core.h"
#include "epollserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "signal.h"

extern struct ep_t ep;
#include <minishell_core.h>

#include <stdio.h>

static int do_connect(void *ptr, int argc, char **argv);
static int do_server(void *ptr, int argc, char **argv);
static int do_close(void *ptr, int argc, char **argv);
static int do_inf(void *ptr, int argc, char **argv);
static int do_interface(void *ptr, int argc, char **argv);
static int do_update(void *ptr, int argc, char **argv);
static int do_quit_system(void *ptr, int argc, char **argv);
static int do_ip(void *ptr, int argc, char **argv);
static int do_port(void *ptr, int argc, char **argv);
static int do_set_hostname(void *ptr, int argc, char **argv);
static int do_run(void *ptr, int argc, char **argv);
static int do_stop(void *ptr, int argc, char **argv);



extern struct cmd_prompt boot_epollserver_root[];
extern struct cmd_prompt boot_connect[];
extern struct cmd_prompt boot_con_ip[];
extern struct cmd_prompt boot_inf[];
extern struct cmd_prompt boot_server[];
extern struct cmd_prompt boot_close[];

static int sg_sockfdid = 0;

struct cmd_prompt boot_epollserver_root[] = {
	PROMPT_NODE(boot_connect   ,      do_connect,
	(char *)"connect"  ,
	(char *)"connect something",
	(int)  NULL),
	PROMPT_NODE(boot_server   ,      do_server,
	(char *)"server"  ,
	(char *)"to do ",
	(int)  NULL),
	PROMPT_NODE(boot_close   ,      do_close,
	(char *)"close"  ,
	(char *)"close connect",
	(int)  NULL),
	PROMPT_NODE(boot_inf   ,      do_inf,
	(char *)"inf"  ,
	(char *)"Config Hostname",
	(int)  NULL),
	PROMPT_NODE(NULL   ,      do_interface,
	(char *)"interface"  ,
	(char *)"Config Hostname",
	(int)  NULL),
	PROMPT_NODE(NULL   ,      do_update,
	(char *)"update"  ,
	(char *)"update program",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      do_quit_system,
	(char *)"quit"  ,
	(char *)"Exit from current command view",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_connect[] = {
	PROMPT_NODE(boot_con_ip   ,      do_ip,
	(char *)"ip"  ,
	(char *)"x.x.x.x",
	(int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_con_ip[] = {
	PROMPT_NODE(NULL    ,      do_port,
	(char *)"port"  ,
	(char *)"number",
	(int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_inf[] = {
	PROMPT_NODE(NULL    ,      do_set_hostname,
	(char *)"name"  ,
	(char *)"Set hostname",
	(int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};


struct cmd_prompt boot_server[] = {
	PROMPT_NODE(NULL    ,      do_run,
	(char *)"run"  ,
	(char *)"run server",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      do_stop,
	(char *)"stop"  ,
	(char *)"stop server",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_close[] = {
	PROMPT_NODE(NULL    ,      NULL,
	(char *)"[1-10]"  ,
	(char *)"ethernet port interface",
	(int)  CMDP_TYPE_PASS),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};



static int do_connect(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	struct ep_con_t client;
	char *pstrAddr;
	unsigned short port;

	// goto _Next;
	if (argc < 3) {
		printf("Usage:\n");
		printf("\tconnect <IP> <port>\n");
		return 0;
	}
// _Next:
	// printf("connect\n");
	// return 0;
	pstrAddr = argv[1];
	port     = (unsigned short)atoi(argv[2]);

	printf("Request connect %s:%d\n",pstrAddr,port);
	if (0 == ep_Connect(&ep,&client, pstrAddr, port)) {
	// if (0 == ep_Connect(&ep,&client, "127.0.0.1", 6000)) {
		printf("client %s:%d\n", 
					inet_ntoa(client.loc_addr.sin_addr),
					htons(client.loc_addr.sin_port));	
	}
	return 0;
}

static int do_server(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	if(argc == 2) {
		if(strcmp(argv[1], "run") == 0) {
			printf("run..\n");
#ifdef TARGET_ARMV7
			ep_Listen(&ep, 6001);
#else
			ep_Listen(&ep, 6000);
#endif
			ep_RunServer(&ep);
			// runserver();

		}
		else if(strcmp(argv[1], "stop") == 0) {
			printf("stop..\n");
			ep_StopServer(&ep);
			// stopserver();
		}
	}
	return 0;
}

static int do_close(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	if (argc == 2) {
		int fd = atoi(argv[1]);
		ep_Close(&ep,NULL,fd);
	}
	return 0;
}

static int do_inf(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	struct list_head *pos, *n;
	struct ep_con_t *pnode;
	int index = 1;


	printf("%-4s%8s%16s%24s\n", "Index", "FD", "locate", "Remote");
	list_for_each_safe(pos, n, &ep.node_head) {
		pnode = list_entry(pos, struct ep_con_t, list);
		printf("%-4d%8d%16s:%-8d",
		       index++,
		       pnode->sockfd,
		       inet_ntoa(pnode->loc_addr.sin_addr),
		       htons(pnode->loc_addr.sin_port));
		printf("%16s:%-8d\n",
		       inet_ntoa(pnode->rem_addr.sin_addr),
		       htons(pnode->rem_addr.sin_port));


		// printf("\nfd :%d\n", pnode->sockfd);
		// printf("%s:", inet_ntoa(pnode->loc_addr.sin_addr));
		// printf("%d-->", htons(pnode->loc_addr.sin_port));
		// printf("%s:", inet_ntoa(pnode->rem_addr.sin_addr));
		// printf("%d\n", htons(pnode->rem_addr.sin_port));

		//printf("%s:%d-->%s:%d\n",
		// printf("%16s:%-8d",inet_ntoa(pnode->loc_addr.sin_addr),
		// 	htons(pnode->loc_addr.sin_port));
		// printf("%16s:%-8d\n",
		// 	inet_ntoa(pnode->rem_addr.sin_addr),
		// 	htons(pnode->rem_addr.sin_port));


	}
	return 0;
	return 0;
}


static int do_interface(void *ptr, int argc, char **argv)
{
	printf("%s()", __FUNCTION__);
	if (argc < 2) {
		return 0;
	}
	sg_sockfdid = atoi(argv[1]);

	// sg_sockfdid = oneframe[slot].fd;
	// sg_frameid = frame;
	// sg_slotid = slot;
	char path[36];
	struct sockaddr_in remoteAddr;
	socklen_t 		 len;
	len = sizeof(struct sockaddr_in);
	getpeername(sg_sockfdid, (struct sockaddr*)&remoteAddr , &len);

	// snprintf(path, 36, "f%d/s%d", sg_frameid, sg_slotid);
	snprintf(path, 36, "%s:", inet_ntoa(remoteAddr.sin_addr));
	sh_editpath(path);
	return 0;
}

// update filename
static int do_update(void *ptr, int argc, char **argv)
{
	// 读取bin文件
	printf("%s()", __FUNCTION__);
	if (argc < 2) {
		return 0;
	}
	char *ppath = argv[1];
	char *pbuf;					// 文件内容
	int flen;					// 文件长度
	FILE *fp;
	printf("Open file [%s]. ", ppath);
	fp = fopen((char*)ppath, "rb");
	if (NULL == fp) {
		printf("open file %s fail\n",ppath);
		return -1;
	}
	// TODO upp头并偏移
	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	pbuf = (char*)malloc(flen);
	if (NULL == pbuf) {
		return -1;
	}
	fread(pbuf, 1, flen, fp);
	fclose(fp);


	tms_Update(sg_sockfdid, 	NULL, 
			ppath,
			flen, (uint8_t*)pbuf);
	return 0;
	free(pbuf);
}

static int do_quit_system(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	sh_editpath("");
	sh_up_prompt_level();
	return 0;
}

static int do_ip(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_port(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_set_hostname(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_run(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_stop(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}




#ifdef __cplusplus
}
#endif