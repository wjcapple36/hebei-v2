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


static int32_t tms_AnalyseUnuse(struct tms_context *pcontext, int8_t *pdata, int32_t len);
// static int32_t tms_Transmit2Dev(struct tms_context *pcontext, int8_t *pdata, int32_t len);
// static int32_t tms_Transmit2Manager(struct tms_context *pcontext, int8_t *pdata, int32_t len);

// 每机框12个槽位，最大支持16机框

// struct tms_devbase sg_devnet[MAX_FRAME + 1][MAX_SLOT] = {{{0}}};
// struct tms_manage sg_manage = {{0}};
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
		// tms_SelectFdByAddr(&base_hdr.dst);
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
		// tms_SelectFdByAddr(&base_hdr.dst);
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
		// tms_SelectFdByAddr(&base_hdr.dst);
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

// hebei2
static void tms_OTDRConv_tms_get_otdrdata(
    struct tms_get_otdrdata *pout,
    struct tms_get_otdrdata *pin)
{
	register uint32_t *p32s, *p32d;
	// register uint16_t *p16s, *p16d;
	// register int loop;

	p32d = (uint32_t *)pout;
	p32s = (uint32_t *)pin;
	for (register uint32_t i = 0; i < sizeof (struct tms_get_otdrdata) / sizeof(int32_t); i++) {
		*p32d = htonl(*p32s);
		p32d++;
		p32s++;
	}
}

static void tms_OTDRConv_tms_fibersection_hdr(
    struct tms_fibersection_hdr *pout,
    struct tms_fibersection_hdr *pin)
{
	pin->count = htonl(pout->count);
}
static void tms_OTDRConv_tms_fibersection_val(
    struct tms_fibersection_val *pout,
    struct tms_fibersection_val *pin,
    struct tms_fibersection_hdr *phdr)
{
	struct tms_fibersection_val *p32s, *p32d;
	register int loop;

	loop = phdr->count;
	// loop = loop * sizeof (struct tms_hebei2_event_val) >> 2;	// 计算有多少个4Byte数据
	// printf("loop %d\n", loop);
	p32d = (struct tms_fibersection_val *)pout;
	p32s = (struct tms_fibersection_val *)pin;
	for (register int i = 0; i < loop; i++) {
		p32d->pipe_num	 = htonl(p32s->pipe_num);
		p32d->fiber_num	 = htonl(p32s->fiber_num);
		p32d->start_coor = htonl(p32s->start_coor);
		p32d->end_coor	 = htonl(p32s->end_coor);

		p32d->fibe_atten_init = htonf(p32s->fibe_atten_init);
		p32d->level1 	      = htonf(p32s->level1);
		p32d->level2 	      = htonf(p32s->level2);
		p32d->listen_level    = htonf(p32s->listen_level);
		p32d++;
		p32s++;
	}


}

static void tms_OTDRConv_tms_otdr_param(
    struct tms_otdr_param *pout,
    struct tms_otdr_param *pin)
{
	tms_Conv_Nx4Byte((uint32_t *)&pout->range, (uint32_t *)&pin->range, sizeof(struct tms_otdr_param) - 20);
}

static void tms_OTDRConv_tms_test_result(
    struct tms_test_result *pout,
    struct tms_test_result *pin)
{
	tms_Conv_Nx4Byte((uint32_t *)&pout->range, (uint32_t *)&pin->range, sizeof(float) * 3);
}

static void tms_OTDRConv_tms_hebei2_data_hdr(
    struct tms_hebei2_data_hdr *pout,
    struct tms_hebei2_data_hdr *pin)
{
	memcpy(pout->dpid, pin->dpid, 12);

	pout->count = htonl(pin->count);
}

static void tms_OTDRConv_tms_hebei2_data_val(
    struct tms_hebei2_data_val *pout,
    struct tms_hebei2_data_val *pin,
    struct tms_hebei2_data_hdr *pdata_hdr)
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


