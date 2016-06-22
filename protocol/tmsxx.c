/**
 ******************************************************************************
 * @file	tmsxx.c
 * @brief	Menglong Wu\n
	TMSxx协议封包、解析

TODO：详细描述

 *


- 2015-4-01, Menglong Wu, DreagonWoo@163.com
 	- 编写封包
*/
#include "glink.h"
#include <arpa/inet.h>
#include "tmsxx.h"
#include "malloc.h"
#include "string.h"
#include <stdio.h>
#include "tmsxx.h"
#include "time.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif



#ifdef USE_INLINE
inline int unuse_echo(const char *__restrict __format, ...)
{
	return 0;
}
#else
int unuse_echo(const char *__restrict __format, ...)
{
	return 0;
}
#endif
#ifdef _MANAGE
int (*fecho)(const char *__restrict __format, ...) = unuse_echo;
#else
int (*fecho)(const char *__restrict __format, ...) = printf;
#endif


void tms_Echo(int en)
{
	if (en) {
		fecho = printf;
	}
	else {
		fecho = unuse_echo;
	}
}

void PrintfMemory(uint8_t *buf, uint32_t len)
{
	for (uint32_t i = 0; i < len; i++) {
		if (i & 0xf) {
			printf(" ");
		}
		else {
			printf("\n");
		}
		printf("%2.2x", (unsigned char) * (buf + i) );
	}
	printf("\n");
}


// static void tms_AddDev(int32_t frame, int32_t slot, struct tms_devbase *pdev);
////////////////////////////////////////////////////////////////////////
// 所有发送接口
// tms_MCUtoDevice \n
// 		tms_MCU_GetDeviceType \n
// 		tms_MCU_RetDeviceType \n
// 		tms_MCU_OSWSwitch \n
// 		tms_MCU_OTDRTest \n
// 		tms_MCU_OLPStartOTDRTest \n
// 		tms_MCU_OLPFinishOTDRTest \n
/**
 * @file	tmsxx.c
 * @section 所有TMSxx数据包封装接口
 - @see tms_MCUtoDevice\n\n
		tms_MCU_GetDeviceType\n
		tms_MCU_RetDeviceType\n\n
		tms_MCU_GetOPMRayPower\n
		tms_MCU_GetOLPRayPower\n\n
		tms_MCU_OSWSwitch\n\n
		tms_MCU_OTDRTest\n
		tms_MCU_OLPStartOTDRTest\n
		tms_MCU_OLPFinishOTDRTest\n\n
		tms_Tick\n
		tms_SetIPAddress\n\n
		tms_GetSerialNumber\n
		tms_RetSerialNumber\n\n
		tms_CfgSMSAuthorization\n
		tms_ClearSMSAuthorization\n\n
		tms_GetDeviceComposition\n
		tms_RetDeviceComposition\n\n
		tms_CfgMCUAnyPort\n
		tms_CfgMCUOSWPort\n
		tms_CfgMCUOLPPort\n\n
		tms_CfgMCUUniteAnyOSW\n
		tms_CfgMCUUniteOPMOSW\n
		tms_CfgMCUUniteOLPOSW\n\n
		tms_CfgMCUAnyPortClear\n
		tms_CfgMCUOPMPortClear\n
		tms_CfgMCUOLPPortClear\n
		tms_CfgMCUUniteOPMOSWClear\n
		tms_CfgMCUUniteOLPOSWClear\n\n
		tms_CfgAnyRefLevel\n
		tms_CfgOPMRefLevel\n
		tms_CfgOLPRefLevel\n\n
		tms_GetOPMOP\n
		tms_GetOLPOP\n
		tms_RetAnyOP\n
		tms_RetOLPOP\n
		tms_RetOPMOP\n\n
		tms_CfgMCUOSWCycle\n
		tms_CfgOSWMode\n
		tms_AlarmOPM\n
		tms_AlarmOPMChange\n\n
		tms_SendSMS\n
		tms_RetSMSState\n\n
		tms_GetVersion\n
		tms_RetVersion\n\n
		tms_Update\n
		tms_Ack\n\n
		tms_Trace MCU输出调试信息\n
		tms_Command 网管下发字符串命令\n
		未完待续
 */

static int32_t tms_AnalyseUnuse(struct tms_context *pcontext, int8_t *pdata, int32_t len);
// static int32_t tms_Transmit2Dev(struct tms_context *pcontext, int8_t *pdata, int32_t len);
// static int32_t tms_Transmit2Manager(struct tms_context *pcontext, int8_t *pdata, int32_t len);

// 每机框12个槽位，最大支持16机框

struct tms_devbase sg_devnet[MAX_FRAME + 1][MAX_SLOT] = {{{0}}};
struct tms_manage sg_manage = {{0}};
int sg_echo_tick = 0;
static int sg_cu_fd = 0;
static int sg_localfd = 0;
static struct ep_t *psg_ep = NULL;

// 数据帧处理方式列表
// 根据sg_analyse_0x1000xxxx、sg_analyse_0x6000xxxx、sg_analyse_0x8000xxxx的dowhat参数
// 选择sg_dowhat处理方式，sg_dowhat的dowhat无意义
struct tms_analyse_array sg_dowhat[8] = {
	{tms_AnalyseUnuse, 1000},
	{tms_Transmit2Dev, 1000},
	{tms_Transmit2Manager, 1000},
	{tms_AnalyseUnuse, 1000},

	{tms_AnalyseUnuse, 1000},
	{tms_AnalyseUnuse, 1000},
	{tms_AnalyseUnuse, 1000},
	{tms_AnalyseUnuse, 1000},
};

// struct pro_list
// {
// 	char name[52];
// 	// int len;
// };



#define ANALYSE_CMDID(pdata) htonl(pdata + sizeof(int32_t) + 24)


/**
 * @brief	自动填充 struct glink_base 帧头，
 * @param	paddr 当paddr 为NULL时用缺省值TMS_DEFAULT_LOCAL_ADDR、TMS_DEFAULT_RMOTE_ADDR填充
 			src、dst，pkid为0
 * @retval	null
 * @remarks
 * @see
 */

void tms_FillGlinkFrame(
    struct glink_base *pbase_hdr,
    struct glink_addr *paddr)
{
	if(paddr == NULL) {
		pbase_hdr->src = TMS_DEFAULT_LOCAL_ADDR;
		pbase_hdr->dst = TMS_DEFAULT_RMOTE_ADDR;
		pbase_hdr->pkid = 0;
	}
	else {
		// printf("tms_FillGlinkFrame()\n");
		// printf("\t src %x dst %x\n", paddr->src, paddr->dst);
		pbase_hdr->src = paddr->src;
		pbase_hdr->dst = paddr->dst;
		pbase_hdr->pkid = paddr->pkid;
	}
}
/**
 * @brief	浮点型转换成网络序，现已经没有什么用途，用内存指针代替
 * @param	null
 * @retval	null
 * @remarks
 * @see
 */

float htonf(float f)
{
	unsigned int i = *(unsigned int *)&f;
	i = htonl(i);
	return *(float *)&i;
}
/**
 * @brief	ID_TICK 0x10000000 心跳
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks	本设备自动回应，应用层不需要调用应答，但是当设备是心跳的发起方，需要
 			应用层调用\n
			由于本系统收到心跳包立即回应，心跳包和回应包完全一样，无法识别发起方
			和应答方，所以两个设备若采用相同的心跳包，则会引起“自激”无限循环。\n
			建议将心跳发起和应答分开
 */
int32_t tms_Tick(int fd, struct glink_addr *paddr)
{
	// static int times = 0;
	struct glink_base  base_hdr;


	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_TICK, 0);
	glink_Send(fd, NULL, &base_hdr, NULL, 0);
	return 0;
}

static int32_t tms_AnalyseTick(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct glink_base *pbase_hdr;
	pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));
	pcontext->tick++;
#ifdef _MANAGE		// 做网管，回应全部心跳
	// tms_Tick(pcontext->fd);
	if (sg_echo_tick == 1) {
		printf("ack any tick fd = %d\n", pcontext->fd);
	}

#else 				// 做MCU
	if (pbase_hdr->src == htonl(GLINK_DEV_ADDR)) {	// 设备发来的心跳 src:0x0a dst:0x0b
		// if (pbase_hdr->src == (GLINK_DEV_ADDR)) {	// 设备发来的心跳 src:0x0a dst:0x0b
		tms_Tick(pcontext->fd, NULL);						// 返回心跳       src:0x0b dst 0x0a
		if (sg_echo_tick == 1) {
			printf("ack dev tick fd = %d\n", pcontext->fd);
		}
	}
	// else if (pbase_hdr->src == htonl(GLINK_CU_ADDR)) {
	else if (pbase_hdr->src == (GLINK_CU_ADDR)) {
		if (sg_echo_tick == 1) {
			printf("ack cu  tick fd = %d\n", pcontext->fd);
		}
	}

#endif
	return 0;
}

#ifdef USE_MD5
#include "protocol/md5.h"
#endif
/**
* @brief	ID_UPDATE 0x10000001 在线升级
* @param[in]	fd 套接字文件描述符
* @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
				修改tmsxx.h设定
* @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
* @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
* @param[in]	type 指定板卡设备类型
* @param[in]	fname 文件的名称
* @param[in]	flen 文件的大小，所含所有字节数
* @param[in]	pdata 文件内容，以二进制方式读取
* @retval	>0 发送成功
* @retval	0 发送失败，该链接失效，立即断开
* @remarks
* @see
*/
int32_t tms_Update(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type,
    uint8_t (*target)[16],
    int32_t flen,
    uint8_t *pdata)
{
	struct tms_dev_update_hdr hdr;
	// uint8_t *pfdata;
	struct tms_dev_md5 md5;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_dev_update_hdr) + flen + sizeof(struct tms_dev_md5);

	printf("Send bin file:\n"
	       "\tf%d/s%d/t%d\n"
	       "\tlen   :%d Byte\n"
	       "\ttarget:%s\n",
	       frame, slot, type, flen, target[0]);
	// Step 2.各字段复制
	hdr.frame = htonl(frame);
	hdr.slot  = htonl(slot);
	hdr.type  = htonl(type);
	memcpy(hdr.target, &target[0][0], 16);
	hdr.flen  = htonl(flen);

	//TODO MD5
	// memcpy(md5.md5, "12345", sizeof("12345"));//debug
	unsigned char md5int[16];
	unsigned char md5str[33];

#ifdef USE_MD5
	CMD5::MD5Int((unsigned char *)pdata, flen , md5int);
	CMD5::MD5Int2Str(md5int, md5str);
	memcpy(md5.md5, md5str, strlen((char *)md5str)); //debug
	printf("\tmd5: %s\n", md5str);
#else
	printf("\tunse MD5\n");
#endif


	// Step 3. 发送
	struct glink_base  base_hdr;


	// PrintfMemory(pdata, flen);

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
		// tms_SelectFdByAddr(paddr->dst);

	}
	glink_Build(&base_hdr, ID_UPDATE, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&hdr,   sizeof(struct tms_dev_update_hdr));
	glink_SendSerial(fd, (uint8_t *)pdata, flen);
	glink_SendSerial(fd, (uint8_t *)&md5,   sizeof(struct tms_dev_md5));
	glink_SendTail(fd);
	return 0;
#if 0
	uint8_t *pmem;
	struct tms_dev_update_hdr *pver_hdr;
	uint8_t *pfdata;
	struct tms_dev_md5 *pmd5;
	int len;

	printf("flen = %d\n", flen);
	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_dev_update_hdr) + flen + sizeof(struct tms_dev_md5);

	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	pver_hdr = (struct tms_dev_update_hdr *)(pmem);
	pfdata   = (uint8_t *)(pmem + sizeof(struct tms_dev_update_hdr));
	pmd5     = (struct tms_dev_md5 *)(pmem + sizeof(struct tms_dev_update_hdr) + flen);

	// Step 2.各字段复制
	pver_hdr->frame = htonl(frame);
	pver_hdr->slot  = htonl(slot);
	pver_hdr->type  = htonl(type);
	memcpy(pver_hdr->target, &target[0][0], 16);
	pver_hdr->flen  = htonl(flen);

	memcpy(pfdata, pdata, flen);
	//TODO MD5
	memcpy(pmd5->md5, "12345", sizeof("12345"));//debug

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_UPDATE, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	return ret;
#endif
}

//0x10000001
static int32_t tms_AnalyseUpdate(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// todo 通知ui
	struct tms_dev_update_hdr *pver_hdr;
	// uint8_t *pfdata;
	struct tms_dev_md5 *pmd5;

	pver_hdr = (struct tms_dev_update_hdr *)(pdata + GLINK_OFFSET_DATA);
	// pfdata   = (uint8_t*)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_dev_update_hdr));
	pmd5     = (struct tms_dev_md5 *)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_dev_update_hdr) + htonl(pver_hdr->flen));



	pver_hdr->frame = htonl(pver_hdr->frame);
	pver_hdr->slot  = htonl(pver_hdr->slot);
	pver_hdr->type  = htonl(pver_hdr->type);
	pver_hdr->flen  = htonl(pver_hdr->flen);

	printf("tms_AnalyseUpdate\n");

	printf("Send bin file:\n"
	       "\tf%d/s%d/t%d\n"
	       "\tlen   :%d Byte\n"
	       "\ttarget:%s\n",
	       pver_hdr->frame, pver_hdr->slot, pver_hdr->type, pver_hdr->flen, pver_hdr->target);
	printf("\tmd5: %s\n\n", pmd5->md5);

	// printf("val:f%d/s%x/t%d\n", pver_hdr->frame, pver_hdr->slot, pver_hdr->type);
	// printf("\tlen %d\n\n", pver_hdr->flen);
	// PrintfMemory(pfdata, pver_hdr->flen);
	// printf("\tmd5 %s\n", pmd5->md5);
	//TODO MD5
	// fun(, , pdata);

	return 0;
}


/**
 * @brief	ID_TRACE 0x10000002 MCU打印调试信息
 * @param[in]	strout 输出信息
 * @param[in]	len strout 字节长度
 * @param[in]	level 显示等级，用于屏蔽不想看的某些等级信息
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 */
#include <unistd.h>
void tms_Trace(struct glink_addr *paddr, char *strout, int32_t len, int level)
{
	struct glink_base  base_hdr;


	tms_FillGlinkFrame(&base_hdr, paddr);



	//todo 找到后台fd
	level = level & 0x3;
	if (0 && level != 0) {
		int fd = tms_GetManageFd();

		// TmsxxV1.2
		if (0 == fd) {
			// fd =
			tms_SelectFdByAddr(&base_hdr.dst);
		}
		// end
		// printf("fd = %d\n", fd);
		if (fd == 0) {
			return ;
		}
		glink_Build(&base_hdr, ID_TRACE0 + level, len);
		glink_Send(fd, NULL, &base_hdr, (uint8_t *)strout, len);
	}
	else if (1 || level == 0) {
		int fd[MANAGE_COUNT];
		// register int count;
		fd[0] = tms_GetManageFd();
		// count = tms_GetTCManageFd(&fd);
		// printf("count %d\n",fd[0]);
		// TmsxxV1.2
		// if (0 == fd) {
		// 	fd[0] = tms_SelectFdByAddr(&base_hdr.dst);
		// }
		// end

		glink_Build(&base_hdr, ID_TRACE0 + level, len);
		glink_Send(fd[0], NULL, &base_hdr, (uint8_t *)strout, len);
	}
	else {
		int fd[MANAGE_COUNT];
		int count;
		// fd[0] = tms_GetManageFd();
		count = tms_GetTCManageFd(&fd);
		printf("count-2 %d\n", count);
		// TmsxxV1.2
		// if (0 == fd) {
		// 	fd[0] = tms_SelectFdByAddr(&base_hdr.dst);
		// }
		// end

		glink_Build(&base_hdr, ID_TRACE0 + 3, len);
		glink_Send(fd[0], NULL, &base_hdr, (uint8_t *)strout, len);

	}

	//return ret;
}
static int tms_AnalyseTrace(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	char *pval;
	pval = (char *)(pdata + GLINK_OFFSET_DATA);
	printf("%s", pval);
	return 0;
}

/**
 * @brief	ID_COMMAMD 0x10000006 网管下发字符串命令
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	strcmd 输出信息
 * @param[in]	len strcmd 字节长度
 * @param[in]	level 显示等级，用于屏蔽不想看的某些等级信息
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 */
int32_t tms_Command(
    int fd,
    struct glink_addr *paddr,
    char *strcmd,
    int32_t len)
{
	struct glink_base  base_hdr;
	//todo 找到后台fd

	// int ret;
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_COMMAMD, len);
	glink_Send(fd, NULL, &base_hdr, (uint8_t *)strcmd, len);
	return 0;
}

static int32_t tms_AnalyseCommand(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// struct glink_base *pbase_hdr;
	// pbase_hdr = (struct glink_base*)(pdata + sizeof(int32_t));
	char *pval = (char *)(pdata + GLINK_OFFSET_DATA);
	dbg_tms("len = %d cmd:[%s]\n", strlen(pval), pval);

	// sh_analyse(pval, strlen(pval));

	// debug
	// tms_AddManage(pcontext->fd, 0);
	sg_localfd = pcontext->fd;
	if (pcontext->ptcb->pf_OnCommand) {
		pcontext->ptcb->pf_OnCommand(pcontext, pval, strlen(pval));
	}
	return 0;
}

/**
 * @brief	MCU向设备发送命令可通用部分接口
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	type 指定板卡设备类型
 * @param[in]	port 指定板卡端口数
 * @param[in]	cmdID 命令ID
 * @param[in]	len 最大长度是sizeof(struct tms_dev_port)，struct tms_dev_port被输入参数
 				frame、slot、type、port填充
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */

int32_t tms_MCUtoDevice(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type,
    int32_t port,
    int32_t cmdID,
    int32_t len)
{
	struct tms_dev_port devPort;

	devPort.frame = htonl(frame);
	devPort.slot  = htonl(slot);
	devPort.type  = htonl(type);
	devPort.port  = htonl(port);


	struct glink_base  base_hdr;
	uint8_t *pmem;
	int ret;

	pmem = (uint8_t *)&devPort;
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdID, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	return ret;
}
/**
 * @brief	通用发送接口，数据长度必须是4的倍数，，不能存在于网络字节序无关的1字节
 * @see
 */

int32_t tms_MCUtoDevice_Ex(
    int     fd,
    struct glink_addr *paddr,
    int32_t cmdID,
    char *pdata,
    int32_t len)
{
	return 0;
}
/**
 * @brief	部分拥有数据分析，数据长度必须是4的倍数，所有数据均是4字节段，不能存在于网络字节序无关的1字节
 * @param	pdata glink帧
 * @param	len glink数据内容字节数（即glink帧datalen - 8）
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
static int32_t tms_AnalyseMCUtoDevice(int8_t *pdata, int32_t len)
{
	register uint32_t *p32, loop;

	p32  = (uint32_t *)(pdata + GLINK_OFFSET_DATA );
	loop = len;
	for (register uint32_t i = 0; i < loop; i++) {
		*p32 = htonl(*p32);
		p32++;
	}
	return 0;
}

#if 0
/**
 * @brief	ID_TEST_NET_STRONG			0x20000000	///< 测试网络强壮性
 */
int32_t tms_TestPacketID(
    int fd,
    struct glink_addr *paddr,
    uint8_t *pdata,
    uint32_t len,
    uint32_t cmdid)
{
#ifdef CONFIG_TEST_NET_STRONG
	// 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdid, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)pdata, len);
	glink_SendTail(fd);

	return 0;
#endif
	return 0;
}

#ifdef CONFIG_TEST_NET_STRONG
void test_save_bin( int8_t *pdata, int32_t len)
{
	struct test_netpacket *pkt;
	pkt = (struct test_netpacket *)(pdata + GLINK_OFFSET_DATA);
	pkt->fname[ sizeof(pkt->fname) - 1] = '\0';
	pkt->save = 0;

	FILE *fp;

	fp = fopen((char *)pkt->fname, "w");
	int ret;
	ret = fwrite(pdata, 1, len, fp);
	printf("ret = %d len %d\n", ret, len);
	fclose(fp);
}
static int32_t tms_AnalyseTestPacketSave(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// printf("%s:%d\n",__FUNCTION__, __LINE__);
	// struct test_netpacket *pkt;
	// pkt = (struct test_netpacket *)(pdata + GLINK_OFFSET_DATA);


	// FILE *fp;

	// fp = fopen((char*)pkt->fname, "w");
	// fwrite(pdata, 1, len, fp);
	// fclose(fp);
	return 0;
}

static int32_t tms_AnalyseTestPacketEcho(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// printf("%s:%d\n",__FUNCTION__, __LINE__);
	// 回显
	static uint8_t pecho[32];
	int ret;
	struct test_netpacket *pkt;
	pkt = (struct test_netpacket *)(pdata + GLINK_OFFSET_DATA);

	// printf("pkt->save %d %d %d\n", pkt->save, len,
	// 	len - GLINK_OFFSET_DATA - GLINK_END_H);
	if (pkt->save == 1) {
		test_save_bin(pdata, len);
	}
	ret = snprintf((char *)pecho, 32, "%8.8d %dB\n",
	               pcontext->net_pack_id,
	               len - GLINK_OFFSET_DATA - GLINK_END_H);
	glink_SendSerial(pcontext->fd, (uint8_t *)pecho, 32);

	// struct glink_base *pbase_hdr;
	// pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));
	// pbase_hdr->cmdid = htonl(ID_TEST_NETPACK_ACK);
	// printf("len %d\n", len);
	// for (int i = 0; i < 10;i++) {
	// 	glink_SendSerial(pcontext->fd, (uint8_t *)pdata, len);
	// 	// sleep(1);
	// }

	return 0;
}

static int32_t tms_AnalyseTestPacketAck(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// printf("%s:%d\n",__FUNCTION__, __LINE__);
	// 回显
	static uint8_t pecho[64];
	int ret;
	struct test_netpacket *pkt;
	pkt = (struct test_netpacket *)(pdata + GLINK_OFFSET_DATA);


	printf("pkt->save %8.8x %d %d\n", pkt->save, len,
	       len - GLINK_OFFSET_DATA - GLINK_END_H);
	if (pkt->save == 1) {
		test_save_bin(pdata, len);
	}

	ret = snprintf((char *)pecho, 64, "%8.8d %8.8d %8.8d %dB\n",
	               pcontext->net_pack_id,
	               pkt->id,
	               pkt->thread_id,
	               len - GLINK_OFFSET_DATA - GLINK_END_H);

	glink_SendSerial(pcontext->fd, (uint8_t *)pecho, ret);


	return 0;
	return 0;
}
#endif

#endif
#if 0
/**
 * @brief	1 ID_RET_DEVTYPE 0x60000000 返回设备类型信息
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_MCUtoDevice
 */
int32_t tms_MCU_GetDeviceType(
    int fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, DEV_OPM, 0, ID_GET_DEVTYPE, 0);
	// return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_GET_DEVTYPE, 0);
}

//0x60000000
static int32_t tms_AnalyseGetDevType(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// sg_dowhat[pcontext->ptr_analyse_arr->dowhat].ptrfun(pcontext, pdata, len);
	struct tms_dev_slot *pval;


	pval = (struct tms_dev_slot *)(pdata + GLINK_OFFSET_DATA);
	pval->frame = htonl(pval->frame);
	pval->slot = htonl(pval->slot);

	printf("tms_AnalyseGetDevType\n");
	printf("\tval:f%d/s%x\n", pval->frame, pval->slot);
	//fun(, , pdev)

	return 0;
}


/**
 * @brief	2 ID_RET_DEVTYPE 0x60000001 返回设备类型信息
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	type 指定板卡设备类型
 * @param[in]	port 指定板卡端口数
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_MCUtoDevice
 */
int32_t tms_MCU_RetDeviceType(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type,
    int32_t port)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OPM, 0, ID_RET_DEVTYPE, sizeof(struct tms_dev_port));
}
int32_t tms_MCU_RetDeviceType_V2(
    int     fd,
    struct glink_addr *paddr,
    struct tms_ret_dev_type *pval)
{
	struct tms_ret_dev_type val;
	val.frame = htonl(pval->frame);
	val.slot  = htonl(pval->slot);
	val.type  = htonl(pval->type);
	val.port  = htonl(pval->port);
	val.reserved0 = htonl(pval->reserved0);
	val.reserved1 = htonl(pval->reserved1);
	val.reserved2 = htonl(pval->reserved2);
	val.reserved3 = htonl(pval->reserved3);

	struct glink_base  base_hdr;
	uint8_t *pmem;
	int ret;

	pmem = (uint8_t *)&val;
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_RET_DEVTYPE, sizeof(struct tms_ret_dev_type));
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_ret_dev_type));
	return ret;
}

//0x60000001
static int32_t tms_AnalyseRetDevType(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// sg_dowhat[pcontext->ptr_analyse_arr->dowhat].ptrfun(pcontext, pdata, len);
	// struct pro_list typelist[] = {
	// 	{"Dev Undefine"},
	// 	{"DEV_PWU"},
	// 	{"DEV_MCU"},
	// 	{"DEV_OPM"},
	// 	{"DEV_OSW"},
	// 	{"DEV_OTDR"},
	// 	{"DEV_OLS"},
	// 	{"DEV_OLP"},
	// 	{"DEV_SMS"},
	// };
	struct tms_ret_dev_type *pval;
	pval = (struct tms_ret_dev_type *)(pdata + GLINK_OFFSET_DATA);
	pval->frame = htonl(pval->frame);
	pval->slot = htonl(pval->slot);
	pval->type = htonl(pval->type);
	pval->port = htonl(pval->port);
	pval->reserved0 = htonl(pval->reserved0);
	pval->reserved1 = htonl(pval->reserved1);
	pval->reserved2 = htonl(pval->reserved2);
	pval->reserved3 = htonl(pval->reserved3);


	printf("tms_AnalyseRetDevType\n");


	struct tms_devbase devbase;
	devbase.fd = pcontext->fd;
	devbase.frame = pval->frame;
	devbase.slot = pval->slot;
	devbase.port = pval->port;
	devbase.type = pval->type;
	devbase.wave = pval->reserved0;
	devbase.reserved0 = pval->reserved1;
	devbase.reserved1 = pval->reserved2;
	devbase.reserved2 = pval->reserved3;

	tms_AddDev(pval->frame, pval->slot, &devbase);

	if (pcontext->ptcb->pf_OnRetDevType) {
		pcontext->ptcb->pf_OnRetDevType(pcontext, pval);
	}


	return 0;
}

// ID_CU_NOTE_NET			0x60000002	工控板与CU通信的网段选择切换通知
static int32_t tms_AnalyseCUNoteNet(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	if (pcontext->ptcb->pf_OnCUNoteNet) {
		pcontext->ptcb->pf_OnCUNoteNet();
	}
	return 0;
}

// ID_CU_NOTE_MANAGE_CONNECT		0x60000003	CU通知工控板网管的连接状态
static int32_t tms_AnalyseCUNoteManageConnect(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	int32_t *pval;

	pval = (int32_t *)(pdata + GLINK_OFFSET_DATA);
	*pval = htonl(*pval);

	if (pcontext->ptcb->pf_OnCUNoteManageConnect) {
		pcontext->ptcb->pf_OnCUNoteManageConnect(*pval);
	}
	return 0;
}

/**
 * @brief	ID_GET_OPM_OLP_RAYPOWER 0x60000004
 			工控板查询某槽位上OPM或OLP模块总的光功率告警
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_MCUtoDevice
 */

// int32_t tms_MCU_GetOPMRayPower(
int32_t tms_MCU_GetOPMAlarm(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OPM, 0, ID_GET_OPM_OLP_RAYPOWER, sizeof(struct tms_dev_type));
}

/**
 * @brief	tms_MCU_GetOLPAlarm 0x60000004
 			工控板查询某槽位上OPM或OLP模块总的光功率告警
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_MCUtoDevice
 */
int32_t tms_MCU_GetOLPAlarm(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OLP, 0, ID_GET_OPM_OLP_RAYPOWER, sizeof(struct tms_dev_type));
}



//ID_GET_OPM_OLP_RAYPOWER 0x60000004
static int32_t tms_AnalyseGetOPMOLPRayPower(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_dev_type *pval;
	pval        = (struct tms_dev_type *)(pdata + GLINK_OFFSET_DATA);
	pval->frame = htonl(pval->frame);
	pval->slot  = htonl(pval->slot);
	pval->type  = htonl(pval->type);

	printf("tms_AnalyseGetOPMOLPRayPower\n");
	printf("val:f%d/s%x/t%x\n", pval->frame, pval->slot, pval->type);
	return 0;
}

/**
 * @brief	ID_CMD_OSW_SWITCH 0x60000005 工控板通知OSW模块切换到某路
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定切换的端口号
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_MCUtoDevice
 */
int32_t tms_MCU_OSWSwitch(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t port)
{
	// 设备类型内部自己完成
	return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OSW, port, ID_CMD_OSW_SWITCH, sizeof(struct tms_dev_port));
}

static int32_t tms_AnalyseMCU_OSWSwitch(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseMCUtoDevice(pdata, sizeof(struct tms_dev_port));
	return 0;
}
/**
 * @brief	ID_CMD_OTDR_TEST 0x60000006
 			OLP模块向工控机请求OTDR测试
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定切换的端口号
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_MCUtoDevice
 */
int32_t tms_MCU_OLPReqOTDRTest(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t port)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OLP, port, ID_CMD_OLP_REQ_OTDR, sizeof(struct tms_dev_port));
}

static int32_t tms_AnalyseMCU_OLPReqOTDRTest(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseMCUtoDevice(pdata, sizeof(struct tms_dev_port));
	printf("tms_AnalyseMCU_OLPReqOTDRTest()\n");
	return 0;
}
/**
 * @brief	ID_CMD_OLP_START_OTDR 0x60000007
 			工控板通知OLP模块OTDR测试开始
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定切换的端口号
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_MCUtoDevice
 */

int32_t tms_MCU_OLPStartOTDRTest(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t port)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OLP, port, ID_CMD_OLP_START_OTDR, sizeof(struct tms_dev_port));
}
// MCU无 tms_MCU_OLPStartOTDRTest 解析函数

/**
 * @brief	ID_CMD_OLP_FINISH_OTDR 0x60000008
 			工控板通知OLP模块OTDR测试完成
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定切换的端口号
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_MCUtoDevice
 */
int32_t tms_MCU_OLPFinishOTDRTest(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t port)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OLP, port, ID_CMD_OLP_FINISH_OTDR, sizeof(struct tms_dev_port));
}
#endif
// MCU无 tms_MCU_OLPFinishOTDRTest 解析函数


////////////////////////////////////////////////////////////////////////
// 网管与MCU之间的通信
// tms_SetIPAddress \n
// 		tms_MCU_GetDeviceType \n
// 		tms_MCU_RetDeviceType \n
// 		tms_MCU_OSWSwitch \n
// 		tms_MCU_OTDRTest \n
// 		tms_MCU_OLPStartOTDRTest \n
// 		tms_MCU_OLPFinishOTDRTest \n

#if 0
/**
 * @brief	1 ID_CHANGE_ADDR 0x80000000 网管修改MCU的IP地址
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	ip 设置的IP
 * @param[in]	mask 设置掩码
 * @param[in]	gw 设置网关
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */

int32_t tms_SetIPAddress(
    int     fd,
    struct glink_addr *paddr,
    uint8_t *ip,
    uint8_t *mask,
    uint8_t *gw)
{
	struct tms_change_addr changeAddr;
	uint8_t *pmem;	// 保护用户数据数据，申请内存，存储网络字节


	pmem = (uint8_t *)&changeAddr;
	memset(&changeAddr, 0, sizeof(struct tms_change_addr));
	strcpy((char *)changeAddr.ip,   (char *)ip);
	strcpy((char *)changeAddr.mask, (char *)mask);
	strcpy((char *)changeAddr.gw,   (char *)gw);
	printf("ip %s mask %s gw %s\n", changeAddr.ip, changeAddr.mask, changeAddr.gw);

	// 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_CHANGE_ADDR, sizeof(struct tms_change_addr));
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_change_addr));
	return ret;
}
static int32_t tms_AnalyseSetIPAddress(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	uint8_t *pip;
	uint8_t *pmask;
	uint8_t *pgw;
	int ret;

	if (len < 16 * 3) {
		return -1;
	}
	pip = (uint8_t *)(pdata + GLINK_OFFSET_DATA);
	pmask = (uint8_t *)(pip + 16);
	pgw = (uint8_t *)(pmask + 16);

	if (pcontext->ptcb->pf_OnSetIPAddress) {
		ret = pcontext->ptcb->pf_OnSetIPAddress(
		          (uint8_t (*)[16])&pip[0],
		          (uint8_t (*)[16])&pmask[0],
		          (uint8_t (*)[16])&pgw[0]);

		struct glink_addr gl;
		struct tms_ack ack;
		struct glink_base *pbase_hdr;

		pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));
		gl.src = htonl(pbase_hdr->src);
		gl.dst = htonl(pbase_hdr->dst);
		gl.pkid = htonl(pbase_hdr->pkid);

		if (ret == 0) {
			ack.errcode = RET_SUCCESS;
		}
		else {
			ack.errcode = RET_UNEXIST;
		}

		ack.cmdid   = ID_CHANGE_ADDR;

		tms_AckEx(pcontext->fd, &gl, &ack);
	}
	return 0;
}


/**
 * @brief	ID_GET_SN 0x80000001 网管查询MCU的设备序列号
 			获取序列号
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
int32_t tms_GetSerialNumber(int fd, struct glink_addr *paddr)
{
	// 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_GET_SN, 0);
	ret = glink_Send(fd, NULL, &base_hdr, NULL, 0);
	return ret;
}

static int32_t tms_AnalyseGetSerialNumber(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	printf("%s()\n", __FUNCTION__);
	if (pcontext->ptcb->pf_OnGetSerialNumber) {
		pcontext->ptcb->pf_OnGetSerialNumber(pcontext);


		struct glink_base *pbase_hdr, glinkbase;
		pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));
		glinkbase.pklen 	= htonl(pbase_hdr->pklen);
		glinkbase.version 	= htonl(pbase_hdr->version);
		glinkbase.src 		= htonl(pbase_hdr->src);
		glinkbase.dst 		= htonl(pbase_hdr->dst);
		glinkbase.type 	= htons(pbase_hdr->type);
		glinkbase.pkid 	= htons(pbase_hdr->pkid);
		glinkbase.reserve 	= htonl(pbase_hdr->reserve);
		glinkbase.cmdid 	= htonl(pbase_hdr->cmdid);
		glinkbase.datalen 	= htonl(pbase_hdr->datalen);
		pcontext->pgb = &glinkbase;

		tms_AddManage(pcontext, pcontext->fd, MT_MANAGE);
	}
	return 0;
}

/**
 * @brief	ID_RET_SN 0x80000002 MCU返回设备序列号
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	psn 序列号值长度128字节
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
int32_t tms_RetSerialNumber(int fd, struct glink_addr *paddr, uint8_t (*psn)[128])
{
	struct glink_base  base_hdr;
	int ret;


	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}

	glink_Build(&base_hdr, ID_RET_SN, 128);
	ret = glink_Send(fd, NULL, &base_hdr, &psn[0][0], 128);
	return ret;
}
static int32_t tms_AnalyseRetSerialNumber(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	uint8_t *pval = (uint8_t *)(pdata + GLINK_OFFSET_DATA);

	if (pcontext->ptcb->pf_OnRetSerialNumber) {
		pcontext->ptcb->pf_OnRetSerialNumber((uint8_t (*)[128])&pval[0]);
	}
	return 0;
}

/**
 * @brief	ID_CFG_SMS 0x80000003 网管发送告警短信发送权限到RTU
 * @param[in]	fd 文件描述符
 * @param[in]	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param[in]	count struct smslist 数组个数
 * @param[in]	smslist 指向一个 struct tms_cfg_sms_val 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_ClearSMSAuthorization
 */
int32_t tms_CfgSMSAuthorization(
    int fd,
    struct glink_addr *paddr,
    int32_t count,
    struct tms_cfg_sms_val *smslist)
{
	uint8_t *pmem;
	struct tms_cfg_sms *pcfg_hdr;
	struct tms_cfg_sms_val         *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_cfg_sms) + count * sizeof(struct tms_cfg_sms_val);

	printf("malloc len %d\n", len);
	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	pcfg_hdr = (struct tms_cfg_sms *)(pmem);
	plist   = (struct tms_cfg_sms_val *)(pmem + sizeof(struct tms_cfg_sms));

	// Step 2.各字段复制
	pcfg_hdr->count = htonl(count);

	// todo :防止循环count次溢出ptlist

	ptlist = smslist;
	for (int i = 0; i < count; i++) {
		plist->time  = htonl(ptlist->time);
		memcpy(plist->phone, ptlist->phone, TLE_LEN);
		plist->type  = htonl(ptlist->type);
		plist->level = htonl(ptlist->level);

		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_CFG_SMS, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	free(pmem);
	return ret;
}

