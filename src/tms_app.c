#include "protocol/tmsxx.h"
#include <string.h>
#include "ep_app.h"
#include "stdio.h"
#include "epollserver.h"
#include "tms_app.h"
// #include "tmsxxdb.h"
#include "sys/wait.h"
// #include <strings.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t OnGetBasicInfo(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	return 0;
}

int32_t OnGetNodeTime(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	time_t t;
	struct tm* local; //本地时间

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
	printf("%s():%d\n",__FUNCTION__, __LINE__);

	return 0;
}
int32_t OnNameAndAddress(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	return 0;
}
int32_t OnFiberSectionCfg(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	return 0;
}
int32_t OnConfigPipeState(struct tms_context *pcontext, struct tms_cfgpip_status *pval)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	uint32_t status = pval->status;
	char ch8[8];

	for (int i = 0; i < 8;i++) {
		ch8[i] = status & (0x01 << i);
		if (ch8[i]) {
			printf("\tch%2d on\n", i + 1);
		}
	}
	// todo 该设备在本机框第几槽位，对应第几通道，写入配置文件
	hh2_dbg("Warning CU 需要多次转发此消息\n");
	
	return 0;
}
int32_t OnGetCycleTestCuv(struct tms_context *pcontext,struct tms_getcyctestcuv *pval)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	printf("\tget pipe %d\n", pval->pipe);
	return 0;
}
int32_t OnGetStatusData(struct tms_context *pcontext, struct tms_getstatus_data *pval)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	printf("\tget pipe status %d\n", pval->pipe);
	return 0;
}
int32_t OnStatusData(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	return 0;
}
int32_t OnCRCCheckout(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	return 0;
}
int32_t OnCheckoutResult(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	return 0;
}
int32_t OnOTDRBasicInfo(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	return 0;
}
int32_t OnConfigNodeTime(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	// TODO set time
	return 0;
}
int32_t OnCurAlarm(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
	return 0;
}
int32_t OnGetOTDRdata_14(struct tms_context *pcontext)
{
	printf("%s():%d\n",__FUNCTION__, __LINE__);
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
	ptcb->pf_OnGetOTDRdata_14	= OnGetOTDRdata_14;
	// ptcb->pf_OnCopy2Use             = unuse_copy2use;
	// ptcb->pf_OnGetDeviceComposition = tms_OnGetDeviceComposition;
	// ptcb->pf_OnRetDeviceComposition = tms_OnRetDeviceComposition;
	// ptcb->pf_OnCommand              = tms_OnCommand;
	// ptcb->pf_OnCUNoteNet            = tms_OnCUNoteNet;
	// ptcb->pf_OnCUNoteManageConnect  = tms_OnCUNoteManageConnect;
	// ptcb->pf_OnRetSerialNumber      = tms_OnRetSerialNumber;
	// ptcb->pf_OnSetIPAddress         = tms_OnSetIPAddress;
	// ptcb->pf_OnRetOTDRTest          = tms_OnRetOTDRTest;
	// ptcb->pf_OnAlarmLine            = tms_OnAlarmLine;
	// ptcb->pf_OnCfgOTDRRef           = tms_OnCfgOTDRRef;
	// ptcb->pf_OnSpAnyGetOTDRTest     = tms_OnSpAnyGetOTDRTest;
	// ptcb->pf_OnSpSendSMS            = tms_OnSpSendSMS;
	// ptcb->pf_OnSpAck                = tms_OnSpAck;
	// ptcb->pf_OnRetDevType           = tms_OnRetDevType;
	// ptcb->pf_OnRetVersion           = tms_OnRetVersion;
	// ptcb->pf_OnRetAnyOP             = tms_OnRetAnyOP;
	// ptcb->pf_OnGetSerialNumber      = tms_OnGetSerialNumber;
	// ptcb->pf_OnInsertTbRoute		= tms_OnInsertTbRoute;
#ifdef _MANAGE

	// 重定向tms_Analysexxx函数处理方式，如果不执行任何tms_SetDoWhat，则默认表示
	// 协议处于MCU工作方式，回调函数的dowhat设置为0表示不做任何转发，收到的数据一律
	// 传递给应用层
	// 1.作为网管和板卡来说都是传递给应用层
	// 2.作为CU和MCU就要仔细修改dowhat的处理方式
	int cmd_0xx000xxxx[100];

	bzero(cmd_0xx000xxxx, sizeof(cmd_0xx000xxxx) / sizeof(int));
	tms_SetDoWhat(0x10000000, sizeof(cmd_0xx000xxxx) / sizeof(int), cmd_0xx000xxxx);
	tms_SetDoWhat(0x60000000, sizeof(cmd_0xx000xxxx) / sizeof(int), cmd_0xx000xxxx);
	tms_SetDoWhat(0x80000000, sizeof(cmd_0xx000xxxx) / sizeof(int), cmd_0xx000xxxx);
#endif

}



#ifdef __cplusplus
}
#endif