static void tms_OTDRConv_tms_hebei2_event_hdr(
    struct tms_hebei2_event_hdr *pout,
    struct tms_hebei2_event_hdr *pin)
{
	// pin->count = pin->count & 0x3ff;				// 限定loop在0~1024以内

	memcpy(pout->eventid, pin->eventid, 12);
	pout->count = htonl(pin->count);
}

static void tms_OTDRConv_tms_hebei2_event_val(
    struct tms_hebei2_event_val *pout,
    struct tms_hebei2_event_val *pin,
    struct tms_hebei2_event_hdr *pevent_hdr)
{
	register int32_t *p32s, *p32d;
	register int loop;



	loop = pevent_hdr->count;
	loop = loop * sizeof (struct tms_hebei2_event_val) >> 2;	// 计算有多少个4Byte数据
	// printf("loop %d\n", loop);
	p32d = (int32_t *)pout;
	p32s = (int32_t *)pin;
	for (register int i = 0; i < loop; i++) {
		*p32d = htonl(*p32s);
		p32d++;
		p32s++;
	}
}

static void tms_OTDRConv_tms_otdr_crc_val(
    struct tms_otdr_crc_val *pout,
    struct tms_otdr_crc_val *pin,
    uint32_t count)
{
	struct tms_otdr_crc_val *p32s, *p32d;
	register int loop;

	loop = count;
	// loop = loop * sizeof (struct tms_hebei2_event_val) >> 2;	// 计算有多少个4Byte数据
	// printf("loop %d\n", loop);
	p32d = (struct tms_otdr_crc_val *)pout;
	p32s = (struct tms_otdr_crc_val *)pin;
	for (register int i = 0; i < loop; i++) {
		p32d->pipe	 = htonl(p32s->pipe);
		p32d->wl	 = htonl(p32s->wl);
		p32d->dr = htonl(p32s->dr);
		p32d->reserved0	 = htonl(p32s->reserved0);

		p32d++;
		p32s++;
	}
}

#ifdef HEBEI2_DBG
void tms_Print_tms_fibersection_hdr(struct tms_fibersection_hdr *pval)
{
	hb2_dbg("fiber_id %s  count %d\n", pval->fiber_id, pval->count);
}
void tms_Print_tms_fibersection_val(struct tms_fibersection_val *pval)
{
	hb2_dbg(
	    "\tpipe_num %d fiber_num %d\n"
	    "\tfiber_route %s fiber_name %s\n"
	    "\tstart_coor %d start_inf %s end_coor %d end_inf %s\n"
	    "\tfibe_atten_init %f\n"
	    "\tlevel1 %f level2 %f level3 %f\n",
	    pval->pipe_num,
	    pval->fiber_num,
	    pval->fiber_route,
	    pval->fiber_name,
	    pval->start_coor,
	    pval->start_inf,
	    pval->end_coor,
	    pval->end_inf,
	    pval->fibe_atten_init,
	    pval->level1,
	    pval->level2,
	    pval->listen_level);
}
void tms_Print_tms_otdr_param(struct tms_otdr_param *pval)
{
	hb2_dbg(
	    "otdr_id %s\n"
	    "\trange %d\n"
	    "\twl %d pw %d time %d\n"
	    "\tgi %f end %f reflect %f\n",
	    pval->otdr_id,
	    pval->range,
	    pval->wl,
	    pval->pw,
	    pval->time,
	    pval->gi,
	    pval->end_threshold,
	    pval->none_reflect_threshold);
}
void tms_Print_tms_test_result(struct tms_test_result *pval)
{
	hb2_dbg(
	    "result %s\n"
	    "\trange %f loss %f atten %f time %s\n",
	    pval->result,
	    pval->range,
	    pval->loss,
	    pval->atten,
	    pval->time);

}


void tms_Print_tms_hebei2_event(struct tms_hebei2_event_hdr *pevent_hdr, struct tms_hebei2_event_val *pevent_val)
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

	struct tms_hebei2_event_val  *ptevent_val;
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

