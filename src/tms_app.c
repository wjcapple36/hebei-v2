#include "protocol/tmsxx.h"
#include <string.h>
#include "ep_app.h"
#include "stdio.h"
#include "epollserver.h"
#include "tms_app.h"
// #include "tmsxxdb.h"
#include "sys/wait.h"
// #include <strings.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

extern struct ep_t ep;
int connect_first_card(char *str_addr, char *str_port)
{
	printf("%s\n", __FUNCTION__);
	struct ep_con_t client;
	char *pstrAddr;
	unsigned short port;

	// goto _Next;
// _Next:
	// printf("connect\n");
	// return 0;
	pstrAddr = str_addr;
	port     = (unsigned short)atoi(str_port);

	printf("Request connect %s:%d\n",pstrAddr,port);
	if (0 == ep_Connect(&ep,&client, pstrAddr, port)) {
	// if (0 == ep_Connect(&ep,&client, "127.0.0.1", 6000)) {
		printf("client %s:%d\n", 
					inet_ntoa(client.loc_addr.sin_addr),
					htons(client.loc_addr.sin_port));	
	}
	return client.sockfd;
}
int32_t OnGetBasicInfo(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	trace_dbg("cmdid %x\n", pcontext->pgb->cmdid );
	
	// 当前节点管理器只支持最大16通道，多了报告信息存在乱码
#define MAX_PIPE (8)
	char strout[64];
	#if 1

	struct tms_otdrbaseinfo     val;
	struct tms_otdr_crc_hdr     otdr_crc_hdr;
	struct tms_otdr_crc_val     otdr_crc_val[MAX_PIPE];
	struct tms_otdr_ch_status   otdr_ch_status;
	struct tms_otdr_param_hdr   otdr_param_hdr;
	struct tms_otdr_param_val   otdr_param_val[MAX_PIPE];
	struct tms_fibersection_hdr fiber_hdr;
	struct tms_fibersection_val fiber_val[MAX_PIPE];


	// 最大通道号有32个，通道号从1开始计数
	// 只需要修改active_pipe里的通道就可构造相应的数据包
	int active_pipe[] = {2, 3, 7, 8};
	
	trace_dbg("节点管理器存在缺陷，具体描述查看源码\n");
	// 节点管理器存在缺陷
	// int active_pipe[] = {1,2,3,4,8,9};// 能支持
	// int active_pipe[] = {1,2,3,4,8,9,10};// 报告乱码

	

	val.otdr_crc_hdr = &otdr_crc_hdr;
	val.otdr_crc_val = &otdr_crc_val[0];
	val.otdr_ch_status = &otdr_ch_status;
	val.otdr_param_hdr = &otdr_param_hdr;
	val.otdr_param_val = &otdr_param_val[0];
	val.fiber_hdr = &fiber_hdr;
	val.fiber_val = &fiber_val[0];


	otdr_crc_hdr.count = MAX_PIPE;//sizeof(active_pipe) / sizeof(active_pipe[0]);
	strcpy(otdr_crc_hdr.id, "OTDRInfo");
	strcpy(otdr_crc_hdr.name, "HelloKuGou");

	struct sockaddr addr;
	socklen_t addrlen = sizeof(struct sockaddr);
	getsockname(pcontext->fd, &addr, &addrlen);
	struct sockaddr_in *addr_in;
	addr_in = (struct sockaddr_in *)&addr;
	snprintf(strout, 64, "%s", inet_ntoa(addr_in->sin_addr));
	strcpy(otdr_crc_hdr.addr, strout);
	strcpy(otdr_crc_hdr.hw_ver, "1.2.3.4");
	strcpy(otdr_crc_hdr.sf_ver, "4.3.2.1");

	for (int i = 0; i < otdr_crc_hdr.count; i++) {
		otdr_crc_val[i].pipe = i + 1;
		otdr_crc_val[i].wl = 1550;
		otdr_crc_val[i].dr = 40;
		strcpy(otdr_crc_val[i].wdm, "what?");
	}

	strcpy(otdr_ch_status.id, "PipeState");
	otdr_ch_status.ch_status = 0;
	for (int i = 0; i < sizeof(active_pipe) / sizeof(active_pipe[0]); i++) {
		otdr_ch_status.ch_status |= (1 << (active_pipe[i] - 1));
	}
	printf("-------%x-------\n", otdr_ch_status.ch_status );



	otdr_param_hdr.count = sizeof(active_pipe) / sizeof(active_pipe[0]);
	strcpy(otdr_param_hdr.id, "OTDRTestParaConfig");

	for (int i = 0; i < sizeof(active_pipe) / sizeof(active_pipe[0]); i++) {
		otdr_param_val[i].pipe = active_pipe[i];
		otdr_param_val[i].range = 10000;
		otdr_param_val[i].wl = 1550;
		otdr_param_val[i].pw = 40;
		otdr_param_val[i].time = 1234;
		otdr_param_val[i].gi = 1.1;
		otdr_param_val[i].end_threshold = 1.1;
		otdr_param_val[i].none_reflect_threshold = 1.10;
	}


	fiber_hdr.count = sizeof(active_pipe) / sizeof(active_pipe[0]);
	strcpy(fiber_hdr.id, "FiberSectionConfig");
	for (int i = 0; i < fiber_hdr.count; i++) {
		fiber_val[i].pipe_num = active_pipe[i];
		fiber_val[i].fiber_num = active_pipe[i];
		snprintf(strout, 64, "route %d", active_pipe[i] + 1);
		strcpy(fiber_val[i].fiber_route, strout);
		snprintf(strout, 64, "name %d", active_pipe[i] + 1);
		strcpy(fiber_val[i].fiber_name, strout);
		fiber_val[i].start_coor = 0;
		snprintf(strout, 64, "start %d", active_pipe[i] + 1);
		strcpy(fiber_val[i].start_inf, strout);
		fiber_val[i].end_coor = 0;
		snprintf(strout, 64, "end %d", active_pipe[i] + 1);
		strcpy(fiber_val[i].end_inf, strout);


		fiber_val[i].fibe_atten_init = 1.1;
		fiber_val[i].level1 = 1.1;
		fiber_val[i].level2 = 1.1;
		fiber_val[i].listen_level = 1.1;
	}

	tms_OTDRBasicInfo(pcontext, NULL, &val);
#endif
	sleep(5);
	
	// 上报模拟告警
	trace_dbg("上报模拟告警，节点管理器不支持逐条发送告警\n");


	struct tms_curalarm alarm;

	struct tms_alarmlist_hdr    alarmlist_hdr;
	struct tms_alarmlist_val    alarmlist_val[10];
	struct tms_alarmline_hdr    alarmline_hdr;
	struct tms_alarmline_val    alarmline_val[1];

	struct tms_ret_otdrdata otdrdata;
	struct tms_ret_otdrparam    ret_otdrparam;
	struct tms_test_result      test_result;
	struct tms_hebei2_data_hdr  hebei2_data_hdr;
	struct tms_hebei2_data_val  hebei2_data_val[1024 * 32], *tmp_data_val;
	struct tms_hebei2_event_hdr hebei2_event_hdr;
	struct tms_hebei2_event_val hebei2_event_val[128];

	printf("%d\n", __LINE__);
	// ************************************************************
	alarm.alarmlist_hdr = &alarmlist_hdr;
	alarm.alarmlist_val = &alarmlist_val[0];
	alarm.alarmline_hdr = &alarmline_hdr;
	alarm.alarmline_val = &alarmline_val[0];
	
	alarmlist_hdr.count = 1;

	
	alarmlist_val[0].pipe = 2;
	alarmlist_val[0].fiber = 2;
	alarmlist_val[0].level = 1;
	alarmlist_val[0].type = 1;
	
	strcpy(alarmlist_val[0].time, "2016-04-32 12:12:33");
	strcpy(alarmlist_val[0].reserved0, "");;
	alarmlist_val[0].location[0] = 0;

	alarmline_hdr.count = 1;
	alarmline_val[0].pipe = 1;
	alarmline_val[0].datalen =  sizeof(otdrdata) +
	                            sizeof(ret_otdrparam) +
	                            sizeof(test_result) +
	                            sizeof(hebei2_data_hdr) +
	                            sizeof(hebei2_data_val) +
	                            sizeof(hebei2_event_hdr) +
	                            sizeof(hebei2_event_val) + 4;




	// ************************************************************

	alarmline_val[0].ret_otdrparam    = &ret_otdrparam;
	alarmline_val[0].test_result      = &test_result;
	alarmline_val[0].hebei2_data_hdr  = &hebei2_data_hdr;
	alarmline_val[0].hebei2_data_val  = hebei2_data_val;
	alarmline_val[0].hebei2_event_hdr = &hebei2_event_hdr;
	alarmline_val[0].hebei2_event_val = hebei2_event_val;


	// ret_otdrparam.pipe   = pval->pipe;
	ret_otdrparam.range  = 30000;//pval->range;
	ret_otdrparam.wl     = 1550;
	ret_otdrparam.pw     = 40;
	ret_otdrparam.time   = 1234;
	ret_otdrparam.gi     = 1.1f;//pval->gi;
	ret_otdrparam.end_threshold          = 1.1;
	ret_otdrparam.none_reflect_threshold = 1.1;

	strcpy(test_result.result, "OTDRTestResultInfo");
	test_result.range = 30000;
	test_result.loss = 2;
	test_result.atten = 2;
	strcpy(test_result.time, "2016-03-02 22:11:31");

	strcpy((char *)hebei2_data_hdr.dpid, "OTDRData");
	hebei2_data_hdr.count = 16000;
	hebei2_data_hdr.count = 15000;

	tmp_data_val = hebei2_data_val;
	for (int i = 0; i < 4000; i++) {
		tmp_data_val->data = 40000 + i;
		tmp_data_val++;
	}
	for (int i = 4000; i < 8000; i++) {
		tmp_data_val->data = 40000 - i;
		tmp_data_val++;
	}
	for (int i = 8000; i < 16000; i++) {
		tmp_data_val->data = 40000 + i;
		tmp_data_val++;
	}
	strcpy((char *)hebei2_event_hdr.eventid, "KeyEvents");
	hebei2_event_hdr.count = 2;

	hebei2_event_val[0].distance   = 10;
	hebei2_event_val[0].event_type = 0;
	hebei2_event_val[0].att        = 3;
	hebei2_event_val[0].loss       = 3;
	hebei2_event_val[0].reflect    = 3;
	hebei2_event_val[0].link_loss  = 3;

	hebei2_event_val[1].distance   = 10000;
	hebei2_event_val[1].event_type = 3;
	hebei2_event_val[1].att        = 4;
	hebei2_event_val[1].loss       = 4;
	hebei2_event_val[1].reflect    = 4;
	hebei2_event_val[1].link_loss  = 4;

	tms_CurAlarm(pcontext->fd, NULL, &alarm);
	sleep(5);
	alarmlist_val[0].pipe = 7;
	alarmlist_val[0].fiber = 7;
	alarmlist_val[0].level = 1;
	alarmlist_val[0].type = 1;

	int fd = connect_first_card("127.0.0.1","6000");	
	tms_CurAlarm(fd, NULL, &alarm);
	close(fd);
	return 0;
}

