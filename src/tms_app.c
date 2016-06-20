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


void tms_Callback(struct tms_callback *ptcb)
{
	bzero(ptcb, sizeof(struct tms_callback));

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