// 0x80000003
static int32_t tms_AnalyseCfgSMSAuthorization(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{

	struct tms_cfg_sms *pcfg_hdr;
	struct tms_cfg_sms_val         *plist, *ptlist;

	// Step 1.分配内存并各指针指向相应内存
	pcfg_hdr = (struct tms_cfg_sms *)(pdata + GLINK_OFFSET_DATA);
	plist   = (struct tms_cfg_sms_val *)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_cfg_sms));

	// Step 2.各字段复制
	pcfg_hdr->count = htonl(pcfg_hdr->count);

	// todo :防止循环count次溢出ptlist
	ptlist = plist;
	for (int i = 0; i < pcfg_hdr->count; i++) {
		ptlist->time  = htonl(ptlist->time);
		ptlist->type  = htonl(ptlist->type);
		ptlist->level = htonl(ptlist->level);
		// printf("%d %d %d %s\n", ptlist->time, ptlist->type, ptlist->level ,ptlist->phone);

		ptlist++;
	}
	ptlist = plist;
	tms_Print_tms_cfg_sms_val(ptlist, pcfg_hdr->count);
	return 0;
}



/**
 * @brief	ID_CFG_SMS_CLEAR 0x80000004 网管发送清除告警短信发送权限到MCU
 * @param[in]	fd 文件描述符
 * @param[in]	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_CfgSMSAuthorization
 */
int32_t tms_ClearSMSAuthorization(
    int     fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_CFG_SMS_CLEAR, 0);
}

static int32_t tms_AnalyseClearSMSAuthorization(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseMCUtoDevice(pdata, 0);
	return 0;
}
/**
 * @brief	ID_GET_COMPOSITION 0x80000005 网管查询板卡组成信息
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定板卡端口数
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_MCUtoDevice
 */
uint32_t tms_GetDeviceComposition(
    int     fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_GET_COMPOSITION, 0);
}

static int32_t tms_AnalyseGetDeviceComposition(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	printf("tms_AnalyseGetDeviceComposition()\n");
	// printf("dowhat() %d\n", pcontext->ptr_analyse_arr->dowhat);

	if (pcontext->ptr_analyse_arr->dowhat != 0) {
		sg_dowhat[pcontext->ptr_analyse_arr->dowhat].ptrfun(pcontext, pdata, len);
	}
	if (pcontext->ptcb->pf_OnGetDeviceComposition) {
		pcontext->ptcb->pf_OnGetDeviceComposition(pcontext, pdata, len);
	}
	return 0;
}


// 板卡组成信息
uint32_t tms_RetDeviceComposition_any(
    int      fd,
    struct glink_addr *paddr,
    uint32_t count,
    struct tms_dev_composition_val *list,
    int32_t cmdid)
{
	uint8_t *pmem;
	struct tms_dev_composition *pcfg_hdr;
	struct tms_dev_composition_val         *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_dev_composition) + count * sizeof(struct tms_dev_composition_val);

	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	pcfg_hdr = (struct tms_dev_composition *)(pmem);
	plist   = (struct tms_dev_composition_val *)(pmem + sizeof(struct tms_dev_composition));

	// Step 2.各字段复制
	pcfg_hdr->count = htonl(count);

	// todo :防止循环count次溢出ptlist
	printf("count %d\n", count);
	ptlist = plist;
	for (uint32_t i = 0; i < count; i++) {
		// printf("r0 %d \n", ptlist->reserved0);
		printf("\tf%d/s%d/t%d/p%d r0%d/r1%d/r2%d/r3%d\n", list->frame, list->slot, list->type, list->port,
		       list->reserved0,
		       list->reserved1,
		       list->reserved2,
		       list->reserved3);
		ptlist->frame  = htonl(list->frame);
		ptlist->slot = htonl(list->slot);
		ptlist->type  = htonl(list->type);
		ptlist->port  = htonl(list->port);
		ptlist->reserved0 = htonl(list->reserved0);
		ptlist->reserved1 = htonl(list->reserved1);
		ptlist->reserved2 = htonl(list->reserved2);
		ptlist->reserved3 = htonl(list->reserved3);
		list++;
		ptlist++;
	}


	//   ------------------
	printf("%s()\n", __FUNCTION__);

	// ptlist = list;
	// for (uint32_t i = 0; i < count; i++) {
	// 	printf("\tf%d/s%d/t%d/p%d\n", ptlist->frame, ptlist->slot, ptlist->type, ptlist->port);
	// 	ptlist++;
	// }

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	printf("pbase src %x dst %x\n", base_hdr.src, base_hdr.dst);
	glink_Build(&base_hdr, cmdid, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	free(pmem);
	return ret;
}

/**
 * @brief	ID_RET_COMPOSITION 0x80000006 MCU返回板卡组成信息
 * @param[in]	fd 文件描述符
 * @param[in]	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param[in]	count tms_dev_composition_val 数组个数
 * @param[in]	list 指向一个 struct tms_dev_composition 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_RetDeviceComposition_any
 */
uint32_t tms_RetDeviceComposition(
    int      fd,
    struct glink_addr *paddr,
    uint32_t count,
    struct tms_dev_composition_val *list)
{
	return tms_RetDeviceComposition_any(fd, paddr, count, list, ID_RET_COMPOSITION);
}



// 0x80000067
static int32_t tms_AnalyseRetDeviceComposition(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	printf("tms_AnalyseRetDeviceComposition()\n");
	// printf("dowhat() %d\n", pcontext->ptr_analyse_arr->dowhat);

	// if (pcontext->ptr_analyse_arr->dowhat != 0) {
	// 	sg_dowhat[pcontext->ptr_analyse_arr->dowhat].ptrfun(pcontext, pdata, len);
	// }

	struct tms_dev_composition 		*pcfg_hdr;
	struct tms_dev_composition_val  *plist, *ptlist;


	pcfg_hdr = (struct tms_dev_composition *)(pdata + GLINK_OFFSET_DATA);
	plist    = (struct tms_dev_composition_val *)  (pdata + GLINK_OFFSET_DATA + sizeof(struct tms_dev_composition));

	pcfg_hdr->count = htonl(pcfg_hdr->count);

	// TODO：防止count溢出
	ptlist = plist;
	for (int i = 0; i < pcfg_hdr->count; i++) {
		ptlist->frame     = htonl(ptlist->frame);
		ptlist->slot      = htonl(ptlist->slot);
		ptlist->type      = htonl(ptlist->type);
		ptlist->port      = htonl(ptlist->port);
		ptlist->reserved0 = htonl(ptlist->reserved0);
		ptlist->reserved1 = htonl(ptlist->reserved1);
		ptlist->reserved2 = htonl(ptlist->reserved2);
		ptlist->reserved3 = htonl(ptlist->reserved3);

		printf("\tf%d/s%d/t%d/p%d r0%d/r1%d/r2%d/r3%d\n", ptlist->frame, ptlist->slot, ptlist->type, ptlist->port,
		       ptlist->reserved0,
		       ptlist->reserved1,
		       ptlist->reserved2,
		       ptlist->reserved3);
		ptlist++;
	}

	if (pcontext->ptcb->pf_OnRetDeviceComposition) {
		// pcontext->ptcb->pf_OnRetDeviceComposition(pcontext, pdata, len, pcfg_hdr, plist);
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief	ID_CFG_MCU_OSW_PORT 0x80000007 网管对OLP模块的工作模式和返回时间设定\n
 			ID_CFG_MCU_OLP_PORT 0x80000009 网管发送OLP模块各光端口关联光缆信息到MCU\n
 			应用层不需要直接调用该函数，有其它函数 tms_CfgMCUOSWPort、tms_CfgMCUOLPPort 封装
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	type 指定板卡设备类型
 * @param[in]	port 指定板卡端口数
 * @param[in]	cmdID 命令ID，可以是 \nID_CFG_MCU_OSW_PORT \nID_CFG_MCU_OLP_PORT
 * @param[in]	dev_name 设备名称
 * @param[in]	cable_name 光缆名称
 * @param[in]	start_name 光缆起始局端名称
 * @param[in]	end_name 光缆末端局端名称
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_CfgMCUOSWPort @see tms_CfgMCUOLPPort
 */
int32_t tms_CfgMCUAnyPort(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t port,
    int32_t type,
    int32_t cmdID,
    uint8_t (*dev_name)[64],
    uint8_t (*cable_name)[64],
    uint8_t (*start_name)[64],
    uint8_t (*end_name)[64])
{
	uint8_t *pmem;
	struct tms_cfg_mcu_osw_port oswmode;

	oswmode.frame = htonl(frame);
	oswmode.slot  = htonl(slot);
	oswmode.type  = htonl(type);
	oswmode.port  = htonl(port);
	memcpy(oswmode.dev_name, &dev_name[0][0], 64);
	memcpy(oswmode.cable_name, &cable_name[0][0], 64);
	memcpy(oswmode.start_name, &start_name[0][0], 64);
	memcpy(oswmode.end_name, &end_name[0][0], 64);

	// // Step 3. 发送
	struct glink_base  base_hdr;
	int ret = 0;

	pmem = (uint8_t *)&oswmode;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdID, sizeof(struct tms_cfg_mcu_osw_port));
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_cfg_mcu_osw_port));
	return ret;
}

static int32_t tms_AnalyseCfgMCUAnyPort(int8_t *pdata)
{
	struct tms_cfg_mcu_osw_port *pval;
	uint16_t *pstr16;
	pval  = (struct tms_cfg_mcu_osw_port *)(pdata + GLINK_OFFSET_DATA );

	pval->frame = htonl(pval->frame);
	pval->slot  = htonl(pval->slot);
	pval->type  = htonl(pval->type);
	pval->port  = htonl(pval->port);

	pstr16  = (uint16_t *)pval->dev_name;
	for (int k = 0; k < 64 / 2; k++) {
		*pstr16 = htons(*pstr16);
		pstr16++;
	}

	pstr16  = (uint16_t *)pval->cable_name;
	for (int k = 0; k < 64 / 2; k++) {
		*pstr16 = htons(*pstr16);
		pstr16++;
	}
	pstr16  = (uint16_t *)pval->start_name;
	for (int k = 0; k < 64 / 2; k++) {
		*pstr16 = htons(*pstr16);
		pstr16++;
	}
	pstr16  = (uint16_t *)pval->end_name;
	for (int k = 0; k < 64 / 2; k++) {
		*pstr16 = htons(*pstr16);
		pstr16++;
	}

	printf("dev_name:%s\n", pval->dev_name);
	printf("cable_name:%s\n", pval->cable_name);
	printf("start_name:%s\n", pval->start_name);
	printf("end_name:%s\n", pval->end_name);

	return 0;
}
/**
 * @brief	ID_CFG_MCU_OSW_PORT 0x80000007 网管对OLP模块的工作模式和返回时间设定
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定板卡端口数
 * @param[in]	dev_name 设备名称
 * @param[in]	cable_name 光缆名称
 * @param[in]	start_name 光缆起始局端名称
 * @param[in]	end_name 光缆末端局端名称
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUAnyPort
 */

int32_t tms_CfgMCUOSWPort(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t port,
    uint8_t (*dev_name)[64],
    uint8_t (*cable_name)[64],
    uint8_t (*start_name)[64],
    uint8_t (*end_name)[64])
{
	return tms_CfgMCUAnyPort(fd, paddr, frame, slot, port, DEV_OSW, ID_CFG_MCU_OSW_PORT, dev_name, cable_name, start_name, end_name);
}

static int32_t tms_AnalyseCfgMCUOSWPort(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseCfgMCUAnyPort(pdata);


	return 0;
}
/**
 * @brief	ID_CFG_MCU_OLP_PORT 0x80000009 网管发送OLP模块各光端口关联光缆信息到MCU
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定板卡端口数
 * @param[in]	dev_name 设备名称
 * @param[in]	cable_name 光缆名称
 * @param[in]	start_name 光缆起始局端名称
 * @param[in]	end_name 光缆末端局端名称
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUAnyPort
 */
int32_t tms_CfgMCUOLPPort(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t port,
    uint8_t (*dev_name)[64],
    uint8_t (*cable_name)[64],
    uint8_t (*start_name)[64],
    uint8_t (*end_name)[64])
{
	return tms_CfgMCUAnyPort(fd, paddr, frame, slot, port, DEV_OLP, ID_CFG_MCU_OLP_PORT, dev_name, cable_name, start_name, end_name);
}

static int32_t tms_AnalyseCfgMCUOLPPort(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseCfgMCUAnyPort(pdata);


	return 0;
}




////////////////////////////////////////////////////////////////////////////////
/**
 * @brief	OPM/OLP与OSW关联\n
 			应用层不需要直接调用该函数，有其它两函 数tms_CfgMCUUniteOPMOSW、tms_CfgMCUUniteOLPOSW 封装\n
 			ID_CFG_MCU_U_OPM_OSW 0x80000011 网管发送OPM光端口与OSW光端口联动关系到MCU\n
 			ID_CFG_MCU_U_OLP_OSW 0x80000013 网管发送OLP模块光端口与OSW模块光端口联动关系到MCU\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	any_frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	any_slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	any_type 指定板卡设备类型
 * @param[in]	cmdID 命令ID，可以是 \nID_CFG_MCU_U_OPM_OSW \nID_CFG_MCU_U_OLP_OSW
 * @param[in]	count struct tms_cfg_mcu_u_any_osw_val 数组个数
 * @param[in]	list 指向一个 struct tms_cfg_mcu_u_any_osw_val 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUUniteOPMOSW @see tms_CfgMCUUniteOLPOSW
 */
int32_t tms_CfgMCUUniteAnyOSW(
    int fd,
    struct glink_addr *paddr,
    int32_t any_frame,
    int32_t any_slot,
    int32_t any_type,
    int32_t cmdID,
    int32_t	count,
    struct tms_cfg_mcu_u_any_osw_val *list)
{
	uint8_t *pmem;
	struct tms_cfg_mcu_u_any_osw *pcfg_hdr;
	struct tms_cfg_mcu_u_any_osw_val         *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_cfg_mcu_u_any_osw) + count * sizeof(struct tms_cfg_mcu_u_any_osw_val);

	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	pcfg_hdr = (struct tms_cfg_mcu_u_any_osw *)(pmem);
	plist   = (struct tms_cfg_mcu_u_any_osw_val *)(pmem + sizeof(struct tms_cfg_mcu_u_any_osw));

	// Step 2.各字段复制
	pcfg_hdr->any_frame = htonl(any_frame);
	pcfg_hdr->any_slot  = htonl(any_slot);
	pcfg_hdr->any_type  = htonl(any_type);;//htonl(DEV_OPM);
	pcfg_hdr->count     = htonl(count);


	// todo :防止循环count次溢出ptlist
	ptlist = list;
	for (int i = 0; i < count; i++) {
		plist->any_port  = htonl(ptlist->any_port);
		plist->osw_frame = htonl(ptlist->osw_frame);
		plist->osw_slot  = htonl(ptlist->osw_slot);
		plist->osw_type  = htonl(DEV_OSW);
		plist->osw_port  = htonl(ptlist->osw_port);

		printf("index %d:\n", i);
		printf("\tolp/opm port:%d\n", ptlist->any_port);
		printf("\tosw frame:%d\n", ptlist->osw_frame);
		printf("\tosw slot:%d\n", ptlist->osw_slot);
		printf("\tosw port:%d\n\n", ptlist->osw_port);
		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdID, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	free(pmem);
	return ret;
}

static int32_t tms_AnalyseCfgMCUUniteAnyOSW(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// uint8_t *pmem;
	struct tms_cfg_mcu_u_any_osw *pcfg_hdr;
	struct tms_cfg_mcu_u_any_osw_val         *plist, *ptlist;

	pcfg_hdr = (struct tms_cfg_mcu_u_any_osw *)(pdata + GLINK_OFFSET_DATA);
	if ( !CHECK_PTR(
	         pcfg_hdr,
	         struct tms_cfg_mcu_u_any_osw,
	         struct tms_cfg_mcu_u_any_osw_val,
	         htonl(pcfg_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	plist   = (struct tms_cfg_mcu_u_any_osw_val *)(pdata + GLINK_OFFSET_DATA +  + sizeof(struct tms_cfg_mcu_u_any_osw));

	// Step 2.各字段复制
	pcfg_hdr->any_frame = htonl(pcfg_hdr->any_frame);
	pcfg_hdr->any_slot  = htonl(pcfg_hdr->any_slot);
	pcfg_hdr->any_type  = htonl(pcfg_hdr->any_type);
	pcfg_hdr->count     = htonl(pcfg_hdr->count);


	// todo :防止循环count次溢出ptlist
	ptlist = plist;
	for (int i = 0; i < pcfg_hdr->count; i++) {
		plist->any_port  = htonl(ptlist->any_port);
		plist->osw_frame = htonl(ptlist->osw_frame);
		plist->osw_slot  = htonl(ptlist->osw_slot);
		plist->osw_type  = htonl(plist->osw_type);
		plist->osw_port  = htonl(ptlist->osw_port);


		printf("index %d:\n", i);
		printf("\tolp/opm port:%d\n", plist->any_port);
		printf("\tosw frame:%d\n", plist->osw_frame);
		printf("\tosw slot:%d\n", plist->osw_slot);
		printf("\tosw port:%d\n\n", plist->osw_port);

		plist++;
		ptlist++;
	}

	return 0;
}

/**
 * @brief	ID_CFG_MCU_U_OPM_OSW 0x80000011 网管发送OPM光端口与OSW光端口联动关系到MCU\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_cfg_mcu_u_any_osw_val 数组个数
 * @param[in]	list 指向一个 struct tms_cfg_mcu_u_any_osw_val 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUUniteAnyOSW
 */
int32_t tms_CfgMCUUniteOPMOSW(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t	count,
    struct tms_cfg_mcu_u_any_osw_val *list)
{
	return tms_CfgMCUUniteAnyOSW(fd, paddr, frame, slot, DEV_OPM, ID_CFG_MCU_U_OPM_OSW, count, list);
}

static int32_t tms_AnalyseCfgMCUUniteOPMOSW(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseCfgMCUUniteAnyOSW(pcontext, pdata, len);


	return 0;
}
/**
 * @brief	ID_CFG_MCU_U_OLP_OSW 0x80000013 网管发送OLP模块光端口与OSW模块光端口联动关系到MCU\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_cfg_mcu_u_any_osw_val 数组个数
 * @param[in]	list 指向一个 struct tms_cfg_mcu_u_any_osw_val 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUUniteAnyOSW
 */
int32_t tms_CfgMCUUniteOLPOSW(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t	count,
    struct tms_cfg_mcu_u_any_osw_val *list)
{
	return tms_CfgMCUUniteAnyOSW(fd, paddr, frame, slot, DEV_OLP, ID_CFG_MCU_U_OLP_OSW, count, list);
}

static int32_t tms_AnalyseCfgMCUUniteOLPOSW(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseCfgMCUUniteAnyOSW(pcontext, pdata, len);


	return 0;
}







////////////////////////////////////////////////////////////////////////////////

/**
 * @brief	清楚OPM/OLP与OSW端口信息\n
 			应用层不需要直接调用该函数，有其它函数 tms_CfgMCUUniteOPMOSWClear、tms_CfgMCUUniteOLPOSWClear、
 			tms_CfgMCUOPMPortClear、tms_CfgMCUOLPPortClear\n
 			ID_CFG_MCU_OSW_PORT_CLEAR 0x80000008   网管发送清除OSW模块各光端口关联光缆信息到MCU\n
 			ID_CFG_MCU_OLP_PORT_CLEAR 0x80000010   网管发送清除OLP模块各光端口关联光缆信息到MCU\n
 			ID_CFG_MCU_U_OPM_OSW_CLEAR 0x80000012  网管发送清除OPM光端口与OSW光端口联动关系到MCU\n
 			ID_CFG_MCU_U_OLP_OSW_CLEAR 0x80000014  网管发送清除OLP模块光端口与OSW模块光端口联动关系到MCU\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	any_frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	any_slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	any_type 指定板卡设备类型
 * @param[in]	cmdID 命令ID，可以是 \nID_CFG_MCU_OSW_PORT_CLEAR \nID_CFG_MCU_OLP_PORT_CLEAR \nID_CFG_MCU_U_OPM_OSW_CLEAR \nID_CFG_MCU_U_OLP_OSW_CLEAR
 * @param[in]	count struct tms_cfg_mcu_any_port_clear_val 数组个数
 * @param[in]	list 指向一个 struct tms_cfg_mcu_any_port_clear_val 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUUniteOPMOSWClear @see	tms_CfgMCUUniteOLPOSWClear @see	tms_CfgMCUOPMPortClear @see	tms_CfgMCUOLPPortClear
 */
int32_t tms_CfgMCUAnyPortClear(
    int fd,
    struct glink_addr *paddr,
    int32_t any_frame,
    int32_t any_slot,
    int32_t any_type,
    int32_t cmdID,
    int32_t	count,
    struct  tms_cfg_mcu_any_port_clear_val *list)
{
	uint8_t *pmem;
	struct tms_cfg_mcu_u_any_osw *pcfg_hdr;
	struct tms_cfg_mcu_any_port_clear_val         *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_cfg_mcu_u_any_osw) + count * sizeof(struct tms_cfg_mcu_any_port_clear_val);

	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	pcfg_hdr = (struct tms_cfg_mcu_u_any_osw *)(pmem);
	plist   = (struct tms_cfg_mcu_any_port_clear_val *)(pmem + sizeof(struct tms_cfg_mcu_u_any_osw));

	// Step 2.各字段复制
	pcfg_hdr->any_frame = htonl(any_frame);
	pcfg_hdr->any_slot  = htonl(any_slot);
	pcfg_hdr->any_type  = htonl(any_type);//htonl(DEV_OPM);
	pcfg_hdr->count     = htonl(count);

	printf("val:f%2.2d/s%2.2d/t%2.2d\n",
	       any_frame,
	       any_slot,
	       DEV_OPM);

	// todo :防止循环count次溢出ptlist
	ptlist = list;
	for (int i = 0; i < count; i++) {
		plist->any_port  = htonl(ptlist->any_port);
		printf("anyport %d\n", ptlist->any_port);
		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdID, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	free(pmem);
	return ret;
}

static int32_t tms_AnalyseCfgMCUAnyPortClear(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// uint8_t *pmem;
	struct tms_cfg_mcu_u_any_osw *pcfg_hdr;
	struct tms_cfg_mcu_any_port_clear_val         *plist, *ptlist;

	// Step 1.分配内存并各指针指向相应内存

	pcfg_hdr = (struct tms_cfg_mcu_u_any_osw *)(pdata + GLINK_OFFSET_DATA);
	plist   = (struct tms_cfg_mcu_any_port_clear_val *)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_cfg_mcu_u_any_osw));


	// Step 2.各字段复制
	pcfg_hdr->any_frame = htonl(pcfg_hdr->any_frame);
	pcfg_hdr->any_slot  = htonl(pcfg_hdr->any_slot);
	pcfg_hdr->any_type  = htonl(pcfg_hdr->any_type);
	pcfg_hdr->count     = htonl(pcfg_hdr->count);

	printf("val:f%2.2d/s%2.2d/t%2.2d\n",
	       pcfg_hdr->any_frame,
	       pcfg_hdr->any_slot,
	       pcfg_hdr->any_type);
	// todo :防止循环count次溢出ptlist
	printf("count %d\n", pcfg_hdr->count);
	ptlist = plist;
	for (int i = 0; i < pcfg_hdr->count; i++) {
		plist->any_port  = htonl(ptlist->any_port);
		printf("\tanyport %d\n", plist->any_port);
		plist++;
		ptlist++;
	}

	return 0;
}

/**
 * @brief	ID_CFG_MCU_OSW_PORT_CLEAR 0x80000008   网管发送清除OSW模块各光端口关联光缆信息到MCU\n

 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	any_frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	any_slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_cfg_mcu_any_port_clear_val 数组个数
 * @param[in]	list 指向一个 struct tms_cfg_mcu_any_port_clear_val 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUAnyPortClear
 */
// 作废
int32_t tms_CfgMCUOPMPortClear(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t any_slot,
    int32_t	count,
    struct tms_cfg_mcu_any_port_clear_val *list)
{
	printf("unuse\nunuse\nunuse\nunuse\nunuse\nunuse\nunuse\nunuse\nunuse\n");
	return tms_CfgMCUAnyPortClear(fd, paddr, frame, any_slot, DEV_OLP, ID_CFG_MCU_OSW_PORT_CLEAR, count, list);
}
// 用这个代替上面的
int32_t tms_CfgMCUOSWPortClear(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t any_slot,
    int32_t	count,
    struct tms_cfg_mcu_any_port_clear_val *list)
{
	return tms_CfgMCUAnyPortClear(fd, paddr, frame, any_slot, DEV_OSW, ID_CFG_MCU_OSW_PORT_CLEAR, count, list);
}

static int32_t tms_AnalyseCfgMCUOPMPortClear(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseCfgMCUAnyPortClear(pcontext, pdata, len);



	return 0;
}
/**
 * @brief	ID_CFG_MCU_OLP_PORT_CLEAR 0x80000010   网管发送清除OLP模块各光端口关联光缆信息到MCU\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	any_frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	any_slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_cfg_mcu_any_port_clear_val 数组个数
 * @param[in]	list 指向一个 struct tms_cfg_mcu_any_port_clear_val 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUAnyPortClear
 */
int32_t tms_CfgMCUOLPPortClear(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t any_slot,
    int32_t	count,
    struct tms_cfg_mcu_any_port_clear_val *list)
{
	return tms_CfgMCUAnyPortClear(fd, paddr, frame, any_slot, DEV_OLP, ID_CFG_MCU_OLP_PORT_CLEAR, count, list);
}


static int32_t tms_AnalyseCfgMCUOLPPortClear(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseCfgMCUAnyPortClear(pcontext, pdata, len);

	return 0;
}
/**
 * @brief	ID_CFG_MCU_U_OPM_OSW_CLEAR 0x80000012  网管发送清除OPM光端口与OSW光端口联动关系到MCU\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	any_frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	any_slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_cfg_mcu_any_port_clear_val 数组个数
 * @param[in]	list 指向一个 struct tms_cfg_mcu_any_port_clear_val 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUAnyPortClear
 */
int32_t tms_CfgMCUUniteOPMOSWClear(
    int fd,
    struct glink_addr *paddr,
    int32_t any_frame,
    int32_t any_slot,
    int32_t	count,
    struct tms_cfg_mcu_any_port_clear_val *list)
{
	return tms_CfgMCUAnyPortClear(fd, paddr, any_frame, any_slot, DEV_OPM, ID_CFG_MCU_U_OPM_OSW_CLEAR, count, list);
}


static int32_t tms_AnalyseCfgMCUUniteOPMOSWClear(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseCfgMCUAnyPortClear(pcontext, pdata, len);


	return 0;
}

/**
 * @brief	ID_CFG_MCU_U_OLP_OSW_CLEAR 0x80000014  网管发送清除OLP模块光端口与OSW模块光端口联动关系到MCU\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_cfg_mcu_any_port_clear_val 数组个数
 * @param[in]	list 指向一个 struct tms_cfg_mcu_any_port_clear_val 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_CfgMCUAnyPortClear
 */
int32_t tms_CfgMCUUniteOLPOSWClear(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t	count,
    struct tms_cfg_mcu_any_port_clear_val *list)
{
	return tms_CfgMCUAnyPortClear(fd, paddr, frame, slot, DEV_OLP, ID_CFG_MCU_U_OLP_OSW_CLEAR, count, list);
}


static int32_t tms_AnalyseCfgMCUUniteOLPOSWClear(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseCfgMCUAnyPortClear(pcontext, pdata, len);
	return 0;
}
#endif

/*todo 分析和构造OTDR曲线数据公共接口

*todo 分析和构造OTDR曲线数据公共接口
*
*/

static void tms_OTDRConv_tms_retotdr_test_param(
    struct tms_retotdr_test_param *pout,
    struct tms_retotdr_test_param *pin)
{
	register uint32_t *p32s, *p32d;
	// register uint16_t *p16s, *p16d;
	// register int loop;

	p32d = (uint32_t *)pout;
	p32s = (uint32_t *)pin;
	for (register uint32_t i = 0; i < sizeof (struct tms_retotdr_test_param) / sizeof(int32_t); i++) {
		*p32d = htonl(*p32s);
		p32d++;
		p32s++;
	}



}

static void tms_OTDRConv_tms_retotdr_data_hdr(
    struct tms_retotdr_data_hdr *pout,
    struct tms_retotdr_data_hdr *pin)
{
	memcpy(pout->dpid, pin->dpid, 12);

	pout->count = htonl(pin->count);
}

static void tms_OTDRConv_tms_retotdr_data_val(
    struct tms_retotdr_data_val *pout,
    struct tms_retotdr_data_val *pin,
    struct tms_retotdr_data_hdr *pdata_hdr)
{
	register uint16_t *p16s, *p16d;
	register int loop;

	// Part B.2
	// loop = pdata_hdr->count >> 1;
	loop = pdata_hdr->count ;
	p16d = (uint16_t *)pout;
	p16s = (uint16_t *)pin;
	for (register int i = 0; i < loop; i++) {
		*p16d = htons(*p16s);
		p16d++;
		p16s++;
	}
	// printf("llooop = %d\n", loop);
}

static void tms_OTDRConv_tms_retotdr_event_hdr(
    struct tms_retotdr_event_hdr *pout,
    struct tms_retotdr_event_hdr *pin)
{
	// pin->count = pin->count & 0x3ff;				// 限定loop在0~1024以内

	memcpy(pout->eventid, pin->eventid, 12);
	pout->count = htonl(pin->count);
}

static void tms_OTDRConv_tms_retotdr_event_val(
    struct tms_retotdr_event_val *pout,
    struct tms_retotdr_event_val *pin,
    struct tms_retotdr_event_hdr *pevent_hdr)
{
	register int32_t *p32s, *p32d;
	register int loop;



	loop = pevent_hdr->count;
	loop = loop * sizeof (struct tms_retotdr_event_val) >> 2;	// 计算有多少个4Byte数据
	// printf("loop %d\n", loop);
	p32d = (int32_t *)pout;
	p32s = (int32_t *)pin;
	for (register int i = 0; i < loop; i++) {
		*p32d = htonl(*p32s);
		p32d++;
		p32s++;
	}
}
static void tms_OTDRConv_tms_retotdr_chain(
    struct tms_retotdr_chain *pout,
    struct tms_retotdr_chain *pin)
{
	register int32_t *p32s, *p32d;
	// register int loop;

	p32d = (int32_t *)&pout->range;
	p32s = (int32_t *)&pin->range;
	memcpy(&pout->inf[0], &pin->inf[0], 20);
	printf("in inf %s\n", pin->inf);
	printf("out inf %s\n", pout->inf);
	*p32d = htonl(*p32s);
	p32d++;
	p32s++;
	*p32d = htonl(*p32s);
	p32d++;
	p32s++;
	*p32d = htonl(*p32s);
	p32d++;
	p32s++;
}


/**
 * @brief	链表结构在本地字节序和网络字节序之间转换，结构满足下面格式\n
 			count + struct A + struct A + ... + struct A
 * @param	null
 * @retval	null
 * @remarks
 * @see
 */

static void tms_Conv_Nx4Byte(
    uint32_t *pout,
    uint32_t *pin,
    int32_t count)
{
	register int32_t *p32s, *p32d;
	register int loop;



	loop = count >> 2;	// 计算有多少个4Byte数据
	// printf("loop %d\n", loop);
	p32d = (int32_t *)pout;
	p32s = (int32_t *)pin;
	for (register int i = 0; i < loop; i++) {
		*p32d = htonl(*p32s);
		p32d++;
		p32s++;
	}
}
////////////////////////////////////////////////////////////////////////////////
#if 0
/**
 * @brief	设置OLP、OPM光功率参考值\n
 			应用层不需要直接调用该函数，有其它函数 tms_CfgOPMRefLevel、tms_CfgOLPRefLevel\n
 			ID_CFG_OPM_REF_LEVEL 0x80000015	网管发送OPM模块各光端口关联光缆的参考光功率及告警门限\n
 			ID_CFG_OLP_REF_LEVEL 0x80000018	网管发送OLP模块各光端口关联光缆的光功率及告警门限到MCU\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	any_frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	any_slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	any_type 指定板卡设备类型
 * @param[in]	cmdID 命令ID，可以是 \nID_CFG_OPM_REF_LEVEL \nID_CFG_OPM_REF_LEVEL
 * @param[in]	count struct tms_cfg_opm_ref_val 或struct tms_cfg_olp_ref_val 数组个数
 * @param[in]	list 指向一个 struct tms_cfg_opm_ref_val 或struct tms_cfg_olp_ref_val 一维数组
 * @retval	null
 * @remarks
 * @see	tms_CfgOPMRefLevel @see tms_CfgOLPRefLevel
 */

int32_t tms_CfgAnyRefLevel(
    int fd,
    struct glink_addr *paddr,
    int32_t any_frame,
    int32_t any_slot,
    int32_t any_type,
    int32_t cmdID,
    int32_t count,
    void    *list)
{
	uint8_t *pmem;
	struct tms_cfg_any_power_ref *pcfg_hdr;
	struct tms_cfg_opm_ref_val         *popm_list, *ptopm_list;
	struct tms_cfg_olp_ref_val         *polp_list, *ptolp_list;
	int len;


	// Step 1.分配内存并各指针指向相应内存
	if (any_type == DEV_OPM) {
		len = sizeof(struct tms_cfg_any_power_ref) + count * sizeof(struct tms_cfg_opm_ref_val);
		printf("len = %d\n", len);
	}
	else {// DEV_OLP
		len = sizeof(struct tms_cfg_any_power_ref) + count * sizeof(struct tms_cfg_olp_ref_val);
	}


	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	pcfg_hdr    = (struct tms_cfg_any_power_ref *)(pmem);
	popm_list   = (struct tms_cfg_opm_ref_val *)(pmem + sizeof(struct tms_cfg_any_power_ref));
	polp_list   = (struct tms_cfg_olp_ref_val *)(pmem + sizeof(struct tms_cfg_any_power_ref));

	// Step 2.各字段复制
	pcfg_hdr->frame = htonl(any_frame);
	pcfg_hdr->slot  = htonl(any_slot);
	pcfg_hdr->type  = htonl(any_type);
	pcfg_hdr->count = htonl(count);



	// todo :防止循环count次溢出ptlist
	if (any_type == DEV_OPM) {
		ptopm_list = (struct tms_cfg_opm_ref_val *)list;
		tms_Print_tms_cfg_opm_ref_val(ptopm_list, count);
		ptopm_list = (struct tms_cfg_opm_ref_val *)list;
		for (int i = 0; i < count; i++) {
			popm_list->port      = htonl(ptopm_list->port);
			popm_list->isminitor = htonl(ptopm_list->isminitor);
			popm_list->wave      = htonl(ptopm_list->wave);
			popm_list->ref_power = htonl(ptopm_list->ref_power);
			popm_list->leve0     = htonl(ptopm_list->leve0);
			popm_list->leve1     = htonl(ptopm_list->leve1);
			popm_list->leve2     = htonl(ptopm_list->leve2);
			popm_list++;
			ptopm_list++;
		}
	}
	else {// DEV_OLP
		ptolp_list = (struct tms_cfg_olp_ref_val *)list;
		tms_Print_tms_cfg_olp_ref_val(ptolp_list, count);
		ptolp_list = (struct tms_cfg_olp_ref_val *)list;
		for (int i = 0; i < count; i++) {
			polp_list->port      = htonl(ptolp_list->port);
			polp_list->isminitor = htonl(ptolp_list->isminitor);
			polp_list->wave      = htonl(ptolp_list->wave);
			polp_list->ref_power = htonl(ptolp_list->ref_power);
			polp_list->leve0     = htonl(ptolp_list->leve0);
			polp_list->leve1     = htonl(ptolp_list->leve1);
			polp_list->leve2     = htonl(ptolp_list->leve2);
			polp_list++;
			ptolp_list++;
		}
	}


	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdID, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	free(pmem);
	return ret;
}


static int32_t tms_AnalyseCfgAnyRefLevel(struct tms_context *pcontext, int8_t *pdata, int32_t len, int32_t any_type)
{
	// uint8_t *pmem;
	struct tms_cfg_any_power_ref *pcfg_hdr;
	struct tms_cfg_opm_ref_val         *popm_list, *ptopm_list;
	struct tms_cfg_olp_ref_val         *polp_list, *ptolp_list;


	// Step 1.分配内存并各指针指向相应内存

	pcfg_hdr    = (struct tms_cfg_any_power_ref *)(pdata + GLINK_OFFSET_DATA);
	popm_list   = (struct tms_cfg_opm_ref_val *)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_cfg_any_power_ref));
	polp_list   = (struct tms_cfg_olp_ref_val *)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_cfg_any_power_ref));

	// Step 2.各字段复制
	pcfg_hdr->frame = htonl(pcfg_hdr->frame);
	pcfg_hdr->slot  = htonl(pcfg_hdr->slot);
	pcfg_hdr->type  = htonl(pcfg_hdr->type);
	pcfg_hdr->count = htonl(pcfg_hdr->count);



	// todo :防止循环count次溢出ptlist
	if (any_type == DEV_OPM) {
		ptopm_list = (struct tms_cfg_opm_ref_val *)popm_list;
		for (int i = 0; i < pcfg_hdr->count; i++) {


			ptopm_list->port      = htonl(ptopm_list->port);
			ptopm_list->isminitor = htonl(ptopm_list->isminitor);
			ptopm_list->wave      = htonl(ptopm_list->wave);
			ptopm_list->ref_power = htonl(ptopm_list->ref_power);
			ptopm_list->leve0     = htonl(ptopm_list->leve0);
			ptopm_list->leve1     = htonl(ptopm_list->leve1);
			ptopm_list->leve2     = htonl(ptopm_list->leve2);
			printf("index:%d\n", i);
			printf("\tport:%d\n", ptopm_list->port);
			printf("\tisminitor:%d\n", ptopm_list->isminitor);
			printf("\twave:%d\n", ptopm_list->wave);
			printf("\tref_power:%d\n", ptopm_list->ref_power);
			printf("\tleve0:%d\n", ptopm_list->leve0);
			printf("\tleve1:%d\n", ptopm_list->leve1);
			printf("\tleve2:%d\n\n", ptopm_list->leve2);
			ptopm_list++;
		}
	}
	else {// DEV_OLP
		ptolp_list = (struct tms_cfg_olp_ref_val *)polp_list;
		for (int i = 0; i < pcfg_hdr->count; i++) {

			ptolp_list->port      = htonl(ptolp_list->port);
			ptolp_list->isminitor = htonl(ptolp_list->isminitor);
			ptolp_list->wave      = htonl(ptolp_list->wave);
			ptolp_list->ref_power = htonl(ptolp_list->ref_power);
			ptolp_list->leve0     = htonl(ptolp_list->leve0);
			ptolp_list->leve1     = htonl(ptolp_list->leve1);
			ptolp_list->leve2     = htonl(ptolp_list->leve2);
			printf("index:%d\n", i);
			printf("\tref_power:%d\n", ptolp_list->ref_power);
			printf("\tleve0:%d\n", ptolp_list->leve0);
			printf("\tleve1:%d\n", ptolp_list->leve1);
			printf("\tleve2:%d\n\n", ptolp_list->leve2);
			ptolp_list++;
		}
	}

	return 0;
}
/**
* @brief	ID_CFG_OPM_REF_LEVEL 0x80000015	网管发送OPM模块各光端口关联光缆的参考光功率及告警门限
* @param[in]	fd 套接字文件描述符
* @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
				修改tmsxx.h设定
* @param[in]	any_frame 指定机框号(0 ~ MAX_FRAME)
* @param[in]	any_slot 指定槽位号(0 ~ MAX_SLOT)
* @param[in]	count struct tms_cfg_opm_ref_val  数组个数
* @param[in]	list 指向一个 struct tms_cfg_opm_ref_val  一维数组
* @retval	null
* @remarks
* @see	tms_CfgAnyRefLevel
*/
int32_t tms_CfgOPMRefLevel(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t count,
    struct tms_cfg_opm_ref_val *list)
{
	return tms_CfgAnyRefLevel(fd, paddr, frame, slot, DEV_OPM, ID_CFG_OPM_REF_LEVEL, count, list);
}
static int32_t tms_AnalyseCfgOPMRefLevel(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// TODO 转发到设备
	// tms_Transmit2Dev(pcontext, pdata, len);
	//-----------------//需要测试
	tms_AnalyseCfgAnyRefLevel(pcontext, pdata, len, DEV_OPM);
	return 0;
}