int32_t OnGetNodeTime(struct tms_context *pcontext)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	time_t t;
	struct tm *local; //本地时间

	t = time(NULL);
	local = localtime(&t); //转为本地时间

	printf(" % d - % d - % d % d: % d: % d\n",
	       local->tm_hour, local->tm_mday, local->tm_wday,
	       local->tm_hour, local->tm_min, local->tm_sec);


	char buf[20];
	strftime(buf, 20, " % Y - % m - % d % H: % M: % S", local);

#ifdef CONFIG_PROC_HEBEI2
	tms_RetNodeTime(pcontext, NULL, buf);
#endif
	return 0;
}
int32_t OnRetNodeTime(struct tms_context *pcontext)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);

	return 0;
}
int32_t OnNameAndAddress(struct tms_context *pcontext)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnFiberSectionCfg(struct tms_context *pcontext, struct tms_fibersectioncfg *pval)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);

#if 0
	struct tms_fibersection_hdr *fiber_hdr;
	struct tms_fibersection_val *fiber_val;
	struct tms_otdr_param       *otdr_param;
	struct tms_test_result      *test_result;
	struct tms_hebei2_data_hdr  *otdr_hdr;
	struct tms_hebei2_data_val  *otdr_val;
	struct tms_hebei2_event_hdr *event_hdr;
	struct tms_hebei2_event_val *event_val;

	fiber_hdr = pval->fiber_hdr;
	fiber_val = pval->fiber_val;
	otdr_param = pval->otdr_param;
	test_result = pval->test_result;
	event_hdr = pval->event_hdr;
	event_val = pval->event_val;

	tms_Print_tms_fibersection_hdr(fiber_hdr);
	tms_Print_tms_fibersection_val(fiber_val);
	tms_Print_tms_otdr_param(otdr_param);
	tms_Print_tms_test_result(test_result);
	tms_Print_tms_hebei2_event(event_hdr, event_val);