void tms_Print_tms_get_otdrdata(struct tms_get_otdrdata *ptest_param)
{
	hb2_dbg("OTDR Param: ");
	if (ptest_param->range > 1000) {
		hb2_dbg("%2.2fKm/", (float)ptest_param->range / 1000);
	}
	else {
		hb2_dbg("%fM/", (float)ptest_param->range);
	}

	if (ptest_param->pw < 1000) {
		hb2_dbg("%2.2fns/%ds/", (float)(ptest_param->pw), ptest_param->time);
	}
	else if (ptest_param->pw < 1000000) {
		hb2_dbg("%2.2fus/%ds/", (float)ptest_param->pw / 1000, ptest_param->time);
	}
	else {
		hb2_dbg("%2.2fms/%ds/", (float)ptest_param->pw / 1000000, ptest_param->time);
	}

	hb2_dbg("   %2.2fdB/Km /%2.2fdB/%2.2fdB\n",
	        ptest_param->gi, ptest_param->end_threshold, ptest_param->none_reflect_threshold);
}
#endif
// end hebei2


////////////////////////////////////////////////////////////////////////
// 数据包分析
// 命令名列表0x1000xxxx、0x6000xxxx、0x8000xxxx
#ifdef PRINT_CMD_NAME_DBG
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
#endif
// 转发网管的数据到设备
// static
int32_t tms_Transmit2Dev(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// int fd;
	// uint32_t frame, slot;
	// struct tms_devbase out;
	// struct tms_dev_slot *pval;

	// pval  = (struct tms_dev_slot *)(pdata + GLINK_OFFSET_DATA);
	// frame = htonl(pval->frame);
	// slot  = htonl(pval->slot);

	// fd = tms_GetDevBaseByLocation(frame, slot, &out);
	// // 色号吧不存在
	// if (fd == 0) {
	// 	// TODO 发送错误码
	// 	return 0;
	// }
	// else {
	// 	return glink_SendSerial(fd, (uint8_t *)pdata, len);
	// }

}
// 转发设备的数据到网管
// static
int32_t tms_Transmit2Manager(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	// int fd;
	// // struct glink_base  base_hdr;
	// uint32_t src, dst;

	// struct glink_base *pbase_hdr;
	// pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));
	// src = htonl(pbase_hdr->src);
	// dst = htonl(pbase_hdr->dst);

	// dbg_tms("tms_Transmit2Manager()\n");
	// dbg_tms("\t%x , %x\n", src, dst);

	// // 过滤设备发往MCU的数据包，不向网管转发
	// if (dst == GLINK_4412_ADDR ||
	//     src == GLINK_4412_ADDR ||
	//     GLINK_MASK_MADDR != (dst & GLINK_MASK_MADDR) ) {
	// 	dbg_tms("can't not transmit to manager\n");
	// 	return 0;
	// }

	// pbase_hdr->dst = htonl(GLINK_MANAGE_ADDR);
	// pbase_hdr->src = htonl(GLINK_4412_ADDR);
	// // PrintfMemory((uint8_t*)pdata,20);
	// // fd = tms_SelectFdByAddr(&dst);
	// dbg_tms("manager fd = %d\n", fd);
	// if (fd == 0) {
	// 	return 0;
	// }
	// return glink_SendSerial(fd, (uint8_t *)pdata, len);
}

