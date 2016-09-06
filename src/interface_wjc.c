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
	ret_host_basic_info(pcontext,NULL);
	ret_total_curalarm2host();
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

	char buf[20] = {0};
	strftime(buf, 20, "%Y-%m-%d %H:%M:%S", local);

	tms_RetNodeTime(pcontext, NULL, buf);
	printf("%s %d ret host time %s \n", __FUNCTION__, __LINE__, buf);
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
int32_t OnNameAndAddress(struct tms_context *pcontext, struct tms_nameandaddr *pval)
{
	struct tms_ack ack;
	int32_t ret;
	struct glink_addr addr;
	addr.dst = pcontext->pgb->src;
	addr.src = ADDR_LOCAL;
	addr.pkid = pcontext->pgb->pkid;

	//该回调函数缺乏输入节点名称，需要woo配合修改
	ack.cmdid = pcontext->pgb->cmdid;
	memcpy(&devMisc.name, pval,sizeof(struct _tagDevNameAddr));
	ret = save_node_name_address(&devMisc);

	if(ret != CMD_RET_OK)
	       ret = CMD_RET_CANT_SAVE;

	ack.errcode = ret;
	tms_AckEx(pcontext->fd, &addr, &ack);	

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
	struct glink_addr addr;
	addr.dst = pcontext->pgb->src;
	addr.src = ADDR_LOCAL;
	addr.pkid = pcontext->pgb->pkid;


	ack.cmdid = pcontext->pgb->cmdid;

	ret = check_fiber_sec_para(pval);

	if(ret != CMD_RET_OK)
		goto usr_exit;
	//host的通道是从1开始计数
	ch = pval->fiber_val[0].pipe_num - ch_offset;
	ret = save_fiber_sec_para(ch, pval,&chFiberSec[ch],&otdrDev[ch] );
	if(ret != CMD_RET_OK)
		ret = CMD_RET_CANT_SAVE;
	else
		ret_total_curalarm2host();

usr_exit:
	ack.errcode = ret;
	tms_AckEx(pcontext->fd, &addr, &ack);
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
	struct glink_addr addr;
	addr.dst = pcontext->pgb->src;
	addr.src = ADDR_LOCAL;
	addr.pkid = pcontext->pgb->pkid;


	ack.cmdid = pcontext->pgb->cmdid;
	//第一机框，只八个通道，取低8位 第二个通道，取高8位
	if(ch_offset == 1)
		devMisc.ch_state.state = pval->status & 0x000000ff;
	else
		devMisc.ch_state.state = (pval->status & 0x0000ff00) >> 8;
	ret = save_node_name_address(&devMisc);
	if(ret != CMD_RET_OK){
		ret = CMD_RET_CANT_SAVE;
		printf("%s %d can't save para, ret %d \n", __FUNCTION__, __LINE__,ret);
	}
	ack.errcode = ret;
	tms_AckEx(pcontext->fd, &addr, &ack);

	// todo 该设备在本机框第几槽位，对应第几通道，写入配置文件
	printf("%s():%d state local 0x%x rcv 0x%x ch_offset %d\n", __FUNCTION__, __LINE__,\
		       	devMisc.ch_state, pval->status, ch_offset);
	
	return ret;
}
int32_t OnGetCycleTestCuv(struct tms_context *pcontext, struct tms_getcyctestcuv *pval)
{
	int ret, ch;
	struct tms_ack ack;
	struct glink_addr addr;
	addr.dst = pcontext->pgb->src;
	addr.src = ADDR_LOCAL;
	addr.pkid = pcontext->pgb->pkid;

	ack.cmdid = pcontext->pgb->cmdid;
	ret = CMD_RET_OK;
	ch = pval->pipe - ch_offset;
	printf("%s():%d ch %d \n", __FUNCTION__, __LINE__, pval->pipe);
	if(ch < 0 || ch > CH_NUM)
	{
		ret = CMD_RET_PARA_INVLADE;
		goto usr_exit;
	}

	ret_host_cyc_curv(pcontext, &otdrDev[ch].curv, ch);
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	printf("\tget pipe status %d\n", pval->pipe);
usr_exit:
	if(ret)
		tms_AckEx(pcontext->fd, &addr, &ack);
	return ret;
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
	int ret, ch;
	struct tms_ack ack;
	struct glink_addr addr;
	addr.dst = pcontext->pgb->src;
	addr.src = ADDR_LOCAL;
	addr.pkid = pcontext->pgb->pkid;

	ack.cmdid = pcontext->pgb->cmdid;
	ret = CMD_RET_OK;
	ch = pval->pipe - ch_offset;
	if(ch < 0 || ch > CH_NUM)
	{
		ret = CMD_RET_PARA_INVLADE;
		goto usr_exit;
	}


	ret_host_statis_data(ch, pcontext, &chFiberSec[ch]);
	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	printf("\tget pipe status %d\n", pval->pipe);
usr_exit:
	if(ret)
		tms_AckEx(pcontext->fd,&addr, &ack);
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
int32_t OnConfigNodeTime(struct tms_context *pcontext,struct tms_confignodetime *pval)
{
	struct tms_ack ack;
	char ctime[20] = {0};
  	char strout[64] = {0};        
	struct glink_addr addr;
	addr.dst = pcontext->pgb->src;
	addr.src = ADDR_LOCAL;
	addr.pkid = pcontext->pgb->pkid;

	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	memcpy(ctime, pval, 19);
	ctime[19] = '\0';  
	snprintf(strout, 64, "/bin/settime.sh \" %s\"", ctime);
	system(strout);
	ack.cmdid =  pcontext->pgb->cmdid;
	ack.errcode = 0;
	printf("%s %d %s \n", __FUNCTION__, __LINE__, strout);
	// TODO set time
	tms_AckEx(pcontext->fd, &addr, &ack);
	ret_total_curalarm2host();
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
	struct glink_addr addr;
	addr.dst = pcontext->pgb->src;
	addr.src = ADDR_LOCAL;
	addr.pkid = pcontext->pgb->pkid;

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
	memcpy(&usrOtdrTest.ch, &pget_otdr_data->pipe, sizeof(struct tms_get_otdrdata) );
	usrOtdrTest.ch -= ch_offset;
	usrOtdrTest.cmd = ack.cmdid;
	usrOtdrTest.src_addr = pcontext->pgb->src;
	usrOtdrTest.state = USR_OTDR_TEST_WAIT;
	
usr_exit:
	ack.errcode = ret;
	tms_AckEx(pcontext->fd,&addr, &ack);
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
	int ret, ch;
	struct tms_ack ack;
	struct glink_addr addr;
	addr.dst = pcontext->pgb->src;
	addr.src = ADDR_LOCAL;
	addr.pkid = pcontext->pgb->pkid;

	ack.cmdid = pcontext->pgb->cmdid;

	ret = CMD_RET_OK;
	ch = pval->pipe - ch_offset;
	printf("%s %d ch %d \n", __FUNCTION__, __LINE__, pval->pipe);
	if(ch < 0 || ch > CH_NUM)
	{
		ret = CMD_RET_PARA_INVLADE;
		ack.errcode = ret;
		goto usr_exit;
	}
	ret_host_std_curv(pcontext, &chFiberSec[ch],ch);
usr_exit:
	if(ret)
		tms_AckEx(pcontext->fd,&addr, &ack);
	return 0;
}
int32_t OnSetOTDRFPGAInfo(struct tms_context *pcontext, struct tms_setotdrfpgainfo *pval)
{
	int32_t ret, ch,i, is_same;
	struct tms_ack ack;
	struct glink_addr addr;
	addr.dst = pcontext->pgb->src;
	addr.src = ADDR_LOCAL;
	addr.pkid = pcontext->pgb->pkid;

	ack.cmdid = pcontext->pgb->cmdid;

	trace_dbg("%s():%d\n", __FUNCTION__, __LINE__);
	hb2_dbg("pipe %d wl %d dr %d wdm %s\n",
	        pval->pipe,
	        pval->wl,
	        pval->dr,
	        pval->wdm);

	ch = pval->pipe - ch_offset;
	if(ch < 0 || ch >  CH_NUM){
		ret = CMD_RET_PARA_INVLADE;
		goto usr_exit;
	}
	//不分序号排序，首先如果在已经存在的段内找到相同的，那么替代，数目不变，否则最后面追加
	is_same = 0;
	for(i = 0; i < chFpgaInfo.num && i < CH_NUM; i++)
	{
		if(chFpgaInfo.para[i].ch == pval->pipe && chFpgaInfo.para[i].lamda)
		{
			is_same = 1;
			memcpy(&chFpgaInfo.para[i], &pval->pipe, sizeof(struct _tagFpgaPara));
		}
	}
	i = chFpgaInfo.num;
	if(!is_same && i >= 0 && i < CH_NUM )
	{
		memcpy(&chFpgaInfo.para[i], &pval->pipe, sizeof(struct _tagFpgaPara));
		chFpgaInfo.num ++;
	}
	ret = save_ch_fpga_info(&chFpgaInfo, CH_NUM);
	if(ret != OP_OK)
		ret = CMD_RET_CANT_SAVE;
usr_exit:
	ack.errcode = ret;
	tms_AckEx(pcontext->fd,&addr,&ack);

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



#ifdef __cplusplus
}
#endif
