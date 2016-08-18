/**
 ******************************************************************************
 * @file	ep_app.c
 * @brief	MengLong Wu\n
 	TMSxxTC 项目epollserver回调处理


*/
#include "autoconfig.h"
#include "epollserver.h"
#include <stdio.h>
#include "bipbuffer.h"
#include "malloc.h"
#include <unistd.h>
#include "./protocol/glink.h"
#include "./protocol/tmsxx.h"

#include "minishell_core.h"
#include "stdlib.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

#include <time.h>
#include "tms_app.h"
#include "ep_app.h"
// #include "tmsxxdb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tms_callback tcb;
struct epapp_callback epapp_cb;
// #define BIPBUFFER_LEN 2048
#define BIPBUFFER_LEN 30000
void ep_unuse(int fd)
{
	// printf("\nep_unuse()\n\tfd = %d\n",fd);
}


int InitSocketStruct(struct tmsxx_app **pss, int fd)
{

	*pss = (struct tmsxx_app *)malloc( sizeof(struct tmsxx_app));
	if (*pss == 0) {
		perror("malloc struct tmsxx_app");
		// exit(1);
		return -1;
	}
	bzero(*pss, sizeof(struct tmsxx_app));

	// ((struct tmsxx_app*)pnode->ptr)->context.fd = pnode->sockfd;
	(**pss).context.fd = fd;
	(**pss).context.ptcb = &tcb;
	// add 2016.6
	pthread_mutex_init (&(**pss).context.mutex, NULL);
	// end add

	bipbuffer_Init(&(*pss)->bb);
	//bipbuffer_AllocateBuffer(&(*pss)->bb,1024*2);
	bipbuffer_AllocateBuffer(&(*pss)->bb, BIPBUFFER_LEN);
	(*pss)->morebyte = 400;

#ifdef CONFIG_TEST_NET_STRONG
	(**pss).context.net_pack_id = 10;
#endif
	return 0;
}

int FreeSocketStruct(struct tmsxx_app **pss)
{
	if(*pss == 0) {
		return 0;
	}
	// add 2016.6
	pthread_mutex_destroy (&(**pss).context.mutex);
	// end add
	bipbuffer_FreeBuffer(&(*pss)->bb);
	free(*pss);

	*pss = NULL;
	return 0;
}


#define PINF_FLAG_ACCEPT   0
#define PINF_FLAG_CONNECT  1
#define PINF_FLAG_REMOVE   2
#define PINF_FLAG_CLOSE    4