// 向所有网管转发
int32_t tms_Transmit2AllManager(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
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


//////////////////////////////////////////////////////////////////////////////////////////
// hebei2
#ifdef HEBEI2_DBG
static int32_t tms_DbgAckSuccess(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	struct glink_base *pbase_hdr;
	pbase_hdr = (struct glink_base *)(pdata + sizeof(int32_t));

	struct tms_ack ack;
	ack.errcode = 0;
	ack.cmdid   = htonl(pbase_hdr->cmdid);
	tms_AckEx(pcontext->fd, NULL, &ack);
}
#endif
// 0x20000000	ID_SETOTDRFPGAINFO
static int32_t tms_AnalyseSetOTDRFPGAInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	tms_DbgAckSuccess(pcontext, pdata, len);
#endif

	struct tms_setotdrfpgainfo *pval;
	pval = (struct tms_setotdrfpgainfo *)(pdata + GLINK_OFFSET_DATA);
	pval->pipe	= htonl(pval->pipe);
	pval->wl	= htonl(pval->wl);
	pval->dr	= htonl(pval->dr);
	pval->reserved0	= htonl(pval->reserved0);

	if (pcontext->ptcb->pf_OnSetOTDRFPGAInfo) {
		pcontext->ptcb->pf_OnSetOTDRFPGAInfo(pcontext, pval);
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
static int32_t tms_AnalyseSetOCVMPara(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	tms_DbgAckSuccess(pcontext, pdata, len);
	hb2_dbg("没有测试，不清楚该命令什么意思\n");
#endif
	struct tms_setocvmpara *pval;
	pval = (struct tms_setocvmpara *)(pdata + GLINK_OFFSET_DATA);
	tms_Conv_Nx4Byte((uint32_t *)pval, (uint32_t *)pval, sizeof(struct tms_setocvmpara));

	if (pcontext->ptcb->pf_OnSetOCVMPara) {
		pcontext->ptcb->pf_OnSetOCVMPara(pcontext, pval);
	}
	return 0;
}

//	0x70000001	ID_SETOCVMPARA
static int32_t tms_AnalyseSetOCVMFPGAInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	tms_DbgAckSuccess(pcontext, pdata, len);
	hb2_dbg("没有测试，不清楚该命令什么意思\n");
#endif
	struct tms_setocvmfpgainfo *pval;
	pval = (struct tms_setocvmfpgainfo *)(pdata + GLINK_OFFSET_DATA);
	tms_Conv_Nx4Byte((uint32_t *)pval, (uint32_t *)pval, sizeof(float));

	if (pcontext->ptcb->pf_OnSetOCVMFPGAInfo) {
		pcontext->ptcb->pf_OnSetOCVMFPGAInfo(pcontext, pval);
	}
	return 0;
}








//	0x80000000	ID_GETBASICINFO
static int32_t tms_AnalyseGetBasicInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	tms_DbgAckSuccess(pcontext, pdata, len);
#endif

	hb2_dbg("Warning 应该返回什么内容，协议里没详细说明\n");
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
	tms_DbgAckSuccess(pcontext, pdata, len);
#endif
	hb2_dbg("Warning CU 需要多次转发此消息\n");
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
	glink_Send(pcontext->fd, NULL, &base_hdr, (uint8_t*)pmem, len);
}


//	0x80000002	ID_RETNodeTime
static int32_t tms_AnalyseRetNodeTime(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}


//	0x80000003	ID_NAMEANDADDRESS
static int32_t tms_AnalyseNameAndAddress(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	hb2_dbg("Warning CU 需要处理此消息\n");
	return 0;
}