/**
 * @brief	ID_CFG_OLP_REF_LEVEL	0x80000018	网管发送OLP模块各光端口关联光缆的光功率及告警门限到MCU\n
 			TMSxxV1.2 OPM与OLP使用相同的协议 ID 号 ID_CFG_OPM_REF_LEVEL
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	any_frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	any_slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_cfg_olp_ref_val  数组个数
 * @param[in]	list 指向一个 struct tms_cfg_olp_ref_val  一维数组
 * @retval	null
 * @remarks
 * @see	tms_CfgAnyRefLevel
 */

int32_t tms_CfgOLPRefLevel(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t count,
    struct tms_cfg_olp_ref_val *list)
{
	// return tms_CfgAnyRefLevel(fd, paddr, frame, slot, DEV_OLP, ID_CFG_OLP_REF_LEVEL, count, list);
	// TMSxxV1.2 OPM与OLP使用相同的协议 ID 号 ID_CFG_OPM_REF_LEVEL
	return tms_CfgAnyRefLevel(fd, paddr, frame, slot, DEV_OLP, ID_CFG_OPM_REF_LEVEL, count, list);

}

static int32_t tms_AnalyseCfgOLPRefLevel(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseCfgAnyRefLevel(pcontext, pdata, len, DEV_OLP);
	return 0;
}
/**
* @brief	ID_GET_OPM_OP 0x80000016 网管查询OPM模块各光端口当前功率
* @param[in]	fd 套接字文件描述符
* @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
				修改tmsxx.h设定
* @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
* @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
* @retval	>0 发送成功
* @retval	0 发送失败，该链接失效，立即断开
* @remarks
* @see	tms_MCUtoDevice
*/
int32_t tms_GetOPMOP(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot)
{
	return tms_MCUtoDevice(fd, paddr,  frame,  slot, DEV_OPM, 0, ID_GET_OPM_OP, sizeof(struct tms_dev_type));
}

//0x80000016
static int32_t tms_AnalyseGetOPMOP(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_dev_slot *pval;
	pval = (struct tms_dev_slot *)(pdata + GLINK_OFFSET_DATA);
	pval->frame = htonl(pval->frame);
	pval->slot = htonl(pval->slot);

	printf("tms_AnalyseGetOPMOP\n");
	printf("val:f%d/s%x\n", pval->frame, pval->slot);
	// fun()

	return 0;
}

/**
 * @brief	ID_GET_OLP_OP 0x80000019 网管查询OLP模块各光端口当前功率\n
 			TMSxxV1.2 OPM与OLP使用相同的协议 ID 号 ID_GET_OPM_OP
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_MCUtoDevice
 */
int32_t tms_GetOLPOP(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot)
{
	// return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OLP, 0, ID_GET_OLP_OP, sizeof(struct tms_dev_type));
	// TMSxxV1.2 OPM与OLP使用相同的协议 ID 号 ID_GET_OPM_OP
	return tms_MCUtoDevice(fd, paddr,  frame,  slot, DEV_OLP, 0, ID_GET_OPM_OP, sizeof(struct tms_dev_type));
}






////////////////////////////////////////////////////////////////////////////////
/**
 * @brief	返回OPM/OLP功率\n
 			应用层不需要直接调用该函数，有其它函数 tms_RetOPMOP、tms_RetOLPOP 封装
 			ID_RET_OPM_OP 0x80000017	MCU返回OPM模块各光端口当前功率\n
 			ID_RET_OLP_OP 0x80000020	MCU返回OLP模块各光端口当前功率\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	any_frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	any_slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	any_type 指定板卡设备类型
 * @param[in]	cmdID 命令ID，可以是 \nID_RET_OPM_OP \nID_RET_OLP_OP
 * @param[in]	count struct tms_any_op_val  数组个数
 * @param[in]	list 指向一个 struct tms_any_op_val  一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_RetOPMOP @see tms_RetOLPOP
 */
int32_t tms_RetAnyOP(
    int fd,
    struct glink_addr *paddr,

    int32_t any_frame,
    int32_t any_slot,
    int32_t any_type,
    int32_t cmdID,
    int32_t count,
    struct tms_any_op_val *list)
{
	uint8_t *pmem;
	struct tms_any_op      *phdr;
	struct tms_any_op_val  *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_any_op) + count * sizeof(struct tms_any_op_val);

	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	phdr  = (struct tms_any_op *)(pmem);
	plist = (struct tms_any_op_val *)(pmem + sizeof(struct tms_any_op));

	// Step 2.各字段复制
	phdr->frame = htonl(any_frame);
	phdr->slot  = htonl(any_slot);
	phdr->type  = htonl(any_type);
	phdr->count = htonl(count);

	// todo :防止循环count次溢出ptlist
	ptlist = list;
	for (int i = 0; i < count; i++) {
		plist->port  = htonl(ptlist->port);
		plist->power = htonl(ptlist->power);
		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdID, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	free(pmem);
	return ret;
}
//0x80000017
static int32_t tms_AnalyseRetAnyOP(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	sg_dowhat[pcontext->ptr_analyse_arr->dowhat].ptrfun(pcontext, pdata, len);
	struct tms_any_op      *phdr;
	struct tms_any_op_val  *plist, *ptlist;

	phdr  = (struct tms_any_op *)(pdata + GLINK_OFFSET_DATA );
	plist = (struct tms_any_op_val *)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_any_op));

	phdr->frame = htonl(phdr->frame);
	phdr->slot  = htonl(phdr->slot);
	phdr->type  = htonl(phdr->type);
	phdr->count = htonl(phdr->count);

	ptlist = plist;
	for (int i = 0; i < phdr->count; i++) {
		ptlist->port  = htonl(ptlist->port);
		ptlist->power = htonl(ptlist->power);
		ptlist++;
	}

	printf("tms_AnalyseRetOPMOP\n");
	printf("val:f%d/s%x/t%d\n", phdr->frame, phdr->slot, phdr->type);
	ptlist = plist;
	for (int i = 0; i < phdr->count; i++) {
		printf("\tport %d power %d\n", ptlist->port, ptlist->power);
		ptlist++;
	}
	// fun()


	//
	if (pcontext->ptcb->pf_OnRetAnyOP) {
		pcontext->ptcb->pf_OnRetAnyOP(pcontext, phdr, plist);
	}

	return 0;
}
/**
 * @brief	ID_RET_OPM_OP 0x80000017	MCU返回OPM模块各光端口当前功率\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_any_op_val  数组个数
 * @param[in]	list 指向一个 struct tms_any_op_val  一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_RetOPMOP @see tms_RetOLPOP
 */
int32_t tms_RetOPMOP(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t count,
    struct tms_any_op_val *list)
{
	return tms_RetAnyOP(fd, 	paddr, frame, slot, DEV_OPM, ID_RET_OPM_OP, count, list);
}

static int32_t tms_AnalyseRetOPMOP(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseRetAnyOP(pcontext, pdata, len);
}

/**
 * @brief	ID_RET_OLP_OP 0x80000020	MCU返回OLP模块各光端口当前功率\n
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_any_op_val  数组个数
 * @param[in]	list 指向一个 struct tms_any_op_val  一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_RetOPMOP @see tms_RetOLPOP
 */
int32_t tms_RetOLPOP(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t count,
    struct tms_any_op_val *list)
{
	return tms_RetAnyOP(fd, paddr, frame, slot, DEV_OLP, ID_RET_OLP_OP, count, list);
}
static int32_t tms_AnalyseRetOLPOP(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseRetAnyOP(pcontext, pdata, len);
}
/*
 * @brief	ID_CFG_OTDR_REF 0x80000021 网管发送各监测光路的OTDR参考曲线数据到MCU
 * @param	fd
 * @param	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param	pref_hdr
 * @param	ptest_param

 * @param	pdata_hdr
 * @param	pdata_val

 * @param	pevent_hdr
 * @param	pevent_val
 * @param	pchain
 * @param	pref_date
 */
int32_t tms_CfgOTDRRef(
    int fd,
    struct glink_addr *paddr,
    struct tms_otdr_ref_hdr       *pref_hdr,
    struct tms_retotdr_test_param *ptest_param,

    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,

    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,

    struct tms_retotdr_chain      *pchain,
    struct tms_cfg_otdr_ref_val   *pref_date)
{

	struct tms_otdr_ref_hdr   ref_hdr;
	struct tms_retotdr_test_param test_param;
	struct tms_retotdr_data_hdr   data_hdr;
	struct tms_retotdr_data_val   data_val[OTDR_SAMPLE_32000];
	struct tms_retotdr_event_hdr  event_hdr;
	struct tms_retotdr_event_val  event_val[OTDR_EVENT_MAX];
	struct tms_retotdr_chain      chain;
	struct tms_cfg_otdr_ref_val      ref_date;
	// register int32_t *p32s, *p32d;
	// register int16_t *p16s, *p16d;
	// register int loop;



	// Part A.1
	ref_hdr.osw_frame  = htonl(pref_hdr->osw_frame);
	ref_hdr.osw_slot   = htonl(pref_hdr->osw_slot);
	ref_hdr.osw_type   = htonl(DEV_OSW);
	ref_hdr.osw_port   = htonl(pref_hdr->osw_port);
	memcpy(ref_hdr.strid, pref_hdr->strid, 20);
	ref_hdr.otdr_port  = htonl(pref_hdr->otdr_port);


	printf("frame %d osw %d type %d port %d  otdr port %d\n",
	       pref_hdr->osw_frame  ,
	       pref_hdr->osw_slot   ,
	       pref_hdr->osw_type   ,
	       pref_hdr->osw_port   ,
	       pref_hdr->otdr_port  );

	tms_OTDRConv_tms_retotdr_test_param(&test_param, ptest_param);


	// Part B.1
	if (pdata_hdr->count > OTDR_SAMPLE_32000) {
		pdata_hdr->count = OTDR_SAMPLE_32000;
	}
	tms_OTDRConv_tms_retotdr_data_hdr(&data_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(&data_val[0], pdata_val, pdata_hdr);
	// // Part C.1
	pevent_hdr->count = pevent_hdr->count & 0x3ff;				// 限定loop在0~1024以内
	tms_OTDRConv_tms_retotdr_event_hdr(&event_hdr, pevent_hdr);
	// Part C.2
	tms_OTDRConv_tms_retotdr_event_val(&event_val[0], pevent_val, pevent_hdr);
	tms_OTDRConv_tms_retotdr_chain(&chain, pchain);


	// 填充参考值
	ref_date.leve0 = htonl(pref_date->leve0);
	ref_date.leve1 = htonl(pref_date->leve1);
	ref_date.leve2 = htonl(pref_date->leve2);



	// Step 3. 发送
	struct glink_base  base_hdr;
	// int ret;
	int len;
	len = 	sizeof(struct tms_otdr_ref_hdr  ) +
	        sizeof(struct tms_retotdr_test_param) +
	        sizeof(struct tms_retotdr_data_hdr  ) +
	        sizeof(struct tms_retotdr_data_val  ) * pdata_hdr->count +
	        sizeof(struct tms_retotdr_event_hdr ) +
	        sizeof(struct tms_retotdr_event_val ) * pevent_hdr->count +
	        sizeof(struct tms_retotdr_chain) +
	        sizeof(struct tms_cfg_otdr_ref_val);


	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_CFG_OTDR_REF, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&ref_hdr,    sizeof(struct tms_otdr_ref_hdr));
	glink_SendSerial(fd, (uint8_t *)&test_param, sizeof(struct tms_retotdr_test_param));
	glink_SendSerial(fd, (uint8_t *)&data_hdr,   sizeof(struct tms_retotdr_data_hdr));
	glink_SendSerial(fd, (uint8_t *)&data_val,   sizeof(struct tms_retotdr_data_val) * pdata_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&event_hdr,  sizeof(struct tms_retotdr_event_hdr));
	glink_SendSerial(fd, (uint8_t *)&event_val,  sizeof(struct tms_retotdr_event_val) * pevent_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&chain,      sizeof(struct tms_retotdr_chain));
	glink_SendSerial(fd, (uint8_t *)&ref_date,   sizeof(struct tms_cfg_otdr_ref_val));
	glink_SendTail(fd);
	return 0;
}

// ID_RET_OTDR_TEST
static int32_t tms_AnalyseCfgOTDRRef(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_otdr_ref_hdr       *pref_hdr;
	struct tms_retotdr_test_param *ptest_param;
	struct tms_retotdr_data_hdr   *pdata_hdr;
	struct tms_retotdr_data_val   *pdata_val;
	struct tms_retotdr_event_hdr  *pevent_hdr;
	struct tms_retotdr_event_val  *pevent_val;
	struct tms_retotdr_chain      *pchain;
	struct tms_cfg_otdr_ref_val   *pref_data;
	// register uint32_t *p32s, *p32d;
	// register uint16_t *p16s, *p16d;
	// register int loop;


	if ((uint32_t)(len - GLINK_OFFSET_DATA) < sizeof(struct tms_otdr_ref_hdr ) +
	    sizeof(struct tms_retotdr_test_param) +
	    sizeof(struct tms_retotdr_data_hdr  ) +
	    sizeof(struct tms_retotdr_data_val  ) +
	    sizeof(struct tms_retotdr_event_hdr ) +
	    sizeof(struct tms_retotdr_event_val ) +
	    sizeof(struct tms_retotdr_chain) ) {
		return -1;
	}


	pref_hdr   = (struct tms_otdr_ref_hdr *)(pdata + GLINK_OFFSET_DATA );
	ptest_param = (struct tms_retotdr_test_param *)(((char *)pref_hdr)   + sizeof(struct tms_otdr_ref_hdr));
	pdata_hdr   = (struct tms_retotdr_data_hdr *)(((char *)ptest_param) + sizeof(struct tms_retotdr_test_param));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pdata_hdr,
	         struct tms_retotdr_data_hdr,
	         struct tms_retotdr_data_val,
	         htonl(pdata_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pdata_val   = (struct tms_retotdr_data_val *)(((char *)pdata_hdr) + sizeof(struct tms_retotdr_data_hdr));
	pevent_hdr  = (struct tms_retotdr_event_hdr *)(((char *)pdata_val) + sizeof(struct tms_retotdr_data_val) * htonl(pdata_hdr->count));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pevent_hdr,
	         struct tms_retotdr_event_hdr,
	         struct tms_retotdr_event_val,
	         htonl(pevent_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pevent_val  = (struct tms_retotdr_event_val *)(((char *)pevent_hdr) + sizeof(struct tms_retotdr_event_hdr));
	pchain      = (struct tms_retotdr_chain *)(((char *)pevent_val) + sizeof(struct tms_retotdr_event_val) * htonl(pevent_hdr->count));
	pref_data   = (struct tms_cfg_otdr_ref_val *)(((char *)pchain) + sizeof(struct tms_retotdr_chain));

	// Part A.1
	pref_hdr->osw_frame  = htonl(pref_hdr->osw_frame);
	pref_hdr->osw_slot   = htonl(pref_hdr->osw_slot);
	pref_hdr->osw_type   = htonl(pref_hdr->osw_type);
	pref_hdr->osw_port   = htonl(pref_hdr->osw_port);
	// memcpy(pref_hdr->time, pref_hdr->time, 20);
	pref_hdr->otdr_port  = htonl(pref_hdr->otdr_port);

	printf("frame %d osw %d type %d port %d  otdr port %d\n",
	       pref_hdr->osw_frame  ,
	       pref_hdr->osw_slot   ,
	       pref_hdr->osw_type   ,
	       pref_hdr->osw_port   ,
	       pref_hdr->otdr_port  );


	// Part A.2
	tms_OTDRConv_tms_retotdr_test_param(ptest_param, ptest_param);
	// Part B.1
	tms_OTDRConv_tms_retotdr_data_hdr(pdata_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(pdata_val, pdata_val, pdata_hdr);
	// Part C.1
	tms_OTDRConv_tms_retotdr_event_hdr(pevent_hdr, pevent_hdr);
	tms_OTDRConv_tms_retotdr_event_val(pevent_val, pevent_val, pevent_hdr);
	tms_OTDRConv_tms_retotdr_chain(pchain, pchain);



	pref_data->leve0 = htonl(pref_data->leve0);
	pref_data->leve1 = htonl(pref_data->leve1);
	pref_data->leve2 = htonl(pref_data->leve2);


	if (pcontext->ptcb->pf_OnCfgOTDRRef) {
		pcontext->ptcb->pf_OnCfgOTDRRef(
		    pcontext,
		    ptest_param,
		    pdata_hdr,  pdata_val,
		    pevent_hdr, pevent_val,
		    pchain,
		    pref_data);
	}

	return 0;
}



int32_t tms_CfgMCUOSWCycle_any(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t count,
    struct tms_cfg_mcu_osw_cycle_val *oswCycleList,
    int cmdid)
{
	uint8_t *pmem;
	struct tms_cfg_mcu_osw_cycle      *pcfg_hdr;
	struct tms_cfg_mcu_osw_cycle_val  *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_cfg_mcu_osw_cycle) + count * sizeof(struct tms_cfg_mcu_osw_cycle_val);
	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	pcfg_hdr = (struct tms_cfg_mcu_osw_cycle *)(pmem);
	plist    = (struct tms_cfg_mcu_osw_cycle_val *)(pmem + sizeof(struct tms_cfg_mcu_osw_cycle));

	// Step 2.各字段复制
	pcfg_hdr->frame = htonl(frame);
	pcfg_hdr->slot  = htonl(slot);
	pcfg_hdr->type  = htonl(DEV_OSW);
	pcfg_hdr->count = htonl(count);


	// todo :防止循环count次溢出ptlist
	ptlist = oswCycleList;
	for (int i = 0; i < count; i++) {
		plist->port     = htonl(ptlist->port);
		plist->iscyc    = htonl(ptlist->iscyc);
		plist->interval = htonl(ptlist->interval);
		printf("port %d iscyc %d interval %d\n",
		       ptlist->port, ptlist->iscyc, ptlist->interval);
		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;


	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdid, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	free(pmem);
	return ret;
}

// 0x80000022
int32_t tms_AnalyseCfgMCUOSWCycle_any(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// uint8_t *pmem;
	struct tms_cfg_mcu_osw_cycle      *pcfg_hdr;
	struct tms_cfg_mcu_osw_cycle_val  *plist, *ptlist;

	// Step 1.分配内存并各指针指向相应内存

	pcfg_hdr = (struct tms_cfg_mcu_osw_cycle *)(pdata + GLINK_OFFSET_DATA );
	if ( !CHECK_PTR(
	         pcfg_hdr,
	         struct tms_cfg_mcu_osw_cycle,
	         struct tms_cfg_mcu_osw_cycle_val,
	         htonl(pcfg_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	plist    = (struct tms_cfg_mcu_osw_cycle_val *)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_cfg_mcu_osw_cycle));

	// Step 2.各字段复制
	pcfg_hdr->frame = htonl(pcfg_hdr->frame);
	pcfg_hdr->slot  = htonl(pcfg_hdr->slot);
	pcfg_hdr->type  = htonl(pcfg_hdr->type);
	pcfg_hdr->count = htonl(pcfg_hdr->count);


	// todo :防止循环count次溢出ptlist
	ptlist = plist;
	for (uint32_t i = 0; i < pcfg_hdr->count; i++) {
		ptlist->port     = htonl(ptlist->port);
		ptlist->iscyc    = htonl(ptlist->iscyc);
		ptlist->interval = htonl(ptlist->interval);

		ptlist++;
	}

	return 0;
}
/**
 * @brief	ID_CFG_MCU_OSW_CYCLE 0x80000022 网管发送OSW各光端口需要周期测试的光缆以及周期间隔到MCU
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	count struct tms_cfg_mcu_osw_cycle_val  数组个数
 * @param[in]	oswCycleList 指向一个 struct tms_cfg_mcu_osw_cycle_val  一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
int32_t tms_CfgMCUOSWCycle(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t count,
    struct tms_cfg_mcu_osw_cycle_val *oswCycleList)
{
	return tms_CfgMCUOSWCycle_any(fd, paddr, frame, slot, count, oswCycleList, ID_CFG_MCU_OSW_CYCLE);
}

// 0x80000022
int32_t tms_AnalyseCfgMCUOSWCycle(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseCfgMCUOSWCycle_any(pcontext, pdata, len);
}

#endif


/**
 * @brief	发送点名测量或周期测试测量
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port
 * @param[in]	val 指向一个 struct tms_getotdr_test_param
 * @param[in]	cmdID
 				ID_GET_OTDR_TEST		网管对OTDR点名测试，等效于tms_GetOTDRTest\n
 				ID_GET_OTDR_TEST_CYC	网管对OTDR周期性测试，等效于tms_GetOTDRTestCycle\n
 * @retval	null
 * @remarks
 * @see	tms_GetOTDRTest
 * @see tms_GetOTDRTestCycle
 */


int32_t tms_AnyGetOTDRTest(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type,
    int32_t port,
    int32_t otdr_port,
    struct tms_getotdr_test_param *val,
    int32_t cmdID)
{
	uint8_t *pmem;
	struct tms_getotdr_test_hdr    *phdr;
	struct tms_getotdr_test_param *potdr;
	uint8_t buf[ sizeof(struct tms_getotdr_test_hdr) + sizeof(struct tms_getotdr_test_param)];

	// Step 1.分配内存并各指针指向相应内存
	phdr  = (struct tms_getotdr_test_hdr *)buf;
	potdr = (struct tms_getotdr_test_param *)(buf + sizeof(struct tms_getotdr_test_hdr));

	// Step 2.各字段复制
	phdr->frame    = htonl(frame);
	phdr->slot     = htonl(slot);
	phdr->type     = htonl(type);
	phdr->port     = htonl(port);
	phdr->reserve0 = htonl(otdr_port);




	tms_OTDRConv_tms_retotdr_test_param(
	    (struct tms_retotdr_test_param *)potdr,
	    (struct tms_retotdr_test_param *)val);

	// Step 3. 发送
	struct glink_base  base_hdr;
	// int ret;

	pmem = (uint8_t *)buf;
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdID, sizeof(buf));
	glink_Send(fd, NULL, &base_hdr, pmem,    sizeof(buf));
	return 0;
}

//0x80000023
static int32_t tms_AnalyseAnyGetOTDRTest(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	//	if (pcontext->ptcb->pf_OnSpAnyGetOTDRTest) {
	//		pcontext->ptcb->pf_OnSpAnyGetOTDRTest(pcontext, pdata, len);
	//	}


	uint8_t *pval  = (uint8_t *)(pdata + GLINK_OFFSET_DATA);
	struct tms_getotdr_test_hdr    *phdr;
	struct tms_getotdr_test_param *potdr;


	// Step 1.分配内存并各指针指向相应内存
	phdr  = (struct tms_getotdr_test_hdr *)pval;
	potdr = (struct tms_getotdr_test_param *)(pval + sizeof(struct tms_getotdr_test_hdr));

	// Step 2.各字段复制
	phdr->frame    = htonl(phdr->frame);
	phdr->slot     = htonl(phdr->slot);
	phdr->type     = htonl(phdr->type);
	phdr->port     = htonl(phdr->port);




	tms_OTDRConv_tms_retotdr_test_param(
	    (struct tms_retotdr_test_param *)potdr,
	    (struct tms_retotdr_test_param *)potdr);

	return 0;
}

/**
 * @brief	ID_GET_OTDR_TEST 0x80000023 网管对OTDR点名测试
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port
 * @param[in]	val 指向一个 struct tms_getotdr_test_param
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks	<h2><center>文金朝注意 </center></h2>\n
	 tms_GetOTDRTest发送OTDR测试请求，OTDR返回测试结果，测试结果由函数tms_AnalyseRetOTDRTest（外部不可直接调用）分析，
	 分析结果转换成“本地字节序”后拷贝到用层缓冲中，数据流的格式按照struct tms_retotdr_test_hdr ~ struct tms_retotdr_chain
	 排列共7个部分，建议通过宏 TMS_PTR_OTDRTest 读取“本协议网络原始数据”，至于保存的数据文件按照你原有的算法处理。\n
 	TMS_PTR_OTDRTest(pdata, ptest_hdr, ptest_param, pdata_hdr, pdata_val, pevent_hdr, pevent_val, pchain)
 * @see	tms_RetOTDRTest
 */
int32_t tms_GetOTDRTest(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type,
    int32_t port,
    int32_t otdr_port,
    struct tms_getotdr_test_param *val)
{
	tms_AnyGetOTDRTest(fd, paddr, frame, slot, type, port, otdr_port, val, ID_GET_OTDR_TEST);
	return 0;
}
static int32_t tms_AnalyseGetOTDRTest(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{

	return tms_AnalyseAnyGetOTDRTest(pcontext, pdata, len);
}

/**
 * @brief	ID_GET_OTDR_TEST_CYC 0x80000039 网管对OTDR周期性测试
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port
 * @param[in]	val 指向一个 struct tms_getotdr_test_param
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks	<h2><center>文金朝注意 </center></h2>\n
	 tms_GetOTDRTest发送OTDR测试请求，OTDR返回测试结果，测试结果由函数tms_AnalyseRetOTDRTest（外部不可直接调用）分析，
	 分析结果转换成“本地字节序”后拷贝到用层缓冲中，数据流的格式按照struct tms_retotdr_test_hdr ~ struct tms_retotdr_chain
	 排列共7个部分，建议通过宏 TMS_PTR_OTDRTest 读取“本协议网络原始数据”，至于保存的数据文件按照你原有的算法处理。\n
 	TMS_PTR_OTDRTest(pdata, ptest_hdr, ptest_param, pdata_hdr, pdata_val, pevent_hdr, pevent_val, pchain)
 * @see	tms_RetOTDRTest
 */
int32_t tms_GetOTDRTestCycle(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type,
    int32_t port,
    int32_t otdr_port,
    struct tms_getotdr_test_param *val)
{
	tms_AnyGetOTDRTest(fd, paddr, frame, slot, type, port, otdr_port, val, ID_GET_OTDR_TEST_CYC);
	return 0;
}

void tms_Print_tms_alarm_line_hdr(struct tms_alarm_line_hdr *pval)
{
	printf("alarm: type %d Lv %d pos %d\n", pval->alarm_type, pval->alarm_level, pval->alarm_position);
	printf("alarm: f%2.2d/s%2.2d/p%2.2d\n", pval->frame, pval->slot, pval->port);
}
void tms_Print_tms_cfg_sms_val(struct tms_cfg_sms_val *pval, int count)
{
	char daty[7][5] = {
		"mon",
		"tues",
		"wed",
		"thur",
		"fri",
		"sat",
		"sun"
	};
	char type[5][10] = {
		"line",
		"power",
		"hw",
		"com",
		"olp",
	};
	char level[3][5] = {
		"lv0",
		"lv1",
		"lv2",
	};
	char lv_occor[1][10] = {"occur"};
	char lv_olp[4][10] = {
		"switch",
		"manual",
		"auto",
		"back",
	};

	printf("date   phone     type    level\n");
	for (int i = 0; i < count; i++) {
		if ((uint32_t)pval[i].time > 7 ||
		    (uint32_t)pval[i].type > 5) {
			printf("invaild param\n");
			break;
		}
		printf("%s  %s   %s    ", daty[pval[i].time - 1],
		       pval[i].phone,
		       type[pval[i].type - 1]);


		if ( (pval[i].type == SMSAT_LINE || pval[i].type == SMSAT_POWERDOWN) &&
		     (uint32_t)pval[i].level < 6) {

			printf("%s", level[pval[i].level - 1]);
		}
		else if ((pval[i].type == SMSAT_HW || pval[i].type == SMSAT_COMMM) &&
		         (uint32_t)pval[i].level < 1) {
			printf("%s", lv_occor[pval[i].level - 1]);
		}
		else if ((pval[i].type == SMSAT_OLP) &&
		         (uint32_t)pval[i].level < 4) {
			printf("%s", lv_olp[pval[i].level - 1]);
		}
		printf("\n");
	}

}
void tms_Print_tms_cfg_opm_ref_val(struct tms_cfg_opm_ref_val *pref_hdr, int count)
{
	printf("OPM ref\n");
	printf("    port  ism  wave  r_power   lv0    lv1  lv2\n");
	for (int i = 0; i < count; i++) {
		// pref_hdr->leve0 = 123;
		printf("%6.1d%5d  %6d%8.1f %6.1f%6.1f%6.1f\n",
		       pref_hdr->port,
		       pref_hdr->isminitor,
		       pref_hdr->wave,
		       pref_hdr->ref_power / 10.0,
		       pref_hdr->leve0 / 10.0,
		       pref_hdr->leve1 / 10.0,
		       pref_hdr->leve2 / 10.0);
		pref_hdr++;
	}
}
void tms_Print_tms_cfg_olp_ref_val(struct tms_cfg_olp_ref_val *pref_hdr, int count)
{
	printf("OLP ref\n");
	printf("    port  ism  wave  r_power   lv0    lv1  lv2\n");
	for (int i = 0; i < count; i++) {
		printf("%6.1d%5d  %6d%8.1f %6.1f%6.1f%6.1f\n",
		       pref_hdr->port,
		       pref_hdr->isminitor,
		       pref_hdr->wave,
		       pref_hdr->ref_power / 10.0,
		       pref_hdr->leve0 / 10.0,
		       pref_hdr->leve1 / 10.0,
		       pref_hdr->leve2 / 10.0);
		pref_hdr++;
	}
}

void tms_Print_tms_otdr_ref_hdr(struct tms_otdr_ref_hdr *pref_hdr)
{
	printf("osw : f%2.2d/s%2.2d/t%2.2d/p%2.2d\notdr: f%2.2d\n",
	       pref_hdr->osw_frame, pref_hdr->osw_slot, pref_hdr->osw_type, pref_hdr->osw_port,
	       pref_hdr->otdr_port);
}
void tms_Print_tms_getotdr_test_hdr(struct tms_getotdr_test_hdr *ptest_hdr)
{
	printf("f%2.2d/s%2.2d/t%2.2d/p%2.2d\n",
	       ptest_hdr->frame,
	       ptest_hdr->slot,
	       ptest_hdr->type,
	       ptest_hdr->port);
}
void tms_Print_tms_retotdr_test_hdr(struct tms_retotdr_test_hdr *ptest_hdr)
{
	printf("time: %s\nosw : f%2.2d/s%2.2d/t%2.2d/p%2.2d\notdr: f%2.2d/s%2.2d/t%2.2d/p%2.2d\n",
	       ptest_hdr->time,
	       ptest_hdr->osw_frame,
	       ptest_hdr->osw_slot,
	       ptest_hdr->osw_type,
	       ptest_hdr->osw_port,

	       ptest_hdr->otdr_frame,
	       ptest_hdr->otdr_slot,
	       ptest_hdr->otdr_type,
	       ptest_hdr->otdr_port
	      );
}
void tms_Print_tms_retotdr_test_param(struct tms_retotdr_test_param *ptest_param)
{
	printf("OTDR Param: ");
	if (ptest_param->rang > 1000) {
		printf("%2.2fKm/", (float)ptest_param->rang / 1000);
	}
	else {
		printf("%fM/", (float)ptest_param->rang);
	}

	if (ptest_param->pw < 1000) {
		printf("%2.2fns/%ds/", (float)(ptest_param->pw), ptest_param->time);
	}
	else if (ptest_param->pw < 1000000) {
		printf("%2.2fus/%ds/", (float)ptest_param->pw / 1000, ptest_param->time);
	}
	else {
		printf("%2.2fms/%ds/", (float)ptest_param->pw / 1000000, ptest_param->time);
	}

	printf("   %2.2fdB/Km /%2.2fdB/%2.2fdB/%dMHz\n",
	       ptest_param->gi, ptest_param->end_threshold, ptest_param->none_reflect_threshold,
	       ptest_param->sample / 1000000);
}

void tms_Print_tms_retotdr_event(struct tms_retotdr_event_hdr *pevent_hdr, struct tms_retotdr_event_val *pevent_val)
{
	// register uint32_t *pevent_hdr;

	// printf("len = %d-----\n", strlen((char*)pevent_hdr->eventid));
	// PrintfMemory((uint8_t*)pevent_hdr->eventid, 16);
	fecho("EventID: %s\n------------------------------------------------------------------------\n",
	      pevent_hdr->eventid);

	// printf("EventID: %s\n",pevent_hdr->eventid);
	// printf("\n------------------------------------------------------------------------\n");
	fecho("%s\t%s\t%8.12s\t%8.12s\t%8.12s\t%8.12s\n",
	      "dist", "type", "att", "lost", "ref", "link");
	fecho("------------------------------------------------------------------------\n");
	// p32d = (uint32_t*)pevent_val;

	struct tms_retotdr_event_val  *ptevent_val;
	ptevent_val = pevent_val;
	for (register int i = 0; i < pevent_hdr->count; i++) {
		fecho("%d\t%d\t%8.2f\t%8.2f\t%8.2f\t%8.2f\n",
		      ptevent_val->distance,
		      ptevent_val->event_type,
		      ptevent_val->att,
		      ptevent_val->loss,
		      ptevent_val->reflect,
		      ptevent_val->link_loss);

		ptevent_val++;
	}
	fecho("------------------------------------------------------------------------\n");
	fecho("                                                       Event count %3d\n", pevent_hdr->count);
	// printf("                                  Event count %d ID %s\n", pevent_hdr->count, pevent_hdr->eventid);
}

void tms_Print_tms_retotdr_chain(struct tms_retotdr_chain *pchain)
{
	printf("Chain inf %s\n", pchain->inf);
	printf("Chain: rang %0.4f\tloss %0.4f\tatt %0.4f\n",
	       pchain->range,
	       pchain->loss,
	       pchain->att);
}

void tms_Print_tms_cfg_otdr_ref_val(struct tms_cfg_otdr_ref_val *pref_data)
{
	printf("Ref lv0 %d lv1 %d lv2 %d\n", pref_data->leve0, pref_data->leve1, pref_data->leve2);
}
void tms_SaveOTDRData(
    struct tms_retotdr_test_hdr   *ptest_hdr,
    struct tms_retotdr_test_param *ptest_param,
    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,
    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,
    struct tms_retotdr_chain      *pchain,
    char *path,
    int32_t flag)
{
	// register uint32_t *p32s, *p32d;
	register uint16_t *p16s, *p16d;
	register int loop;

	loop = pdata_hdr->count;
	p16d = (uint16_t *)pdata_val;
	p16s = (uint16_t *)pdata_val;


	FILE *fp;
	float tmp;
	fp = fopen((char *)path, "w");


	for (register int i = 0; i < loop; i++) {
		tmp = (float) * p16d / 1000.0;
		fprintf(fp, "%f\n", tmp);
		p16d++;
		p16s++;
	}
	fclose(fp);
}


int32_t tms_SaveOTDRBin(
    struct tms_retotdr_test_hdr   *ptest_hdr,
    struct tms_retotdr_test_param *ptest_param,
    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,
    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,
    struct tms_retotdr_chain      *pchain,
    char *path)
{
	FILE *fp;
	// float tmp;
	// char *pbyte;

	if (NULL == ptest_hdr ||
	    NULL == ptest_param ||
	    NULL == pdata_hdr ||
	    NULL == pdata_val ||
	    NULL == pevent_hdr ||
	    NULL == pevent_val ||
	    NULL == pchain) {
		return -1;
	}
	fp = fopen((char *)path, "wb");
	// fwrite("abcdefg",1,7,fp);

	fwrite(ptest_hdr, 1, sizeof(struct tms_retotdr_test_hdr), fp);
	fwrite(ptest_param, 1, sizeof(struct tms_retotdr_test_param), fp);
	fwrite(pdata_hdr, 1, sizeof(struct tms_retotdr_data_hdr), fp);
	fwrite(pdata_val, 1, sizeof(struct tms_retotdr_data_val) * pdata_hdr->count, fp);
	fwrite(pevent_hdr, 1, sizeof(struct tms_retotdr_event_hdr), fp);
	fwrite(pevent_val, 1, sizeof(struct tms_retotdr_event_val) * pevent_hdr->count, fp);
	fwrite(pchain	, 1, sizeof(struct tms_retotdr_chain), fp);

	fclose(fp);
	return 0;
}

int32_t tms_OpenOTDRBin(
    char **pbuf,
    // struct tms_retotdr_test_hdr   **ptest_hdr,
    // struct tms_retotdr_test_param **ptest_param,
    // struct tms_retotdr_data_hdr   **pdata_hdr,
    // struct tms_retotdr_data_val   **pdata_val,
    // struct tms_retotdr_event_hdr  **pevent_hdr,
    // struct tms_retotdr_event_val  **pevent_val,
    // struct tms_retotdr_chain      **pchain,
    char *path)
{
	FILE *fp;
	// float tmp;
	// char *pbuf;
	int len;


	fp = fopen((char *)path, "rb");
	if (NULL == fp) {
		printf("open file %s fail\n", path);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	*pbuf = (char *)malloc(len);
	if (NULL == pbuf) {
		return -1;
	}
	fread(*pbuf, 1, len, fp);
	fclose(fp);


	return 0;
}

/**
 * @brief	转发点名测量或周期测试测量结果
 * @param	fd
 * @param	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param	ptest_hdr
 * @param	ptest_param

 * @param	pdata_hdr
 * @param	pdata_val

 * @param	pevent_hdr
 * @param	pevent_val
 * @param	cmdID 可以为
 			ID_RET_OTDR_TEST	MCU返回OTDR测量曲线，等效于 tms_RetOTDRTest \n
 			ID_RET_OTDR_TEST_CYC MCU返回OTDR周期性测量曲线，等效于 tms_RetOTDRTestCycle


 * @retval	null
 * @remarks
 * @see	tms_RetOTDRTest
 * @see	tms_RetOTDRTestCycle
 */

int32_t tms_AnyRetOTDRTest(
    int fd,
    struct glink_addr *paddr,
    struct tms_retotdr_test_hdr   *ptest_hdr,
    struct tms_retotdr_test_param *ptest_param,

    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,

    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,

    struct tms_retotdr_chain      *pchain,
    int32_t cmdID)
{

	struct tms_retotdr_test_hdr   test_hdr;
	struct tms_retotdr_test_param test_param;
	struct tms_retotdr_data_hdr   data_hdr;
	struct tms_retotdr_data_val   data_val[OTDR_SAMPLE_32000];
	struct tms_retotdr_event_hdr  event_hdr;
	struct tms_retotdr_event_val  event_val[OTDR_EVENT_MAX];
	struct tms_retotdr_chain      chain;
	// register int32_t *p32s, *p32d;
	// register int16_t *p16s, *p16d;
	// register int loop;


	// 打印信息
	printf("------------------------------------------------------------------------\n");
	tms_Print_tms_retotdr_test_hdr(ptest_hdr);
	tms_Print_tms_retotdr_test_param(ptest_param);
	printf("------------------------------------------------------------------------\n");


	// Part A.1
	test_hdr.osw_frame  = htonl(ptest_hdr->osw_frame);
	test_hdr.osw_slot   = htonl(ptest_hdr->osw_slot);
	test_hdr.osw_type   = htonl(DEV_OSW);
	test_hdr.osw_port   = htonl(ptest_hdr->osw_port);
	memcpy(test_hdr.time, ptest_hdr->time, 20);
	test_hdr.otdr_frame = htonl(ptest_hdr->otdr_frame);
	test_hdr.otdr_slot  = htonl(ptest_hdr->otdr_slot);
	test_hdr.otdr_type  = htonl(DEV_OTDR);
	test_hdr.otdr_port  = htonl(ptest_hdr->otdr_port);

	tms_OTDRConv_tms_retotdr_test_param(&test_param, ptest_param);


	// // Part B.1
	if (pdata_hdr->count > OTDR_SAMPLE_32000) {
		pdata_hdr->count = OTDR_SAMPLE_32000;
	}
	tms_OTDRConv_tms_retotdr_data_hdr(&data_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(&data_val[0], pdata_val, pdata_hdr);
	// // Part C.1
	pevent_hdr->count = pevent_hdr->count & 0x3ff;				// 限定loop在0~1024以内
	tms_OTDRConv_tms_retotdr_event_hdr(&event_hdr, pevent_hdr);
	// Part C.2

	tms_OTDRConv_tms_retotdr_event_val(&event_val[0], pevent_val, pevent_hdr);

	tms_OTDRConv_tms_retotdr_chain(&chain, pchain);


	// Step 3. 发送
	struct glink_base  base_hdr;
	// int ret;
	int len;
	len = 	sizeof(struct tms_retotdr_test_hdr  ) +
	        sizeof(struct tms_retotdr_test_param) +
	        sizeof(struct tms_retotdr_data_hdr  ) +
	        sizeof(struct tms_retotdr_data_val  ) * pdata_hdr->count +
	        sizeof(struct tms_retotdr_event_hdr ) +
	        sizeof(struct tms_retotdr_event_val ) * pevent_hdr->count +
	        sizeof(struct tms_retotdr_chain);

	if(paddr == NULL) {
		base_hdr.src = TMS_DEFAULT_LOCAL_ADDR;
		base_hdr.dst = TMS_DEFAULT_RMOTE_ADDR;
	}
	else {
		base_hdr.src = paddr->src;
		base_hdr.dst = paddr->dst;

	}

	glink_Build(&base_hdr, cmdID, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&test_hdr,   sizeof(struct tms_retotdr_test_hdr));
	glink_SendSerial(fd, (uint8_t *)&test_param, sizeof(struct tms_retotdr_test_param));
	glink_SendSerial(fd, (uint8_t *)&data_hdr,   sizeof(struct tms_retotdr_data_hdr));
	glink_SendSerial(fd, (uint8_t *)&data_val,   sizeof(struct tms_retotdr_data_val) * pdata_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&event_hdr,  sizeof(struct tms_retotdr_event_hdr));
	glink_SendSerial(fd, (uint8_t *)&event_val,  sizeof(struct tms_retotdr_event_val) * pevent_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&chain,      sizeof(struct tms_retotdr_chain));
	glink_SendTail(fd);

	return 0;
}


// ID_RET_OTDR_TEST
static int32_t tms_AnalyseAnyRetOTDRTest(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_retotdr_test_hdr   *ptest_hdr;
	struct tms_retotdr_test_param *ptest_param;
	struct tms_retotdr_data_hdr   *pdata_hdr;
	struct tms_retotdr_data_val   *pdata_val;
	struct tms_retotdr_event_hdr  *pevent_hdr;
	struct tms_retotdr_event_val  *pevent_val;
	struct tms_retotdr_chain      *pchain;



	if ((uint32_t)(len - GLINK_OFFSET_DATA) < sizeof(struct tms_retotdr_test_hdr ) +
	    sizeof(struct tms_retotdr_test_param) +
	    sizeof(struct tms_retotdr_data_hdr  ) +
	    sizeof(struct tms_retotdr_data_val  ) +
	    sizeof(struct tms_retotdr_event_hdr ) +
	    sizeof(struct tms_retotdr_event_val ) +
	    sizeof(struct tms_retotdr_chain) ) {
		return -1;
	}


	ptest_hdr   = (struct tms_retotdr_test_hdr *)(pdata + GLINK_OFFSET_DATA );
	ptest_param = (struct tms_retotdr_test_param *)(((char *)ptest_hdr)   + sizeof(struct tms_retotdr_test_hdr));
	pdata_hdr   = (struct tms_retotdr_data_hdr *)(((char *)ptest_param) + sizeof(struct tms_retotdr_test_param));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pdata_hdr,
	         struct tms_retotdr_data_hdr,
	         struct tms_retotdr_data_val,
	         htonl(pdata_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pdata_val   = (struct tms_retotdr_data_val *)(((char *)pdata_hdr) + sizeof(struct tms_retotdr_data_hdr));
	pevent_hdr  = (struct tms_retotdr_event_hdr *)(((char *)pdata_val) + sizeof(struct tms_retotdr_data_val) * htonl(pdata_hdr->count));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pevent_hdr,
	         struct tms_retotdr_event_hdr,
	         struct tms_retotdr_event_val,
	         htonl(pevent_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pevent_val  = (struct tms_retotdr_event_val *)(((char *)pevent_hdr) + sizeof(struct tms_retotdr_event_hdr));
	pchain      = (struct tms_retotdr_chain *)(((char *)pevent_val) + sizeof(struct tms_retotdr_event_val) * htonl(pevent_hdr->count));


	PrintfMemory((uint8_t *)ptest_hdr, sizeof(struct tms_retotdr_test_hdr));

	// Part A.1
	ptest_hdr->osw_frame  = htonl(ptest_hdr->osw_frame);
	ptest_hdr->osw_slot   = htonl(ptest_hdr->osw_slot);
	ptest_hdr->osw_type   = htonl(ptest_hdr->osw_type);
	ptest_hdr->osw_port   = htonl(ptest_hdr->osw_port);
	ptest_hdr->otdr_frame = htonl(ptest_hdr->otdr_frame);
	ptest_hdr->otdr_slot  = htonl(ptest_hdr->otdr_slot);
	ptest_hdr->otdr_type  = htonl(ptest_hdr->otdr_type);
	ptest_hdr->otdr_port  = htonl(ptest_hdr->otdr_port);

	// Part A.2
	tms_OTDRConv_tms_retotdr_test_param(ptest_param, ptest_param);
	// Part B.1
	tms_OTDRConv_tms_retotdr_data_hdr(pdata_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(pdata_val, pdata_val, pdata_hdr);
	// Part C.1
	tms_OTDRConv_tms_retotdr_event_hdr(pevent_hdr, pevent_hdr);
	tms_OTDRConv_tms_retotdr_event_val(pevent_val, pevent_val, pevent_hdr);
	tms_OTDRConv_tms_retotdr_chain(pchain, pchain);


	// MCU 帮OTDR填写测试时间
	if ( '\0' == ptest_hdr->time[0] ) {
		//    time_t now;

		// struct tm *timenow;
		// time(&now);
		// timenow = localtime(&now);

		// sprintf((char*) ptest_hdr->time,"%2.2d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d",
		// 	timenow->tm_yday + 1798,timenow->tm_mon + 1,timenow->tm_mday,
		// 	timenow->tm_hour , timenow->tm_min , timenow->tm_sec);

		time_t now;
		struct tm *ptr ;
		//    char szBuffer[64];
		const char *pFormat = "%Y-%m-%d %H:%M:%S";
		now = time(NULL);
		ptr = localtime(&now);
		strftime( (char *)ptest_hdr->time, (size_t)20, pFormat, ptr);
	}



	if (pcontext->ptcb->pf_OnRetOTDRTest) {
		pcontext->ptcb->pf_OnRetOTDRTest(
		    pcontext,
		    ptest_hdr,  ptest_param,
		    pdata_hdr,  pdata_val,
		    pevent_hdr, pevent_val,
		    pchain);
	}
	return 0;
}


/**
 * @brief	ID_RET_OTDR_TEST 0x80000024 MCU返回OTDR测量曲线
 * @param	fd
 * @param	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param	ptest_hdr
 * @param	ptest_param

 * @param	pdata_hdr
 * @param	pdata_val

 * @param	pevent_hdr
 * @param	pevent_val

struct tms_retotdr_chain    *pchain
 * @retval	null
 * @remarks	<h2><center>文金朝注意 </center></h2>\n
	 tms_GetOTDRTest发送OTDR测试请求，OTDR返回测试结果，测试结果由函数tms_AnalyseRetOTDRTest（外部不可直接调用）分析，
	 分析结果转换成“本地字节序”后拷贝到用层缓冲中，数据流的格式按照struct tms_retotdr_test_hdr ~ struct tms_retotdr_chain
	 排列共7个部分，建议通过宏 TMS_PTR_OTDRTest 读取“本协议网络原始数据”，至于保存的数据文件按照你原有的算法处理。\n
 	TMS_PTR_OTDRTest(pdata, ptest_hdr, ptest_param, pdata_hdr, pdata_val, pevent_hdr, pevent_val, pchain)
 * @see	tms_GetOTDRTest\nTMS_PTR_OTDRTest\ntms_retotdr_test_hdr\ntms_retotdr_test_param\ntms_retotdr_data_hdr\n
 tms_retotdr_data_val\n tms_retotdr_event_hdr\n tms_retotdr_event_val\n tms_retotdr_chain\n
 */
int32_t tms_RetOTDRTest(
    int fd,
    struct glink_addr *paddr,
    struct tms_retotdr_test_hdr   *ptest_hdr,
    struct tms_retotdr_test_param *ptest_param,

    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,

    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,

    struct tms_retotdr_chain      *pchain)
{
	return tms_AnyRetOTDRTest(fd, paddr, ptest_hdr, ptest_param,
	                          pdata_hdr, pdata_val, pevent_hdr, pevent_val, pchain, ID_RET_OTDR_TEST);
}

static int32_t tms_AnalyseRetOTDRTest(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseAnyRetOTDRTest(pcontext, pdata, len);
	return 0;
}

/**
 * @brief	ID_RET_OTDR_TEST_CYC 0x80000040 MCU返回OTDR周期性测量曲线
 * @param	fd
 * @param	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param	ptest_hdr
 * @param	ptest_param

 * @param	pdata_hdr
 * @param	pdata_val

 * @param	pevent_hdr
 * @param	pevent_val

struct tms_retotdr_chain    *pchain
 * @retval	null
 * @remarks	<h2><center>文金朝注意 </center></h2>\n
	 tms_GetOTDRTest发送OTDR测试请求，OTDR返回测试结果，测试结果由函数tms_AnalyseRetOTDRTest（外部不可直接调用）分析，
	 分析结果转换成“本地字节序”后拷贝到用层缓冲中，数据流的格式按照struct tms_retotdr_test_hdr ~ struct tms_retotdr_chain
	 排列共7个部分，建议通过宏 TMS_PTR_OTDRTest 读取“本协议网络原始数据”，至于保存的数据文件按照你原有的算法处理。\n
 	TMS_PTR_OTDRTest(pdata, ptest_hdr, ptest_param, pdata_hdr, pdata_val, pevent_hdr, pevent_val, pchain)
 * @see	tms_GetOTDRTest\nTMS_PTR_OTDRTest\ntms_retotdr_test_hdr\ntms_retotdr_test_param\ntms_retotdr_data_hdr\n
 tms_retotdr_data_val\n tms_retotdr_event_hdr\n tms_retotdr_event_val\n tms_retotdr_chain\n
 */
int32_t tms_RetOTDRTestCycle(
    int fd,
    struct glink_addr *paddr,
    struct tms_retotdr_test_hdr   *ptest_hdr,
    struct tms_retotdr_test_param *ptest_param,

    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,

    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,

    struct tms_retotdr_chain      *pchain)
{
	return tms_AnyRetOTDRTest(fd, paddr, ptest_hdr, ptest_param,
	                          pdata_hdr, pdata_val, pevent_hdr, pevent_val, pchain, ID_RET_OTDR_TEST_CYC);
}

static int32_t tms_AnalyseRetOTDRTestCycle(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseAnyRetOTDRTest(pcontext, pdata, len);
	return 0;
}

/**
* @brief	ID_CFG_OLP_MODE 0x80000025 网管对OLP模块的工作模式和返回时间设定
* @param[in]	fd 套接字文件描述符
* @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
				修改tmsxx.h设定
* @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
* @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
* @param[in]	mode 切换模式，可以是下面的值\n
				OLP_SWITCH_MODE_UNBACK 0 保护不返回\n
				OLP_SWITCH_MODE_BACK 1 表示保护返回
* @param[in]	backtime 返回时间，单位分钟
* @param[in]	protect 切换模式，可以是下面的值\n
				1 表示保护线路为主路\n
				2 表示保护线路为备路
* @retval	>0 发送成功
* @retval	0 发送失败，该链接失效，立即断开
* @remarks
* @see
*/
// 废除,tms_CfgOLPModeEx代替
int32_t tms_CfgOLPMode(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t mode,
    int32_t backtime,
    int32_t protect,
    int32_t sw_gate)
{
	uint8_t *pmem;
	struct tms_cfg_olp_mode olpmode;

	printf("!!!!!!!!!!!!已经作废函数!!!!!!!!!!!!!\n");
	// Step 2.各字段复制
	// olpmode.frame    = htonl(frame);
	// olpmode.slot     = htonl(slot);
	// olpmode.type     = htonl(DEV_OLP);
	// olpmode.mode     = htonl(mode);
	// olpmode.backtime = htonl(backtime);
	// olpmode.protect  = htonl(protect);
	// olpmode.sw_gate  = htonl(sw_gate);

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	pmem = (uint8_t *)&olpmode;
	glink_Build(&base_hdr, ID_CFG_OLP_MODE, sizeof(struct tms_cfg_olp_mode));
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_cfg_olp_mode));
	return ret;
}
// 0x80000025
int32_t tms_CfgOLPModeEx(
    int fd,
    struct glink_addr *paddr,
    struct tms_cfg_olp_mode *pval)
{
	uint8_t *pmem;
	struct tms_cfg_olp_mode olpmode;

	// Step 2.各字段复制
	// olpmode.frame    = htonl(pval->frame);
	// olpmode.slot     = htonl(pval->slot);
	// olpmode.type     = htonl(pval->type);
	// olpmode.flag     = htonl(pval->flag);
	// olpmode.mode     = htonl(pval->mode);
	// olpmode.backtime = htonl(pval->backtime);
	// olpmode.protect  = htonl(pval->protect);
	// olpmode.sw_gate  = htonl(pval->sw_gate);


	olpmode.frame        = htonl(pval->frame);
	olpmode.slot         = htonl(pval->slot);
	olpmode.type         = htonl(pval->type);
	olpmode.flag         = htonl(pval->flag);
	olpmode.mode         = htonl(pval->mode);
	olpmode.back_time    = htonl(pval->back_time);
	olpmode.protect_port = htonl(pval->protect_port);
	olpmode.work_line    = htonl(pval->work_line);
	olpmode.sw_level1    = htonl(pval->sw_level1);
	olpmode.sw_level2    = htonl(pval->sw_level2);
	olpmode.tx_ref       = htonl(pval->tx_ref);
	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	pmem = (uint8_t *)&olpmode;
	glink_Build(&base_hdr, ID_CFG_OLP_MODE, sizeof(struct tms_cfg_olp_mode));
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_cfg_olp_mode));
	return ret;
}

int32_t tms_AnalyseCfgOSWMode(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// tms_Transmit2Dev(pcontext, pdata, len);
	return 0;
}
/**
 * @brief	ID_CMD_OLP_SWITCH 0x80000026 网管对OLP模块的指令倒换命令
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	sw_port	倒换到主路/备路\n
		OLP_SWITCH_A        0	切换到 A 路\n
		OLP_SWITCH_B        1	切换到 B 路

 * @param	null
 * @retval	null
 * @remarks
 * @see
 */
int32_t tms_MCU_OLPSwitch(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t sw_port)
{
	// 设备类型内部自己完成
	return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OLP, sw_port, ID_CMD_OLP_SWITCH, sizeof(struct tms_dev_port));
}

int32_t tms_MCU_AnalyseOLPSwitch(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// tms_Transmit2Dev(pcontext, pdata, len);
	return 0;
}
/**
 * @brief	ID_REPORT_OLP_ACTION 0x80000027 MCU上报OLP人工切换或自动切换的动作信息
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	sw_type	倒换类型\n
		OLP_SWITCH_ACTION_PERSION  0	人工切换\n
		OLP_SWITCH_ACTION_AUTO     1	自动保护倒换\n
		OLP_SWITCH_ACTION_BACK     2	保护返回\n
 * @param[in]	sw_port	倒换到主路/备路\n
		OLP_SWITCH_ACTION_MASTER       0	切换到主光路\n
		OLP_SWITCH_ACTION_SLAVE        1	切换到备光路
 * @param	null
 * @retval	null
 * @remarks
 * @see
 */
int32_t tms_ReportOLPAction(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t sw_type,
    int32_t sw_port)
{
	uint8_t *pmem;
	struct tms_report_olp_action olpaction;

	// Step 2.各字段复制
	olpaction.frame    = htonl(frame);
	olpaction.slot     = htonl(slot);
	olpaction.type     = htonl(DEV_OLP);
	olpaction.sw_type  = htonl(sw_type);
	olpaction.sw_port  = htonl(sw_port);

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	pmem = (uint8_t *)&olpaction;
	tms_FillGlinkFrame(&base_hdr, paddr);

	glink_Build(&base_hdr, ID_REPORT_OLP_ACTION, sizeof(struct tms_report_olp_action));
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		ret =  tms_SendAllManager(&base_hdr, pmem, sizeof(struct tms_report_olp_action));
		return ret;
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_report_olp_action));
	return ret;
}

// 0x80000027
int32_t tms_ReportOLPActionEx(
    int fd,
    struct glink_addr *paddr,
    struct tms_report_olp_action *val)
{
	uint8_t *pmem;
	struct tms_report_olp_action olpaction;

	// Step 2.各字段复制
	olpaction.frame    = htonl(val->frame);
	olpaction.slot     = htonl(val->slot);
	olpaction.type     = htonl(DEV_OLP);
	olpaction.sw_type  = htonl(val->sw_type);
	olpaction.sw_port  = htonl(val->sw_port);
	memcpy(olpaction.time, val->time, 20);

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	pmem = (uint8_t *)&olpaction;
	tms_FillGlinkFrame(&base_hdr, paddr);

	glink_Build(&base_hdr, ID_REPORT_OLP_ACTION, sizeof(struct tms_report_olp_action));
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		ret =  tms_SendAllManager(&base_hdr, pmem, sizeof(struct tms_report_olp_action));
		return ret;
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_report_olp_action));
	return ret;
}