#endif
	return 0;
}
int32_t OnConfigPipeState(struct tms_context *pcontext, struct tms_cfgpip_status *pval)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	uint32_t status = pval->status;
	char ch8[8];

	for (int i = 0; i < 8; i++) {
		ch8[i] = status & (0x01 << i);
		if (ch8[i]) {
			printf("\tch % 2d on\n", i + 1);
		}
	}
	// todo 该设备在本机框第几槽位，对应第几通道，写入配置文件
	hb2_dbg("Warning CU 需要多次转发此消息\n");

	return 0;
}
int32_t OnGetCycleTestCuv(struct tms_context *pcontext, struct tms_getcyctestcuv *pval)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	printf("\tget pipe % d\n", pval->pipe);
	return 0;
}
int32_t OnGetStatusData(struct tms_context *pcontext, struct tms_getstatus_data *pval)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	printf("\tget pipe status % d\n", pval->pipe);
	return 0;
}
int32_t OnStatusData(struct tms_context *pcontext)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnCRCCheckout(struct tms_context *pcontext)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnCheckoutResult(struct tms_context *pcontext)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnOTDRBasicInfo(struct tms_context *pcontext)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnConfigNodeTime(struct tms_context *pcontext)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	// TODO set time
	return 0;
}
int32_t OnCurAlarm(struct tms_context *pcontext)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnGetOTDRData(struct tms_context *pcontext, struct tms_get_otdrdata *pval)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	struct tms_ret_otdrdata otdrdata;
	struct tms_ret_otdrparam    ret_otdrparam;
	struct tms_test_result      test_result;
	struct tms_hebei2_data_hdr  hebei2_data_hdr;
	struct tms_hebei2_data_val  hebei2_data_val[1024 * 32], *tmp_data_val;
	struct tms_hebei2_event_hdr hebei2_event_hdr;
	struct tms_hebei2_event_val hebei2_event_val[128];


	otdrdata.ret_otdrparam    = &ret_otdrparam;
	otdrdata.test_result      = &test_result;
	otdrdata.hebei2_data_hdr  = &hebei2_data_hdr;
	otdrdata.hebei2_data_val  = hebei2_data_val;
	otdrdata.hebei2_event_hdr = &hebei2_event_hdr;
	otdrdata.hebei2_event_val = hebei2_event_val;


	// ret_otdrparam.pipe   = pval->pipe;
	ret_otdrparam.range  = 30000;//pval->range;
	ret_otdrparam.wl     = pval->wl;
	ret_otdrparam.pw     = pval->pw;
	ret_otdrparam.time   = pval->time;
	ret_otdrparam.gi     = 1.1f;//pval->gi;
	ret_otdrparam.end_threshold          = pval->end_threshold;
	ret_otdrparam.none_reflect_threshold = pval->none_reflect_threshold;

	strcpy(test_result.result, "OTDRTestResultInfo");
	test_result.range = 30000;
	test_result.loss = 2;
	test_result.atten = 2;
	strcpy(test_result.time, "2016 - 03 - 02 22: 11: 31");

	strcpy((char *)hebei2_data_hdr.dpid, "OTDRData");
	hebei2_data_hdr.count = 16000;
	hebei2_data_hdr.count = 15000;

	tmp_data_val = hebei2_data_val;
	for (int i = 0; i < 4000; i++) {
		tmp_data_val->data = 40000 + i;
		tmp_data_val++;
	}
	for (int i = 4000; i < 8000; i++) {
		tmp_data_val->data = 40000 - i;
		tmp_data_val++;
	}
	for (int i = 8000; i < 16000; i++) {
		tmp_data_val->data = 40000 + i;
		tmp_data_val++;
	}
	strcpy((char *)hebei2_event_hdr.eventid, "KeyEvents");
	hebei2_event_hdr.count = 2;

	hebei2_event_val[0].distance   = 10;
	hebei2_event_val[0].event_type = 0;
	hebei2_event_val[0].att        = 3;
	hebei2_event_val[0].loss       = 3;
	hebei2_event_val[0].reflect    = 3;
	hebei2_event_val[0].link_loss  = 3;

	hebei2_event_val[1].distance   = 10000;
	hebei2_event_val[1].event_type = 3;
	hebei2_event_val[1].att        = 4;
	hebei2_event_val[1].loss       = 4;
	hebei2_event_val[1].reflect    = 4;
	hebei2_event_val[1].link_loss  = 4;