//	0x80000004	ID_FIBERSECTIONCFG
static int32_t tms_AnalyseFiberSectionCfg(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	tms_DbgAckSuccess(pcontext, pdata, len);
	hb2_dbg("待测试\n");
#endif


	struct tms_fibersection_hdr *fiber_hdr;
	struct tms_fibersection_val *fiber_val;
	struct tms_otdr_param       *otdr_param;
	struct tms_test_result      *test_result;
	struct tms_hebei2_data_hdr  *otdr_hdr;
	struct tms_hebei2_data_val  *otdr_val;
	struct tms_hebei2_event_hdr *event_hdr;
	struct tms_hebei2_event_val *event_val;

	fiber_hdr = (struct tms_fibersection_hdr *)(pdata + GLINK_OFFSET_DATA);
	if ( !CHECK_PTR(
	         fiber_hdr,
	         struct tms_fibersection_hdr,
	         struct tms_fibersection_val,
	         htonl(fiber_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	fiber_val    = (struct tms_fibersection_val *)(((char *)fiber_hdr) + sizeof(struct tms_fibersection_hdr));
	otdr_param  = (struct tms_otdr_param *)(((char *)fiber_val) + sizeof(struct tms_fibersection_val) * htonl(fiber_hdr->count));
	test_result = (struct tms_test_result *)(((char *)otdr_param) + sizeof(struct tms_otdr_param));

	otdr_hdr     = (struct tms_hebei2_data_hdr *)(((char *)test_result) + sizeof(struct tms_test_result));
	otdr_val     = (struct tms_hebei2_data_val *)(((char *)otdr_hdr) + sizeof(struct tms_hebei2_data_hdr));
	event_hdr   = (struct tms_hebei2_event_hdr *)(((char *)otdr_val) + sizeof(struct tms_hebei2_data_val) * htonl(otdr_hdr->count));
	if ( !CHECK_PTR(
	         event_hdr,
	         struct tms_hebei2_event_hdr,
	         struct tms_hebei2_event_val,
	         htonl(event_hdr->count),
	         pdata + len)) {
		printf("Err: %s():%d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	event_val = (struct tms_hebei2_event_val *)(((char *)event_hdr) + sizeof(struct tms_hebei2_event_hdr));


	tms_OTDRConv_tms_fibersection_hdr(fiber_hdr, fiber_hdr);
	// printf("hdr count %d\n", fiber_hdr->count);

	tms_OTDRConv_tms_fibersection_val(fiber_val, fiber_val, fiber_hdr);
	// printf("pipe_num %d fiber_name %d route %s name %s %f %f\n", fiber_val->pipe_num,
	// fiber_val->fiber_num,
	// fiber_val->fiber_route,
	// fiber_val->fiber_name,
	// fiber_val->level1,
	// fiber_val->level2);

	tms_OTDRConv_tms_otdr_param(otdr_param, otdr_param);
	// printf("otdr_id %s range %d %f wl %d\n", (otdr_param->otdr_id), (otdr_param->range),
	// otdr_param->gi,
	// otdr_param->wl);

	tms_OTDRConv_tms_test_result(test_result, test_result);
	// printf("result %s range %f loss %f atten %f\n",
	// test_result->result,
	// test_result->range,
	// test_result->loss,
	// test_result->atten);

	tms_OTDRConv_tms_hebei2_data_hdr(otdr_hdr, otdr_hdr);
	// printf("otdr data count %d\n", otdr_hdr->count);
	tms_OTDRConv_tms_hebei2_data_val(otdr_val, otdr_val, otdr_hdr);
	tms_OTDRConv_tms_hebei2_event_hdr(event_hdr, event_hdr);
	// printf("id %s count %d\n", event_hdr->eventid, event_hdr->count);
	tms_OTDRConv_tms_hebei2_event_val(event_val, event_val, event_hdr);


#ifdef HEBEI2_DBG
	tms_Print_tms_fibersection_hdr(fiber_hdr);
	tms_Print_tms_fibersection_val(fiber_val);
	tms_Print_tms_otdr_param(otdr_param);
	tms_Print_tms_test_result(test_result);
	tms_Print_tms_hebei2_event(event_hdr, event_val);
#endif
	return 0;
}


//	0x80000005	ID_CONFIGPIPESTATE
static int32_t tms_AnalyseConfigPipeState(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	tms_DbgAckSuccess(pcontext, pdata, len);
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
	uint32_t pdata = *idata;
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
static int32_t tms_AnalyseCheckoutResult(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{

}

//	0x80000011	ID_OTDRBASICINFO

int32_t tms_OTDRBasicInfo(
    struct tms_context *pcontext,
    struct glink_addr *paddr,
    struct tms_otdrbaseinfo *pval)
{
	struct tms_otdr_crc_hdr     otdr_crc_hdr,      *potdr_crc_hdr;
	struct tms_otdr_crc_val     otdr_crc_val[8],   *potdr_crc_val;
	struct tms_otdr_ch_status   otdr_ch_status,    *potdr_ch_status;
	struct tms_otdr_param_hdr   otdr_param_hdr,    *potdr_param_hdr;
	struct tms_otdr_param_val   otdr_param_val[8], *potdr_param_val;
	struct tms_fibersection_hdr fiber_hdr,         *pfiber_hdr;
	struct tms_fibersection_val fiber_val[8],      *pfiber_val;

	if (potdr_crc_hdr->count > 8 ||
	    potdr_param_hdr->count > 8 ||
	    pfiber_hdr->count > 8) {

		perror("param err\n");
		return -1;
	}

	potdr_crc_hdr   = pval->otdr_crc_hdr;
	potdr_crc_val   = pval->otdr_crc_val;
	potdr_ch_status = pval->otdr_ch_status;
	potdr_param_hdr = pval->otdr_param_hdr;
	potdr_param_val = pval->otdr_param_val;
	pfiber_hdr      = pval->fiber_hdr;
	pfiber_val      = pval->fiber_val;

	memcpy(&otdr_crc_hdr,      potdr_crc_hdr,   sizeof(struct tms_otdr_crc_hdr));
	memcpy(&otdr_crc_val[0],   potdr_crc_val,   sizeof(struct tms_otdr_crc_val)* potdr_crc_hdr->count);
	memcpy(&otdr_ch_status,    potdr_ch_status, sizeof(struct tms_otdr_ch_status));
	memcpy(&otdr_param_hdr,    potdr_param_hdr, sizeof(struct tms_otdr_param_hdr));
	memcpy(&otdr_param_val[0], potdr_param_val, sizeof(struct tms_otdr_param_val) * potdr_param_hdr->count);
	memcpy(&fiber_hdr,         pfiber_hdr,      sizeof(struct tms_fibersection_hdr));
	memcpy(&fiber_val[0],      pfiber_val,      sizeof(struct tms_fibersection_val) * pfiber_hdr->count);

	
	// 转字节序
	// todo CRC
	otdr_crc_hdr.crc   = htonl(otdr_crc_hdr.crc);
	otdr_crc_hdr.count = htonl(otdr_crc_hdr.count);
	tms_OTDRConv_tms_otdr_crc_val(&otdr_crc_val[0], &otdr_crc_val[0], potdr_crc_hdr->count);
	otdr_ch_status.ch_status = htonl(otdr_ch_status.ch_status);
	otdr_param_hdr.count = htonl(otdr_param_hdr.count);
	for (int i = 0; i < potdr_param_hdr->count; i++) {
		tms_OTDRConv_tms_get_otdrdata(
		    (struct tms_get_otdrdata *)&otdr_param_val[i],
		    (struct tms_get_otdrdata *)&otdr_param_val[i]);
	}
	tms_OTDRConv_tms_fibersection_hdr(&fiber_hdr, &fiber_hdr);
	tms_OTDRConv_tms_fibersection_val(&fiber_val[0], &fiber_val[0], &fiber_hdr);


	int len;
	len = sizeof(struct tms_otdr_crc_hdr) +
	      sizeof(struct tms_otdr_crc_val) * potdr_crc_hdr->count +
	      sizeof(struct tms_otdr_ch_status) +
	      sizeof(struct tms_otdr_param_hdr) +
	      sizeof(struct tms_otdr_param_val) * potdr_param_hdr->count +
	      sizeof(struct tms_fibersection_hdr) +
	      sizeof(struct tms_fibersection_val) * pfiber_hdr->count;

	struct glink_base  base_hdr;
	// PrintfMemory(pdata, flen);
	tms_FillGlinkFrame(&base_hdr, paddr);
	glink_Build(&base_hdr, ID_OTDRBASICINFO, len);
	glink_SendHead(pcontext->fd, &base_hdr);
	glink_SendSerial(pcontext->fd, (uint8_t *)&otdr_crc_hdr,      sizeof(struct tms_otdr_crc_hdr));
	glink_SendSerial(pcontext->fd, (uint8_t *)&otdr_crc_val[0],   sizeof(struct tms_otdr_crc_val)* potdr_crc_hdr->count);
	glink_SendSerial(pcontext->fd, (uint8_t *)&otdr_ch_status,    sizeof(struct tms_otdr_ch_status));
	glink_SendSerial(pcontext->fd, (uint8_t *)&otdr_param_hdr,    sizeof(struct tms_otdr_param_hdr));
	glink_SendSerial(pcontext->fd, (uint8_t *)&otdr_param_val[0], sizeof(struct tms_otdr_param_val) * potdr_param_hdr->count);
	glink_SendSerial(pcontext->fd, (uint8_t *)&fiber_hdr,         sizeof(struct tms_fibersection_hdr));
	glink_SendSerial(pcontext->fd, (uint8_t *)&fiber_val[0],      sizeof(struct tms_fibersection_val) * pfiber_hdr->count);
	glink_SendTail(pcontext->fd);
	return 0;
}

static int32_t tms_AnalyseOTDRBasicInfo(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	if (pcontext->ptcb->pf_OnOTDRBasicInfo) {
		pcontext->ptcb->pf_OnOTDRBasicInfo(pcontext);
	}

}

//	0x80000012	ID_CONFIGNODETIME
static int32_t tms_AnalyseConfigNodeTime(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	tms_DbgAckSuccess(pcontext, pdata, len);
#endif
	if (pcontext->ptcb->pf_OnConfigNodeTime) {
		pcontext->ptcb->pf_OnConfigNodeTime(pcontext);
	}

}

// 0x80000013 ID_CURALARM
static int32_t tms_AnalyseCurAlarm(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	if (pcontext->ptcb->pf_OnCurAlarm) {
		pcontext->ptcb->pf_OnCurAlarm(pcontext);
	}

}

// 0x80000014	ID_GETOTDRDATA_14
static int32_t tms_AnalyseGetOTDRData(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	tms_DbgAckSuccess(pcontext, pdata, len);
#endif

	struct tms_get_otdrdata *potdr = (struct tms_get_otdrdata *)(pdata + GLINK_OFFSET_DATA);

	tms_OTDRConv_tms_get_otdrdata(
	    (struct tms_get_otdrdata *)potdr,
	    (struct tms_get_otdrdata *)potdr);
#ifdef HEBEI2_DBG
	tms_Print_tms_get_otdrdata(potdr);
#endif
	if (pcontext->ptcb->pf_OnGetOTDRData) {
		pcontext->ptcb->pf_OnGetOTDRData(pcontext);
	}

}


// 0x80000019	ID_RETOTDRDATA_19
static int32_t tms_AnalyseRetOTDRData(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
	return 0;
}
// 0x80000020	ID_GETSTANDARDCURV
static int32_t tms_AnalyseGetStandardCurv(struct tms_context *pcontext, int8_t *pdata, int32_t len)
{
#ifdef HEBEI2_DBG
	tms_DbgAckSuccess(pcontext, pdata, len);
#endif
	struct tms_getstandardcurv *pval;
	pval = (struct tms_getstandardcurv *)(pdata + GLINK_OFFSET_DATA);

	pval->pipe = htonl(pval->pipe);
	if (pcontext->ptcb->pf_OnGetCycleTestCuv) {
		pcontext->ptcb->pf_OnGetStandardCurv(pcontext, pval);
	}
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

		// fd = tms_SelectFdByAddr(&paddr->dst);
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
	{	tms_AnalyseSetOTDRFPGAInfo	, 0}, //	0x20000000	ID_CHECKOUTRESULT
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
	{	tms_AnalyseSetOCVMPara	, 0}, //	0x70000000	ID_SETOCVMPARA
	{	tms_AnalyseSetOCVMFPGAInfo	, 0}, //	0x70000001	ID_SETOCVMFPGAINFO

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
#if PRINT_CMD_NAME_DBG
		tms_PrintCmdName(cmdid);
#endif
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
	// struct tms_devbase *pdev;

	// pdev = &sg_devnet[0][0];
	// for (uint32_t i = 0; i < sizeof(sg_devnet) /  sizeof(struct tms_devbase) ; i++) {
	// 	pdev[i].frame = MAX_FRAME;
	// }
	// bzero(&sg_manage, sizeof(struct tms_manage));
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
struct _ep_find_val {
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