// void PrintConnectRemoveInf(int fd, int flag)
void PrintConnectRemoveInf(struct ep_con_t *pconNode, int flag)
{
	char strout[1024] = "\0";
	char retout;
	int offset = 0;
	int empty = 1024;

	// struct sockaddr_in locateAddr,remoteAddr;
	// socklen_t 		 len;
	time_t now;
	struct tm *timenow;
	// int ret;

	// int fd;
	// fd = pconNode->sockfd;

	// len = sizeof(struct sockaddr_in);
	// getsockname(fd, (struct sockaddr*)&locateAddr, &len);
	// len = sizeof(struct sockaddr_in);
	// ret = getpeername(fd, (struct sockaddr*)&remoteAddr, &len);
	// printf("ret = %d\n",ret);
	// perror("getpeername err:");


	time(&now);
	timenow = localtime(&now);


	// printf("%-4d%8d%16s:%-8d",
	// printf("%8d%16s:%-8d",
	// 	pconNode->sockfd,
	// 	inet_ntoa(pconNode->loc_addr.sin_addr),
	// 	htons(pconNode->loc_addr.sin_port));
	retout = snprintf(strout + offset, empty - offset, "%8d%16s:%-8d",
	                  pconNode->sockfd,
	                  inet_ntoa(pconNode->loc_addr.sin_addr),
	                  htons(pconNode->loc_addr.sin_port));
	offset += retout;

	if (flag == PINF_FLAG_ACCEPT) {
		// printf("<----");
		retout = snprintf(strout + offset, empty - offset, "<----");
		offset += retout;
	}
	else if (flag == PINF_FLAG_CONNECT) {
		// printf("---->");
		retout = snprintf(strout + offset, empty - offset, "---->");
		offset += retout;
	}
	else if (flag == PINF_FLAG_REMOVE) {
		// printf("---xx");
		retout = snprintf(strout + offset, empty - offset, "---xx");
		offset += retout;
	}
	else {//PINF_FLAG_CLOSE
		// printf("xx---");
		retout = snprintf(strout + offset, empty - offset, "xx---");
		offset += retout;
	}
	// printf("%16s:%-8d",
	// 	inet_ntoa(remoteAddr.sin_addr),
	// 	htons(remoteAddr.sin_port));
	retout = snprintf(strout + offset, empty - offset, "%16s:%-8d",
	                  inet_ntoa(pconNode->rem_addr.sin_addr),
	                  htons(pconNode->rem_addr.sin_port));
	offset += retout;


	retout = snprintf(strout + offset, empty - offset, "%d-%d-%d %d:%d:%d\n",
	                  timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday,
	                  timenow->tm_hour, timenow->tm_min, timenow->tm_sec);
	offset += retout;
	// todo 发送 strout,长度offset+1 或empty

	// fd = tms_GetManageFd();
	printf("%s", strout);
	// tms_Trace(NULL, strout, offset + 1, ID_TRACE0);
}
int epFUI_OnAccept(struct ep_t *pep, struct ep_con_t *pnode)
{
	printf("----OnAccept----\n");
	//	ep_KeepAlive(pep,pnode,3,30,4,1);
	ep_KeepAlive(pep, pnode, 2, 15, 3, 1);
	InitSocketStruct((struct tmsxx_app **)&pnode->ptr, pnode->sockfd);
	// ((struct tmsxx_app*)pnode->ptr)->fd = pnode->sockfd;
	// ((struct tmsxx_app*)pnode->ptr)->context.fd = pnode->sockfd;

	PrintConnectRemoveInf(pnode, PINF_FLAG_ACCEPT);
	// printf("%8d%16s:%-8d",
	// 	pnode->sockfd,
	// 	inet_ntoa(pnode->loc_addr.sin_addr),
	// 	htons(pnode->loc_addr.sin_port));
	// printf("%16s:%-8d\n",
	// 	inet_ntoa(pnode->rem_addr.sin_addr),
	// 	htons(pnode->rem_addr.sin_port));

	epapp_cb.pf_Accept(pnode->sockfd);
	// sleep(3);
	// tms_GetVersion(pnode->sockfd,  0, 0,DEV_OPM);
	// printf("send over\n");
	// sleep(3);

	return 0;

}

int epFUI_OnConnect(struct ep_t *pep, struct ep_con_t *pnode)
{
	printf("----OnConnect----\n");
	// printf("start\n")	;
	InitSocketStruct((struct tmsxx_app **)&pnode->ptr, pnode->sockfd);
	// ((struct tmsxx_app*)pnode->ptr)->fd = pnode->sockfd;
	((struct tmsxx_app *)pnode->ptr)->context.fd = pnode->sockfd;
	// printf("connect struct ok");
	PrintConnectRemoveInf(pnode, PINF_FLAG_CONNECT);

	return 0;


}