#ifdef HEBEI2_DBG
	hb2_dbg("无法测试周期测量曲线，需要苏宁网管，发送周期测量，返回ID_RETOTDRDATA_19\n");
#endif
	if (pcontext->pgb->cmdid == ID_GETOTDRDATA_15) {
		tms_RetOTDRData(pcontext->fd, NULL, &otdrdata, ID_RETOTDRDATA_17);
	}
	else if (pcontext->pgb->cmdid == ID_GETOTDRDATA_14) {

		tms_RetOTDRData(pcontext->fd, NULL, &otdrdata, ID_RETOTDRDATA_16);
	}
	else {
		// 需要苏宁配合
		tms_RetOTDRData(pcontext->fd, NULL, &otdrdata, ID_RETOTDRDATA_19);
		trace_dbg("unknow cmd\n");
	}

	return 0;
}
int32_t OnGetStandardCurv(struct tms_context *pcontext, struct tms_getstandardcurv *pval)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	hb2_dbg("pipe % d\n", pval->pipe);


	struct tms_ret_otdrdata otdrdata;
	struct tms_ret_otdrparam    ret_otdrparam;
	struct tms_test_result      test_result;
	struct tms_hebei2_data_hdr  hebei2_data_hdr;
	struct tms_hebei2_data_val  hebei2_data_val[1024 * 32], *tmp_data_val;
	struct tms_hebei2_event_hdr hebei2_event_hdr;
	struct tms_hebei2_event_val hebei2_event_val[128];


	otdrdata.ret_otdrparam    = &ret_otdrparam;
	otdrdata.test_result      = &test_result;
	otdrdata.hebei2_data_hdr  = &hebei2_data_hdr;
	otdrdata.hebei2_data_val  = hebei2_data_val;
	otdrdata.hebei2_event_hdr = &hebei2_event_hdr;
	otdrdata.hebei2_event_val = hebei2_event_val;


	// ret_otdrparam.pipe   = pval->pipe;
	ret_otdrparam.range  = 30000;//pval->range;
	ret_otdrparam.wl     = 1310;
	ret_otdrparam.pw     = 10;
	ret_otdrparam.time   = 0x12345678;
	ret_otdrparam.gi     = 1.1f;//pval->gi;
	ret_otdrparam.end_threshold          = 1.1;
	ret_otdrparam.none_reflect_threshold = 1.1;

	strcpy(test_result.result, "OTDRTestResultInfo");
	test_result.range = 30000;
	test_result.loss = 2;
	test_result.atten = 2;
	strcpy(test_result.time, "2016 - 03 - 02 22: 11: 31");

	strcpy((char *)hebei2_data_hdr.dpid, "OTDRData");
	hebei2_data_hdr.count = 16000;
	hebei2_data_hdr.count = 15000;

	tmp_data_val = hebei2_data_val;
	for (int i = 0; i < 4000; i++) {
		tmp_data_val->data = 40000 + i;
		tmp_data_val++;
	}
	for (int i = 4000; i < 8000; i++) {
		tmp_data_val->data = 40000 - i;
		tmp_data_val++;
	}
	for (int i = 8000; i < 16000; i++) {
		tmp_data_val->data = 40000 + i;
		tmp_data_val++;
	}
	strcpy((char *)hebei2_event_hdr.eventid, "KeyEvents");
	hebei2_event_hdr.count = 2;

	hebei2_event_val[0].distance   = 10;
	hebei2_event_val[0].event_type = 0;
	hebei2_event_val[0].att        = 3;
	hebei2_event_val[0].loss       = 3;
	hebei2_event_val[0].reflect    = 3;
	hebei2_event_val[0].link_loss  = 3;

	hebei2_event_val[1].distance   = 10000;
	hebei2_event_val[1].event_type = 3;
	hebei2_event_val[1].att        = 4;
	hebei2_event_val[1].loss       = 4;
	hebei2_event_val[1].reflect    = 4;
	hebei2_event_val[1].link_loss  = 4;

	tms_RetOTDRData(pcontext->fd, NULL, &otdrdata, ID_RETOTDRDATA_18);

	return 0;
}
int32_t OnSetOTDRFPGAInfo(struct tms_context *pcontext, struct tms_setotdrfpgainfo *pval)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	hb2_dbg("pipe % d wl % d dr % d wdm % s\n",
	        pval->pipe,
	        pval->wl,
	        pval->dr,
	        pval->wdm);
}
int32_t OnSetOCVMPara(struct tms_context *pcontext, struct tms_setocvmpara *pval)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	hb2_dbg("cable_len	% f"
	        "host_thr	% d"
	        "slave_thr	% d"
	        "amend		% f\n",
	        pval->cable_len,
	        pval->host_thr,
	        pval->slave_thr,
	        pval->amend);
	return 0;
}