int32_t tms_AnalyseReportOLPAction(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_report_olp_action *pval;
	pval  = (struct tms_report_olp_action *)(pdata + GLINK_OFFSET_DATA);

	// Step 2.各字段复制
	pval->frame    = htonl(pval->frame);
	pval->slot     = htonl(pval->slot);
	pval->type     = htonl(pval->type);
	pval->sw_type  = htonl(pval->sw_type);
	pval->sw_port  = htonl(pval->sw_port);
	printf("tms_AnalyseReportOLPAction()\n");
	printf("val:f%d/s%d/t%d\n", pval->frame, pval->slot, pval->type);
	printf("\tsw_type: %d sw_port: %d\n", pval->sw_type, pval->sw_port);

	return 0;
}
/**
* @brief	ID_ALARM_OPM 0x80000028 MCU上报某槽位总的光功率告警
* @param[in]	fd 套接字文件描述符
* @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
				修改tmsxx.h设定
* @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
* @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
* @param[in]	count struct tms_alarm_opm_val  数组个数
* @param[in]	list 指向一个 struct tms_alarm_opm_val  一维数组
* @retval	>0 发送成功
* @retval	0 发送失败，该链接失效，立即断开
* @remarks
* @see
*/
// 作废，用tms_AlarmOPMEx代替
int32_t tms_AlarmOPM(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t count,
    struct tms_alarm_opm_val *list)
{
	uint8_t *pmem;
	struct tms_alarm_opm      *phdr;
	struct tms_alarm_opm_val  *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_alarm_opm) + count * sizeof(struct tms_alarm_opm_val);

	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	phdr  = (struct tms_alarm_opm *)(pmem);
	plist = (struct tms_alarm_opm_val *)(pmem + sizeof(struct tms_alarm_opm));

	// Step 2.各字段复制
	// phdr->alarm_type  = htonl(alarm_type);
	phdr->alarm_type  = htonl(1);//1表示光功率告警，写死
	phdr->frame       = htonl(frame);
	phdr->slot        = htonl(slot);
	phdr->count       = htonl(count);
	// phdr->type		  = htonl(DEV_OPM);

	// todo :防止循环count次溢出ptlist
	ptlist = list;
	for (int i = 0; i < count; i++) {
		plist->port   = htonl(ptlist->port);
		plist->levelx = htonl(ptlist->levelx);
		plist->power  = htonl(ptlist->power);
		memcpy(plist->time, ptlist->time, 20);
		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);

	glink_Build(&base_hdr, ID_ALARM_OPM, len);
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		ret = tms_SendAllManager(&base_hdr, pmem, len);
		goto _Free;
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
_Free:
	;
	free(pmem);
	return ret;
}

int32_t tms_AlarmOPMEx(
    int fd,
    struct glink_addr *paddr,
    struct tms_alarm_opm      *hdr,
    struct tms_alarm_opm_val  *list)
{
	uint8_t *pmem;
	struct tms_alarm_opm      *phdr;
	struct tms_alarm_opm_val  *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_alarm_opm) + hdr->count * sizeof(struct tms_alarm_opm_val);

	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	phdr  = (struct tms_alarm_opm *)(pmem);
	plist = (struct tms_alarm_opm_val *)(pmem + sizeof(struct tms_alarm_opm));

	// Step 2.各字段复制
	// phdr->alarm_type  = htonl(alarm_type);
	phdr->alarm_type  = htonl(1);//1表示光功率告警，写死
	phdr->frame       = htonl(hdr->frame);
	phdr->slot        = htonl(hdr->slot);
	phdr->count       = htonl(hdr->count);
	// phdr->type		  = htonl(hdr->type);

	// todo :防止循环count次溢出ptlist
	ptlist = list;
	for (int i = 0; i < hdr->count; i++) {
		plist->port   = htonl(ptlist->port);
		plist->levelx = htonl(ptlist->levelx);
		plist->power  = htonl(ptlist->power);
		memcpy(plist->time, ptlist->time, 20);
		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);

	glink_Build(&base_hdr, ID_ALARM_OPM, len);
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		ret = tms_SendAllManager(&base_hdr, pmem, len);
		goto _Free;
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
_Free:
	;
	free(pmem);
	return ret;
}

/**
 * @brief	ID_ALARM_OPM_CHANGE 0x80000029 MCU上报某槽位变化的光功率告警
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	alarm_cnt 有多少个端口正处于有告警的状态
 * @param[in]	count struct tms_alarm_opm_val  数组个数
 * @param[in]	list 指向一个 struct tms_alarm_opm_val  一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
// 作废， 用tms_AlarmOPMChangeEx代替
int32_t tms_AlarmOPMChange(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    // int     alarm_type,
    int 	alarm_cnt,
    int32_t count,
    struct  tms_alarm_opm_val *list)
{
	uint8_t *pmem;
	struct tms_alarm_opm_change      *phdr;
	struct tms_alarm_opm_val         *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_alarm_opm_change) + count * sizeof(struct tms_alarm_opm_val);

	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	phdr  = (struct tms_alarm_opm_change *)(pmem);
	plist = (struct tms_alarm_opm_val *)(pmem + sizeof(struct tms_alarm_opm_change));

	// Step 2.各字段复制
	// phdr->alarm_type  = htonl(alarm_type);
	phdr->alarm_type  = htonl(1);//1表示光功率告警，写死
	phdr->frame       = htonl(frame);
	phdr->slot        = htonl(slot);
	phdr->alarm_cnt   = htonl(alarm_cnt);
	phdr->count       = htonl(count);
	printf("count :%d\n", phdr->count);

	// todo :防止循环count次溢出ptlist
	ptlist = list;
	for (int i = 0; i < count; i++) {
		plist->port   = htonl(ptlist->port);
		plist->levelx = htonl(ptlist->levelx);
		plist->power  = htonl(ptlist->power);
		memcpy(plist->time, ptlist->time, 20);

		printf("\tport %d Lvx %d power %d times %s\n", ptlist->port, ptlist->levelx, ptlist->power, ptlist->time);

		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);

	glink_Build(&base_hdr, ID_ALARM_OPM_CHANGE, len);
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		return tms_SendAllManager(&base_hdr, pmem, len);
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);

	free(pmem);
	return ret;
}

//0x80000029
int32_t tms_AlarmOPMChangeEx(
    int     fd,
    struct glink_addr *paddr,
    struct tms_alarm_opm_change *hdr,
    struct  tms_alarm_opm_val *list)
{
	uint8_t *pmem;
	struct tms_alarm_opm_change      *phdr;
	struct tms_alarm_opm_val         *plist, *ptlist;
	int len;

	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_alarm_opm_change) + hdr->count * sizeof(struct tms_alarm_opm_val);

	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	phdr  = (struct tms_alarm_opm_change *)(pmem);
	plist = (struct tms_alarm_opm_val *)(pmem + sizeof(struct tms_alarm_opm_change));

	// Step 2.各字段复制
	// phdr->alarm_type  = htonl(alarm_type);
	phdr->alarm_type  = htonl(1);//1表示光功率告警，写死
	phdr->frame       = htonl(hdr->frame);
	phdr->slot        = htonl(hdr->slot);
	//	phdr->type        = htonl(hdr->type);
	phdr->alarm_cnt   = htonl(hdr->alarm_cnt);
	phdr->count       = htonl(hdr->count);
	printf("count :%d\n", phdr->count);

	// todo :防止循环count次溢出ptlist
	ptlist = list;
	for (int i = 0; i < hdr->count; i++) {
		plist->port   = htonl(ptlist->port);
		plist->levelx = htonl(ptlist->levelx);
		plist->power  = htonl(ptlist->power);
		memcpy(plist->time, ptlist->time, 20);

		printf("\tport %d Lvx %d power %d times %s\n", ptlist->port, ptlist->levelx, ptlist->power, ptlist->time);

		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);

	glink_Build(&base_hdr, ID_ALARM_OPM_CHANGE, len);
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		return tms_SendAllManager(&base_hdr, pmem, len);
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);

	free(pmem);
	return ret;
}


/**
 * @brief	ID_ALARM_LINE			0x80000030	MCU返回线路告警
 * @param	fd
 * @param	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param	palarm 告警头
 * @param	ptest_hdr OTDR返回数据头
 * @param	ptest_param OTDR返回测试曲线参数

 * @param	pdata_hdr OTDR 测试曲线头
 * @param	pdata_val OTDR 测试采样点

 * @param	pevent_hdr OTDR 事件列表头
 * @param	pevent_val OTDR 事件内容
 * @param	pchain  曲线链路信息
 * @param	pchain  曲线链路信息
 * @param	cmdID 命令ID，这里固定填死 ID_ALARM_LINE
 * @remarks
 * @see
 */

