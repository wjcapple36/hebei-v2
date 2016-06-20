/**
 ******************************************************************************
 * @file	epollserver.h
 * @brief	Menglong Wu\n
	epoll server 服务框架程序
 
linux环境采用网络服务器框架，框架采用epoll位基础
 *
*/

#ifndef _EPOLL_SERVER_H_
#define _EPOLL_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 ******************************************************************************
 linux和Windows头文件
 *****************************************************************************
*/



#include "ossocket.h"
#include <assert.h>
// #include "bipbuffer.h"
#ifndef WIN32
	#include <errno.h>

	#include <linux/list.h>


	#include <sys/types.h>          /* See NOTES */
	#include <sys/socket.h>

	#include <sys/epoll.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
 	#include <linux/socket.h>
#else
//todo
#endif



/**
 ******************************************************************************
 默认宏定义
 *****************************************************************************
*/

#define EP_MAX_BUF 	1024 ///< 接收、发送缓存大小，1500为10M以太网数据段MTU
#define EP_DEF_PORT 6000 ///< 默认监听端口

#define EP_SERVER	1
#define EP_CLIENT	2
#define __in__			///<输入参数
#define __out__			///<输出参数
#define __inout__		///<输入输出参数
#define __opt__			///<可选参数，可以为0





#ifdef _DEBUG
#define ep_Debug printf
#else
#define ep_Debug(format, args...)
 // unuse_printf
#endif

/**
 ******************************************************************************
 数据结构//! A test class. 
 *****************************************************************************
*/
//epoll_event data域指针数据
struct ep_con_t;


struct ep_evdata_t
{
	int fd;		 /*!< Specifies the GPIO pins to be configured.*/
	struct ep_con_t *pen;				/*!< Enum value TVal2. */ 
};



/*
* --ep_t
*    |
*    |--ep_con_t
*    |
*    |--ep_con_t
*/

/**
 * @brief 节点描述符，TMS节点设备，每个连接看成一个节点
 */
struct ep_con_t
{
	int 			sockfd;				///<操作系统对此socket文件描述符 
	int 			type;				///<节点类型
	unsigned int 	this_addr;			///<此struct ep_con_t的对应list_head的物理地址
	OS_MUTEX 		mutex;				///<互斥

	//context
	struct sockaddr_in loc_addr;			///<节点网络地址
	struct sockaddr_in rem_addr;			///<节点网络地址
	void 	*ptr;///<应用层私有数据结构，应用层管理内存分配与释放
#ifndef WIN32
	struct epoll_event event;			///<epoll event事件
#else
	//TODO: IOCP
#endif
	// struct bipbuffer bb;
	// unsigned char 	rbuf[EP_MAX_BUF];	///<接收缓存
	// int 		  	rlen;				///<接收缓存长度
	// unsigned char 	sbuf[EP_MAX_BUF];	///<发送缓存
	// int 		  	slen;				///<发送缓存长度



	//根据连接类型（socket/485/232）而确定读写方式
	// ssize_t 		(*write)(int fd, const void *buf, size_t count);
	// ssize_t 		(*read)(int fd, void *buf, size_t count);

	struct list_head  list;				///<节点链表
};

struct ep_t;
int ep_Interface(struct ep_t *tep,int version);




/**
 * @brief epoll server对象实例
 */
struct ep_t
{
	int					epfd;		///<epoll 文件描述符
	//struct ep_con_t	listen;
	struct ep_con_t		*listen;
	int 				cs;

	int 				thread_cnt;	///<线程数
	struct list_head 	thread_head;///<线程链表

	OS_MUTEX 			node_mutex;	///<节点链表互斥量
	int 				node_cnt;	///<节点链表节点数（不包括头节点）
	struct list_head 	node_head;	///<存放ep_con_t节点

	OS_MUTEX 			msg_mutex;	///<消息队列互斥量
	int 				msg_cnt;	///<消息队列数
	struct list_head 	msg_head;	///<存放消息队列

	void 				*pprivate;	///<私有
	//消息处理函数，前缀pFUI_On
	/**
	 * @brief	pFUI_OnAccept 有新连接
	 * @param	null
	 * @retval	null
	 * @remarks	
	 * @see	
	 */
	
	int (*pFUI_OnAccept)(struct ep_t *pep, struct ep_con_t *pnode);///<存放消息队列
	int (*pFUI_OnConnect)(struct ep_t *pep, struct ep_con_t *pnode);
	// int (*pFUI_OnSend)(int cmd,void *arg);
	int (*pFUI_OnRecv)(struct ep_t *pep, struct ep_con_t *pnode);
	int (*pFUI_OnError)(struct ep_t *pep, struct ep_con_t *pnode);
	int (*pFUI_OnRemoveClose)(struct ep_t *pep, struct ep_con_t *pnode);
	int (*pFUI_OnRelease)(struct ep_t *pep);

	//epoll server处理方法，前缀pFep_
	int (*pFep_CreateThread)(struct ep_t *pep,int num);
	int (*pFep_DestoryThread)(struct ep_t *pep,int num);


	// int (*pFep_ListCon)(int cmd,void *arg);
	// int (*pFep_Aram)(int cmd,void *arg);
	int (*pFUI_Close)(struct ep_t *pep, struct ep_con_t *pnode);
	
	//数据包处理方法，前缀pFTMS_
	// int (*pFTMS_ListCon)(int cmd,void *arg);
};

struct ep_thread_arg_t
{
	struct ep_t *pep;

	OS_THREAD pthread;

};


//void *ep_WorkThread(void *arg);
// int ep_Recv(struct ep_t *pep,struct ep_con_t *pepNode,int len);
// int ep_Send(struct ep_t *pep,struct epoll_event *ev,int len);

int ep_KeepAlive(
		__in__ 	struct ep_t 	*pep, 
		__in__	struct ep_con_t *pconNode,
		__in__	int idle,
		__in__	int interval,
		__in__	int count,
		__in__	int enable);
int ep_Connect(
	__in__ 		struct ep_t 			*pep, 
	__out__ 	struct ep_con_t 		*pnode, 
	__in__ 		const char 				*strAddr, 
	__in__ 		const unsigned short 	port);

void dbg_Server(const char *str);


int 
ep_Listen(
	struct ep_t   *pep,
	unsigned short port);

int ep_CreateThread(
	__in__ struct ep_t 	*pep, 
	__in__ int 	         num);

int ep_CloseThread(
		__in__ struct ep_t 	*pep);

int 
ep_Interface(
	struct ep_t *pep,
	int version);

int 
ep_Release(struct ep_t *pep);

// struct ep_con_t *
// ep_AddClient(struct ep_t *pep,struct ep_con_t *pepNode);

// int ep_AddClient(
// 	__in__ 	struct ep_t *pep,
// 	__out	struct ep_con_t *pepNode);

// int ep_DelConNode(
// 	struct ep_t *pep,
// 	struct ep_con_t *pepNode);

int ep_RunServer(struct ep_t *pep);
int ep_StopServer(struct ep_t *pep);
int ep_Close(
		__in__ 	struct ep_t 	*pep, 
		__out__ struct ep_con_t *panode, 
		__out__ int 		     fd);

typedef int (*ep_callback)(struct ep_con_t *ppconNode, void *ptr);
int ep_Ergodic(
	__inout__ struct ep_t *pep,
	__in__    ep_callback cb,
	__in__    void *ptr);




#ifdef __cplusplus
}
#endif


#endif