int32_t OnSetOCVMFPGAInfo(struct tms_context *pcontext, struct tms_setocvmfpgainfo *pval)
{
	trace_dbg(" % s(): % d\n", __FUNCTION__, __LINE__);
	hb2_dbg("max cable len	% f"
	        "wl	% s\n",
	        pval->max_cable_len,
	        pval->wl);
	return 0;
}

void tms_Callback(struct tms_callback *ptcb)
{
	bzero(ptcb, sizeof(struct tms_callback));
	ptcb->pf_OnGetBasicInfo		= OnGetBasicInfo;
	ptcb->pf_OnGetNodeTime		= OnGetNodeTime;
	ptcb->pf_OnRetNodeTime		= OnRetNodeTime;
	ptcb->pf_OnNameAndAddress	= OnNameAndAddress;
	ptcb->pf_OnFiberSectionCfg	= OnFiberSectionCfg;
	ptcb->pf_OnConfigPipeState	= OnConfigPipeState;
	ptcb->pf_OnGetCycleTestCuv	= OnGetCycleTestCuv;
	ptcb->pf_OnGetStatusData	= OnGetStatusData;
	ptcb->pf_OnStatusData		= OnStatusData;
	ptcb->pf_OnCRCCheckout		= OnCRCCheckout;


	ptcb->pf_OnCheckoutResult	= OnCheckoutResult;
	ptcb->pf_OnOTDRBasicInfo	= OnOTDRBasicInfo;
	ptcb->pf_OnConfigNodeTime	= OnConfigNodeTime;
	ptcb->pf_OnCurAlarm		= OnCurAlarm;
	ptcb->pf_OnGetOTDRData		= OnGetOTDRData;
	ptcb->pf_OnGetStandardCurv	= OnGetStandardCurv;

	// 70000000
	ptcb->pf_OnSetOCVMPara		= OnSetOCVMPara;
	ptcb->pf_OnSetOCVMFPGAInfo	= OnSetOCVMFPGAInfo;

	// 20000000
	ptcb->pf_OnSetOTDRFPGAInfo	= OnSetOTDRFPGAInfo;

#ifdef _MANAGE

	// 重定向tms_Analysexxx函数处理方式，如果不执行任何tms_SetDoWhat，则默认表示
	// 协议处于MCU工作方式，回调函数的dowhat设置为0表示不做任何转发，收到的数据一律
	// 传递给应用层
	// 1.作为网管和板卡来说都是传递给应用层
	// 2.作为CU和MCU就要仔细修改dowhat的处理方式
	// int cmd_0xx000xxxx[100];

	// bzero(cmd_0xx000xxxx, sizeof(cmd_0xx000xxxx) / sizeof(int));
	// tms_SetDoWhat(0x10000000, sizeof(cmd_0xx000xxxx) / sizeof(int), cmd_0xx000xxxx);
	// tms_SetDoWhat(0x60000000, sizeof(cmd_0xx000xxxx) / sizeof(int), cmd_0xx000xxxx);
	// tms_SetDoWhat(0x80000000, sizeof(cmd_0xx000xxxx) / sizeof(int), cmd_0xx000xxxx);
#endif

}

#ifdef __cplusplus
}
#endif