int32_t tms_AlarmLine(
    int     fd,
    struct glink_addr *paddr,
    struct tms_alarm_line_hdr     *palarm,
    struct tms_retotdr_test_hdr   *ptest_hdr,
    struct tms_retotdr_test_param *ptest_param,

    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,

    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,

    struct tms_retotdr_chain      *pchain,
    int32_t cmdID)
{
	struct tms_alarm_line_hdr     alarm;
	struct tms_retotdr_test_hdr   test_hdr;
	struct tms_retotdr_test_param test_param;
	struct tms_retotdr_data_hdr   data_hdr;
	struct tms_retotdr_data_val   data_val[OTDR_SAMPLE_32000];
	struct tms_retotdr_event_hdr  event_hdr;
	struct tms_retotdr_event_val  event_val[OTDR_EVENT_MAX];
	struct tms_retotdr_chain      chain;
	// register int32_t *p32s, *p32d;
	// register int16_t *p16s, *p16d;
	// register int loop;


	// 打印信息
	printf("------------------------------------------------------------------------\n");
	tms_Print_tms_retotdr_test_hdr(ptest_hdr);
	tms_Print_tms_retotdr_test_param(ptest_param);
	printf("------------------------------------------------------------------------\n");


	alarm.alarm_type = htonl(palarm->alarm_type);
	alarm.alarm_level = htonl(palarm->alarm_level);
	alarm.frame = htonl(palarm->frame);
	alarm.slot = htonl(palarm->slot);
	alarm.port = htonl(palarm->port);
	alarm.alarm_position = htonl(palarm->alarm_position);
	memcpy(alarm.time, palarm->time, 20);
	alarm.reserve0 = 0;
	// Part A.1 以后这个头可能需要和 tms_AnyRetOTDRTest 整合
	test_hdr.osw_frame  = htonl(ptest_hdr->osw_frame);
	test_hdr.osw_slot   = htonl(ptest_hdr->osw_slot);
	test_hdr.osw_type   = htonl(DEV_OSW);
	test_hdr.osw_port   = htonl(ptest_hdr->osw_port);
	memcpy(test_hdr.time, ptest_hdr->time, 20);
	test_hdr.otdr_frame = htonl(ptest_hdr->otdr_frame);
	test_hdr.otdr_slot  = htonl(ptest_hdr->otdr_slot);
	test_hdr.otdr_type  = htonl(DEV_OTDR);
	test_hdr.otdr_port  = htonl(ptest_hdr->otdr_port);

	tms_OTDRConv_tms_retotdr_test_param(&test_param, ptest_param);


	// // Part B.1
	if (pdata_hdr->count > OTDR_SAMPLE_32000) {
		pdata_hdr->count = OTDR_SAMPLE_32000;
	}
	tms_OTDRConv_tms_retotdr_data_hdr(&data_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(&data_val[0], pdata_val, pdata_hdr);
	// // Part C.1
	pevent_hdr->count = pevent_hdr->count & 0x3ff;				// 限定loop在0~1024以内
	tms_OTDRConv_tms_retotdr_event_hdr(&event_hdr, pevent_hdr);
	// Part C.2

	tms_OTDRConv_tms_retotdr_event_val(&event_val[0], pevent_val, pevent_hdr);

	tms_OTDRConv_tms_retotdr_chain(&chain, pchain);


	// Step 3. 发送
	struct glink_base  base_hdr;
	// int ret;
	int len;
	len = 	sizeof(struct tms_alarm_line_hdr)     +
	        sizeof(struct tms_retotdr_test_hdr  ) +
	        sizeof(struct tms_retotdr_test_param) +
	        sizeof(struct tms_retotdr_data_hdr  ) +
	        sizeof(struct tms_retotdr_data_val  ) * pdata_hdr->count +
	        sizeof(struct tms_retotdr_event_hdr ) +
	        sizeof(struct tms_retotdr_event_val ) * pevent_hdr->count +
	        sizeof(struct tms_retotdr_chain);



	tms_FillGlinkFrame(&base_hdr, paddr);
	glink_Build(&base_hdr, cmdID, len);
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		return tms_SendAllManagerDot(&base_hdr,
		                             8,
		                             NULL,
		                             (uint8_t *)&alarm,      sizeof(struct tms_alarm_line_hdr),
		                             (uint8_t *)&test_hdr,   sizeof(struct tms_retotdr_test_hdr),
		                             (uint8_t *)&test_param, sizeof(struct tms_retotdr_test_param),
		                             (uint8_t *)&data_hdr,   sizeof(struct tms_retotdr_data_hdr),
		                             (uint8_t *)&data_val,   sizeof(struct tms_retotdr_data_val) * pdata_hdr->count,
		                             (uint8_t *)&event_hdr,  sizeof(struct tms_retotdr_event_hdr),
		                             (uint8_t *)&event_val,  sizeof(struct tms_retotdr_event_val) * pevent_hdr->count,
		                             (uint8_t *)&chain,      sizeof(struct tms_retotdr_chain));
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&alarm,      sizeof(struct tms_alarm_line_hdr));
	glink_SendSerial(fd, (uint8_t *)&test_hdr,   sizeof(struct tms_retotdr_test_hdr));
	glink_SendSerial(fd, (uint8_t *)&test_param, sizeof(struct tms_retotdr_test_param));
	glink_SendSerial(fd, (uint8_t *)&data_hdr,   sizeof(struct tms_retotdr_data_hdr));
	glink_SendSerial(fd, (uint8_t *)&data_val,   sizeof(struct tms_retotdr_data_val) * pdata_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&event_hdr,  sizeof(struct tms_retotdr_event_hdr));
	glink_SendSerial(fd, (uint8_t *)&event_val,  sizeof(struct tms_retotdr_event_val) * pevent_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&chain,      sizeof(struct tms_retotdr_chain));
	glink_SendTail(fd);

	return 0;
}
// ID_ALARM_LINE			0x80000030
// 已测试通过
static int32_t tms_AnalyseAlarmLine(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_alarm_line_hdr     *palarm;
	struct tms_retotdr_test_hdr   *ptest_hdr;
	struct tms_retotdr_test_param *ptest_param;
	struct tms_retotdr_data_hdr   *pdata_hdr;
	struct tms_retotdr_data_val   *pdata_val;
	struct tms_retotdr_event_hdr  *pevent_hdr;
	struct tms_retotdr_event_val  *pevent_val;
	struct tms_retotdr_chain      *pchain;

	if ((uint32_t)(len - GLINK_OFFSET_DATA) < sizeof(struct tms_alarm_line_hdr) +
	    sizeof(struct tms_retotdr_test_hdr ) +
	    sizeof(struct tms_retotdr_test_param) +
	    sizeof(struct tms_retotdr_data_hdr  ) +
	    sizeof(struct tms_retotdr_data_val  ) +
	    sizeof(struct tms_retotdr_event_hdr ) +
	    sizeof(struct tms_retotdr_event_val ) +
	    sizeof(struct tms_retotdr_chain) ) {
		return -1;
	}

	palarm      = (struct tms_alarm_line_hdr *)(pdata + GLINK_OFFSET_DATA );
	printf("%d %d %d\n",
	       htonl(palarm->frame),
	       htonl(palarm->slot),
	       htonl(palarm->port));
	ptest_hdr   = (struct tms_retotdr_test_hdr *)(((char *)palarm)      + sizeof(struct tms_alarm_line_hdr) );
	ptest_param = (struct tms_retotdr_test_param *)(((char *)ptest_hdr)   + sizeof(struct tms_retotdr_test_hdr));
	pdata_hdr   = (struct tms_retotdr_data_hdr *)(((char *)ptest_param) + sizeof(struct tms_retotdr_test_param));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pdata_hdr,
	         struct tms_retotdr_data_hdr,
	         struct tms_retotdr_data_val,
	         htonl(pdata_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pdata_val   = (struct tms_retotdr_data_val *)(((char *)pdata_hdr) + sizeof(struct tms_retotdr_data_hdr));
	pevent_hdr  = (struct tms_retotdr_event_hdr *)(((char *)pdata_val) + sizeof(struct tms_retotdr_data_val) * htonl(pdata_hdr->count));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pevent_hdr,
	         struct tms_retotdr_event_hdr,
	         struct tms_retotdr_event_val,
	         htonl(pevent_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pevent_val  = (struct tms_retotdr_event_val *)(((char *)pevent_hdr) + sizeof(struct tms_retotdr_event_hdr));
	pchain      = (struct tms_retotdr_chain *)(((char *)pevent_val) + sizeof(struct tms_retotdr_event_val) * htonl(pevent_hdr->count));


	// Part x 告警数据头
	palarm->alarm_type 	   = htonl(palarm->alarm_type);
	palarm->alarm_level    = htonl(palarm->alarm_level);
	palarm->frame          = htonl(palarm->frame);
	palarm->slot           = htonl(palarm->slot);
	palarm->port           = htonl(palarm->port);
	palarm->alarm_position = htonl(palarm->alarm_position);

	// Part A.1
	ptest_hdr->osw_frame  = htonl(ptest_hdr->osw_frame);
	ptest_hdr->osw_slot   = htonl(ptest_hdr->osw_slot);
	ptest_hdr->osw_type   = htonl(ptest_hdr->osw_type);
	ptest_hdr->osw_port   = htonl(ptest_hdr->osw_port);
	ptest_hdr->otdr_frame = htonl(ptest_hdr->otdr_frame);
	ptest_hdr->otdr_slot  = htonl(ptest_hdr->otdr_slot);
	ptest_hdr->otdr_type  = htonl(ptest_hdr->otdr_type);
	ptest_hdr->otdr_port  = htonl(ptest_hdr->otdr_port);

	// Part A.2
	tms_OTDRConv_tms_retotdr_test_param(ptest_param, ptest_param);
	// Part B.1
	tms_OTDRConv_tms_retotdr_data_hdr(pdata_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(pdata_val, pdata_val, pdata_hdr);
	// Part C.1
	tms_OTDRConv_tms_retotdr_event_hdr(pevent_hdr, pevent_hdr);
	tms_OTDRConv_tms_retotdr_event_val(pevent_val, pevent_val, pevent_hdr);
	tms_OTDRConv_tms_retotdr_chain(pchain, pchain);


	tms_Print_tms_alarm_line_hdr(palarm);
	if (pcontext->ptcb->pf_OnRetOTDRTest) {
		pcontext->ptcb->pf_OnRetOTDRTest(
		    pcontext,
		    ptest_hdr,  ptest_param,
		    pdata_hdr,  pdata_val,
		    pevent_hdr, pevent_val,
		    pchain);
	}

	return 0;
}
/**
 * @brief	ID_ALARM_HW 0x80000031 MCU返回硬件告警
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	count struct tms_alarm_hw_val  数组个数
 * @param[in]	list 指向一个 struct tms_alarm_hw_val  一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
int32_t tms_AlarmHW(
    int     fd,
    struct glink_addr *paddr,
    int32_t count,
    struct  tms_alarm_hw_val *list)
{
	uint8_t *pmem;
	struct tms_alarm_hw *pcfg_hdr;
	struct tms_alarm_hw_val         *plist, *ptlist;
	int len;
	// uint16_t *preason;

	// printf("count = %d\n", count);
	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_alarm_hw) + count * sizeof(struct tms_alarm_hw_val);

	// printf("count = %d\n", count);
	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	// printf("count = %d\n", count);
	pcfg_hdr = (struct tms_alarm_hw *)(pmem);
	// printf("count = %d\n", count);
	plist    = (struct tms_alarm_hw_val *)(pmem + sizeof(struct tms_alarm_hw));

	// printf("count = %d\n", count);
	// Step 2.各字段复制
	pcfg_hdr->alarm_type = htonl(3);// 硬件告警固定写3
	pcfg_hdr->count = htonl(count);

	// printf("count = %d\n", count);
	// todo :防止循环count次溢出ptlist
	ptlist = list;
	for (int i = 0; i < count; i++) {
		plist->level  = htonl(ptlist->level);
		plist->frame  = htonl(ptlist->frame);
		plist->slot   = htonl(ptlist->slot);
		plist->type   = htonl(ptlist->type);
		plist->reason = htonl(ptlist->reason);

#if 0
		// 将short按照网络字节发送，你妈B
		memcpy(plist->reason, ptlist->reason, 128);
		preason  = (uint16_t *)plist->reason;
		// PrintfMemory((uint8_t*)preason, 128);
		for (int k = 0; k < 128 / 2; k++) {
			*preason = htons(*preason);
			preason++;
		}
		preason  = (uint16_t *)ptlist->reason;
		// PrintfMemory((uint8_t*)preason, 128);
#endif


		memcpy(plist->time,   ptlist->time,   20);

		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	glink_Build(&base_hdr, ID_ALARM_HW, len);
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		ret = tms_SendAllManager(&base_hdr, pmem, len);
		goto _Free;
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
_Free:
	;
	free(pmem);
	return ret;
}

// ID_ALARM_HW 0x80000031 MCU返回硬件告警
static int32_t tms_AnalyseAlarmHW(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_alarm_hw 		*pcfg_hdr;
	struct tms_alarm_hw_val  *plist, *ptlist;
	// uint16_t *preason;


	pcfg_hdr = (struct tms_alarm_hw *)(pdata + GLINK_OFFSET_DATA);
	plist    = (struct tms_alarm_hw_val *)  (pdata + GLINK_OFFSET_DATA + sizeof(struct tms_alarm_hw));
	PrintfMemory((uint8_t *)plist, sizeof(struct tms_alarm_hw_val));

	pcfg_hdr->alarm_type  = htonl(pcfg_hdr->alarm_type);
	pcfg_hdr->count = htonl(pcfg_hdr->count);

	// TODO：防止count溢出
	ptlist = plist;
	for (int i = 0; i < pcfg_hdr->count; i++) {
		ptlist->level      = htonl(ptlist->level);
		ptlist->frame      = htonl(ptlist->frame);
		ptlist->slot       = htonl(ptlist->slot);
		ptlist->reason     = htonl(ptlist->reason);
		ptlist++;

#if 0
		preason  = (uint16_t *)plist->reason;
		for (int k = 0; k < 128 / 2; k++) {
			*preason = htons(*preason);
			preason++;
		}
#endif
	}

	printf("tms_AnalyseAlarmHW\n");
	ptlist = plist;
	for (int i = 0; i < pcfg_hdr->count; i++) {
		printf("\tlevel %d frame %d slot %d reason %s time %s\n",
		       ptlist->level      ,
		       ptlist->frame ,
		       ptlist->slot      ,
		       (char *)ptlist->reason ,
		       (char *)ptlist->time );
		ptlist++;
	}
	printf("\t\tcount %d\n", pcfg_hdr->count);

	// fun()

	return 0;
}
/**
 * @brief	ID_RET_OTDR_CYC 0x80000032 MCU返回OTDR周期性测试曲线
 * @param	fd
 * @param	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param	ptest_hdr
 * @param	ptest_param

 * @param	pdata_hdr
 * @param	pdata_val

 * @param	pevent_hdr
 * @param	pevent_val

struct tms_retotdr_chain    *pchain
 * @retval	null
 * @remarks	原来该命令头有周期测试曲线数目，允许一次发送多条曲线，现实现只允许发送一条\n
 			理由是，节省接收方的内存开销，一条曲线约36000字节，过长的字节不利于小内存的接收方处理
 			帧结构内容与tms_RetOTDRTest差不多，只是在数据包前加入数子1，表示曲线数。

 * @see	tms_GetOTDRTest\nTMS_PTR_OTDRTest\ntms_retotdr_test_hdr\ntms_retotdr_test_param\ntms_retotdr_data_hdr\n
 tms_retotdr_data_val\n tms_retotdr_event_hdr\n tms_retotdr_event_val\n tms_retotdr_chain\n
 */
int32_t tms_RetOTDRCycle(
    int    fd,
    struct glink_addr *paddr,
    struct tms_retotdr_test_hdr   *ptest_hdr,
    struct tms_retotdr_test_param *ptest_param,

    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,

    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,

    struct tms_retotdr_chain      *pchain)
{
#if 0
	int32_t linecount;
	struct tms_retotdr_test_hdr   test_hdr;
	struct tms_retotdr_test_param test_param;
	struct tms_retotdr_data_hdr   data_hdr;
	struct tms_retotdr_data_val   data_val[OTDR_SAMPLE_32000];
	struct tms_retotdr_event_hdr  event_hdr;
	struct tms_retotdr_event_val  event_val[OTDR_EVENT_MAX];
	struct tms_retotdr_chain      chain;
	register int32_t *p32s, *p32d;
	register int16_t *p16s, *p16d;
	register int loop;

	// 固定写死，只能发送一条曲线
	linecount = htonl(1);
	// Part A.1
	test_hdr.osw_frame  = htonl(ptest_hdr->osw_frame);
	test_hdr.osw_slot   = htonl(ptest_hdr->osw_slot);
	test_hdr.osw_type   = htonl(DEV_OSW);
	test_hdr.osw_port   = htonl(ptest_hdr->osw_port);
	memcpy(test_hdr.time, ptest_hdr->time, 20);
	test_hdr.otdr_frame = htonl(ptest_hdr->otdr_frame);
	test_hdr.otdr_slot  = htonl(ptest_hdr->otdr_slot);
	test_hdr.otdr_type  = htonl(DEV_OTDR);
	test_hdr.otdr_port  = htonl(ptest_hdr->otdr_port);

	tms_OTDRConv_tms_retotdr_test_param(&test_param, ptest_param);


	// // Part B.1
	if (pdata_hdr->count > OTDR_SAMPLE_32000) {
		pdata_hdr->count = OTDR_SAMPLE_32000;
	}
	tms_OTDRConv_tms_retotdr_data_hdr(&data_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(&data_val[0], pdata_val, pdata_hdr);
	// Part C.1
	pevent_hdr->count = pevent_hdr->count & 0x3ff;				// 限定loop在0~1024以内
	tms_OTDRConv_tms_retotdr_event_hdr(&event_hdr, pevent_hdr);
	// Part C.2
	tms_OTDRConv_tms_retotdr_event_val(&event_val[0], pevent_val, pevent_hdr);

	tms_OTDRConv_tms_retotdr_chain(&chain, pchain);


	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;
	int len;
	len = 	sizeof(int32_t) +
	        sizeof(struct tms_retotdr_test_hdr  ) +
	        sizeof(struct tms_retotdr_test_param) +
	        sizeof(struct tms_retotdr_data_hdr  ) +
	        sizeof(struct tms_retotdr_data_val  ) * pdata_hdr->count +
	        sizeof(struct tms_retotdr_event_hdr ) +
	        sizeof(struct tms_retotdr_event_val ) * pevent_hdr->count +
	        sizeof(struct tms_retotdr_chain);

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}

	glink_Build(&base_hdr, ID_RET_OTDR_CYC, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&linecount,   sizeof(int32_t));
	glink_SendSerial(fd, (uint8_t *)&test_hdr,   sizeof(struct tms_retotdr_test_hdr));
	glink_SendSerial(fd, (uint8_t *)&test_param, sizeof(struct tms_retotdr_test_param));
	glink_SendSerial(fd, (uint8_t *)&data_hdr,   sizeof(struct tms_retotdr_data_hdr));
	glink_SendSerial(fd, (uint8_t *)&data_val,   sizeof(struct tms_retotdr_data_val) * pdata_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&event_hdr,  sizeof(struct tms_retotdr_event_hdr));
	glink_SendSerial(fd, (uint8_t *)&event_val,  sizeof(struct tms_retotdr_event_val) * pevent_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&chain,      sizeof(struct tms_retotdr_chain));
	glink_SendTail(fd);
#endif



	return 0;
}
// 0x80000032
static int32_t tms_AnalyseRetOTDRCycle(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#if 0
	struct tms_retotdr_test_hdr   *ptest_hdr;
	struct tms_retotdr_test_param *ptest_param;
	struct tms_retotdr_data_hdr   *pdata_hdr;
	struct tms_retotdr_data_val   *pdata_val;
	struct tms_retotdr_event_hdr  *pevent_hdr;
	struct tms_retotdr_event_val  *pevent_val;
	struct tms_retotdr_chain      *pchain;
	register uint32_t *p32s, *p32d;
	register uint16_t *p16s, *p16d;
	register int loop;


	if ((uint32_t)(len - GLINK_OFFSET_DATA) < sizeof(int32_t) +
	    sizeof(struct tms_retotdr_test_hdr ) +
	    sizeof(struct tms_retotdr_test_param) +
	    sizeof(struct tms_retotdr_data_hdr  ) +
	    sizeof(struct tms_retotdr_data_val  ) +
	    sizeof(struct tms_retotdr_event_hdr ) +
	    sizeof(struct tms_retotdr_event_val ) +
	    sizeof(struct tms_retotdr_chain) ) {
		return -1;
	}

	ptest_hdr   = (struct tms_retotdr_test_hdr *)(pdata + GLINK_OFFSET_DATA + sizeof(int32_t));
	ptest_param = (struct tms_retotdr_test_param *)(((char *)ptest_hdr)   + sizeof(struct tms_retotdr_test_hdr));
	pdata_hdr   = (struct tms_retotdr_data_hdr *)(((char *)ptest_param) + sizeof(struct tms_retotdr_test_param));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pdata_hdr,
	         struct tms_retotdr_data_hdr,
	         struct tms_retotdr_data_val,
	         htonl(pdata_hdr->count),
	         pdata + len)) {
		return -1;
	}
	pdata_val   = (struct tms_retotdr_data_val *)(((char *)pdata_hdr) + sizeof(struct tms_retotdr_data_hdr));
	pevent_hdr  = (struct tms_retotdr_event_hdr *)(((char *)pdata_val) + sizeof(struct tms_retotdr_data_val) * htonl(pdata_hdr->count));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pevent_hdr,
	         struct tms_retotdr_event_hdr,
	         struct tms_retotdr_event_val,
	         htonl(pevent_hdr->count),
	         pdata + len)) {
		return -1;
	}
	pevent_val  = (struct tms_retotdr_event_val *)(((char *)pevent_hdr) + sizeof(struct tms_retotdr_event_hdr));
	pchain      = (struct tms_retotdr_chain *)(((char *)pevent_val) + sizeof(struct tms_retotdr_event_val) * htonl(pevent_hdr->count));

	// Part A.1
	ptest_hdr->osw_frame  = htonl(ptest_hdr->osw_frame);
	ptest_hdr->osw_slot   = htonl(ptest_hdr->osw_slot);
	ptest_hdr->osw_type   = htonl(ptest_hdr->osw_type);
	ptest_hdr->osw_port   = htonl(ptest_hdr->osw_port);
	ptest_hdr->otdr_frame = htonl(ptest_hdr->otdr_frame);
	ptest_hdr->otdr_slot  = htonl(ptest_hdr->otdr_slot);
	ptest_hdr->otdr_type  = htonl(ptest_hdr->otdr_type);
	ptest_hdr->otdr_port  = htonl(ptest_hdr->otdr_port);

	printf("lsieijlwerw\n");
	// Part A.2
	tms_OTDRConv_tms_retotdr_test_param(ptest_param, ptest_param);
	// Part B.1
	tms_OTDRConv_tms_retotdr_data_hdr(pdata_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(pdata_val, pdata_val, pdata_hdr);
	// Part C.1
	tms_OTDRConv_tms_retotdr_event_hdr(pevent_hdr, pevent_hdr);
	tms_OTDRConv_tms_retotdr_event_val(pevent_val, pevent_val, pevent_hdr);
	tms_OTDRConv_tms_retotdr_chain(pchain, pchain);

	if (pcontext->ptcb->pf_OnRetOTDRTest) {
		pcontext->ptcb->pf_OnRetOTDRTest(
		    pcontext,
		    ptest_hdr,  ptest_param,
		    pdata_hdr,  pdata_val,
		    pevent_hdr, pevent_val,
		    pchain);
	}

#endif
	return 0;
}



/**
 * @brief	ID_RET_OTDR_CYC 0x80000032 MCU返回OTDR周期性测试曲线
 * @param	fd
 * @param	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param	ptest_hdr
 * @param	ptest_param

 * @param	pdata_hdr
 * @param	pdata_val

 * @param	pevent_hdr
 * @param	pevent_val

struct tms_retotdr_chain    *pchain
 * @retval	null
 * @remarks	原来该命令头有周期测试曲线数目，允许一次发送多条曲线，现实现只允许发送一条\n
 			理由是，节省接收方的内存开销，一条曲线约36000字节，过长的字节不利于小内存的接收方处理
 			帧结构内容与tms_RetOTDRTest差不多，只是在数据包前加入数子1，表示曲线数。

 * @see	tms_GetOTDRTest\nTMS_PTR_OTDRTest\ntms_retotdr_test_hdr\ntms_retotdr_test_param\ntms_retotdr_data_hdr\n
 tms_retotdr_data_val\n tms_retotdr_event_hdr\n tms_retotdr_event_val\n tms_retotdr_chain\n
 */
int32_t tms_RetOTDRCycle_V2(
    int    fd,
    struct glink_addr *paddr,
    struct tms_alarm_line_hdr     *palarm,
    struct tms_retotdr_test_hdr   *ptest_hdr,
    struct tms_retotdr_test_param *ptest_param,

    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,

    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,

    struct tms_retotdr_chain      *pchain)
{
	int32_t linecount;
	struct tms_alarm_line_hdr     alarm;
	struct tms_retotdr_test_hdr   test_hdr;
	struct tms_retotdr_test_param test_param;
	struct tms_retotdr_data_hdr   data_hdr;
	struct tms_retotdr_data_val   data_val[OTDR_SAMPLE_32000];
	struct tms_retotdr_event_hdr  event_hdr;
	struct tms_retotdr_event_val  event_val[OTDR_EVENT_MAX];
	struct tms_retotdr_chain      chain;
	// register int32_t *p32s, *p32d;
	// register int16_t *p16s, *p16d;
	// register int loop;

	// 固定写死，只能发送一条曲线
	linecount = htonl(1);


	// Part A.0
	alarm.alarm_type = htonl(palarm->alarm_type);
	alarm.alarm_level = htonl(palarm->alarm_level);
	alarm.frame = htonl(palarm->frame);
	alarm.slot = htonl(palarm->slot);
	alarm.port = htonl(palarm->port);
	alarm.alarm_position = htonl(palarm->alarm_position);
	memcpy(alarm.time, palarm->time, 20);
	alarm.reserve0 = htonl(palarm->reserve0);

	// Part A.1
	test_hdr.osw_frame  = htonl(ptest_hdr->osw_frame);
	test_hdr.osw_slot   = htonl(ptest_hdr->osw_slot);
	test_hdr.osw_type   = htonl(DEV_OSW);
	test_hdr.osw_port   = htonl(ptest_hdr->osw_port);
	memcpy(test_hdr.time, ptest_hdr->time, 20);
	test_hdr.otdr_frame = htonl(ptest_hdr->otdr_frame);
	test_hdr.otdr_slot  = htonl(ptest_hdr->otdr_slot);
	test_hdr.otdr_type  = htonl(DEV_OTDR);
	test_hdr.otdr_port  = htonl(ptest_hdr->otdr_port);

	tms_OTDRConv_tms_retotdr_test_param(&test_param, ptest_param);


	// // Part B.1
	if (pdata_hdr->count > OTDR_SAMPLE_32000) {
		pdata_hdr->count = OTDR_SAMPLE_32000;
	}
	tms_OTDRConv_tms_retotdr_data_hdr(&data_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(&data_val[0], pdata_val, pdata_hdr);
	// Part C.1
	pevent_hdr->count = pevent_hdr->count & 0x3ff;				// 限定loop在0~1024以内
	tms_OTDRConv_tms_retotdr_event_hdr(&event_hdr, pevent_hdr);
	// Part C.2
	tms_OTDRConv_tms_retotdr_event_val(&event_val[0], pevent_val, pevent_hdr);

	tms_OTDRConv_tms_retotdr_chain(&chain, pchain);


	// Step 3. 发送
	struct glink_base  base_hdr;
	// int ret;
	int len;
	len = 	sizeof(int32_t) +
	        sizeof(struct tms_alarm_line_hdr) +
	        sizeof(struct tms_retotdr_test_hdr  ) +
	        sizeof(struct tms_retotdr_test_param) +
	        sizeof(struct tms_retotdr_data_hdr  ) +
	        sizeof(struct tms_retotdr_data_val  ) * pdata_hdr->count +
	        sizeof(struct tms_retotdr_event_hdr ) +
	        sizeof(struct tms_retotdr_event_val ) * pevent_hdr->count +
	        sizeof(struct tms_retotdr_chain);

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}

	glink_Build(&base_hdr, ID_RET_OTDR_CYC, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&linecount,  sizeof(int32_t));
	glink_SendSerial(fd, (uint8_t *)&alarm,   	sizeof(struct tms_alarm_line_hdr));
	glink_SendSerial(fd, (uint8_t *)&test_hdr,   sizeof(struct tms_retotdr_test_hdr));
	glink_SendSerial(fd, (uint8_t *)&test_param, sizeof(struct tms_retotdr_test_param));
	glink_SendSerial(fd, (uint8_t *)&data_hdr,   sizeof(struct tms_retotdr_data_hdr));
	glink_SendSerial(fd, (uint8_t *)data_val,   sizeof(struct tms_retotdr_data_val) * pdata_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&event_hdr,  sizeof(struct tms_retotdr_event_hdr));
	glink_SendSerial(fd, (uint8_t *)event_val,  sizeof(struct tms_retotdr_event_val) * pevent_hdr->count);
	glink_SendSerial(fd, (uint8_t *)&chain,      sizeof(struct tms_retotdr_chain));
	glink_SendTail(fd);
	return 0;
}
// 0x80000032
static int32_t tms_AnalyseRetOTDRCycle_V2(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	int32_t     *plinecount;
	struct tms_alarm_line_hdr     *palarm;
	struct tms_retotdr_test_hdr   *ptest_hdr;
	struct tms_retotdr_test_param *ptest_param;
	struct tms_retotdr_data_hdr   *pdata_hdr;
	struct tms_retotdr_data_val   *pdata_val;
	struct tms_retotdr_event_hdr  *pevent_hdr;
	struct tms_retotdr_event_val  *pevent_val;
	struct tms_retotdr_chain      *pchain;
	// register uint32_t *p32s, *p32d;
	// register uint16_t *p16s, *p16d;
	// register int loop;


	if ((uint32_t)(len - GLINK_OFFSET_DATA) < sizeof(int32_t) +
	    sizeof(struct tms_alarm_line_hdr) +
	    sizeof(struct tms_retotdr_test_hdr ) +
	    sizeof(struct tms_retotdr_test_param) +
	    sizeof(struct tms_retotdr_data_hdr  ) +
	    sizeof(struct tms_retotdr_data_val  ) +
	    sizeof(struct tms_retotdr_event_hdr ) +
	    sizeof(struct tms_retotdr_event_val ) +
	    sizeof(struct tms_retotdr_chain) ) {
		return -1;
	}

	plinecount  = (int32_t *)(pdata + GLINK_OFFSET_DATA );
	palarm      = (struct tms_alarm_line_hdr *)(((char *)plinecount) + sizeof(int32_t) );
	ptest_hdr   = (struct tms_retotdr_test_hdr *)(((char *)palarm)      + sizeof(struct tms_alarm_line_hdr) );

	// ptest_hdr   = (struct tms_retotdr_test_hdr *)(pdata + GLINK_OFFSET_DATA + sizeof(int32_t));
	ptest_param = (struct tms_retotdr_test_param *)(((char *)ptest_hdr)   + sizeof(struct tms_retotdr_test_hdr));
	pdata_hdr   = (struct tms_retotdr_data_hdr *)(((char *)ptest_param) + sizeof(struct tms_retotdr_test_param));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pdata_hdr,
	         struct tms_retotdr_data_hdr,
	         struct tms_retotdr_data_val,
	         htonl(pdata_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pdata_val   = (struct tms_retotdr_data_val *)(((char *)pdata_hdr) + sizeof(struct tms_retotdr_data_hdr));
	// PrintfMemory((uint8_t *)&pdata_val, 20);
	pevent_hdr  = (struct tms_retotdr_event_hdr *)(((char *)pdata_val) + sizeof(struct tms_retotdr_data_val) * htonl(pdata_hdr->count));
	// 指针合法性检测，防止指针超过pdata
	if ( !CHECK_PTR(
	         pevent_hdr,
	         struct tms_retotdr_event_hdr,
	         struct tms_retotdr_event_val,
	         htonl(pevent_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	pevent_val  = (struct tms_retotdr_event_val *)(((char *)pevent_hdr) + sizeof(struct tms_retotdr_event_hdr));
	pchain      = (struct tms_retotdr_chain *)(((char *)pevent_val) + sizeof(struct tms_retotdr_event_val) * htonl(pevent_hdr->count));

	palarm->alarm_type 		= htonl(palarm->alarm_type);
	palarm->alarm_level 	= htonl(palarm->alarm_level);
	palarm->frame 			= htonl(palarm->frame);
	palarm->slot 			= htonl(palarm->slot);
	palarm->port 			= htonl(palarm->port);
	palarm->alarm_position 	= htonl(palarm->alarm_position);
	palarm->reserve0 		= htonl(palarm->reserve0);

	// Part A.1
	ptest_hdr->osw_frame  = htonl(ptest_hdr->osw_frame);
	ptest_hdr->osw_slot   = htonl(ptest_hdr->osw_slot);
	ptest_hdr->osw_type   = htonl(ptest_hdr->osw_type);
	ptest_hdr->osw_port   = htonl(ptest_hdr->osw_port);
	ptest_hdr->otdr_frame = htonl(ptest_hdr->otdr_frame);
	ptest_hdr->otdr_slot  = htonl(ptest_hdr->otdr_slot);
	ptest_hdr->otdr_type  = htonl(ptest_hdr->otdr_type);
	ptest_hdr->otdr_port  = htonl(ptest_hdr->otdr_port);

	printf("lsieijlwerw\n");
	// Part A.2
	tms_OTDRConv_tms_retotdr_test_param(ptest_param, ptest_param);
	// Part B.1
	tms_OTDRConv_tms_retotdr_data_hdr(pdata_hdr, pdata_hdr);
	// Part B.2
	tms_OTDRConv_tms_retotdr_data_val(pdata_val, pdata_val, pdata_hdr);
	// Part C.1
	tms_OTDRConv_tms_retotdr_event_hdr(pevent_hdr, pevent_hdr);
	tms_OTDRConv_tms_retotdr_event_val(pevent_val, pevent_val, pevent_hdr);
	tms_OTDRConv_tms_retotdr_chain(pchain, pchain);


	if (pcontext->ptcb->pf_OnRetOTDRTest) {
		pcontext->ptcb->pf_OnRetOTDRTest(
		    pcontext,
		    ptest_hdr,  ptest_param,
		    pdata_hdr,  pdata_val,
		    pevent_hdr, pevent_val,
		    pchain);
	}


	return 0;
}





/**
 * @brief	ID_CMD_SMS_TEXT 0x80000033 网管发送短信内容到RTU
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	phone 手机号码16字节+861234567890123
 * @param[in]	count 短信内容含有的字符数目
 * @param[in]	text 短信内容，由于有中文字符，使用Unicode编码
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
int32_t tms_SendSMS(
    int fd,
    struct glink_addr *paddr,
    char (*phone)[16],
    int32_t count,
    wchar_t *text)
{
	uint8_t *pmem;
	uint32_t len;
	uint8_t *pphone;
	uint32_t *pcount;
	uint16_t *ptext, *pttext;

	// Step 1.分配内存并各指针指向相应内存
	if (count > 1400) {
		printf("To many character in one SMS\n");
		return -2;
	}

	len  = sizeof(char) * 16 + count + count * 2;
	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	pphone = (uint8_t *)  pmem;
	pcount = (uint32_t *)(pmem + sizeof(char) * 16);
	ptext  = (uint16_t *)  (pmem + sizeof(char) * 16 + sizeof(uint32_t));


	// Step 2.各字段复制
	memcpy(pphone, &phone[0][0], 16);
	*pcount = htonl(count);

	pttext = ptext;
	for (int i = 0; i < count; i++) {
		// *(pttext+i) = htons(*text);
		*(pttext + i) = (*text);
		ptext++;
		text++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_CMD_SMS_TEXT, len);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
	free(pmem);
	return ret;
}

int32_t tms_AnalyseSendSMS(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// MCU和业务办都需要
	// tms_Transmit2Dev(pcontext, pdata, len);
	// todo 需要测试
	if (pcontext->ptcb->pf_OnSpSendSMS) {
		pcontext->ptcb->pf_OnSpSendSMS(pcontext, pdata, len);
	}

	return 0;
}

/**
* @brief	ID_CMD_SMS_ERROR 0x80000034 RTU返回短信操作返回码
* @param[in]	fd 套接字文件描述符
* @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
				修改tmsxx.h设定
* @param[in]	phone 手机号码16字节+861234567890123
* @param[in]	state 短信发送状态码\0
				0 发送成功
				1 发送失败
* @retval	>0 发送成功
* @retval	0 发送失败，该链接失效，tms_RetSMSState立即断开
* @remarks
* @see
*/
int32_t tms_RetSMSError(
    int fd,
    struct glink_addr *paddr,
    char phone[16],
    uint32_t state)
{
	uint8_t *pmem;
	struct tms_sms_state smsstate;

	// Step 2.各字段复制
	memcpy(smsstate.phone, phone, 16);
	smsstate.state = htonl(state);

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	pmem = (uint8_t *)&smsstate;
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_CMD_SMS_ERROR, sizeof(struct tms_sms_state));
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_sms_state));
	// free(pmem);
	return ret;
}
int32_t tms_AnalyseRetSMSError(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// MCU和业务办都需要
	// tms_Transmit2Manager(pcontext, pdata, len);
	// todo 需要测试

	struct tms_sms_state *pval;
	pval  = (struct tms_sms_state *)(pdata + GLINK_OFFSET_DATA);
	pval->state = htonl(pval->state);


	return 0;
}

/**
 * @brief	ID_GET_VERSION 0x80000035 网管查询板卡板本号及软件版本
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	type 本设备板卡类型
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_RetVersion
 */
int32_t tms_GetVersion(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, type, 0, ID_GET_VERSION, sizeof(struct tms_dev_type));
}
// 0x80000035
static int32_t tms_AnalyseGetVersion(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_dev_type *pval;
	pval  = (struct tms_dev_type *)(pdata + GLINK_OFFSET_DATA);
	pval->frame = htonl(pval->frame);
	pval->slot  = htonl(pval->slot);
	pval->type  = htonl(pval->type);

	printf("tms_AnalyseGetVersion\n");
	printf("val:f%d/s%x/t%d\n", pval->frame, pval->slot, pval->type);

	return 0;
}


/**
 * @brief	ID_RET_VERSION 0x80000036 RTU返回软件版本
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	type 本设备板卡类型
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @see	tms_GetVersion
 */
int32_t tms_RetVersion(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type,
    uint8_t  *version)
{
	struct tms_dev_version devVersion;

	devVersion.frame = htonl(frame);
	devVersion.slot  = htonl(slot);
	devVersion.type  = htonl(type);
	memcpy(devVersion.vsersion, version, 48);


	struct glink_base  base_hdr;
	uint8_t *pmem;
	int ret;

	pmem = (uint8_t *)&devVersion;
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_RET_VERSION, sizeof(struct tms_dev_version));
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_dev_version));
	return ret;
}

//0x80000036
static int32_t tms_AnalyseRetVersion(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_dev_version *pval;
	pval  = (struct tms_dev_version *)(pdata + GLINK_OFFSET_DATA);
	pval->frame = htonl(pval->frame);
	pval->slot  = htonl(pval->slot);
	pval->type  = htonl(pval->type);
	// fun();

	printf("tms_AnalyseRetVersion\n");
	// printf("val:f%d/s%x/t%d\n", pval->frame, pval->slot, pval->type);
	// printf("\tversion:%s\n", pval->vsersion);
	if (pcontext->ptcb->pf_OnRetVersion) {
		pcontext->ptcb->pf_OnRetVersion(pcontext, pval);
	}

	return 0;
}


/**
 * @brief	适合如下格式的帧 count + struct A + struct A...
 * @param	count 结构体长度
 * @param	plist 链表指针
 * @param	size sizeof (struct A)
 * @param	cmdid 命令名
 * @retval	null
 * @remarks
 * @see
 */

int32_t tms_ListFrame(
    int fd,
    struct glink_addr *paddr,
    int32_t count,
    void *plist,
    int32_t size,
    int32_t cmdid)
{
	int tcount;

	tcount = htonl(count);
	tms_Conv_Nx4Byte((uint32_t *)plist, (uint32_t *)plist, size * count);
	struct glink_base  base_hdr;
	int len;

	len = 	sizeof(int32_t) + (size * count);

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, cmdid, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&tcount,   sizeof(int32_t));
	glink_SendSerial(fd, (uint8_t *)plist,   size * count);
	glink_SendTail(fd);

	return 0;
}
int32_t tms_AnalyseListFrame(struct tms_context *pcontext, int8_t *pdata, int32_t len, int32_t size)
{
	int32_t *pcount;
	int32_t *plist;

	pcount = (int32_t *)(pdata + GLINK_OFFSET_DATA);
	if ( !CHECK_PTR(
	         pcount,
	         int32_t,
	         size,
	         htonl(*pcount),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	plist   = (int32_t *)(pdata + GLINK_OFFSET_DATA +  + sizeof(int32_t));
	*pcount = htonl(*pcount);
	tms_Conv_Nx4Byte((uint32_t *)plist, (uint32_t *)plist, size * (*pcount));

	return 0;
}



/**
 * @brief	ID_GET_OTDR_PARAM		0x80000050 ///<网管查询OTDR板卡参数信息
 * @see	tms_MCUtoDevice
 */
int32_t tms_MCU_GetOTDRParam(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, DEV_OTDR, 0, ID_GET_OTDR_PARAM, sizeof(int32_t) * 3);
	// return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_GET_DEVTYPE, 0);
}

static int32_t tms_AnalyseDelAll_TbRoute(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseMCUtoDevice(pdata, len);
}


/**
 * @brief	ID_RET_OTDR_PARAM 0x80000051 ///<MCU返回OTDR板卡参数信息
 * @see	tms_MCUtoDevice
 */
static int32_t tms_AnalyseRetOTDRParam(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_otdr_produce_hdr *phdr;
	struct tms_otdr_produce_val *plist, *ptlist;

	if (len < (int32_t) ( sizeof(struct tms_otdr_produce_hdr) + sizeof(struct tms_otdr_produce_val))) {
		return -1;
	}
	phdr = (struct tms_otdr_produce_hdr *)(pdata + GLINK_OFFSET_DATA);
	// if ( !CHECK_PTR(
	// 		phdr,
	// 		struct tms_otdr_produce_hdr,
	// 		struct tms_otdr_produce_val,
	// 		htonl(phdr->count),
	// 		pdata + len)) {
	// 	return -1;
	// }
	plist = (struct tms_otdr_produce_val *)(((char *)phdr)   + sizeof(struct tms_otdr_produce_hdr));
	printf("tms_AnalyseRetOTDRParam\n");

	phdr->frame = htonl(phdr->frame);
	phdr->slot = htonl(phdr->slot);
	phdr->type = htonl(phdr->type);
	phdr->count = htons(phdr->count);


	printf("count : %d\n", phdr->count);
	ptlist = plist;
	for (int i = 0; i < phdr->count; i++) {
		ptlist->index = htons(ptlist->index);
		ptlist->dB    = htons(ptlist->dB);


		printf("index %d db %d\n", ptlist->index, ptlist->dB);
		ptlist++;
	}


	return 0;
}


/**
 * @brief	ID_GET_DEV_PRODUCE 0x80000052 网管查询板卡硬件生产信息
 * @see	tms_MCUtoDevice
 */
int32_t tms_GetDevProduce(
    int     fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, type, 0, ID_GET_DEV_PRODUCE, sizeof(int32_t) * 3);
}