void PrintfMemory(uint8_t *buf, uint32_t len);;
int epFUI_OnRecv(struct ep_t *pep, struct ep_con_t *pnode)
{
	// static int rtimes = 0;
	// printf("\r--------!!!!!! OnRecv !!!!! --------%8.8x\r",rtimes++);
	int ret;
	// char rbuf[100];
	struct tmsxx_app *pss;




	int32_t retRecv, retFramelen, recvTotal;
	int32_t reserved;
	char *pdata;

	// printf("epFUI_OnRecv()\n");
	if (pnode->ptr == NULL) {
		printf("wait \n");
		sleep(1);
		return 1;
	}

#ifdef CONFIG_TEST_NET_STRONG
	struct tms_context *ptms_context = &(((struct tmsxx_app *)(pnode->ptr))->context);
	ptms_context->net_pack_id++;

#endif

	pss = (struct tmsxx_app *)pnode->ptr;
	//pss->morebyte = 100;
	if (pss->enable_lbb == 0) {
		pdata = bipbuffer_Reserve(&pss->bb, pss->morebyte, &reserved);
	}
	else {
		// printf("bipbuffer_Reserve lbb  ");
		pdata = bipbuffer_Reserve(&pss->lbb, pss->lbyte, &reserved);
		int size;
		bipbuffer_GetContiguousBlock(&pss->lbb, &size);
		// printf("size %d\n",size);
	}

	// printf("1-1 ");
	// sleep(1);
	if (pdata == NULL) {
		retRecv = 1;//无用，必须大于0
		// printf("2-0 ");
	}
	// 固定环形缓存
	else if (pss->enable_lbb == 0) {
		// printf("want to recv %d\n",reserved);
		retRecv = recv(pnode->sockfd, pdata, reserved, 0);
		// printf("retRecv %d\n", retRecv);
		// printf("recv count = %d\n", retRecv);
		// printf("2-1 %d ", retRecv);
		// printf("this times recv %d\n",retRecv);
		bipbuffer_Commit(&pss->bb, retRecv);
	}
	// 大型环形缓存，只够存储一帧数据，填满缓存前不找合法帧，
	// 填满后无论是否找到合法帧均释放
	else {
		retRecv = recv(pnode->sockfd, pdata, reserved, 0);
		// printf("recv count = %d\n", retRecv);
		// printf("2-2 %d ", retRecv);
		bipbuffer_Commit(&pss->lbb, retRecv);
		pss->lbyte -= retRecv;


		if (pss->enable_lbb == 1 && pss->lbyte > 0) {
			// printf("end 2-3\n");
			return retRecv;
		}
		else {
			// printf("e-4 ");
			struct bipbuffer tbb;
			tbb      = pss->lbb;
			pss->lbb = pss->bb;
			pss->bb  = tbb;
		}
	}
	//bipbuffer_PrintMemory(&pss->bb);

_Again:
	;
	// printf("_Again:");
	pdata = bipbuffer_GetContiguousBlock(&pss->bb, &recvTotal);
	if (bipbuffer_GetUsedSize(&pss->bb) >= 40 && recvTotal < 40) {
		int unuse;
		// printf("a -1 ");
		bipbuffer_GetUnContiguousBlock(&pss->bb, &unuse);
	}
	if (recvTotal >= 40) {
		// printf("a -2 ");
		ret = glink_FindFrame((int8_t *)pdata, &recvTotal, &retFramelen);
	}
	else {
		// printf("a -3 ");
		ret = -6;
		retFramelen = 0;
	}

	// printf("ret %d retRecv %d %recvTotal %d retFramelen %d\n",
	// 	ret,retRecv,recvTotal,retFramelen);
	if (ret == 0) {
		// printf("a -4 ");
		bipbuffer_DecommitBlock(&pss->bb, retFramelen);

		// tms_Analyse(pnode->sockfd, (int8_t*)pdata, retFramelen);
#ifdef CONFIG_PROC_HEBEI2
		tms_Analyse(    &(((struct tmsxx_app *)(pnode->ptr))->context),
		                (int8_t *)pdata,
		                retFramelen);
#endif

		pss->morebyte = 40;
		if (pss->enable_lbb == 0) {
			goto _Again;
		}
		//if (reserved > 0) {

		//}
	}
	else if (ret == -2) {

		// printf("frame err Decommit %d %d\n",retFramelen,reserved);
		bipbuffer_DecommitBlock(&pss->bb, retFramelen);
		// printf("a -5 %d\n", retFramelen);

		pss->morebyte = 40;
		//if (reserved > 0) {
		goto _Again;
		//}
	}
	else if (ret == -3) {
		// #ifdef CONFIG_TEST_NET_STRONG
		// 	struct tms_context *ptms_context = &(((struct tmsxx_app*)(pnode->ptr))->context);
		// 	ptms_context->net_pack_id++;

		// #endif
		// printf("a -6 \n");
		// printf("recvTotal %d retFramelen %d used %d\n",recvTotal,retFramelen,bipbuffer_GetUsedSize(&pss->bb));
		if (recvTotal + retFramelen <= bipbuffer_GetUsedSize(&pss->bb)) {
			int unuse;
			bipbuffer_GetUnContiguousBlock(&pss->bb, &unuse);
			printf("a-k\n");
		}
		pss->morebyte = 40;//retFramelen;
		bipbuffer_GetContiguousBlock(&pss->bb, &reserved);
		// printf("after copy recvTotal %d reserved %d retFramelen %d\n",
		// 	recvTotal,reserved,retFramelen);
		bipbuffer_PrintMemory(&pss->bb);
		if (reserved >=  recvTotal + retFramelen) {
			printf("a -7 \n ");
			goto _Again;
		}

		if (retFramelen + recvTotal > BIPBUFFER_LEN) {
			// 初始化大块临时缓存
			printf("large bipbuffer\n");
			char *pbb_buf, *plbb_buf;
			int pbb_len, plbb_unuselen;

			pss->lbyte = recvTotal + retFramelen;
			bipbuffer_Init(&pss->lbb);
			bipbuffer_AllocateBuffer(&pss->lbb, pss->lbyte);


			pbb_buf = bipbuffer_GetContiguousBlock(&pss->bb, &pbb_len);
			//plbb_buf = bipbuffer_GetContiguousBlock(&pss->lbb, &plbb_unuselen);
			plbb_buf = bipbuffer_Reserve(&pss->lbb, pbb_len, &plbb_unuselen);
			memcpy(plbb_buf, pbb_buf, pbb_len);

			bipbuffer_Commit(&pss->lbb, pbb_len);
			bipbuffer_DecommitBlock(&pss->bb, pbb_len);
			pss->lbyte -= pbb_len;
			pss->enable_lbb = 1;
		}


	}


	if (pss->enable_lbb == 1 && pss->lbyte <= 0) {
		// printf("free....\n");
		struct bipbuffer tbb;

		pss->enable_lbb = 0;
		tbb      = pss->lbb;
		pss->lbb = pss->bb;
		pss->bb  = tbb;

		bipbuffer_FreeBuffer(&pss->lbb);

	}
	//printf("end \n");
	return retRecv;


	// ret = recv(pnode->sockfd, rbuf, sizeof(rbuf), 0);
	// // printf("recv data %s",rbuf);
	// // 返回值必须是recv函数的返回值
	// return ret;
}

