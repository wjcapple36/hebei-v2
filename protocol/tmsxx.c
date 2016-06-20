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
int sg_echo_tick = 0;

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


	// tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {
		// fd =
		// del 2016
		// tms_SelectFdByAddr(&base_hdr.dst);
		// end del
	}
	glink_Build(&base_hdr, ID_TICK, 0);
	glink_Send(fd, &base_hdr, NULL, 0);
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
	{"ID_CHANGE_ADDR"},
	{"ID_GET_SN"},
	{"ID_RET_SN"},
	{"ID_CFG_SMS"},
	{"ID_CFG_SMS_CLEAR"},
	{"ID_GET_COMPOSITION"},
	{"ID_RET_COMPOSITION"},
	{"ID_CFG_MCU_OSW_PORT"},
	{"ID_CFG_MCU_OSW_PORT_CLEAR"},
	{"ID_CFG_MCU_OLP_PORT"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_CFG_MCU_OLP_PORT_CLEAR"},
	{"ID_CFG_MCU_U_OPM_OSW"},
	{"ID_CFG_MCU_U_OPM_OSW_CLEAR"},
	{"ID_CFG_MCU_U_OLP_OSW"},
	{"ID_CFG_MCU_U_OLP_OSW_CLEAR"},
	{"ID_CFG_OPM_REF_LEVEL"},
	{"ID_GET_OPM_OP"},
	{"ID_RET_OPM_OP"},
	{"ID_CFG_OLP_REF_LEVEL"},
	{"ID_GET_OLP_OP"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_RET_OLP_OP"},
	{"ID_CFG_OTDR_REF"},
	{"ID_CFG_MCU_OSW_CYCLE"},
	{"ID_GET_OTDR_TEST"},
	{"ID_RET_OTDR_TEST"},
	{"ID_CFG_OLP_MODE"},
	{"ID_CMD_OLP_SWITCH"},
	{"ID_REPORT_OLP_ACTION"},
	{"ID_ALARM_OPM"},
	{"ID_ALARM_OPM_CHANGE"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_ALARM_LINE"},
	{"ID_ALARM_HW"},
	{"ID_RET_OTDR_CYC"},
	{"ID_CMD_SMS_TEXT"},
	{"ID_CMD_SMS_ERROR"},
	{"ID_GET_VERSION"},
	{"ID_RET_VERSION"},
	{"ID_ADJUST_TIME"},
	{"ID_CMD_ACK"},
	{"ID_GET_OTDR_TEST_CYC"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_RET_OTDR_TEST_CYC"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_GET_OTDR_PARAM"},
	{"ID_RET_OTDR_PARAM"},
	{"ID_GET_DEV_PRODUCE"},
	{"ID_RET_DEV_PRODUCE"},
	{"ID_INSERT_TBROUTE"},
	{"ID_DELALL_TBROUTE"},
	{"ID_INSERT_TBUNIT"},
	{"ID_DEL_TBUNIT"},
	{"ID_DELALL_TBUNIT"},
	{"ID_INSERT_TBCYCTEST"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_DEL_TBCYCTEST"},
	{"ID_DELALL_TBCYCTEST"},
	{"ID_INSERT_TBOTDRREFDATA"},
	{"ID_DELALL_TBOTDRREFDATA"},
	{"--"},
	{"ID_GET_COMPOSITION_RT"},
	{"ID_RET_COMPOSITION_RT"},
	{"ID_ACK_COMPOSITION"},
	{"ID_RET_ALARM_HW_CHANGE"},
	{"ID_GET_OP_GATE"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_RET_OP_GATE"},
	{"ID_GET_MCU_TIME"},
	{"ID_RET_MCU_TIME"},
	{"ID_ALARM_SOUND_ON_OFF"},
	{"ID_GET_ALARM_SOUND_STATE"},
	{"ID_RET_ALARM_SOUND_STATE"},
	{"ID_GET_TOTAL_OP_ALARM"},
	{"ID_RET_TOTAL_OP_ALARM"},
	{"ID_GET_TOTAL_HW_ALARM"},
	{"ID_GET_OLP_ACTION_LOG"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_RET_OLP_ACTION_LOG"},
	{"ID_GET_ALARM_POWER"},
	{"ID_RET_ALARM_POWER"},
	{"ID_GET_MCU_OSW_PORT"},
	{"ID_RET_MCU_OSW_PORT"},
	{"ID_GET_OTDR_REF"},
	{"ID_RET_OTDR_REF"},
	{"ID_GET_TBROUTE"},
	{"ID_RET_TBROUTE"},
	{"ID_GET_TBUNIT"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"--"},
	{"ID_RET_TBUNIT"},
	{"ID_GET_TBCYCTEST"},
	{"ID_RET_TBCYCTEST"},
	{"ID_GET_OLP_LINE"},
	{"ID_RET_OLP_LINE"},
	{"ID_GET_OLP_INFO"},
	{"ID_RET_OLP_INFO"},
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

	// del 2016
	// fd = tms_GetDevBaseByLocation(frame, slot, &out);
	// end del
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
	// del 2016
	// fd = tms_SelectFdByAddr(&dst);
	// end del
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
		// del 2016
		// fd = tms_SelectFdByAddr(&dst);
		// end del
		// 没有该地址的网管
		if (0 == fd) {
			continue;
		}
		dbg_tms("send all manager %x\n", dst);
		pbase_hdr->dst = htonl(dst);
		glink_Send(fd, pbase_hdr, pdata, len);
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
		// del 2016
		// fd = tms_SelectFdByAddr(&dst);
		// end del
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
	ack.reserve1 = htonl(pack->reserve1);
	ack.reserve2 = htonl(pack->reserve2);
	ack.reserve3 = htonl(pack->reserve3);
	ack.reserve4 = htonl(pack->reserve4);


	struct glink_base  base_hdr;
	uint8_t *pmem;
	int ret;


	pmem = (uint8_t *)&ack;

	// tms_FillGlinkFrame(&base_hdr, paddr);
	if (0 == fd) {

		// del 2016
		// fd = tms_SelectFdByAddr(&paddr->dst);
		// end del
		printf("fd = 0 find %d %x\n", fd, paddr->dst);
	}
	glink_Build(&base_hdr, ID_CMD_ACK, sizeof(struct tms_ack));
	ret = glink_Send(fd, &base_hdr, pmem, sizeof(struct tms_ack));
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
	pval->reserve1 = htonl(pval->reserve1);
	pval->reserve2 = htonl(pval->reserve2);
	pval->reserve3 = htonl(pval->reserve3);
	pval->reserve4 = htonl(pval->reserve4);


	printf("tms_AnalyseAck\n");
	if ((uint32_t)pval->errcode >= sizeof(list) / sizeof(struct pro_list)) {
		printf("\terror errcode [%2.2d]!!!\n", pval->errcode);
	}
	else {
		printf("\tf%d/s%d/t%d/p%d err [%2.2d] %s \t\tcmdid [0x%8.8x] ",
		       (pval->reserve1 >> 16) & 0xffff,
		       (pval->reserve1 >> 0) & 0xffff,
		       (pval->reserve2 >> 16) & 0xffff,
		       (pval->reserve2 >> 0) & 0xffff,
		       pval->errcode, list[pval->errcode].name, pval->cmdid);
		tms_PrintCmdName(pval->cmdid);
	}

	return 0;
}


static struct tms_analyse_array sg_analyse_0x1000xxxx[] = {
	// {	tms_AnalyseTick	, 1}, //	0x10000000	ID_TICK
	// {	tms_AnalyseUpdate	, PROCCESS_2DEV_AND_COPY2USE}, //	0x10000001	ID_UPDATE
	// {	tms_AnalyseTrace	, 1}, //	0x10000002	ID_TRACE0
	// {	tms_AnalyseTrace	, 1}, //	0x10000003	ID_TRACE1
	// {	tms_AnalyseTrace	, 1}, //	0x10000004	ID_TRACE2
	// {	tms_AnalyseCommand	, 1}, //	0x10000005	ID_TRACE3
	// {	tms_AnalyseCommand	, 1}, //	0x10000006	ID_COMMAND
};


#ifdef CONFIG_TEST_NET_STRONG
static struct tms_analyse_array sg_analyse_0x2000xxxx[] = {
	// { 	tms_AnalyseTestPacketSave, 1},
	// { 	tms_AnalyseTestPacketEcho, 1},
	// { 	tms_AnalyseTestPacketAck, 1},
};

#endif
static struct tms_analyse_array sg_analyse_0x6000xxxx[] = {
	// {	tms_AnalyseGetDevType	, 9}, //	0x60000000	ID_GET_DEVTYPE
	// {	tms_AnalyseRetDevType	, 5}, //	0x60000001	ID_RET_DEVTYPE
	// {	tms_AnalyseCUNoteNet	, 1}, //	0x60000002	ID_CU_NOTE_NET
	// {	tms_AnalyseCUNoteManageConnect	, 0}, //	0x60000003	ID_CU_NOTE_MANAGE_CONNECT
	// {	tms_AnalyseGetOPMOLPRayPower	, 2}, //	0x60000004	ID_GET_OPM_OLP_RAYPOWER
	// {	tms_AnalyseMCU_OSWSwitch	, 2}, //	0x60000005	ID_CMD_OSW_SWITCH
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

struct tms_analyse_array sg_analyse_0x8000xxxx[] = {
	// {	tms_AnalyseSetIPAddress	,1},//	0x80000000	ID_CHANGE_ADDR
	// {	tms_AnalyseGetSerialNumber	,0},//	0x80000001	ID_GET_SN
	// {	tms_AnalyseRetSerialNumber	,1},//	0x80000002	ID_RET_SN
	// {	tms_AnalyseCfgSMSAuthorization	,0},//	0x80000003	ID_CFG_SMS
	// {	tms_AnalyseClearSMSAuthorization	,0},//	0x80000004	ID_CFG_SMS_CLEAR
	// {	tms_AnalyseGetDeviceComposition	,0},//	0x80000005	ID_GET_COMPOSITION
	// {	tms_AnalyseRetDeviceComposition	,0},//	0x80000006	ID_RET_COMPOSITION
	// {	tms_AnalyseCfgMCUOSWPort	,0},//	0x80000007	ID_CFG_MCU_OSW_PORT
	// {	tms_AnalyseCfgMCUOPMPortClear	,0},//	0x80000008	ID_CFG_MCU_OSW_PORT_CLEAR
	// {	tms_AnalyseCfgMCUOLPPort	,0},//	0x80000009	ID_CFG_MCU_OLP_PORT
	// {	tms_AnalyseUnuse	,8},//	0x8000000A	--
	// {	tms_AnalyseUnuse	,8},//	0x8000000B	--
	// {	tms_AnalyseUnuse	,8},//	0x8000000C	--
	// {	tms_AnalyseUnuse	,8},//	0x8000000D	--
	// {	tms_AnalyseUnuse	,8},//	0x8000000E	--
	// {	tms_AnalyseUnuse	,8},//	0x8000000F	--
	// {	tms_AnalyseCfgMCUOLPPortClear	,0},//	0x80000010	ID_CFG_MCU_OLP_PORT_CLEAR
	// {	tms_AnalyseCfgMCUUniteOPMOSW	,0},//	0x80000011	ID_CFG_MCU_U_OPM_OSW
	// {	tms_AnalyseCfgMCUUniteOPMOSWClear	,0},//	0x80000012	ID_CFG_MCU_U_OPM_OSW_CLEAR
	// {	tms_AnalyseCfgMCUUniteOLPOSW	,0},//	0x80000013	ID_CFG_MCU_U_OLP_OSW
	// {	tms_AnalyseCfgMCUUniteOLPOSWClear	,0},//	0x80000014	ID_CFG_MCU_U_OLP_OSW_CLEAR
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
	case 0x10000000:
		if (cmdl >= sizeof(sg_analyse_0x1000xxxx) / sizeof(struct tms_analyse_array)) {
			fecho("0x10000000 out of cmd memory!!!\n");
			goto _Unknow;
		}
		pcontext->ptr_analyse_arr = sg_analyse_0x1000xxxx + cmdl;
		pwhichArr = &sg_analyse_0x1000xxxx[cmdl];
		// sg_analyse_0x1000xxxx[cmdl].ptrfun(pcontext, pdata, len);
		break;

#ifdef CONFIG_TEST_NET_STRONG
	case 0x20000000:
		if (cmdl >= sizeof(sg_analyse_0x2000xxxx) / sizeof(struct tms_analyse_array)) {
			fecho("0x10000000 out of cmd memory!!!\n");
			goto _Unknow;
		}
		pcontext->ptr_analyse_arr = sg_analyse_0x2000xxxx + cmdl;
		pwhichArr = &sg_analyse_0x2000xxxx[cmdl];
		break;
#endif

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
	case 0x10000000:
		if ((uint32_t)count >= sizeof(sg_analyse_0x1000xxxx) / sizeof(struct tms_analyse_array)) {
			count = sizeof(sg_analyse_0x1000xxxx) / sizeof(struct tms_analyse_array);

		}
		p = sg_analyse_0x1000xxxx;
		break;
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
#ifdef __cplusplus
}
#endif