static int32_t tms_AnalyseGetDevProduce(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseMCUtoDevice(pdata, len);

}


// ID_RET_DEV_PRODUCE 0x80000053 ///<MCU返回板卡硬件生产信息
int32_t tms_RetDevProduce(
    int     fd,
    struct glink_addr *paddr,
    struct tms_dev_produce *pval)
{
	pval->frame     = htonl(pval->frame);
	pval->slot      = htonl(pval->slot);
	pval->type      = htonl(pval->type);
	pval->reserved0 = htonl(pval->reserved0);
	pval->reserved1 = htonl(pval->reserved1);

	struct glink_base  base_hdr;
	int len;

	len = 	sizeof(struct tms_dev_produce);
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_RET_DEV_PRODUCE, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&paddr,   len);
	glink_SendTail(fd);


	return 0;
}

static int32_t tms_AnalyseRetDevProduce(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_dev_produce 		*pval;

	pval = (struct tms_dev_produce *)(pdata + GLINK_OFFSET_DATA);

	pval->frame = htonl(pval->frame);
	pval->slot  = htonl(pval->slot);
	pval->type  = htonl(pval->type);
	pval->reserved0 = htonl(pval->reserved0);
	pval->reserved1  = htonl(pval->reserved1);
	pval->hw_ver[19] = '\0';
	pval->date[19] = '\0';
	pval->sn[63] = '\0';



	printf("\tval:f%2.2d/s%2.2d/t%2.2d\n",
	       pval->frame,
	       pval->slot,
	       pval->type);
	printf("\t%s %s %s \n", pval->hw_ver, pval->date, pval->sn);
	return 0;
}

///////////////////////////////////////////////////////////////////////////
// 与 tb_route 表相关协议
/**
 * @brief	ID_INSERT_TBROUTE 0x80000054 ///<网管发送添加“模块级联表”内容到MCU
 * @see	tms_MCUtoDevice
 */
int32_t tms_TbRoute_Insert(
    int fd,
    struct glink_addr *paddr,
    int count,
    struct tms_route *plist)
{
	return tms_ListFrame(fd, paddr, count, plist, sizeof(struct tms_route), ID_INSERT_TBROUTE);
#if 0
	struct tms_route *ptlist;
	int tcount;


	tcount = htonl(count);
	ptlist = plist;
	for (int i = 0 ; i < count; i++) {
		ptlist->ip_src = htonl(ptlist->ip_src);
		ptlist->ip_dst = htonl(ptlist->ip_dst);
		ptlist->frame_a = htonl(ptlist->frame_a);
		ptlist->slot_a = htonl(ptlist->slot_a);
		ptlist->type_a = htonl(ptlist->type_a);
		ptlist->port_a = htonl(ptlist->port_a);
		ptlist->frame_b = htonl(ptlist->frame_b);
		ptlist->slot_b = htonl(ptlist->slot_b);
		ptlist->type_b = htonl(ptlist->type_b);
		ptlist->port_b = htonl(ptlist->port_b);
		plist++;
	}


	struct glink_base  base_hdr;
	int len;
	len = 	sizeof(int32_t  ) +
	        sizeof(struct tms_route) + count;

	ptlist = plist;
	glink_Build(&base_hdr, ID_INSERT_TBROUTE, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&tcount,   sizeof(int32_t));
	glink_SendSerial(fd, (uint8_t *)&ptlist,   sizeof(struct tms_route) * count);
	glink_SendTail(fd);

	return 0;
#endif
}

int32_t tms_AnalyseTbRoute_Insert(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	if (0 != tms_AnalyseListFrame(pcontext, pdata, len , sizeof(struct tms_route)) ) {
		return -1;
	}

	int *pcount;
	struct tms_route *plist;

	printf("tms_AnalyseInsert_TbUnit\n");

	pcount  = (int *)(pdata + GLINK_OFFSET_DATA );
	plist = (struct tms_route *)(pdata + GLINK_OFFSET_DATA + sizeof(int));
	if (pcontext->ptcb->pf_OnInsertTbRoute) {
		pcontext->ptcb->pf_OnInsertTbRoute(pcontext, *pcount, plist);
	}

	return 0;
#if 0
	int32_t *pcount;
	struct tms_route *plist, *ptlist;

	pcount = (int32_t *)(pdata + GLINK_OFFSET_DATA);
	if ( !CHECK_PTR(
	         pcount,
	         int32_t,
	         struct tms_route,
	         htonl(*pcount),
	         pdata + len)) {
		printf("frame err\n");
		return -1;
	}



	*pcount = htonl(*pcount);
	ptlist = plist;
	for (int i = 0 ; i < *pcount; i++) {
		ptlist->ip_src = htonl(ptlist->ip_src);
		ptlist->ip_dst = htonl(ptlist->ip_dst);

		ptlist->frame_a = htonl(ptlist->frame_a);
		ptlist->slot_a = htonl(ptlist->slot_a);
		ptlist->type_a = htonl(ptlist->type_a);
		ptlist->port_a = htonl(ptlist->port_a);

		ptlist->frame_b = htonl(ptlist->frame_b);
		ptlist->slot_b = htonl(ptlist->slot_b);
		ptlist->type_b = htonl(ptlist->type_b);
		ptlist->port_b = htonl(ptlist->port_b);
		plist++;
	}

	return 0;
#endif
}
/**
 * @brief	ID_DELALL_TBROUTE 0x80000055 ///<网管发送清除“全部”“模块级联表”到MCU
 * @see	tms_MCUtoDevice
 */
int32_t tms_DelAll_TbRoute(
    int     fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, DEV_MCU, 0, ID_DELALL_TBROUTE, 0);
	// return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_GET_DEVTYPE, 0);
}
// #define tms_AnalyseDelAll_TbRoute tms_AnalyseUnuse
///////////////////////////////////////////////////////////////////////////
// 与 tb_unit 表相关协议
int32_t tms_TbUnit_any(
    int fd,
    struct glink_addr *paddr,
    int count,
    struct tms_unit *plist,
    int cmdid)
{
	return tms_ListFrame(fd, paddr, count, plist, sizeof(struct tms_unit), cmdid);
#if 0
	struct tms_unit *ptlist;
	int tcount;


	tcount = htonl(count);
	ptlist = plist;
	for (int i = 0 ; i < count; i++) {
		ptlist->frame_a = htonl(ptlist->frame_a);
		ptlist->slot_a = htonl(ptlist->slot_a);
		ptlist->type_a = htonl(ptlist->type_a);
		ptlist->port_a = htonl(ptlist->port_a);
		ptlist->frame_b = htonl(ptlist->frame_b);
		ptlist->slot_b = htonl(ptlist->slot_b);
		ptlist->type_b = htonl(ptlist->type_b);
		ptlist->port_b = htonl(ptlist->port_b);
		ptlist++;
	}


	struct glink_base  base_hdr;
	int len;
	len = 	sizeof(int32_t  ) +
	        sizeof(struct tms_route) + count;

	ptlist = plist;
	glink_Build(&base_hdr, cmdid, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&tcount,   sizeof(int32_t));
	glink_SendSerial(fd, (uint8_t *)&ptlist,   sizeof(struct tms_unit) * count);
	glink_SendTail(fd);

	return 0;
#endif
}

int32_t tms_AnalyseTbUnit_any(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseListFrame(pcontext, pdata, len , sizeof(struct tms_unit));
#if 0
	int32_t *pcount;
	struct tms_unit *plist, *ptlist;

	pcount = (int32_t *)(pdata + GLINK_OFFSET_DATA);
	if ( !CHECK_PTR(
	         pcount,
	         int32_t,
	         struct tms_unit,
	         htonl(*pcount),
	         pdata + len)) {
		printf("frame err\n");
		return -1;
	}



	*pcount = htonl(*pcount);
	ptlist = plist;
	for (int i = 0 ; i < *pcount; i++) {
		ptlist->frame_a = htonl(ptlist->frame_a);
		ptlist->slot_a = htonl(ptlist->slot_a);
		ptlist->type_a = htonl(ptlist->type_a);
		ptlist->port_a = htonl(ptlist->port_a);
		ptlist->frame_b = htonl(ptlist->frame_b);
		ptlist->slot_b = htonl(ptlist->slot_b);
		ptlist->type_b = htonl(ptlist->type_b);
		ptlist->port_b = htonl(ptlist->port_b);
		ptlist++;
	}


	return 0;
#endif
}

/**
 * @brief	ID_INSERT_TBUNIT 0x80000056 ///<网管发送添加“光端口联动触发表”到MCU
 * @see	tms_MCUtoDevice
 */
int32_t tms_Insert_TbUnit(
    int fd,
    struct glink_addr *paddr,
    int count,
    struct tms_unit *plist)
{
	return tms_ListFrame(fd, paddr, count, plist, sizeof(struct tms_unit), ID_INSERT_TBUNIT);
}

int32_t tms_AnalyseInsert_TbUnit(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	if (0 != tms_AnalyseListFrame(pcontext, pdata, len , sizeof(struct tms_unit))) {
		return -1;
	}


	return 0;
}
/**
* @brief	ID_DEL_TBUNIT 0x80000057 ///<网管“逐条”删除“光端口联动触发表”到MCU
* @see	tms_MCUtoDevice
*/
int32_t tms_Del_TbUnit(
    int fd,
    struct glink_addr *paddr,
    int count,
    struct tms_unit *plist)
{
	return tms_ListFrame(fd, paddr, count, plist, sizeof(struct tms_unit), ID_DEL_TBUNIT);
}
int32_t tms_AnalyseDel_TbUnit(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseListFrame(pcontext, pdata, len , sizeof(struct tms_unit));
}

/**
 * @brief	ID_DELALL_TBUNIT 0x80000058 ///<网管清除“全部”“光端口联动触发表”到MCU
 * @see	tms_MCUtoDevice
 */
int32_t tms_DelAll_TbUnit(
    int     fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, DEV_MCU, 0, ID_DELALL_TBUNIT, 0);
	// return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_GET_DEVTYPE, 0);
}
#define tms_AnalyseDelAll_TbUnit tms_AnalyseUnuse




/**
 * @brief	ID_INSERT_TBCYCTEST 0x80000059 ///<网管发送添加“周期性测试表”到MCU
 * @see	tms_MCUtoDevice
 */
int32_t tms_Insert_TbCycTest(
    int fd,
    struct glink_addr *paddr,
    int32_t count,
    struct tms_cyctest *plist)
{
	return tms_ListFrame(fd, paddr, count, plist, sizeof(struct tms_cyctest), ID_INSERT_TBCYCTEST);
#if 0
	struct tms_cyctest *ptlist;
	int tcount;



	tcount = htonl(count);
	tms_Conv_Nx4Byte((uint32_t *)plist, (uint32_t *)plist, count);


	struct glink_base  base_hdr;
	int len;
	len = 	sizeof(int32_t  ) +
	        sizeof(struct tms_cyctest) + count;

	ptlist = plist;
	glink_Build(&base_hdr, cmdid, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&tcount,   sizeof(int32_t));
	glink_SendSerial(fd, (uint8_t *)&ptlist,   sizeof(struct tms_cyctest) * count);
	glink_SendTail(fd);

	return 0;
#endif
}

int32_t tms_AnalyseInsert_TbCycTest(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseListFrame(pcontext, pdata, len, sizeof(struct tms_cyctest));
#if 0
	int32_t *pcount;
	struct tms_cyctest *plist, *ptlist;

	pcount = (int32_t *)(pdata + GLINK_OFFSET_DATA);
	if ( !CHECK_PTR(
	         pcount,
	         int32_t,
	         struct tms_cyctest,
	         htonl(*pcount),
	         pdata + len)) {
		printf("frame err\n");
		return -1;
	}

	*pcount = htonl(*pcount);
	ptlist = plist;
	for (int i = 0 ; i < *pcount; i++) {
		ptlist->frame = htonl(ptlist->frame);
		ptlist->slot = htonl(ptlist->slot);
		ptlist->type = htonl(ptlist->type);
		ptlist->port = htonl(ptlist->port);
		ptlist->reserved0 = htonl(ptlist->reserved0);
		ptlist->iscyc = htonl(ptlist->iscyc);
		ptlist++;
	}
#endif

	return 0;
}

/**
 * @brief	ID_DEL_TBCYCTEST 0x80000060 ///<网管发送“逐条”删除“周期性测试表”关系到MCU
 * @see	tms_MCUtoDevice
 */
int32_t tms_Del_TbCycTest(
    int fd,
    struct glink_addr *paddr,
    int32_t count,
    struct tms_cyctest *plist,
    int32_t cmdid)
{
	return tms_ListFrame(fd, paddr, count, plist, sizeof(struct tms_cyctest), ID_DEL_TBCYCTEST);
}

int32_t tms_AnalyseDel_TbCycTest(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseListFrame(pcontext, pdata, len , sizeof(struct tms_cyctest));
}
/**
 * @brief	ID_DELALL_TBCYCTEST 0x80000061 ///<网管发送清除“全部”“周期性测试表”关系到MCU
 * @see	tms_MCUtoDevice
 */
int32_t tms_DelAll_TbCycTest(
    int     fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, DEV_MCU, 0, ID_DELALL_TBCYCTEST, 0);
}
#define tms_AnalyseDelAll_TbCycTest tms_AnalyseUnuse


/**
 * @brief	ID_INSERT_TBOTDRHISTDATA 0x80000062 ///<网管发送“逐条”清除各监测光路的OTDR参考曲线数据到MCU
 * @see	tms_MCUtoDevice
 */
int32_t tms_Del_TbOTDRRefData(
    int fd,
    struct glink_addr *paddr,
    int32_t count,
    struct tms_dev_port *plist,
    int32_t cmdid)
{
	return tms_ListFrame(fd, paddr, count, plist, sizeof(struct tms_dev_port), ID_DEL_TBOTDRREFDATA);
}

int32_t tms_AnalyseDel_TbOTDRRefData(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return tms_AnalyseListFrame(pcontext, pdata, len , sizeof(struct tms_dev_port) );
}


/**
 * @brief	ID_DELALL_TBOTDRREFDATA 0x80000063 ///<网管发送清除“全部”各监测光路的OTDR参考曲线数据到MCU
 * @see	tms_MCUtoDevice
 */
int32_t tms_DelAll_TbOTDRHistData(
    int     fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, DEV_MCU, 0, ID_DELALL_TBOTDRREFDATA, 0);
	// return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_GET_DEVTYPE, 0);
}
#define tms_AnalyseDelAll_TbOTDRHistData tms_AnalyseUnuse


/**
 * @brief	ID_GET_COMPOSITION_RT 0x80000065 ///<网管查询实时板卡组成信息
 * @see	tms_MCUtoDevice
 */
uint32_t tms_GetDeviceCompositionRT(
    int     fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_GET_COMPOSITION_RT, 0);
}

#define tms_AnalyseGetDeviceCompositionRT tms_AnalyseUnuse
#if 0
/**
 * @brief	ID_RET_COMPOSITION_RT 0x80000066 ///<MCU返回实时板卡组成信息
 * @param[in]	fd 文件描述符
 * @param[in]	paddr 是struct glink_addr指针，描述该glink帧的源地址、目的地址，如果为空则默认用
 			TMS_DEFAULT_LOCAL_ADDR填充src、TMS_DEFAULT_RMOTE_ADDR填充dst，采用默认方式某些帧
 			可能被接收端忽略，所以应用层需关注是否填写正确
 * @param[in]	count tms_dev_composition_val 数组个数
 * @param[in]	list 指向一个 struct tms_dev_composition 一维数组
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_RetDeviceComposition_any
 */
uint32_t tms_RetDeviceCompositionRT(
    int      fd,
    struct glink_addr *paddr,
    uint32_t count,
    struct tms_dev_composition_val *list)
{
	printf("%s:%d\n", __FUNCTION__, __LINE__);
	return tms_RetDeviceComposition_any(fd, paddr, count, list, ID_RET_COMPOSITION_RT);
}
#endif

#if 0
/**
 * @brief	ID_RET_ALARM_HW_CHANGE		0x80000068 ///< MCU返回变化的硬件告警
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_RetDeviceComposition_any
 */
uint32_t tms_RetAlarmHWChange(
    int      fd,
    struct glink_addr *paddr,
    int32_t total,
    int32_t count,
    struct tms_alarm_hw_change_val *list)
{
	uint8_t *pmem;
	struct tms_alarm_hw_change *pcfg_hdr;
	struct tms_alarm_hw_change_val         *plist, *ptlist;
	int len;
	// uint16_t *preason;

	// printf("count = %d\n", count);
	// Step 1.分配内存并各指针指向相应内存
	len = sizeof(struct tms_alarm_hw_change) + count * sizeof(struct tms_alarm_hw_change_val);

	// printf("count = %d\n", count);
	pmem = (uint8_t *)malloc(len);
	if (pmem == NULL) {
		return -1;
	}
	pcfg_hdr = (struct tms_alarm_hw_change *)(pmem);
	plist    = (struct tms_alarm_hw_change_val *)(pmem + sizeof(struct tms_alarm_hw_change));

	// Step 2.各字段复制
	pcfg_hdr->type = htonl(3);
	pcfg_hdr->total      = htonl(total);
	pcfg_hdr->count      = htonl(count);

	// printf("count = %d\n", count);
	// todo :防止循环count次溢出ptlist
	ptlist = list;
	for (int i = 0; i < count; i++) {
		plist->level  = htonl(ptlist->level);
		plist->frame  = htonl(ptlist->frame);
		plist->slot   = htonl(ptlist->slot);
		plist->type   = htonl(ptlist->type);
		plist->reason = htonl(ptlist->reason);

#if 0
		// 将short按照网络字节发送，你妈B
		memcpy(plist->reason, ptlist->reason, 128);
		preason  = (uint16_t *)plist->reason;
		// PrintfMemory((uint8_t*)preason, 128);
		for (int k = 0; k < 128 / 2; k++) {
			*preason = htons(*preason);
			preason++;
		}
		preason  = (uint16_t *)ptlist->reason;
		// PrintfMemory((uint8_t*)preason, 128);
#endif

		memcpy(plist->time,   ptlist->time,   20);

		plist++;
		ptlist++;
	}

	// Step 3. 发送
	struct glink_base  base_hdr;
	int ret;

	tms_FillGlinkFrame(&base_hdr, paddr);

	glink_Build(&base_hdr, ID_RET_ALARM_HW_CHANGE, len);
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		ret = tms_SendAllManager(&base_hdr, pmem, len);
		goto _Free;
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	ret = glink_Send(fd, NULL, &base_hdr, pmem, len);
_Free:
	;
	free(pmem);
	return ret;
}


/**
 * @brief	ID_RET_MCU_TIME		0x80000072 ///< MCU返回当前运行时间
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	time 时间20byte
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * see
 */int32_t tms_RetMCUTime(int fd, struct glink_addr *paddr, uint8_t *time)
{
	struct glink_base  base_hdr;
	int ret;


	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}

	glink_Build(&base_hdr, ID_RET_MCU_TIME, 20);
	ret = glink_Send(fd, NULL, &base_hdr, time, 20);
	return ret;
}


//0x80000073 ID_ALARM_SOUND_ON_OFF
static int32_t tms_AnalyseAlarmSoundONOFF(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	int32_t *pval;
	pval = (int32_t *)(pdata + GLINK_OFFSET_DATA);

	printf("%s():%d\n", __FUNCTION__, __LINE__);
	printf("raw    %8.8x\n", *pval);
	*pval = htonl(*pval);
	printf("change %8.8x\n", *pval);


	// printf("tms_AnalyseGetOPMOP\n");
	// printf("val:f%d/s%x\n", pval->frame, pval->slot);
	// fun()

	return 0;
}


/**
 * @brief	ID_RET_ALARM_SOUND_STATE		0x80000075 ///< 返回MCU告警声音关闭状态
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
 				修改tmsxx.h设定
 * @param[in]	time 时间20byte
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
int32_t tms_RetAlarmSoundState(int fd, struct glink_addr *paddr, uint32_t state)
{
	struct glink_base  base_hdr;
	int32_t data_state;
	int ret;

	data_state = htonl(state);

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}

	glink_Build(&base_hdr, ID_RET_ALARM_SOUND_STATE, 4);
	ret = glink_Send(fd, NULL, &base_hdr, (uint8_t *)&data_state, 4);
	return ret;
}

/**
 * @brief	ID_GET_TOTAL_OP_ALARM		0x80000076 ///< 查询总的光功告警
 */

int32_t tms_GetTotalOPAlarm(int fd, struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, 0, 0 , 0, DEV_MCU, 0, ID_GET_TOTAL_OP_ALARM, 0);
}

/**
 * @brief	ID_RET_TOTAL_OP_ALARM	0x80000077 ///< 主控返回总的光功告警
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
// 作废，用tms_RetTotalOPAlarmEx代替
int32_t tms_RetTotalOPAlarm(int fd, struct glink_addr *paddr,
                            int32_t type,
                            int32_t count,
                            struct tms_total_op_alarm_val *pval)
{
	int len;
	struct tms_total_op_alarm_hdr hdr;
	struct tms_total_op_alarm_val *ptlist;

	hdr.type  = htonl(type);
	hdr.count = htonl(count);

	ptlist = pval;
	for (int i = 0; i < count; i++) {
		ptlist->frame		= htonl(ptlist->frame);
		ptlist->slot		= htonl(ptlist->slot);
		ptlist->port		= htonl(ptlist->port);
		ptlist->alarm_level	= htonl(ptlist->alarm_level);
		ptlist->cur_power	= htonl(ptlist->cur_power);
		ptlist++;
	}
	struct glink_base  base_hdr;


	len = sizeof(struct tms_total_op_alarm_hdr) + count * sizeof(struct tms_total_op_alarm_val);
	tms_FillGlinkFrame(&base_hdr, paddr);
	glink_Build(&base_hdr, ID_RET_TOTAL_OP_ALARM, len);
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		return tms_SendAllManagerDot(&base_hdr,
		                             2,
		                             NULL,
		                             (uint8_t *)&hdr,      sizeof(struct tms_total_op_alarm_hdr),
		                             (uint8_t *)pval,   sizeof(struct tms_total_op_alarm_val));
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	// glink_Build(&base_hdr, ID_GET_TOTAL_OP_ALARM, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&hdr,   sizeof(struct tms_total_op_alarm_hdr));
	glink_SendSerial(fd, (uint8_t *)pval,   count * sizeof(struct tms_total_op_alarm_val));

	glink_SendTail(fd);


	return 0;
}

// 0x80000077
int32_t tms_RetTotalOPAlarmEx(
    int fd,
    struct glink_addr *paddr,
    struct tms_total_op_alarm_hdr *phdr,
    struct tms_total_op_alarm_val *pval)
{
	int len;
	struct tms_total_op_alarm_hdr hdr;
	struct tms_total_op_alarm_val *ptlist;

	hdr.type  = htonl(phdr->type);
	hdr.count = htonl(phdr->count);

	ptlist = pval;
	for (int i = 0; i < phdr->count; i++) {
		ptlist->frame		= htonl(ptlist->frame);
		ptlist->slot		= htonl(ptlist->slot);
		ptlist->port		= htonl(ptlist->port);
		ptlist->alarm_level	= htonl(ptlist->alarm_level);
		ptlist->cur_power	= htonl(ptlist->cur_power);
		ptlist++;
	}
	struct glink_base  base_hdr;


	len = sizeof(struct tms_total_op_alarm_hdr) + phdr->count * sizeof(struct tms_total_op_alarm_val);
	tms_FillGlinkFrame(&base_hdr, paddr);
	glink_Build(&base_hdr, ID_GET_TOTAL_OP_ALARM, len);
	if (paddr != NULL && paddr->dst == GLINK_MASK_MADDR) {
		return tms_SendAllManagerDot(&base_hdr,
		                             2,
		                             NULL,
		                             (uint8_t *)&hdr,      sizeof(struct tms_total_op_alarm_hdr),
		                             (uint8_t *)&pval,   sizeof(struct tms_total_op_alarm_val));
	}
	if (0 == fd) {
		fd = tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_GET_TOTAL_OP_ALARM, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&hdr,   sizeof(struct tms_total_op_alarm_hdr));
	glink_SendSerial(fd, (uint8_t *)&pval,   phdr->count * sizeof(struct tms_total_op_alarm_val));
	glink_SendTail(fd);


	return 0;
}
// 0x80000077
static int32_t tms_AnalyseRetTotalOPAlarm(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}

/**
 * @brief	ID_GET_TOTAL_HW_ALARM	0x80000078 ///< 查询总的硬件告警
 * @see
 */

int32_t tms_GetTotalHwAlarm(int fd, struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, 0, 0 , 0, DEV_MCU, 0, ID_GET_TOTAL_HW_ALARM, 0);
}


/**
 * @brief	ID_GET_OLP_ACTION_LOG	0x80000079 ///< 网管查询OLP切换记录
 * @see
 */

int32_t tms_GetOLPActionLog(int fd, struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, 0, 0 , 0, DEV_MCU, 0, ID_GET_OLP_ACTION_LOG, 0);
}


/**
 * @brief	ID_RET_OLP_ACTION_LOG	0x80000080 ///< 返回OLP切换记录
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */
int32_t tms_RetOLPActionLog(int fd, struct glink_addr *paddr,
                            int32_t count,
                            struct tms_olp_action_log_val *pval)
{
	int len;
	struct tms_olp_action_log_hdr hdr;
	struct tms_olp_action_log_val *ptlist;

	hdr.count = htonl(count);

	ptlist = pval;
	for (int i = 0; i < count; i++) {
		ptlist->frame	= htonl(ptlist->frame);
		ptlist->slot	= htonl(ptlist->slot);
		ptlist->type	= htonl(ptlist->type);
		ptlist->sw_type	= htonl(ptlist->sw_type);
		ptlist->sw_port	= htonl(ptlist->sw_port);
		ptlist++;
	}
	struct glink_base  base_hdr;


	len = sizeof(struct tms_olp_action_log_hdr) + count * sizeof(struct tms_olp_action_log_val);
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_RET_OLP_ACTION_LOG, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&hdr,   sizeof(struct tms_olp_action_log_hdr));
	glink_SendSerial(fd, (uint8_t *)pval,   count * sizeof(struct tms_olp_action_log_val));
	glink_SendTail(fd);


	return 0;
}
// 0x80000080
static int32_t tms_AnalyseRetOLPActionLog(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}

// 0x80000081
// undo

// 0x80000082
// tms_CfgSMSAuthorization

// 0x80000083
static int32_t tms_AnalyseGetMCUOSWPort(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_get_mcuosw_port *pval;


	pval  = (struct tms_get_mcuosw_port *)(pdata + GLINK_OFFSET_DATA );
	tms_Conv_Nx4Byte((uint32_t *)pval, (uint32_t *)pval, sizeof(struct tms_get_mcuosw_port));
}
// 0x80000084
// tms_CfgMCUOSWPort

// 0x80000085
static int32_t tms_AnalyseGetOTDRRef(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_get_otdrref *pval;


	pval  = (struct tms_get_otdrref *)(pdata + GLINK_OFFSET_DATA );
	tms_Conv_Nx4Byte((uint32_t *)pval, (uint32_t *)pval, sizeof(struct tms_get_otdrref));
	return 0;
}

// 0x80000086
// tms_CfgOTDRRef


// 0x80000087
// undo

// 0x80000088
// tms_TbRoute_Insert


// 0x80000089
// undo

// 0x80000090
// tms_Insert_TbUnit

// 0x80000091
// undo

// 0x80000092
// tms_Insert_TbCycTest



// #define ID_GET_OLP_LINE 0x80000093 ///< 网管查询OLP当前所在线路
static int32_t tms_AnalyseGetOLPLine(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_get_olp_line *pval;

	pval  = (struct tms_get_olp_line *)(pdata + GLINK_OFFSET_DATA);
	pval->frame = htonl(pval->frame);
	pval->slot  = htonl(pval->slot);
	pval->type  = htonl(pval->type);

	printf("%s():%d\n", __FUNCTION__, __LINE__);
	printf("\tval:f%d/s%x/t%d\n", pval->frame, pval->slot, pval->type);

	return 0;
}

// #define ID_RET_OLP_LINE 0x80000094 ///< MCU返回OLP当前所在线路
int32_t tms_RetOLPLine(int fd, struct glink_addr *paddr,
                       struct tms_ret_olp_line *pval)
{
	int len ;
	struct tms_ret_olp_line val;

	val.frame        = htonl(pval->frame);
	val.slot        = htonl(pval->slot);
	val.type        = htonl(pval->type);
	val.current_line        = htonl(pval->current_line);

	struct glink_base  base_hdr;


	len = sizeof(struct tms_ret_olp_line);
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_RET_OLP_ACTION_LOG, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&val,   sizeof(struct tms_ret_olp_line));
	glink_SendTail(fd);


	return 0;
}


// #define ID_GET_OLP_INFO 0x80000095 ///< 网管查询OLP模块的工作模式、返回时间和切换门限

static int32_t tms_AnalyseGetOLPInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_get_olp_info *pval;

	pval  = (struct tms_get_olp_info *)(pdata + GLINK_OFFSET_DATA);
	pval->frame = htonl(pval->frame);
	pval->slot  = htonl(pval->slot);
	pval->type  = htonl(pval->type);

	printf("%s():%d\n", __FUNCTION__, __LINE__);
	printf("\tval:f%d/s%x/t%d\n", pval->frame, pval->slot, pval->type);

	return 0;
}
// #define ID_RET_OLP_INFO 0x80000096 ///< MCU返回模块的工作模式、返回时间和切换门限
int32_t tms_RetOLPInfo(int fd, struct glink_addr *paddr,
                       struct tms_ret_olp_info *pval)
{
	int len ;
	struct tms_ret_olp_info val;

	val.frame        = htonl(pval->frame);
	val.slot         = htonl(pval->slot);
	val.type         = htonl(pval->type);
	val.flag         = htonl(pval->flag);
	val.mode         = htonl(pval->mode);
	val.back_time    = htonl(pval->back_time);
	val.protect_port = htonl(pval->protect_port);
	val.work_line    = htonl(pval->work_line);
	val.sw_level1    = htonl(pval->sw_level1);
	val.sw_level2    = htonl(pval->sw_level2);
	val.tx_ref       = htonl(pval->tx_ref);
	val.tx_jump      = htonl(pval->tx_jump);

	struct glink_base  base_hdr;


	len = sizeof(struct tms_ret_olp_info);
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_RET_OLP_ACTION_LOG, len);
	glink_SendHead(fd, &base_hdr);
	glink_SendSerial(fd, (uint8_t *)&val,   sizeof(struct tms_ret_olp_info));
	glink_SendTail(fd);


	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////
// 数据包分析
// 命令名列表0x1000xxxx、0x6000xxxx、0x8000xxxx
static struct pro_list g_cmdname_0x1000xxxx[] = {
	{"ID_TICK"},
	{"ID_UPDATE"},
	{"ID_TRACE0"},
	{"ID_TRACE1"},
	{"ID_TRACE2"},
	{"ID_R_COMMAMD"},
	{"ID_COMMAND"},



};


static struct pro_list g_cmdname_0x6000xxxx[] = {
	{"ID_GET_DEVTYPE"},
	{"ID_RET_DEVTYPE"},
	{"ID_CU_NOTE_NET"},
	{"ID_CU_NOTE_MANAGE_CONNECT"},
	{"ID_GET_OPM_OLP_RAYPOWER"},
	{"ID_CMD_OSW_SWITCH"},
	{"ID_CMD_OLP_REQ_OTDR"},
	{"ID_CMD_OLP_START_OTDR"},
	{"ID_CMD_OLP_FINISH_OTDR"},
	{"ID_GET_ALARM_TEST"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_RET_ALARM_TEST"},
	{"ID_GET_DEV_STATE_FROM_TU"},
	{"ID_RET_DEV_STATE_FROM_TU"},
	{"ID_GET_POWER_STATE_FROM_TU"},
	{"ID_RET_POWER_STATE_FROM_TU"},
	{"ID_MCU_GET_DEV_ALARM"},
	{"ID_DEV_RET_MCU_ALARM"},
	{"ID_OLP_REQUEST_OTDR"},

};


static struct pro_list g_cmdname_0x8000xxxx[] = {
	{"ID_GETBASICINFO"},
	{"ID_GETNODETIME"},
	{"ID_RETNodeTime"},
	{"ID_NAMEANDADDRESS"},
	{"ID_FIBERSECTIONCFG"},
	{"ID_CONFIGPIPESTATE"},
	{"ID_GETCYCLETESTCUV"},
	{"ID_GETSTATUSDATA"},
	{"ID_STATISDATA"},
	{"ID_CRCCHECKOUT"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_CHECKOUTRESULT"},
	{"ID_OTDRBASICINFO"},
	{"ID_CONFIGNODETIME"},
	{"ID_CURALARM"},
	{"ID_GETOTDRDATA_14"},
	{"ID_GETOTDRDATA_15"},
	{"ID_RETOTDRDATA_16"},
	{"ID_RETOTDRDATA_17"},
	{"ID_RETOTDRDATA_18"},
	{"ID_RETOTDRDATA_19"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_GETSTANDARDCURV"},
	{"ID_ERROR"},
};


/**
 * @brief	调试用，打印命令名称
 * @param[in]	cmdid
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 */

void tms_PrintCmdName(int32_t cmdid)
{
	uint32_t cmdh, cmdl;
	cmdh = cmdid & 0xf0000000;
	cmdl = cmdid & 0x0fffffff;

	switch(cmdh) {
	case 0x80000000:
		if (cmdl >= sizeof(g_cmdname_0x8000xxxx) / sizeof(struct pro_list)) {
			printf("0x80000000 out of cmd name memory!!! ");
			goto _Unknow;
		}
		printf("%s\n", g_cmdname_0x8000xxxx[cmdl].name);
		break;
	case 0x60000000:
		if (cmdl >= sizeof(g_cmdname_0x6000xxxx) / sizeof(struct pro_list)) {
			printf("0x60000000 out of cmd name memory!!!\n");
			goto _Unknow;
		}
		printf("%s\n", g_cmdname_0x6000xxxx[cmdl].name);

		break;
	case 0x10000000:
		if (cmdl >= sizeof(g_cmdname_0x1000xxxx) / sizeof(struct pro_list)) {
			printf("0x10000000 out of cmd name memory!!!\n");
			goto _Unknow;
		}
		printf("%s\n", g_cmdname_0x1000xxxx[cmdl].name);
		break;
	default:
_Unknow:
		;
		printf("unname\n");
		break;
	}
}

// 转发网管的数据到设备
// static
int32_t tms_Transmit2Dev(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	int fd;
	uint32_t frame, slot;
	struct tms_devbase out;
	struct tms_dev_slot *pval;

	pval  = (struct tms_dev_slot *)(pdata + GLINK_OFFSET_DATA);
	frame = htonl(pval->frame);
	slot  = htonl(pval->slot);

	fd = tms_GetDevBaseByLocation(frame, slot, &out);
	// 色号吧不存在
	if (fd == 0) {
		// TODO 发送错误码
		return 0;
	}
	else {
		return glink_SendSerial(fd, (uint8_t *)pdata, len);
	}

}
// 转发设备的数据到网管
// static
int32_t tms_Transmit2Manager(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	int fd;
	// struct glink_base  base_hdr;
	uint32_t src, dst;

	struct glink_base *pbase_hdr;
	pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));
	src = htonl(pbase_hdr->src);
	dst = htonl(pbase_hdr->dst);

	dbg_tms("tms_Transmit2Manager()\n");
	dbg_tms("\t%x , %x\n", src, dst);

	// 过滤设备发往MCU的数据包，不向网管转发
	if (dst == GLINK_4412_ADDR ||
	    src == GLINK_4412_ADDR ||
	    GLINK_MASK_MADDR != (dst & GLINK_MASK_MADDR) ) {
		dbg_tms("can't not transmit to manager\n");
		return 0;
	}