int epFUI_OnError(struct ep_t *pep, struct ep_con_t *pnode)
{
	printf("----OnError----\n");
	perror("which Err:");
	// printf("errcode %d %s\n", errno, strerror(errno));
	// printf
	return 0;
}

int epFUI_OnRemoveClose(struct ep_t *pep, struct ep_con_t *pnode)
{
	// tms_DelManage(pnode->sockfd);
	// tms_DelManage(&(((struct tmsxx_app*)(pnode->ptr))->context),
	// pnode->sockfd);
	printf("----OnRemoveClose----\n");
	// printf("close addr %8.8x\n",pnode->ptr);
	FreeSocketStruct((struct tmsxx_app **)&pnode->ptr);

	PrintConnectRemoveInf(pnode, PINF_FLAG_REMOVE);

	// printf("%8d%16s:%-8d",
	// 	pnode->sockfd,
	// 	inet_ntoa(pnode->loc_addr.sin_addr),
	// 	htons(pnode->loc_addr.sin_port));
	// printf("%16s:%-8d\n",
	// 	inet_ntoa(pnode->rem_addr.sin_addr),
	// 	htons(pnode->rem_addr.sin_port));

	// tms_RemoveDev(pnode->sockfd);


	epapp_cb.pf_RemoteClose(pnode->sockfd);
	NotifyCU(pnode->sockfd);
	tms_RemoveAnyMangerContext(pnode->sockfd);
	// tms_DelManage(pnode->sockfd);
	return 0;

}
int epFUI_OnClose(struct ep_t *pep, struct ep_con_t *pnode)
{
	// tms_DelManage(pnode->sockfd);
	// tms_DelManage(&(((struct tmsxx_app*)(pnode->ptr))->context),
	// pnode->sockfd);
	PrintConnectRemoveInf(pnode, PINF_FLAG_CLOSE);
	NotifyCU(pnode->sockfd);
	tms_RemoveAnyMangerContext(pnode->sockfd);
	// tms_RemoveDev(pnode->sockfd);

	return 0;
}
int epFUI_OnRelease(struct ep_t *pep)
{
	return 0;
}

void ep_Callback(struct ep_t *pep)
{
	pep->pFUI_OnAccept      = epFUI_OnAccept;
	pep->pFUI_OnConnect     = epFUI_OnConnect;
	pep->pFUI_OnRecv        = epFUI_OnRecv;
	pep->pFUI_OnError       = epFUI_OnError;
	pep->pFUI_OnRemoveClose = epFUI_OnRemoveClose;
	pep->pFUI_OnRelease     = epFUI_OnRelease;
	pep->pFUI_Close         = epFUI_OnClose;


	epapp_cb.pf_Accept      = ep_unuse;
	epapp_cb.pf_RemoteClose = ep_unuse;
}


