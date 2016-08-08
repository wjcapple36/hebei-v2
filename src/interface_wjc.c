#include "protocol/tmsxx.h"
#include <string.h>
#include "ep_app.h"
#include "stdio.h"
#include "epollserver.h"
#include "tms_app.h"
// #include "tmsxxdb.h"
#include "sys/wait.h"
// #include <strings.h>

#include "../schedule/otdr_ch/otdr_ch.h"
#include "../schedule/common/hb_app.h"
#include "../schedule/common/global.h"
#ifdef __cplusplus
extern "C" {
#endif
//通道的偏移量，CU通过通道来确定偏移来确定与哪一个otdr通信


int32_t OnGetBasicInfo(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	return 0;
}

int32_t OnGetNodeTime(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	time_t t;
	struct tm *local; //本地时间

	t = time(NULL);
	local = localtime(&t); //转为本地时间

	printf("%d-%d-%d %d:%d:%d\n",
	       local->tm_hour, local->tm_mday, local->tm_wday,
	       local->tm_hour, local->tm_min, local->tm_sec);


	char buf[20];
	strftime(buf, 20, "%Y-%m-%d %H:%M:%S", local);

#ifdef CONFIG_PROC_HEBEI2
	tms_RetNodeTime(pcontext, NULL, buf);
#endif
	return 0;
}
int32_t OnRetNodeTime(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);

	return 0;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  OnNameAndAddress 处理收到节点名称和地址
 *
 * @param pcontext
 *
 * @returns   0成功，以及无法保存的命令
 */
/* ----------------------------------------------------------------------------*/
int32_t OnNameAndAddress(struct tms_context *pcontext)
{
	struct tms_ack ack;
	int32_t ret;
	//该回调函数缺乏输入节点名称，需要woo配合修改
	ack.cmdid = pcontext->pgb->cmdid;
	ret = save_node_name_address(&devMisc);

	if(ret != CMD_RET_OK)
	       ret = CMD_RET_CANT_SAVE;

	ack.errcode = ret;
	tms_AckEx(pcontext->fd, NULL, &ack);	

	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	return 0;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  OnFiberSectionCfg 主机配置光纤段参数
 *
 * @param pcontext	主机相关地址
 * @param pval		具体参数
 *
 * @returns   		0 ，参数非法，无法保存
 */
/* ----------------------------------------------------------------------------*/
int32_t OnFiberSectionCfg(struct tms_context *pcontext,struct tms_fibersectioncfg *pval)
{
	int32_t ret,ch;
	struct tms_ack ack;

	ack.cmdid = pcontext->pgb->cmdid;

	ret = check_fiber_sec_para(pval);

	if(ret != CMD_RET_OK)
		goto usr_exit;

	ch = pval->fiber_val[0].pipe_num;
	ret = save_fiber_sec_para(ch, pval);
	if(ret != CMD_RET_OK)
		ret = CMD_RET_CANT_SAVE;
usr_exit:
	ack.errcode = ret;
	tms_AckEx(pcontext->fd, NULL,&ack);
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  OnConfigPipeState 主机配置通道使用状态
 *
 * @param pcontext
 * @param pval
 *
 * @returns   0，无法保存
 */
/* ----------------------------------------------------------------------------*/
int32_t OnConfigPipeState(struct tms_context *pcontext, struct tms_cfgpip_status *pval)
{
	int32_t ret;
	struct tms_ack ack;

	ack.cmdid = pcontext->pgb->cmdid;
	devMisc.ch_state.state = pval->status;
	ret = save_node_name_address(&devMisc);
	if(ret != CMD_RET_OK){
		ret = CMD_RET_CANT_SAVE;
		printf("%s %d can't save para, ret %d \n", __FUNCTION__, __LINE__,ret);
		ret = CMD_RET_OK;
	}
	ack.errcode = ret;
	tms_AckEx(pcontext->fd, NULL, &ack);

	// todo 该设备在本机框第几槽位，对应第几通道，写入配置文件
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	
	return ret;
}
int32_t OnGetCycleTestCuv(struct tms_context *pcontext, struct tms_getcyctestcuv *pval)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	printf("\tget pipe %d\n", pval->pipe);
	return 0;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  OnGetStatusData 获取某通道的统计数据
 *
 * @param pcontext
 * @param pval
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t OnGetStatusData(struct tms_context *pcontext, struct tms_getstatus_data *pval)
{
	int ret;
	struct tms_ack ack;
	ack.cmdid = pcontext->pgb->cmdid;
	ret = CMD_RET_OK;



	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	printf("\tget pipe status %d\n", pval->pipe);
	return 0;
}
int32_t OnStatusData(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnCRCCheckout(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnCheckoutResult(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnOTDRBasicInfo(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	return 0;
}
int32_t OnConfigNodeTime(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	// TODO set time
	return 0;
}
int32_t OnCurAlarm(struct tms_context *pcontext)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	return 0;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  OnGetOTDRData 用户点名测量
 *
 * @param pcontext
 * @param pget_otdr_data
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t OnGetOTDRData(struct tms_context *pcontext,struct tms_get_otdrdata *pget_otdr_data)
{
	int ret;
	struct tms_ack ack;
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	ack.cmdid = pcontext->pgb->cmdid;
	

	ret = check_usr_otdr_test_para(pget_otdr_data);
	if(ret != CMD_RET_OK)
		goto usr_exit;
	//正在累加中，不能响应用户的测量需求
	if(usrOtdrTest.state == USR_OTDR_TEST_ACCUM ||
			usrOtdrTest.state == USR_OTDR_TEST_WAIT){
		ret = CMD_RET_EXIST_CMD;
		goto usr_exit;
	}
	//给点名测量传递参数
	memcpy(&usrOtdrTest.ch, &pget_otdr_data->pipe, sizeof(struct _tagUsrOtdrTest) - 8);
	usrOtdrTest.ch--;
	usrOtdrTest.cmd = ack.cmdid;
	usrOtdrTest.src_addr = pcontext->pgb->src;
	usrOtdrTest.state = USR_OTDR_TEST_WAIT;
	
usr_exit:
	ack.errcode = ret;
	tms_AckEx(pcontext->fd,NULL, &ack);
	return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  OnGetStandardCurv 获取标准曲线
 *
 * @param pcontext
 * @param pval
 *
 * @returns   标准曲线
 */
/* ----------------------------------------------------------------------------*/
int32_t OnGetStandardCurv(struct tms_context *pcontext, struct tms_getstandardcurv *pval)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	hb2_dbg("pipe %d\n", pval->pipe);
	return 0;
}
int32_t OnSetOTDRFPGAInfo(struct tms_context *pcontext, struct tms_setotdrfpgainfo *pval)
{
	int32_t ret, ch;
	struct tms_ack ack;
	ack.cmdid = pcontext->pgb->cmdid;

	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	hb2_dbg("pipe %d wl %d dr %d wdm %s\n",
	        pval->pipe,
	        pval->wl,
	        pval->dr,
	        pval->wdm);
	if(pval->pipe < ch_offset || pval->pipe > (ch_offset + CH_NUM)){
		ret = CMD_RET_PARA_INVLADE;
		goto usr_exit;
	}
	ch = pval->pipe;
	memcpy(&chFpgaInfo[ch].para, &pval->pipe, sizeof(struct _tagFpgaPara));
	chFpgaInfo[ch].initial = 1;
	ret = save_ch_fpga_info(&chFpgaInfo, CH_NUM);
	if(ret != OP_OK)
		ret = CMD_RET_CANT_SAVE;
usr_exit:
	ack.errcode = ret;
	tms_AckEx(pcontext->fd,NULL,&ack);

	return ret;

}
int32_t OnSetOCVMPara(struct tms_context *pcontext, struct tms_setocvmpara *pval)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	hb2_dbg("cable_len	%f"
	        "host_thr	%d"
	        "slave_thr	%d"
	        "amend		%f\n",
	        pval->cable_len,
	        pval->host_thr,
	        pval->slave_thr,
	        pval->amend);
	return 0;
}

int32_t OnSetOCVMFPGAInfo(struct tms_context *pcontext, struct tms_setocvmfpgainfo *pval)
{
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	hb2_dbg("max cable len	%f"
	        "wl	%s\n",
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
int g_cu_socket = 0;
void NotifyCU(int fd)
{
	if (g_cu_socket == fd) {
		g_cu_socket = 0;
		system("echo 0 > /sys/class/leds/leda/brightness");
	}
}

void *ThreadConnectCU(void *arg)
{
	// struct tmsxx_app *ptmsapp;
	// struct tms_context *pcontext;
	struct ep_t *pep = (struct ep_t*)arg;
	struct ep_con_t client;

	int server_fd;
	// int server_cnt;
	uint32_t server_addr;
	struct glink_addr gl_addr;
	bzero(&client, sizeof(struct ep_con_t));


	usleep(3000000);//延时3s，防止x86下efence奔溃

	gl_addr.pkid = 0;
	gl_addr.src = TMS_DEFAULT_LOCAL_ADDR;

	
	struct tmsxx_app *ptapp = (struct tmsxx_app *)client.ptr;
	while(1) {
		if (g_cu_socket != 0) {
			tms_Tick(client.sockfd, NULL);
			sleep(5);
			continue;
		}
		
		if (0 == ep_Connect(pep,&client, "192.168.0.200", 6000) ) {
			system("echo 1 > /sys/class/leds/leda/brightness");
			g_cu_socket = client.sockfd;	
		}
		
		// else if (0 == ep_Connect(pep,&client, "192.168.1.200", 6000) ) {
		// 	g_cu_socket = client.sockfd;	
		// 	system("echo 1 > /sys/class/leds/leda/brightness");
		// }
		// else if (0 == ep_Connect(pep,&client, "192.168.1.251", 6000) ) {
		// 	g_cu_socket = client.sockfd;	
		// 	system("echo 1 > /sys/class/leds/leda/brightness");
		// }
		sleep(1);
	}

	return NULL;
}


#ifdef __cplusplus
}
#endif