	pbase_hdr->dst = htonl(GLINK_MANAGE_ADDR);
	pbase_hdr->src = htonl(GLINK_4412_ADDR);
	// PrintfMemory((uint8_t*)pdata,20);
	fd = tms_SelectFdByAddr(&dst);
	dbg_tms("manager fd = %d\n", fd);
	if (fd == 0) {
		return 0;
	}
	return glink_SendSerial(fd, (uint8_t *)pdata, len);
}

// 向所有网管转发
int32_t tms_Transmit2AllManager(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}

/**
 * @brief	向所有网管群发发送
 			只能应用于动态分配内存的接口
 * @param	pbase_hdr glink_base 描述信息
 * @param	pdata 数据内容
 * @param	len 数据长度
 * @see	tms_SendAllManagerDot
 */

int32_t tms_SendAllManager(struct glink_base  *pbase_hdr, uint8_t *pdata, int32_t len)
{
	int fd;
	uint32_t dst;
	// 以后改用 tms_SelectFdByIndex
	printf("%8.8x %8.8x\n", pbase_hdr->src, pbase_hdr->dst);
	for (dst = 0x3a; dst <= 0x3f; dst++) {
		fd = tms_SelectFdByAddr(&dst);
		// 没有该地址的网管
		if (0 == fd) {
			continue;
		}
		dbg_tms("send all manager %x\n", dst);
		pbase_hdr->dst = htonl(dst);
		glink_Send(fd, NULL, pbase_hdr, pdata, len);
	}
	return 1;
}

/**
 * @brief	向所有网管群发发送，类似printf的可变参数，群发
 			应用于连续、非连续内存的接口
 * @param	pbase_hdr glink_base 描述信息
 * @param	group 有多少组参数，参数 data，len 为一组
 * @param	fmt 无用，填 NULL
 * @param	pdata 参数以 data1，len1，data2，len2...规则
  * @see	tms_SendAllManager
 */

int32_t tms_SendAllManagerDot(struct glink_base  *pbase_hdr, int group, uint8_t *fmt, ...)
{
	va_list args;
	int fd;
	uint32_t dst;
	uint8_t *pdata;
	uint32_t len;
	int i;



	// 以后改用 tms_SelectFdByIndex
	for (dst = 0x3a; dst <= 0x3f; dst++) {
		fd = tms_SelectFdByAddr(&dst);
		// 没有该地址的网管
		if (0 == fd) {
			continue;
		}
		dbg_tms("send all manager %x\n", dst);
		pbase_hdr->dst = htonl(dst);


		glink_SendHead(fd, pbase_hdr);
		va_start(args, (const char *)fmt);
		for (i = 0; i < group; i++) {


			pdata = va_arg(args, uint8_t *);
			len = va_arg(args, int);
			glink_SendSerial(fd, (uint8_t *)pdata,      len);
		}
		glink_SendTail(fd);
		va_end(args);
	}


	return 0;
}
// 拷贝本地字节序到本地用户空间
static int32_t tms_Copy2Use(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{

	struct glink_base *pbase_hdr;
	pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));
#if 0
	pbase_hdr->cmdid   = htonl(pbase_hdr->cmdid);
	pbase_hdr->datalen = htonl(pbase_hdr->datalen);

	if (pcontext->ptcb->pf_OnCopy2Use) {
		pcontext->ptcb->pf_OnCopy2Use(
		    (char *)(pdata + GLINK_OFFSET_CMDID),
		    pbase_hdr->datalen,
		    1,
		    pcontext->fd);
	}
	return 0;
#endif

#if 1
	// TMSxxV1.2 修改，为了上层能得到序列号
	// 为了今后的扩展，最后一个参数改成指针，获取glink帧所有数据，目前只返回源地址、目的地址、序列号
	// pbase_hdr->pklen   = htonl(pbase_hdr->pklen);
	// pbase_hdr->version = htonl(pbase_hdr->version);
	// pbase_hdr->src     = htonl(pbase_hdr->src);
	// pbase_hdr->dst     = htonl(pbase_hdr->dst);
	// pbase_hdr->type    = htonl(pbase_hdr->type);
	// pbase_hdr->pkid    = htons(pbase_hdr->pkid);
	// pbase_hdr->reserve = htonl(pbase_hdr->reserve);
	// pbase_hdr->cmdid   = htonl(pbase_hdr->cmdid);
	// pbase_hdr->datalen = htonl(pbase_hdr->datalen);


	// 可以考虑在这里提取 glink 头指针 pcontext->pgb = htonl(pbase_hdr->xxx);
	pbase_hdr->pklen 	= htonl(pbase_hdr->pklen);
	pbase_hdr->version 	= htonl(pbase_hdr->version);
	pbase_hdr->src 		= htonl(pbase_hdr->src);
	pbase_hdr->dst 		= htonl(pbase_hdr->dst);
	pbase_hdr->type 	= htons(pbase_hdr->type);
	pbase_hdr->pkid 	= htons(pbase_hdr->pkid);
	pbase_hdr->reserve 	= htonl(pbase_hdr->reserve);
	pbase_hdr->cmdid 	= htonl(pbase_hdr->cmdid);
	pbase_hdr->datalen 	= htonl(pbase_hdr->datalen);
	pcontext->pgb = pbase_hdr;

	// 强制指向指针
	// pcontext->pgb = pbase_hdr;

	if (pcontext->ptcb->pf_OnCopy2Use) {
		pcontext->ptcb->pf_OnCopy2Use(
		    (char *)(pdata + GLINK_OFFSET_CMDID),
		    pbase_hdr->datalen,
		    1,
		    pcontext);
	}

	return 0;
#endif
}
static int32_t tms_AnalyseUnuse(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// printf("tms_AnalyseUnuse\n");
	return 0;
}

#if 0
/**
 * @brief	ID_GET_DEV_STATE_FROM_TU 0x60000011 TU工控板向TU板卡查询其所在机框的板卡插拔状态
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定板卡端口数
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_MCUtoDevice
 */
uint32_t tms_GetDevStateFromTU(
    int     fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_GET_DEV_STATE_FROM_TU, 0);
}


/**
 * @brief	ID_RET_DEV_STATE_FROM_TU	0x60000012 ///<TU板卡返回其所在机框的板卡插拔状态
 */
static int32_t tms_AnalyseRetDevStateFromTU(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseMCUtoDevice(pdata, sizeof(struct tms_dev_status_from_tu));
	return 0;
}
/**
 * @brief	ID_GET_POWER_STATE_FROM_TU 0x60000013 TU工控板向TU板卡查询其所在机框的板卡插拔状态
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定板卡端口数
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_MCUtoDevice
 */
uint32_t tms_GetPowerStateFromTU(
    int     fd,
    struct glink_addr *paddr)
{
	return tms_MCUtoDevice(fd, paddr, 0, 0, 0, 0, ID_GET_POWER_STATE_FROM_TU, 0);
}
//todo---

/**
 * @brief	ID_RET_POWER_STATE_FROM_TU 	0x60000014 ///<TU板卡返回其所在机框的电源组成信息
 */
static int32_t tms_AnalyseRetPowerStateFromTU(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// return tms_AnalyseMCUtoDevice(pdata, sizeof(struct tms_mcu_get_dev_alarm));
	struct tms_ret_power_state_from_tu *pcfg_hdr;
	struct tms_ret_power_state_from_tu_val  *plist, *ptlist;

	pcfg_hdr = (struct tms_ret_power_state_from_tu *)(pdata + GLINK_OFFSET_DATA);
	if ( !CHECK_PTR(
	         pcfg_hdr,
	         struct tms_ret_power_state_from_tu,
	         struct tms_ret_power_state_from_tu_val,
	         htonl(pcfg_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	plist    = (struct tms_ret_power_state_from_tu_val *)  (pdata + GLINK_OFFSET_DATA + sizeof(struct tms_ret_power_state_from_tu));
	// PrintfMemory((uint8_t *)plist, sizeof(struct tms_alarm_hw_val));

	pcfg_hdr->frame = htonl(pcfg_hdr->frame);
	pcfg_hdr->slot = htonl(pcfg_hdr->slot);
	pcfg_hdr->type = htonl(pcfg_hdr->type);
	pcfg_hdr->count = htonl(pcfg_hdr->count);

	// TODO：防止count溢出
	ptlist = plist;
	for (int i = 0; i < pcfg_hdr->count; i++) {
		ptlist->frame		= htonl(ptlist->frame);
		ptlist->slot		= htonl(ptlist->slot);
		ptlist->type		= htonl(ptlist->type);
		ptlist->port		= htonl(ptlist->port);
		ptlist->reserve0	= htonl(ptlist->reserve0);
		ptlist->reserve1	= htonl(ptlist->reserve1);
		ptlist->reserve2	= htonl(ptlist->reserve2);
		ptlist->reserve3	= htonl(ptlist->reserve3);
		ptlist++;
	}
	return 0;
}

/**
 * @brief	ID_MCU_GET_DEV_ALARM	 	0x60000015 ///<工控板查询某槽位上总的硬件告警
 * @param[in]	fd 套接字文件描述符
 * @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	port 指定板卡端口数
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see	tms_MCUtoDevice
 */
uint32_t tms_MCUGeTDevAlarm(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type)
{
	return tms_MCUtoDevice(fd, paddr, frame, slot, type, 0, ID_MCU_GET_DEV_ALARM, sizeof(struct tms_mcu_get_dev_alarm));
}



// ID_DEV_RET_MCU_ALARM	 	0x60000016 ///<各业务单板向MCU返回总的硬件告警
static int32_t tms_AnalyseDevRetMCUAlarm(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// 下面仅仅是个模板
	// int tcount;
	// #define CHECK_PTR(ptrA, struct_A, struct_B, B_Count, PtrEnd)
	struct tms_dev_ret_mcu_alarm *pcfg_hdr;
	struct tms_dev_ret_mcu_alarm_val  *plist, *ptlist;

	pcfg_hdr = (struct tms_dev_ret_mcu_alarm *)(pdata + GLINK_OFFSET_DATA);
	if ( !CHECK_PTR(
	         pcfg_hdr,
	         struct tms_dev_ret_mcu_alarm,
	         struct tms_dev_ret_mcu_alarm_val,
	         htonl(pcfg_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	plist    = (struct tms_dev_ret_mcu_alarm_val *)  (pdata + GLINK_OFFSET_DATA + sizeof(struct tms_dev_ret_mcu_alarm));
	// PrintfMemory((uint8_t *)plist, sizeof(struct tms_alarm_hw_val));

	pcfg_hdr->alarm_type = htonl(pcfg_hdr->alarm_type);
	pcfg_hdr->frame = htonl(pcfg_hdr->frame);
	pcfg_hdr->slot = htonl(pcfg_hdr->slot);
	pcfg_hdr->type = htonl(pcfg_hdr->type);
	pcfg_hdr->count = htonl(pcfg_hdr->count);

	// TODO：防止count溢出
	ptlist = plist;
	for (int i = 0; i < pcfg_hdr->count; i++) {
		ptlist->level      = htonl(ptlist->level);
		ptlist->frame      = htonl(ptlist->frame);
		ptlist->slot       = htonl(ptlist->slot);
		ptlist->reason     = htonl(ptlist->reason);
		ptlist++;
	}
	return 0;
}

// ID_OLP_REQUEST_OTDR		0x60000017 ///<OLP板卡向MCU请求OTDR测试
static int32_t tms_AnalyseOLPRequestOTDR(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	tms_AnalyseMCUtoDevice(pdata, sizeof(struct tms_olp_request_otdr));
	return 0;
}



//0x80000028
static int32_t tms_AnalyseAlarmOPM(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_alarm_opm      *phdr;
	struct tms_alarm_opm_val  *plist, *ptlist;

	phdr  = (struct tms_alarm_opm *)(pdata + GLINK_OFFSET_DATA );
	plist = (struct tms_alarm_opm_val *)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_alarm_opm));

	phdr->alarm_type  = htonl(phdr->alarm_type);
	phdr->frame = htonl(phdr->frame);
	phdr->slot  = htonl(phdr->slot);
	phdr->count = htonl(phdr->count);

	ptlist = plist;
	for (int i = 0; i < phdr->count; i++) {
		ptlist->port  = htonl(ptlist->port);
		ptlist->levelx  = htonl(ptlist->levelx);
		ptlist->power = htonl(ptlist->power);
		ptlist++;
	}

	printf("tms_AnalyseAlarmOPM\n");
	printf("val:f%d/s%d/t%d\n", phdr->frame, phdr->slot, phdr->alarm_type);
	printf("count %d\n", phdr->count);
	ptlist = plist;
	for (int i = 0; i < phdr->count; i++) {
		printf("\tport %d Lvx %d power %d times %s\n", ptlist->port, ptlist->levelx, ptlist->power, ptlist->time);
		ptlist++;
	}

	return 0;
}





// 0x80000029
static int32_t tms_AnalyseAlarmOPMChange(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_alarm_opm_change      *phdr;
	struct tms_alarm_opm_val  *plist, *ptlist;

	printf("--------------------------\n");
	printf("tms_AnalyseAlarmOPMChange\n");
	phdr  = (struct tms_alarm_opm_change *)(pdata + GLINK_OFFSET_DATA );
	if ( !CHECK_PTR(
	         phdr,
	         struct tms_alarm_opm_change,
	         struct tms_alarm_opm_val,
	         htonl(phdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	plist = (struct tms_alarm_opm_val *)(pdata + GLINK_OFFSET_DATA + sizeof(struct tms_alarm_opm_change));

	phdr->alarm_type  = htonl(phdr->alarm_type);
	phdr->frame		  = htonl(phdr->frame);
	phdr->slot 		  = htonl(phdr->slot);
	phdr->alarm_cnt   = htonl(phdr->alarm_cnt);
	phdr->count       = htonl(phdr->count);


	ptlist = plist;
	for (int i = 0; i < phdr->count; i++) {
		ptlist->port  = htonl(ptlist->port);
		ptlist->levelx  = htonl(ptlist->levelx);
		ptlist->power = htonl(ptlist->power);
		ptlist++;
		printf("%d ", i);
	}

	printf("\n");
	printf("val:f%d/s%x alarm type:%d alarm count:%d hdr->count %d\n", phdr->frame, phdr->slot, phdr->alarm_type, phdr->alarm_cnt, phdr->count);
	ptlist = plist;
	for (int i = 0; i < phdr->count; i++) {
		printf("\tport %d Lvx %d power %d times %s\n", ptlist->port, ptlist->levelx, ptlist->power, ptlist->time);
		ptlist++;
	}

	printf("--------------------------\n");
	return 0;
}


/**
 * @brief
 * @param	null
 * @retval	null
 * @remarks
 * @see
 */
int32_t tms_AdjustTime(
    int fd,
    struct glink_addr *paddr,
    int8_t (*time)[20])
{
	int8_t pdata[20];

	memcpy(pdata, &time[0][0], 20);

	struct glink_base  base_hdr;
	uint8_t *pmem;

	pmem = (uint8_t *)&pdata;
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_ADJUST_TIME, sizeof(struct tms_ack));
	glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_ack));
	return 0;
}



//0x80000037
static int32_t tms_AnalyseAdjustTime(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	printf("tms_AnalyseAdjustTime\n");
	return 0;
}

/**
* @brief	ID_CMD_ACK 0x80000038 RTU返回应答码
* @param[in]	fd 套接字文件描述符
* @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
				修改tmsxx.h设定
* @param[in]	errcode 错误代码
* @param[in]	cmdid 因为哪个命令而相应的ID
* @retval	>0 发送成功
* @retval	0 发送失败，该链接失效，立即断开
* @remarks
*/
int32_t tms_Ack(
    int fd,
    struct glink_addr *paddr,
    int errcode,
    int cmdid)
{
	struct tms_ack ack;

	ack.errcode = htonl(errcode);
	ack.cmdid   = htonl(cmdid);

	struct glink_base  base_hdr;
	uint8_t *pmem;
	int ret;

	pmem = (uint8_t *)&ack;
	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_CMD_ACK, sizeof(struct tms_ack));
	ret = glink_Send(fd, NULL, &base_hdr, pmem, sizeof(struct tms_ack));
	return ret;
}

//0x80000038
static int32_t tms_AnalyseAck(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// 特殊处理
	if (pcontext->ptcb->pf_OnSpAck) {
		pcontext->ptcb->pf_OnSpAck(pcontext, pdata, len);
	}


	static struct pro_list list[] = {
		{"RET_SUCCESS"},
		{"RET_UNEXIST"},
		{"RET_COMMU_ABORT"},
		{"RET_UNMATCH_FRAME"},
		{"RET_UNMATCH_SLOT"},
		{"RET_UNMATCH_TYPE"},
		{"RET_PARAM_INVALID"},
		{"RET_IGNORE_SAVE"},
		{"RET_WAIT"},
		{"RET_OTDR_ILLEGAL"},
		{"RET_OTDR_TIMEOUT"},
		{"RET_UPDATE_ILLEGAL"},
		{"RET_CMD_INVALID"},
		{"RET_OLP_CANT_SWITCH"},
		{"RET_OSW_SWITCH_ABORT"},
		{"RET_SEND_CMMD_TIMEOUT"},
		{"RET_UNEXIST_ROW"},
		{"RET_OLP_REFUSE"},
		{"RET_RESOURCE_LOW"},
		{"RET_SAVE_SOURCE"},
	};
	struct tms_ack *pval;
	pval = (struct tms_ack *)(pdata + GLINK_OFFSET_DATA);
	pval->errcode = htonl(pval->errcode);
	pval->cmdid   = htonl(pval->cmdid);
	// pval->reserve1 = htonl(pval->reserve1);
	// pval->reserve2 = htonl(pval->reserve2);
	// pval->reserve3 = htonl(pval->reserve3);
	// pval->reserve4 = htonl(pval->reserve4);


	printf("tms_AnalyseAck\n");
	if ((uint32_t)pval->errcode >= sizeof(list) / sizeof(struct pro_list)) {
		printf("\terror errcode [%2.2d]!!!\n", pval->errcode);
	}
	else {
		// printf("\tf%d/s%d/t%d/p%d err [%2.2d] %s \t\tcmdid [0x%8.8x] ",
		// (pval->reserve1 >> 16) & 0xffff,
		// (pval->reserve1 >> 0) & 0xffff,
		// (pval->reserve2 >> 16) & 0xffff,
		// (pval->reserve2 >> 0) & 0xffff,
		// pval->errcode, list[pval->errcode].name, pval->cmdid);
		tms_PrintCmdName(pval->cmdid);
	}

	return 0;
}


static int tms_AnalyseSMSError(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_sms_state *pval;
	pval = (struct tms_sms_state *)(pdata + GLINK_OFFSET_DATA);
	pval->state = htonl(pval->state);

	printf("tms_AnalyseSMSError()\n");
	printf("SMS Phone:%s Code:%d", pval->phone, pval->state);
	if (pval->state == 0) {
		printf("Success\n");
	}
	else if (pval->state == 1) {
		printf("Error\n");
	}
	return 0;
}

// 无脑应答设备
static int tms_AckDevice(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// pcontext->fd,
	// int32_t tms_AckEx(
	// int fd,
	// struct glink_addr *paddr,
	// struct tms_ack *pack)

	// int fd;
	struct tms_ack ack;
	// uint32_t src,dst;
	struct glink_base *pbase_hdr;
	struct glink_addr base_hdr;

	pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));

	// printf(" ..................%s %x id %x\n", __FUNCTION__, htonl(pbase_hdr->src) ,htonl(pbase_hdr->cmdid));
	if (htonl(pbase_hdr->src) != GLINK_DEV_ADDR) {
		return 0;
	}

	base_hdr.pkid = htonl(pbase_hdr->pkid);
	base_hdr.src = htonl(pbase_hdr->dst);
	base_hdr.dst = htonl(pbase_hdr->src);

	ack.cmdid = htonl(pbase_hdr->cmdid);
	ack.errcode = htonl(RET_SUCCESS);
	// ack.reserve1 = 0;
	// ack.reserve2 = 0;
	// ack.reserve3 = 0;
	// ack.reserve4 = 0;
	tms_AckEx(pcontext->fd, &base_hdr, &ack);


	// struct tms_context *popt;
	// popt = (tms_context *)ptr_opt;
	// gl_addr.dst = popt->pgb->src;
	// gl_addr.src = ADDR_MCU;
	// gl_addr.pkid = popt->pgb->pkid;
	// //fd = tms_SelectFdByAddr(&gl_addr.dst);
	// fd = popt->fd;
	// bzero(&ack, sizeof(tms_ack));
	// ack.cmdid = res_cmd;
	// ack.errcode = res_code;
	// ack.reserve1 = MCU_CARD&0x0000FFFF;
	// ack.reserve2 = MCU_CARD<<16&0xFFFF0000;
	// ack.reserve3 = opt;
	// tms_AckEx(fd, &gl_addr,&ack);
	// #if USR_DEBUG
	// qDebug("mcu_ack fd %d src 0x%x dst 0x%x res_code %d cmd 0x%x",
	// 	fd, gl_addr.src, gl_addr.dst, res_code, res_cmd);
	// #endif
	return 0;
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////
// hebei2

// 0x20000000	ID_CHECKOUTRESULT
static int32_t tms_AnalyseCheckoutResult(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	if (pcontext->ptcb->pf_OnCheckoutResult) {
		pcontext->ptcb->pf_OnCheckoutResult(pcontext);
	}
	return 0;
}

// 0x60000000	ID_SETSMSINFO
static int32_t tms_AnalyseSetSMSInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}
// 0x60000001	ID_SMSINFO
static int32_t tms_AnalyseSMSInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}
// 0x60000002	ID_SENDSMSINFO
static int32_t tms_AnalyseSendSMSInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}
// 0x60000003	ID_SENDSMSINFORETCODE
static int32_t tms_AnalyseSendSMSInfoRetCode(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}




//	0x70000000	ID_SETOTDRFPGAINFO
static int32_t tms_AnalyseSetOTDRFPGAInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}

//	0x70000001	ID_SETOCVMPARA
static int32_t tms_AnalyseSetOCVMPara(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}








//	0x80000000	ID_GETBASICINFO
static int32_t tms_AnalyseGetBasicInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	struct tms_ack ack;
	ack.errcode = 0;
	ack.cmdid = ID_GETBASICINFO;
	tms_AckEx(pcontext->fd, NULL, &ack);
#endif
	hh2_dbg("Warning 应该返回什么内容，协议里没详细说明\n");
	if (pcontext->ptcb->pf_OnGetBasicInfo) {
		pcontext->ptcb->pf_OnGetBasicInfo(pcontext);
	}

	// struct tms_context con;
	// int ret = tms_SelectContextByFD(6,&con);
	// printf("ret = %d %d\n", ret, con.fd);
	return 0;
}



//	0x80000001	ID_GETNODETIME
static int32_t tms_AnalyseGetNodeTime(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	struct tms_ack ack;
	ack.errcode = 0;
	ack.cmdid = ID_RETNODETIME;
	tms_AckEx(pcontext->fd, NULL, &ack);
#endif
	hh2_dbg("Warning CU 需要多次转发此消息\n");
	if (pcontext->ptcb->pf_OnGetNodeTime) {
		pcontext->ptcb->pf_OnGetNodeTime(pcontext);
	}
	return 0;
}

int32_t tms_RetNodeTime(
    struct tms_context *pcontext,
    struct glink_addr *paddr,
    char *tm)
{
	struct glink_base  base_hdr;
	char *pmem;
	int len;

	pmem = tm;
	len = strlen(tm);

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == pcontext->fd) {
		// fd =tms_SelectFdByAddr(&base_hdr.dst);
	}
	glink_Build(&base_hdr, ID_RETNODETIME, len);
	glink_Send(pcontext->fd, NULL, &base_hdr, pmem, len);
}


//	0x80000002	ID_RETNodeTime
static int32_t tms_AnalyseRetNodeTime(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}


//	0x80000003	ID_NAMEANDADDRESS
static int32_t tms_AnalyseNameAndAddress(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	hh2_dbg("Warning CU 需要处理此消息\n");
	return 0;
}


//	0x80000004	ID_FIBERSECTIONCFG
static int32_t tms_AnalyseFiberSectionCfg(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}


//	0x80000005	ID_CONFIGPIPESTATE
static int32_t tms_AnalyseConfigPipeState(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	struct tms_ack ack;
	ack.errcode = 0;
	ack.cmdid = ID_CONFIGPIPESTATE;
	tms_AckEx(pcontext->fd, NULL, &ack);
#endif
	struct tms_cfgpip_status *pval;
	pval = (struct tms_cfgpip_status *)(pdata + GLINK_OFFSET_DATA);
	//
	pval->status = htonl(pval->status);

	if (pcontext->ptcb->pf_OnConfigPipeState) {
		pcontext->ptcb->pf_OnConfigPipeState(pcontext, pval);
	}
	return 0;
}


//	0x80000006	ID_GETCYCLETESTCUV
static int32_t tms_AnalyseGetCycleTestCuv(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_getcyctestcuv *pval;
	pval = (struct tms_getcyctestcuv *)(pdata + GLINK_OFFSET_DATA);
	
	pval->pipe = htonl(pval->pipe);
	if (pcontext->ptcb->pf_OnGetCycleTestCuv) {
		pcontext->ptcb->pf_OnGetCycleTestCuv(pcontext, pval);
	}
	return 0;
}


//	0x80000007	ID_GETSTATUSDATA
static int32_t tms_AnalyseGetStatusData(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct tms_getstatus_data *pval;
	pval = (struct tms_getstatus_data *)(pdata + GLINK_OFFSET_DATA);
	
	pval->pipe = htonl(pval->pipe);
	if (pcontext->ptcb->pf_OnGetStatusData) {
		pcontext->ptcb->pf_OnGetStatusData(pcontext, pval);
	}
	return 0;
}


//	0x80000008	ID_STATUSDATA
int32_t tms_RetStatusData(struct tms_context *pcontext, 
	struct glink_addr *paddr,
	struct tms_getstatus_data_hdr *hdr,
	struct tms_getstatus_data_val *val,
	int32_t ilen)
{
	struct tms_getstatus_data_hdr data_hdr;
	struct tms_getstatus_data_val data_val[8], *ptdata;
	int len;

	if (ilen >= 8) {
		ilen = 8;
	}
	memcpy(&data_hdr, hdr, sizeof(struct tms_getstatus_data_hdr));
	memcpy(data_val, val, sizeof(struct tms_getstatus_data_val) * ilen);
	
	len = sizeof(struct tms_getstatus_data_hdr) +
		sizeof(struct tms_getstatus_data_val) * ilen;

	data_hdr.count = htonl(data_hdr.count);

	ptdata = &data_val[0];
	for (int i = 0; i < ilen; i++) {
		ptdata->pipe = htonl(ptdata->pipe);
		ptdata->section_num = htonl(ptdata->section_num);
		ptdata->section_atten = htonl(ptdata->section_atten);
		ptdata++;
	}

	struct glink_base  base_hdr;
	tms_FillGlinkFrame(&base_hdr, paddr);
	glink_Build(&base_hdr, ID_STATUSDATA, len);

	pthread_mutex_lock(&pcontext->mutex);
	glink_SendHead(pcontext->fd, &base_hdr);
	glink_SendSerial(pcontext->fd, (uint8_t *)&data_hdr,   sizeof(struct tms_getstatus_data_hdr));
	glink_SendSerial(pcontext->fd, (uint8_t *)data_val, sizeof(struct tms_getstatus_data_val) * ilen);
	glink_SendTail(pcontext->fd);
	pthread_mutex_lock(&pcontext->mutex);
}
static int32_t tms_AnalyseStatusData(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}


//	0x80000009	ID_CRCCHECKOUT
static int32_t tms_AnalyseCRCCheckout(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{

	if (pcontext->ptcb->pf_OnCRCCheckout) {
		pcontext->ptcb->pf_OnCRCCheckout(pcontext);
	}
	return 0;
}


//	0x80000010	ID_CHECKOUTRESULT
int32_t tms_CheckoutResult(struct tms_context *pcontext, 
	struct glink_addr *paddr,
	uint32_t *idata)
{
	uint32_t pdata = idata;
	pdata = htonl(pdata);

	struct glink_base  base_hdr;
	tms_FillGlinkFrame(&base_hdr, paddr);
	glink_Build(&base_hdr, ID_STATUSDATA, sizeof(int32_t));


	pthread_mutex_lock(&pcontext->mutex);
	glink_SendHead(pcontext->fd, &base_hdr);
	glink_SendSerial(pcontext->fd, (uint8_t *)&pdata,   sizeof(uint32_t));
	glink_SendTail(pcontext->fd);
	pthread_mutex_lock(&pcontext->mutex);
}

static int32_t tms_AnalyseOTDRBasicInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	if (pcontext->ptcb->pf_OnOTDRBasicInfo) {
		pcontext->ptcb->pf_OnOTDRBasicInfo(pcontext);
	}

}

//	0x80000011	ID_OTDRBASICINFO
static int32_t tms_AnalyseConfigNodeTime(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	struct tms_ack ack;
	ack.errcode = 0;
	ack.cmdid = ID_CONFIGNODETIME;
	tms_AckEx(pcontext->fd, NULL, &ack);
#endif
	if (pcontext->ptcb->pf_OnConfigNodeTime) {
		pcontext->ptcb->pf_OnConfigNodeTime(pcontext);
	}

}

//	0x80000012	ID_CONFIGNODETIME
static int32_t tms_AnalyseCurAlarm(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	if (pcontext->ptcb->pf_OnCurAlarm) {
		pcontext->ptcb->pf_OnCurAlarm(pcontext);
	}

}

//	0x80000013	ID_CURALARM
static int32_t tms_AnalyseGetOTDRdata_14(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	if (pcontext->ptcb->pf_OnGetOTDRdata_14) {
		pcontext->ptcb->pf_OnGetOTDRdata_14(pcontext);
	}

}

// 0x80000014	ID_GETOTDRDATA_14
static int32_t tms_AnalyseGetOTDRData(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}
// 0x80000019	ID_RETOTDRDATA_19
static int32_t tms_AnalyseRetOTDRData(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}
// 0x80000020	ID_GETSTANDARDCURV
static int32_t tms_AnalyseGetStandardCurv(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}

/**
* @brief	ID_CMD_ACK 0x80000038 RTU返回应答码
* @param[in]	fd 套接字文件描述符
* @param[in]	paddr glink帧源地址和目的地址，如果填NULL则采用默认的
				TMS_DEFAULT_LOCAL_ADDR和TMS_DEFAULT_RMOTE_ADDR填充，两个宏的意义通过
				修改tmsxx.h设定
* @param[in]	errcode 错误代码
* @param[in]	cmdid 因为哪个命令而相应的ID
* @retval	>0 发送成功
* @retval	0 发送失败，该链接失效，立即断开
* @remarks
*/
int32_t tms_AckEx(
    int fd,
    struct glink_addr *paddr,
    struct tms_ack *pack)
{
	struct tms_ack ack;

	ack.errcode  = htonl(pack->errcode);
	ack.cmdid 	 = htonl(pack->cmdid);
	// ack.reserve1 = htonl(pack->reserve1);
	// ack.reserve2 = htonl(pack->reserve2);
	// ack.reserve3 = htonl(pack->reserve3);
	// ack.reserve4 = htonl(pack->reserve4);


	struct glink_base  base_hdr;
	uint8_t *pmem;
	int ret;


	pmem = (uint8_t *)&ack;

	tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {

		fd = tms_SelectFdByAddr(&paddr->dst);
		printf("fd = 0 find %d %x\n", fd, paddr->dst);
	}
	// glink_Build(&base_hdr, ID_CMD_ACK, sizeof(struct tms_ack));
	// ret = glink_Send(fd,NULL, &base_hdr, pmem, sizeof(struct tms_ack));

	glink_Build(&base_hdr, ID_ERROR, 8);
	ret = glink_Send(fd, NULL, &base_hdr, pmem, 8);
	return ret;
}


// static struct tms_analyse_array sg_analyse_0x1000xxxx[] = {
// 	// {	tms_AnalyseTick	, 1}, //	0x10000000	ID_TICK
// 	// {	tms_AnalyseUpdate	, PROCCESS_2DEV_AND_COPY2USE}, //	0x10000001	ID_UPDATE
// 	// {	tms_AnalyseTrace	, 1}, //	0x10000002	ID_TRACE0
// 	// {	tms_AnalyseTrace	, 1}, //	0x10000003	ID_TRACE1
// 	// {	tms_AnalyseTrace	, 1}, //	0x10000004	ID_TRACE2
// 	// {	tms_AnalyseCommand	, 1}, //	0x10000005	ID_TRACE3
// 	// {	tms_AnalyseCommand	, 1}, //	0x10000006	ID_COMMAND
// };


// #ifdef CONFIG_TEST_NET_STRONG
static struct tms_analyse_array sg_analyse_0x2000xxxx[] = {
	{	tms_AnalyseCheckoutResult	, 0}, //	0x20000000	ID_CHECKOUTRESULT
	// { 	tms_AnalyseTestPacketEcho, 1},
	// { 	tms_AnalyseTestPacketAck, 1},
};

// #endif
static struct tms_analyse_array sg_analyse_0x6000xxxx[] = {
	{	tms_AnalyseSetSMSInfo	, 8}, //	0x60000000	ID_SETSMSINFO
	{	tms_AnalyseSMSInfo	, 8}, //	0x60000001	ID_SMSINFO
	{	tms_AnalyseSendSMSInfo	, 8}, //	0x60000002	ID_SENDSMSINFO
	{	tms_AnalyseSendSMSInfoRetCode	, 8}, //	0x60000003	ID_SENDSMSINFORETCODE

	// {	tms_AnalyseMCU_OLPReqOTDRTest	, 0}, //	0x60000006	ID_CMD_OLP_REQ_OTDR
	// {	tms_AnalyseUnuse	, 2}, //	0x60000007	ID_CMD_OLP_START_OTDR
	// {	tms_AnalyseUnuse	, 2}, //	0x60000008	ID_CMD_OLP_FINISH_OTDR
	// {	tms_AnalyseUnuse	, 8}, //	0x60000009	ID_GET_ALARM_TEST
	// {	tms_AnalyseUnuse	, 8}, //	0x6000000A	--
	// {	tms_AnalyseUnuse	, 8}, //	0x6000000B	--
	// {	tms_AnalyseUnuse	, 8}, //	0x6000000C	--
	// {	tms_AnalyseUnuse	, 8}, //	0x6000000D	--
	// {	tms_AnalyseUnuse	, 8}, //	0x6000000E	--
	// {	tms_AnalyseUnuse	, 8}, //	0x6000000F	ID_GET_ALARM_TEST
	// {	tms_AnalyseAnyRetOTDRTest	, 0}, //	0x60000010	ID_RET_ALARM_TEST
	// {	tms_AnalyseUnuse	, 8}, //	0x60000011	ID_GET_DEV_STATE_FROM_TU
	// {	tms_AnalyseRetDevStateFromTU	, 0}, //	0x60000012	ID_RET_DEV_STATE_FROM_TU
	// {	tms_AnalyseUnuse	, 8}, //	0x60000013	ID_GET_POWER_STATE_FROM_TU
	// {	tms_AnalyseRetPowerStateFromTU	, 0}, //	0x60000014	ID_RET_POWER_STATE_FROM_TU
	// {	tms_AnalyseUnuse	, 8}, //	0x60000015	ID_MCU_GET_DEV_ALARM
	// {	tms_AnalyseDevRetMCUAlarm	, 0}, //	0x60000016	ID_DEV_RET_MCU_ALARM
	// {	tms_AnalyseDevRetMCUAlarm	, 0}, //	0x60000017	ID_OLP_REQUEST_OTDR

};
struct tms_analyse_array sg_analyse_0x7000xxxx[] = {
	{	tms_AnalyseSetOTDRFPGAInfo	, 0}, //	0x70000000	ID_SETOTDRFPGAINFO
	{	tms_AnalyseSetOCVMPara	, 0}, //	0x70000001	ID_SETOCVMPARA
};

struct tms_analyse_array sg_analyse_0x8000xxxx[] = {
	{	tms_AnalyseGetBasicInfo	, 0}, //	0x80000000	ID_GETBASICINFO
	{	tms_AnalyseGetNodeTime	, 1}, //	0x80000001	ID_GETNODETIME
	{	tms_AnalyseRetNodeTime	, 1}, //	0x80000002	ID_RETNodeTime
	{	tms_AnalyseNameAndAddress	, 8}, //	0x80000003	ID_NAMEANDADDRESS
	{	tms_AnalyseFiberSectionCfg	, 0}, //	0x80000004	ID_FIBERSECTIONCFG
	{	tms_AnalyseConfigPipeState	, 0}, //	0x80000005	ID_CONFIGPIPESTATE
	{	tms_AnalyseGetCycleTestCuv	, 0}, //	0x80000006	ID_GETCYCLETESTCUV
	{	tms_AnalyseGetStatusData	, 0}, //	0x80000007	ID_GETSTATUSDATA
	{	tms_AnalyseStatusData	, 0}, //	0x80000008	ID_STATISDATA
	{	tms_AnalyseCRCCheckout	, 0}, //	0x80000009	ID_CRCCHECKOUT
	{	tms_AnalyseUnuse	, 8}, //	0x8000000A	--
	{	tms_AnalyseUnuse	, 8}, //	0x8000000B	--
	{	tms_AnalyseUnuse	, 8}, //	0x8000000C	--
	{	tms_AnalyseUnuse	, 8}, //	0x8000000D	--
	{	tms_AnalyseUnuse	, 8}, //	0x8000000E	--
	{	tms_AnalyseUnuse	, 8}, //	0x8000000F	--
	{	tms_AnalyseCheckoutResult	, 0}, //	0x80000010	ID_CHECKOUTRESULT
	{	tms_AnalyseOTDRBasicInfo	, 0}, //	0x80000011	ID_OTDRBASICINFO
	{	tms_AnalyseConfigNodeTime	, 0}, //	0x80000012	ID_CONFIGNODETIME
	{	tms_AnalyseCurAlarm	, 0}, //	0x80000013	ID_CURALARM
	{	tms_AnalyseGetOTDRData	, 0}, //	0x80000014	ID_GETOTDRDATA_14
	{	tms_AnalyseGetOTDRData	, 0}, //	0x80000015	ID_GETOTDRDATA_15
	{	tms_AnalyseRetOTDRData	, 0}, //	0x80000016	ID_RETOTDRDATA_16
	{	tms_AnalyseRetOTDRData	, 0}, //	0x80000017	ID_RETOTDRDATA_17
	{	tms_AnalyseRetOTDRData	, 0}, //	0x80000018	ID_RETOTDRDATA_18
	{	tms_AnalyseRetOTDRData	, 8}, //	0x80000019	ID_RETOTDRDATA_19
	{	tms_AnalyseUnuse	, 8}, //	0x8000001A	0
	{	tms_AnalyseUnuse	, 8}, //	0x8000001B	0
	{	tms_AnalyseUnuse	, 8}, //	0x8000001C	0
	{	tms_AnalyseUnuse	, 8}, //	0x8000001D	0
	{	tms_AnalyseUnuse	, 8}, //	0x8000001E	0
	{	tms_AnalyseUnuse	, 8}, //	0x8000001F	0
	{	tms_AnalyseGetStandardCurv	, 0}, //	0x80000020	ID_GETSTANDARDCURV
	{	tms_AnalyseUnuse	, 0}, //	0x80000021	ID_ERROR