void SetEPAppCallback(struct epapp_callback *p)
{
	if (p == NULL) {
		return;
	}
	if (p->pf_Accept) {
		epapp_cb.pf_Accept = p->pf_Accept;
	}
	if (p->pf_RemoteClose) {
		epapp_cb.pf_RemoteClose = p->pf_RemoteClose;
	}
}


int ConnectCU(struct ep_t *pep)
{
	struct ep_con_t client;

	if (0 == ep_Connect(pep, &client, "127.0.0.1", 6500) ) {
	}
	return 0;
}

/*
 *本系统至少有4个线程
 * main主线程
 *      |-----epollserver网络线程
 *      |-----自动连接CU线程，该线程每10S执行一次
 *      |-----minishell控制台线程
 *      |
 *      |......主线开放的其他线程
 */

static pthread_t g_pthreadshell, g_pthreadconnect_cu;
void *ThreadConnectCU(void *arg);
#include <minishell_core.h>
#if defined(CONFIG_USE_MINISHELL_EX) && defined(CONFIG_CMD_BOOT)
extern struct cmd_prompt boot_root[];
#endif
void *ThreadShell(void *arg)
{
#if defined(CONFIG_CMD_BOOT)
	int ret = 1;

	ret = -1;
	sh_whereboot(boot_root);
	

	char hostname[HOSTNAME_PATH];
	struct sh_detach_depth depth;
	char *cmd[12];


	depth.cmd = cmd;
	depth.len = 12;
	depth.seps = " \t";
	depth.hostname = hostname;
	gethostname(hostname, HOSTNAME_PATH - 1);
	while(ret == -1) {
		// ret = sh_enter();

		sh_enter_ex(&depth, NULL);
		if (ret == 0) {
			break;
		}
		sleep(4);
	}



	// 主线程应该调用它，shell线程不能自杀
	// pthread_cancel(g_pthreadshell);
	// pthread_join(g_pthreadshell,0);

	pthread_cancel(g_pthreadconnect_cu);
	pthread_join(g_pthreadconnect_cu, 0);


	exit(0);
#endif
}






// 以字母顺序排序所有命令提示
void ShortAllCommandlist()
{
	// sh_sort_ex(cmd_cr, sizeof(cmd_cr) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_olplist_ref, sizeof(cmd_olplist_ref) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_olplist_oplevel1, sizeof(cmd_olplist_oplevel1) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_olplist_op, sizeof(cmd_olplist_op) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_updatelist, sizeof(cmd_updatelist) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_displist, sizeof(cmd_displist) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_opmreflist, sizeof(cmd_opmreflist) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_olpreflist, sizeof(cmd_olpreflist) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_opmlist, sizeof(cmd_opmlist) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_olplist, sizeof(cmd_olplist) / sizeof(struct cmd_prompt));
	// sh_sort_ex(cmd_boot, sizeof(cmd_boot) / sizeof(struct cmd_prompt));
}
// extern void cmd_InitTmsxxEnv();
int ThreadRunServerAndShell(struct ep_t *pep)
{
	ShortAllCommandlist();
	// cmd_InitTmsxxEnv();
	// tmsdb_Echo(1);       	// 关闭数据库回显
	// tmsdb_CheckDb();		// 创建数据库

	tms_Init();
#ifdef CONFIG_APP_HEBEI2
	tms_Callback(&tcb);
#endif
	// tms_UseEpollServer(pep);
	ep_Interface(pep, 2);           // 初始化ep接口
	ep_Callback(pep);               // 设在epollserver在本工程的回掉函数

#ifdef _MANAGE
	if(ep_Listen(pep, 6000)) {    // 监听TCP 0.0.0.0:6500端口
		return 0;
	}
#else
	if(ep_Listen(pep, 6000)) {    // 监听TCP 0.0.0.0:6500端口
		return 0;
	}
#endif

	ep_RunServer(pep);             // 运行epollserver线程
	tms_connect();

	pthread_create(&g_pthreadshell, NULL, ThreadShell, pep);
// #ifdef AUTOCONNECT_DBG 
	pthread_create(&g_pthreadconnect_cu,NULL,ThreadConnectCU,pep);
// #endif

	return 0;
}




#ifdef __cplusplus
}
#endif