	// {	tms_AnalyseCfgOPMRefLevel	,2},//	0x80000015	ID_CFG_OPM_REF_LEVEL
	// {	tms_AnalyseGetOPMOP	,4},//	0x80000016	ID_GET_OPM_OP
	// {	tms_AnalyseRetOPMOP	,5},//	0x80000017	ID_RET_OPM_OP
	// {	tms_AnalyseCfgOLPRefLevel	,0},//	0x80000018	ID_CFG_OLP_REF_LEVEL
	// {	tms_AnalyseUnuse	,2},//	0x80000019	ID_GET_OLP_OP
	// {	tms_AnalyseUnuse	,8},//	0x8000001A	--
	// {	tms_AnalyseUnuse	,8},//	0x8000001B	--
	// {	tms_AnalyseUnuse	,8},//	0x8000001C	--
	// {	tms_AnalyseUnuse	,8},//	0x8000001D	--
	// {	tms_AnalyseUnuse	,8},//	0x8000001E	--
	// {	tms_AnalyseUnuse	,8},//	0x8000001F	--
	// {	tms_AnalyseRetOLPOP	,5},//	0x80000020	ID_RET_OLP_OP
	// {	tms_AnalyseCfgOTDRRef	,0},//	0x80000021	ID_CFG_OTDR_REF
	// {	tms_AnalyseCfgMCUOSWCycle	,0},//	0x80000022	ID_CFG_MCU_OSW_CYCLE
	// {	tms_AnalyseGetOTDRTest	,9},//	0x80000023	ID_GET_OTDR_TEST
	// {	tms_AnalyseRetOTDRTest	,5},//	0x80000024	ID_RET_OTDR_TEST
	// {	tms_AnalyseCfgOSWMode	,2},//	0x80000025	ID_CFG_OLP_MODE
	// {	tms_MCU_AnalyseOLPSwitch	,2},//	0x80000026	ID_CMD_OLP_SWITCH
	// {	tms_AnalyseReportOLPAction	,5},//	0x80000027	ID_REPORT_OLP_ACTION
	// {	tms_AnalyseAlarmOPM	,5},//	0x80000028	ID_ALARM_OPM
	// {	tms_AnalyseAlarmOPMChange	,5},//	0x80000029	ID_ALARM_OPM_CHANGE
	// {	tms_AnalyseUnuse	,8},//	0x8000002A	--
	// {	tms_AnalyseUnuse	,8},//	0x8000002B	--
	// {	tms_AnalyseUnuse	,8},//	0x8000002C	--
	// {	tms_AnalyseUnuse	,8},//	0x8000002D	--
	// {	tms_AnalyseUnuse	,8},//	0x8000002E	--
	// {	tms_AnalyseUnuse	,8},//	0x8000002F	--
	// {	tms_AnalyseAlarmLine	,0},//	0x80000030	ID_ALARM_LINE
	// {	tms_AnalyseAlarmHW	,9},//	0x80000031	ID_ALARM_HW
	// {	tms_AnalyseRetOTDRCycle	,0},//	0x80000032	ID_RET_OTDR_CYC
	// {	tms_AnalyseSendSMS	,9},//	0x80000033	ID_CMD_SMS_TEXT
	// {	tms_AnalyseSMSError	,5},//	0x80000034	ID_CMD_SMS_ERROR
	// {	tms_AnalyseGetVersion	,2},//	0x80000035	ID_GET_VERSION
	// {	tms_AnalyseRetVersion	,5},//	0x80000036	ID_RET_VERSION
	// {	tms_AnalyseAdjustTime	,0},//	0x80000037	ID_ADJUST_TIME
	// {	tms_AnalyseAck	,5},//	0x80000038	ID_CMD_ACK
	// {	tms_AnalyseUnuse	,9},//	0x80000039	ID_GET_OTDR_TEST_CYC
	// {	tms_AnalyseUnuse	,8},//	0x8000003A	--
	// {	tms_AnalyseUnuse	,8},//	0x8000003B	--
	// {	tms_AnalyseUnuse	,8},//	0x8000003C	--
	// {	tms_AnalyseUnuse	,8},//	0x8000003D	--
	// {	tms_AnalyseUnuse	,8},//	0x8000003E	--
	// {	tms_AnalyseUnuse	,8},//	0x8000003F	--
	// {	tms_AnalyseRetOTDRTest	,5},//	0x80000040	ID_RET_OTDR_TEST_CYC
	// {	tms_AnalyseUnuse	,8},//	0x80000041	--
	// {	tms_AnalyseUnuse	,8},//	0x80000042	--
	// {	tms_AnalyseUnuse	,8},//	0x80000043	--
	// {	tms_AnalyseUnuse	,8},//	0x80000044	--
	// {	tms_AnalyseUnuse	,8},//	0x80000045	--
	// {	tms_AnalyseUnuse	,8},//	0x80000046	--
	// {	tms_AnalyseUnuse	,8},//	0x80000047	--
	// {	tms_AnalyseUnuse	,8},//	0x80000048	--
	// {	tms_AnalyseUnuse	,8},//	0x80000049	--
	// {	tms_AnalyseUnuse	,8},//	0x8000004A	--
	// {	tms_AnalyseUnuse	,8},//	0x8000004B	--
	// {	tms_AnalyseUnuse	,8},//	0x8000004C	--
	// {	tms_AnalyseUnuse	,8},//	0x8000004D	--
	// {	tms_AnalyseUnuse	,8},//	0x8000004E	--
	// {	tms_AnalyseUnuse	,8},//	0x8000004F	--
	// {	tms_AnalyseDelAll_TbRoute	,2},//	0x80000050	ID_GET_OTDR_PARAM
	// {	tms_AnalyseRetOTDRParam	,5},//	0x80000051	ID_RET_OTDR_PARAM //2016-4-26
	// {	tms_AnalyseGetDevProduce	,2},//	0x80000052	ID_GET_DEV_PRODUCE
	// {	tms_AnalyseRetDevProduce	,5},//	0x80000053	ID_RET_DEV_PRODUCE
	// {	tms_AnalyseTbRoute_Insert	,0},//	0x80000054	ID_INSERT_TBROUTE
	// {	tms_AnalyseDelAll_TbRoute	,0},//	0x80000055	ID_DELALL_TBROUTE
	// {	tms_AnalyseInsert_TbUnit	,0},//	0x80000056	ID_INSERT_TBUNIT
	// {	tms_AnalyseDel_TbUnit	,0},//	0x80000057	ID_DEL_TBUNIT
	// {	tms_AnalyseDelAll_TbUnit	,0},//	0x80000058	ID_DELALL_TBUNIT
	// {	tms_AnalyseInsert_TbCycTest	,0},//	0x80000059	ID_INSERT_TBCYCTEST
	// {	tms_AnalyseUnuse	,8},//	0x8000005A	--
	// {	tms_AnalyseUnuse	,8},//	0x8000005B	--
	// {	tms_AnalyseUnuse	,8},//	0x8000005C	--
	// {	tms_AnalyseUnuse	,8},//	0x8000005D	--
	// {	tms_AnalyseUnuse	,8},//	0x8000005E	--
	// {	tms_AnalyseUnuse	,8},//	0x8000005F	--
	// {	tms_AnalyseDel_TbCycTest	,0},//	0x80000060	ID_DEL_TBCYCTEST
	// {	tms_AnalyseDelAll_TbCycTest	,0},//	0x80000061	ID_DELALL_TBCYCTEST
	// {	tms_AnalyseDel_TbOTDRRefData	,0},//	0x80000062	ID_INSERT_TBOTDRREFDATA
	// {	tms_AnalyseDelAll_TbOTDRHistData	,0},//	0x80000063	ID_DELALL_TBOTDRREFDATA
	// {	tms_AnalyseUnuse	,8},//	0x80000064	--
	// {	tms_AnalyseGetDeviceCompositionRT	,0},//	0x80000065	ID_GET_COMPOSITION_RT
	// {	tms_AnalyseRetDeviceComposition	,3},//	0x80000066	ID_RET_COMPOSITION_RT
	// {	tms_AnalyseRetDeviceComposition	,0},//	0x80000067	ID_ACK_COMPOSITION
	// {	tms_AnalyseUnuse	,8},//	0x80000068	ID_RET_ALARM_HW_CHANGE
	// {	tms_AnalyseUnuse	,2},//	0x80000069	ID_GET_OP_GATE
	// {	tms_AnalyseUnuse	,8},//	0x8000006A	--
	// {	tms_AnalyseUnuse	,8},//	0x8000006B	--
	// {	tms_AnalyseUnuse	,8},//	0x8000006C	--
	// {	tms_AnalyseUnuse	,8},//	0x8000006D	--
	// {	tms_AnalyseUnuse	,8},//	0x8000006E	--
	// {	tms_AnalyseUnuse	,8},//	0x8000006F	--
	// {	tms_AnalyseUnuse	,3},//	0x80000070	ID_RET_OP_GATE
	// {	tms_AnalyseUnuse	,0},//	0x80000071	ID_GET_MCU_TIME
	// {	tms_AnalyseUnuse	,8},//	0x80000072	ID_RET_MCU_TIME
	// {	tms_AnalyseAlarmSoundONOFF	,0},//	0x80000073	ID_ALARM_SOUND_ON_OFF
	// {	tms_AnalyseUnuse	,0},//	0x80000074	ID_GET_ALARM_SOUND_STATE
	// {	tms_AnalyseUnuse	,8},//	0x80000075	ID_RET_ALARM_SOUND_STATE
	// {	tms_AnalyseUnuse	,0},//	0x80000076	ID_GET_TOTAL_OP_ALARM
	// {	tms_AnalyseRetTotalOPAlarm	,8},//	0x80000077	ID_RET_TOTAL_OP_ALARM
	// {	tms_AnalyseUnuse	,0},//	0x80000078	ID_GET_TOTAL_HW_ALARM
	// {	tms_AnalyseUnuse	,0},//	0x80000079	ID_GET_OLP_ACTION_LOG
	// {	tms_AnalyseUnuse	,8},//	0x8000007A	--
	// {	tms_AnalyseUnuse	,8},//	0x8000007B	--
	// {	tms_AnalyseUnuse	,8},//	0x8000007C	--
	// {	tms_AnalyseUnuse	,8},//	0x8000007D	--
	// {	tms_AnalyseUnuse	,8},//	0x8000007E	--
	// {	tms_AnalyseUnuse	,8},//	0x8000007F	--
	// {	tms_AnalyseRetOLPActionLog	,8},//	0x80000080	ID_RET_OLP_ACTION_LOG
	// {	tms_AnalyseUnuse	,0},//	0x80000081	ID_GET_ALARM_POWER
	// {	tms_AnalyseUnuse	,8},//	0x80000082	ID_RET_ALARM_POWER
	// {	tms_AnalyseGetMCUOSWPort	,0},//	0x80000083	ID_GET_MCU_OSW_PORT
	// {	tms_AnalyseUnuse	,8},//	0x80000084	ID_RET_MCU_OSW_PORT
	// {	tms_AnalyseGetOTDRRef	,0},//	0x80000085	ID_GET_OTDR_REF
	// {	tms_AnalyseUnuse	,8},//	0x80000086	ID_RET_OTDR_REF
	// {	tms_AnalyseUnuse	,0},//	0x80000087	ID_GET_TBROUTE
	// {	tms_AnalyseUnuse	,8},//	0x80000088	ID_RET_TBROUTE
	// {	tms_AnalyseUnuse	,0},//	0x80000089	ID_GET_TBUNIT
	// {	tms_AnalyseUnuse	,8},//	0x8000008A	--
	// {	tms_AnalyseUnuse	,8},//	0x8000008B	--
	// {	tms_AnalyseUnuse	,8},//	0x8000008C	--
	// {	tms_AnalyseUnuse	,8},//	0x8000008D	--
	// {	tms_AnalyseUnuse	,8},//	0x8000008E	--
	// {	tms_AnalyseUnuse	,8},//	0x8000008F	--
	// {	tms_AnalyseUnuse	,8},//	0x80000090	ID_RET_TBUNIT
	// {	tms_AnalyseUnuse	,0},//	0x80000091	ID_GET_TBCYCTEST
	// {	tms_AnalyseUnuse	,8},//	0x80000092	ID_RET_TBCYCTEST
	// {	tms_AnalyseGetOLPLine	,2},//	0x80000093	ID_GET_OLP_LINE
	// {	tms_AnalyseUnuse	,8},//	0x80000094	ID_RET_OLP_LINE
	// {	tms_AnalyseGetOLPInfo	,2},//	0x80000095	ID_GET_OLP_INFO
	// {	tms_AnalyseUnuse	,8},//	0x80000096	ID_RET_OLP_INFO

};
/**
 * @brief	分析TMSxx协议数据帧，传递到该函数的数据必须是一个合法的glink帧结构
 * @param[in]	pcontext TMSxx 设备上下文描述 struct tms_context
 * @param[in]	pdata 帧内容
 * @param[in]	len 帧长度
 * @retval	0 总是返回0
 * @remarks	该函数是TMSxx协议解析入口，具体解析都根据命令ID投递到解析函数，解析函数
 			对外不可见，每个解析函数解析成功后会投递给回调函数，函数列表在struct tms_callback，
 			应用层通过修改相应回调函数得到数据内容，struct tms_callback是struct tms_context成员
 * @see	struct tms_callback
 */
int32_t tms_Analyse(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	uint32_t cmdid, cmdh, cmdl;
	struct glink_base *pbase_hdr;// glinkbase;
	struct tms_analyse_array *pwhichArr = NULL;

#ifdef CONFIG_ACK_DEVICE
	tms_AckDevice(pcontext, pdata, len);
#endif
	pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));
	cmdid = htonl(pbase_hdr->cmdid);
	cmdh = cmdid & 0xf0000000;
	cmdl = cmdid & 0x0fffffff;

	// if (cmdid != ID_TICK &&
	// 	(ID_TRACE0 > cmdid &&  ID_TRACE3 < cmdid)  )
	// #ifdef CONFIG_TEST_NET_STRONG
	// 	if (cmdid != ID_TICK && cmdh != 0x20000000) {
	// 		fecho("\n[frame]:-----[ %d ] cmdid [%8.8x] fd [%d]", len, cmdid, pcontext->fd);
	// 		tms_PrintCmdName(cmdid);
	// 	}
	// #else
	if (cmdid != ID_TICK) {
		fecho("\n[frame]:-----[ %d ] cmdid [%8.8x] fd [%d]", len, cmdid, pcontext->fd);
		tms_PrintCmdName(cmdid);
	}
	// #endif


	// 可以考虑在这里提取 glink 头指针 pcontext->pgb = htonl(pbase_hdr->xxx);
	// pbase_hdr->pklen 	= htonl(pbase_hdr->pklen);
	// pbase_hdr->version 	= htonl(pbase_hdr->version);
	// pbase_hdr->src 		= htonl(pbase_hdr->src);
	// pbase_hdr->dst 		= htonl(pbase_hdr->dst);
	// pbase_hdr->type 		= htons(pbase_hdr->type);
	// pbase_hdr->pkid 		= htons(pbase_hdr->pkid);
	// pbase_hdr->reserve 	= htonl(pbase_hdr->reserve);
	// pbase_hdr->cmdid 	= htonl(pbase_hdr->cmdid);
	// pbase_hdr->datalen 	= htonl(pbase_hdr->datalen);
	// pcontext->pgb = pbase_hdr;

	// printf("----------id %x len %x-----------\n", pbase_hdr->cmdid,glinkbase.datalen);
	// PrintfMemory((uint8_t*)pdata,20);
	switch(cmdh) {
	case 0x80000000:
		if (cmdl >= sizeof(sg_analyse_0x8000xxxx) / sizeof(struct tms_analyse_array)) {
			fecho("0x80000000 out of cmd memory!!! ");
			goto _Unknow;
		}
		pcontext->ptr_analyse_arr = sg_analyse_0x8000xxxx + cmdl;
		pwhichArr = &sg_analyse_0x8000xxxx[cmdl];// + cmdl;
		// sg_analyse_0x8000xxxx[cmdl].ptrfun(pcontext, pdata, len);
		break;
	case 0x60000000:
		if (cmdl >= sizeof(sg_analyse_0x6000xxxx) / sizeof(struct tms_analyse_array)) {
			fecho("0x60000000 out of cmd memory!!!\n");
			goto _Unknow;
		}
		pcontext->ptr_analyse_arr = sg_analyse_0x6000xxxx + cmdl;
		pwhichArr = &sg_analyse_0x6000xxxx[cmdl];;
		// sg_analyse_0x6000xxxx[cmdl].ptrfun(pcontext, pdata, len);
		break;
	// case 0x10000000:
	// 	if (cmdl >= sizeof(sg_analyse_0x1000xxxx) / sizeof(struct tms_analyse_array)) {
	// 		fecho("0x10000000 out of cmd memory!!!\n");
	// 		goto _Unknow;
	// 	}
	// 	pcontext->ptr_analyse_arr = sg_analyse_0x1000xxxx + cmdl;
	// 	pwhichArr = &sg_analyse_0x1000xxxx[cmdl];
	// 	// sg_analyse_0x1000xxxx[cmdl].ptrfun(pcontext, pdata, len);
	// 	break;

	// #ifdef CONFIG_TEST_NET_STRONG
	case 0x20000000:
		if (cmdl >= sizeof(sg_analyse_0x2000xxxx) / sizeof(struct tms_analyse_array)) {
			fecho("0x10000000 out of cmd memory!!!\n");
			goto _Unknow;
		}
		pcontext->ptr_analyse_arr = sg_analyse_0x2000xxxx + cmdl;
		pwhichArr = &sg_analyse_0x2000xxxx[cmdl];
		break;
	// #endif

	default:
_Unknow:
		;
		fecho("unknow command id 0x%8.8x\n", cmdid);
		break;
	}


	// 未知cmdid
	if (pwhichArr == NULL) {
		return 0;
	}

	switch(pwhichArr->dowhat) {
	case PROCCESS_2DEV:
		tms_Transmit2Dev( pcontext, pdata, len);
		pwhichArr->ptrfun( pcontext, pdata, len);
		break;
	case PROCCESS_2MANAGE:
		fecho("PROCCESS_2MANAGE\n");
		tms_Transmit2Manager( pcontext, pdata, len);
		pwhichArr->ptrfun( pcontext, pdata, len);
		break;
	case PROCCESS_CALLBACK:
		pwhichArr->ptrfun(pcontext, pdata, len);
		break;
	case PROCCESS_COPY2USE:
		fecho("PROCCESS_COPY2USE\n");
		pwhichArr->ptrfun(pcontext, pdata, len);
		tms_Copy2Use(pcontext, pdata, len);
		break;
	case PROCCESS_2DEV_AND_COPY2USE:
		fecho("PROCCESS_2DEV_AND_COPY2USE\n");
		tms_Transmit2Dev( pcontext, pdata, len);
		pwhichArr->ptrfun(pcontext, pdata, len);
		tms_Copy2Use(pcontext, pdata, len);
		break;
	case PROCCESS_2MANAGE_AND_COPY2USE:
		fecho("PROCCESS_2MANAGE_AND_COPY2USE\n");
		tms_Transmit2Manager( pcontext, pdata, len);
		pwhichArr->ptrfun( pcontext, pdata, len);
		tms_Copy2Use( pcontext, pdata, len);
		break;
	case PROCCESS_DONOT:
		break;
	case PROCCESS_SPECAIAL:
		fecho("specail help!!!!!!!\n");
		pwhichArr->ptrfun(pcontext, pdata, len);
		tms_Copy2Use(pcontext, pdata, len);
		break;
	case PROCCESS_2MANAGE_OR_COPY2USE:
		fecho("manage or copy2use????\n");
		break;
	case PROCCESS_2DEV_OR_COPY2USE:
		fecho("do nothing\n");
		break;
	default:
		fecho("undefine dowhat!!!!!\n");
		break;
	}

	return 0;
}




////////////////////////////////////////////////////////////////////////
// 网管与MCU之间的通信
/**
 * @file	tmsxx.c
 * @section 文金朝注意，应用层查询TMSxx设备信息接口
 - @see tms_Init\n
		 tms_GetDevBaseByLocation\n
		 tms_GetDevBaseByFd\n
		 tms_RemoveDev\n
		 tms_GetFrame\n
		未完待续
 */

/**
 * @brief	初始化tms协议，必须在任何tms_xx函数前调用
 */

void tms_Init()
{
	struct tms_devbase *pdev;

	pdev = &sg_devnet[0][0];
	for (uint32_t i = 0; i < sizeof(sg_devnet) /  sizeof(struct tms_devbase) ; i++) {
		pdev[i].frame = MAX_FRAME;
	}
	bzero(&sg_manage, sizeof(struct tms_manage));
}


/**
 * @brief	通过机框编号和槽位号位置获取设备基本信息
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[out]	pout 返回设备信息
 * @retval	0 设备并未返回设备信息，此槽位未检测到连接，pout值无效，pout->fd设置成0
 * @retval	>0 有效fd描述符
 * @remarks
 * @see	tms_GetDevBaseByFd
 */
int32_t tms_GetDevBaseByLocation(int32_t frame, int32_t slot, struct tms_devbase *pout)
{
	if ((uint32_t)frame >= MAX_FRAME || (uint32_t)slot > MAX_SLOT) {
		pout->fd = 0;
		return 0;
	}
	memcpy(pout , &sg_devnet[frame][slot], sizeof(struct tms_devbase));
	return sg_devnet[frame][slot].fd;
}


/**
 * @brief	通过socket文件描述符在设备状态表（sg_devnet）中获取设备信息设备状态表是一个struct tms_devbase
 			数组，默认数组大小是16x12，可以通过修改宏MAX_FRAME、MAX_SLOT配置
 * @param[in]	fd socket文件描述符
 * @param[out]	pout 设备信息描述
 * @retval	0 找到设备
 * @retval	-1 并未记录该fd对应的设备信息，请等待
 * @remarks	任何一个连接都有一个独立fd，当连接建立时不一定调用tms_GetDevBase能
 			获取有效struct tms_devbase，必须等到连接设备返回设备类型帧，才会在
 			跟新TMSxx设备状态表
 * @see	tms_GetDevBaseByLocation
 */

int32_t tms_GetDevBaseByFd(int fd, struct tms_devbase *pout)
{
	struct tms_devbase *pdev;

	if (fd == 0) {
		return -1;
	}

	pdev = &sg_devnet[0][0];
	for (int i = 0; i < MAX_FRAME * MAX_SLOT; i++) {
		if (pdev->fd == fd) {
			pout->fd   = pdev->fd;
			pout->frame = i / MAX_SLOT;
			pout->slot  = i % MAX_SLOT;
			pout->port = pdev->port;
			pout->type = pdev->type;
			return 0;
		}
		pdev++;
	}
	return -1;
}

int32_t tms_GetTempFd()
{
	return sg_localfd;
}
/**
 * @brief	获取支持控制台得网管fd
 * @param[out]	fd 所有
 * @retval	0 没有连接网管
 * @retval	>0 有效fd个数，所有fd在输出值fd中
 * @remarks
 * @see	tms_AddManage\n tms_DelManage\n tms_ManageCount
 */
int32_t tms_GetTCManageFd(
    int (*fd)[MANAGE_COUNT])
{
	int count = 0;

	for (register int i = 0; i < MANAGE_COUNT; i++) {
		if (sg_manage.fdtc[i] != 0) {
			fd[0][count] = sg_manage.fd[i];
			count++;
		}
	}
	return count;
}
/**
 * @brief	获取网管fd无论有多少网管只返回一个有用的fd
 * @param	null
 * @retval	0 没有连接网管
 * @retval	>0 有效fd
 * @remarks
 * @see	tms_AddManage\n tms_DelManage\n tms_ManageCount
 */
int32_t tms_GetManageFd()
{
	for (int i = 0; i < MANAGE_COUNT; i++) {
		if (sg_manage.fd[i] != 0) {
			return sg_manage.fd[i];
		}
	}
	return 0;
}

/**
 * @brief	通过寻找 Glink的目的地址寻找响应的 socket 连接，
 * @param	paddr Glink 的目的地址，不能为0，今后有可能将其升级为某新结构体的指针
 * @retval	>0 找到与addr 对应的有效连接
 * @retval	0 不存在有效的网管连接
 * @remarks	TMSxxV1.2 废除 tms_GetManageFd 而代之，当接收到 ID_GET_SN 数据包则表明发送方是网管，
 			将网管的源地址添加到内部缓存tms_AddMange\n

 			对于网管的fd管理:\n
 			accept  ---> 收到 ID_GET_SN 数据包 ---> tms_AddMange ---> tms_SelectManageByAddr
 			对于设备的fd管理其实也可以用该方法，但是目前依旧采用 tms_GetDevBaseByLocation、
 			tms_GetDevBaseByFd\n
 			accept  ---> 收到 ID_RET_DEVTYPE 数据包 ---> tms_AddDev --->
 			tms_GetDevBaseByLocation\tms_GetDevBaseByFd
 * @see
 */

int32_t tms_SelectFdByAddr(uint32_t *paddr)
{
	// return tms_GetCUFd();
	// addr 无效
	if (0 == paddr) {
		return 0;
	}
	// 找到有效对应的网管
	for (int i = 0; i < MANAGE_COUNT; i++) {
		if (sg_manage.fd_addr[i] == (int)*paddr) {
			return sg_manage.fd[i];
		}
	}
	// 未找到
	return 0;
}

int32_t tms_CountList(struct tms_man_base *list, int32_t count)
{
	// int index = 0;
	struct tms_man_base *plist;
	// int i;
	int ret;

	plist = list;
	ret = 0;
	for (int i = 0; i < MANAGE_COUNT; i++) {
		if (sg_manage.fd_addr[i] == 0) {
			continue;
		}

		if (ret >= count) {
			return ret;
		}
		ret++;
		plist->fd = sg_manage.fd[i];
		plist->addr = sg_manage.fd_addr[i];
		plist++;
	}
	return ret;
}

/**
 * @brief	修改
 * @param	null
 * @retval	null
 * @remarks
 * @see
 */

int32_t tms_SetCUFd(int fd)
{
	sg_cu_fd = fd;

	return 0;
}

/**
 * @brief	获取CUfd
 * @retval	0 无效fd，未连接CU
 * @retval	>0 有效CU fd
 * @remarks
 * @see
 */

int32_t tms_GetCUFd()
{
	return sg_cu_fd;
}

void tms_UseEpollServer(struct ep_t *p)
{
	psg_ep = p;
}

/**
 * @brief	将网管添加到TMSxx管理缓存中
 * @param[in]	fd socket文件描述符
 * @param[in]	type 网管类型\n
 				MT_MANAGE 0 普通网管\n
 				MT_TRACE 1 支持命令行的网管
 * @retval	0  添加成功
 * @retval	-1 当前连接网管数过多
 * @remarks
 * @see	tms_GetManageFd\n tms_DelManage\n tms_ManageCount
 */

// int32_t tms_AddManage(int fd, int32_t type)
int32_t tms_AddManage(struct tms_context *pcontext, int fd, int32_t type)
{
	// 下面有问题的
	// for (register int i = 0;i < MANAGE_COUNT; i++) {
	// 	if (sg_manage.fd[i] == 0) {
	// 		sg_manage.fd[i] = fd;
	// 		if (type == 1) {
	// 			sg_manage.fdtc[i] = fd;
	// 		}
	// 		return 0;
	// 	}
	// }
	// return -1;

	register int i;
	printf("%s()\n", __FUNCTION__);
	for ( i = 0; i < MANAGE_COUNT; i++) {
		if (sg_manage.fd[i] == fd) {
			printf("have use fd %d\n", fd);
			return 0;
		}
	}
	for ( i = 0; i < MANAGE_COUNT; i++) {
		if (sg_manage.fd[i] == 0) {
			sg_manage.fd[i] = fd;
			if (type == 1) {
				sg_manage.fdtc[i] = fd;
			}

			// printf("fd = %d\n", fd);
			// printf("pcontext %x\n", (int)pcontext);
			// 一般pcontext 不为NULL 则 pgb 也不会为NULL，tms_AddManage不对外开放
			if (pcontext) {
				// pcontext->ptcb->pf_OnCUNoteManageConnect(1);
				sg_manage.fd_addr[i]   = pcontext->pgb->src;
				sg_manage.fdtc_addr[i] = pcontext->pgb->src;
				printf("----src %x dst %x--------\n", pcontext->pgb->src,
				       pcontext->pgb->dst);
				printf("add fd\n");
			}

			// 采用epollserver
			if (psg_ep != NULL && i == MANAGE_COUNT - 1 ) {
				printf("auto close\n");
				ep_Close(psg_ep, NULL, fd);
			}
			return 0;
		}
	}


	return -1;
}
/**
 * @brief	将网管从到TMSxx管理缓存中移除
 * @param[in]	fd socket文件描述符
 * @retval	null
 * @remarks
 * @see	tms_GetManageFd\n tms_AddManage\n tms_ManageCount
 */
int32_t tms_DelManage(struct tms_context *pcontext, int fd)
{
	// 下面有问题的
	// for (register int i = 0;i < MANAGE_COUNT; i++) {
	// 	if (sg_manage.fd[i]  != 0) {
	// 		sg_manage.fd[i]   = 0;
	// 		sg_manage.fdtc[i] = 0;
	// 		return 0;
	// 	}
	// }
	// return -1;
#if 0
	for (register int i = 0; i < MANAGE_COUNT; i++) {
		if (sg_manage.fd[i]  != 0 && sg_manage.fd[i] == fd) {
			printf("-------tms_DelManage--------   %d\n", fd);
			sg_manage.fd[i]   = 0;
			sg_manage.fdtc[i] = 0;
			return 0;
		}
	}
	return -1;
#endif

	int count = 0;
	for (register int i = 0; i < MANAGE_COUNT; i++) {
		if (sg_manage.fd[i]  != 0 ) {
			count++;
			if (sg_manage.fd[i] == fd) {
				printf("-------tms_DelManage--------   %d\n", fd);
				sg_manage.fd[i]   = 0;
				sg_manage.fdtc[i] = 0;
				sg_manage.fd_addr[i] = 0;
				printf("find mamage");
			}
			count--;
		}
	}
	if (count == 0 && pcontext) {
		pcontext->ptcb->pf_OnCUNoteManageConnect(0);
	}
	return -1;

}

/**
 * @brief	当前连接的网管数
 * @retval	null
 * @remarks
 * @see	tms_GetManageFd\n tms_AddManage\n tms_DelManage
 */
int32_t tms_ManageCount()
{
	register int32_t count = 0;

	for (register int i = 0; i < MANAGE_COUNT; i++) {
		if (sg_manage.fd[i] != 0) {
			count++;
		}
	}
	return count;
}


/**
 * @brief	将设备从TMSxx设备状态表中移除
 * @param[in]	fd socket文件描述符
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks	当设备断开连接后应用层该调用该函数，从而得到最新的设备状态表\n
 			以后可能会取消该函数的对外可见性
 * @see	tms_AddDev
 */
int32_t tms_RemoveDev(int fd)
{
	struct tms_devbase *pdev;

	pdev = &sg_devnet[0][0];
	for (int i = 0; i < MAX_FRAME * MAX_SLOT; i++) {
		if (pdev->fd == fd) {
			pdev->fd   = 0;
			pdev->frame  = 0;
			pdev->slot   = 0;
			pdev->port = 0;
			pdev->type = 0;
			break;
		}
		pdev++;
	}
	return 0;
}
int32_t tms_RemoveByLocation(int32_t frame, int32_t slot)
{
	struct tms_devbase *pdev;

	if ((uint32_t)frame >= MAX_FRAME || (uint32_t)slot > MAX_SLOT) {
		return -1;
	}

	pdev = &sg_devnet[frame][slot];

	pdev->fd   = 0;
	pdev->frame  = 0;
	pdev->slot   = 0;
	pdev->port = 0;
	pdev->type = 0;

	return 0;
}

/**
 * @brief	将设备从TMSxx设备状态表中移除
 * @param[in]	frame 指定机框号(0 ~ MAX_FRAME)
 * @param[in]	slot 指定槽位号(0 ~ MAX_SLOT)
 * @param[in]	pdev 设备信息描述
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks	该函数不需要外部调用，在TMSxx协议调用\n
 			以后可能会取消该函数的对外可见性
 * @see	tms_RemoveDev
 */
// static
void tms_AddDev(int32_t frame, int32_t slot, struct tms_devbase *pdev)
{
	if ((uint32_t)frame >= MAX_FRAME || (uint32_t)slot > MAX_SLOT) {
		return ;
	}

	sg_devnet[frame][slot].fd        = pdev->fd;
	sg_devnet[frame][slot].frame     = pdev->frame;
	sg_devnet[frame][slot].slot      = pdev->slot;
	sg_devnet[frame][slot].type      = pdev->type;
	sg_devnet[frame][slot].port      = pdev->port;
	sg_devnet[frame][slot].wave      = pdev->wave;
	sg_devnet[frame][slot].reserved0 = pdev->reserved0;
	sg_devnet[frame][slot].reserved1 = pdev->reserved1;
	sg_devnet[frame][slot].reserved2 = pdev->reserved2;
}



/**
 * @brief	获取某机框配置
 * @param	null
 * @retval	>0 发送成功
 * @retval	0 发送失败，该链接失效，立即断开
 * @remarks
 * @see
 */

int32_t tms_GetFrame(int32_t frame, struct tms_devbase (*pdev)[MAX_SLOT])
{
	if ((uint32_t)frame >= MAX_FRAME) {
		return 0;
	}
	memcpy(&pdev[0][0], sg_devnet[frame], MAX_SLOT * sizeof(struct tms_devbase));
	return 0;
}

/**
 * @brief	根据设备在TMSxx网络所处位置不同，设置不同回调处理方式
 * @param	cmdh 命令头可以是0x80000000、0x60000000、0x10000000
 * @param	count arr数组长度
 * @param	arr 处理方式，每一个元素值表示对应回调函数的处理方式\n
			0 解析后传递给应用层\n
			1 下发到子板卡，不传递给应用层\n
			2 上传到网管，不传递给应用层\n
			3 下发到子板卡，也传递给应用层\n
			4 上传到网管，也传递给应用层
 */

void tms_SetDoWhat(int cmdh, int count, int *arr)
{
	struct tms_analyse_array *p;

	switch(cmdh) {
	case 0x80000000:
		if ((uint32_t)count >= sizeof(sg_analyse_0x8000xxxx) / sizeof(struct tms_analyse_array)) {
			count = sizeof(sg_analyse_0x8000xxxx) / sizeof(struct tms_analyse_array);
		}
		p = sg_analyse_0x8000xxxx;
		break;
	case 0x60000000:
		if ((uint32_t)count >= sizeof(sg_analyse_0x6000xxxx) / sizeof(struct tms_analyse_array)) {
			count = sizeof(sg_analyse_0x6000xxxx) / sizeof(struct tms_analyse_array);

		}
		p = sg_analyse_0x6000xxxx;
		break;
	// case 0x10000000:
	// 	if ((uint32_t)count >= sizeof(sg_analyse_0x1000xxxx) / sizeof(struct tms_analyse_array)) {
	// 		count = sizeof(sg_analyse_0x1000xxxx) / sizeof(struct tms_analyse_array);

	// 	}
	// 	p = sg_analyse_0x1000xxxx;
	// 	break;
	default:
		return ;
	}
	for (int i = 0; i < count; i++) {
		p->dowhat = (arr[i] & 0x07);
	}
}

// int32_t tms_Echo(int type, )
// {

// }

int g_manger = 0, g_node_manger = 0;

#include <epollserver.h>
#include <assert.h>
extern struct ep_t ep;
struct _ep_find_val
{
	int fd;
	struct tms_context *context;
};
int _ep_find(struct ep_con_t *ppconNode, void *ptr)
{
	struct tmsxx_app *papp = (struct tmsxx_app *)ppconNode->ptr;
	struct tms_context *pcontext = (struct tms_context *)&papp->context;

	struct _ep_find_val *pval = (struct _ep_find_val *)ptr;

	printf("fd %d\n", pcontext->fd);
	
// 
	if(pcontext->fd == pval->fd) {
		pval->fd = pcontext->fd;
		memcpy(pval->context, pcontext, sizeof(struct tms_context));
		return -1;
	}
	return 0;
}
int32_t  tms_SelectContextByFD(int fd, struct tms_context *context)
{
	struct _ep_find_val val;

	assert(context != NULL);

	val.fd = fd;
	val.context = context;
	ep_Ergodic(&ep, _ep_find, &val);
	if (context->fd == val.fd) {
		return 1;
	}
	return 0;
}

int32_t tms_SelectMangerContext(struct tms_context *context)
{
	struct tms_context con;
	int ret;

	ret = tms_SelectContextByFD(g_manger, &con);

	if (ret) {
		memcpy(context, &con, sizeof(struct tms_context));
		return 1;
	}
	return 0;
}

int32_t tms_SelectNodeMangerContext(struct tms_context *context)
{
	struct tms_context con;
	int ret;

	ret = tms_SelectContextByFD(g_node_manger, &con);

	if (ret) {
		memcpy(context, &con, sizeof(struct tms_context));
		return 1;
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
