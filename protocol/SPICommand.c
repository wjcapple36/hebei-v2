// SPICommand.cpp: implementation of the CSPICommand class.
//
//////////////////////////////////////////////////////////////////////

// #include "stdafx.h"
// #include "SPICommand.h"

// #ifdef _DEBUG
// 	#undef THIS_FILE
// 	static char THIS_FILE[] = __FILE__;
// 	#define new DEBUG_NEW
// #endif



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#include "struct.h"
#include "protocol/SPICommand.h"

#ifdef __cplusplus
extern "C" {
#endif

// #include "tmsxx.h"
#if 0
CSPICommand()
{

}

~CSPICommand()
{

}
#endif
int GetDistanceNumber(int iRange)
{
	int iDistanceNumber = 0;
	switch(iRange) {
	case 1000: {
		iDistanceNumber = 0;
		break;
	}
	case 5000: {
		iDistanceNumber = 1;
		break;
	}
	case 10000: {
		iDistanceNumber = 2;
		break;
	}
	case 30000: {
		iDistanceNumber = 3;
		break;
	}
	case 60000: {
		iDistanceNumber = 4;
		break;
	}
	case 100000: {
		iDistanceNumber = 5;
		break;
	}
	case 180000: {
		iDistanceNumber = 6;
		break;
	}
	default: {
		// hb2_dbg(_T("不存在该量程!\n"));
		break;
	}
	}
	return iDistanceNumber;
}

int GetPulseWidthNumber(int iPulseWidth)
{
	int iPulseWidthNumber = 0;
	switch(iPulseWidth) {
	case 5: {
		iPulseWidthNumber = 0;
		break;
	}
	case 10: {
		iPulseWidthNumber = 1;
		break;
	}
	case 20: {
		iPulseWidthNumber = 2;
		break;
	}
	case 40: {
		iPulseWidthNumber = 3;
		break;
	}
	case 80: {
		iPulseWidthNumber = 4;
		break;
	}
	case 160: {
		iPulseWidthNumber = 5;
		break;
	}
	case 320: {
		iPulseWidthNumber = 6;
		break;
	}
	case 640: {
		iPulseWidthNumber = 7;
		break;
	}
	case 1280: {
		iPulseWidthNumber = 8;
		break;
	}
	case 2560: {
		iPulseWidthNumber = 9;
		break;
	}
	case 5000: {
		iPulseWidthNumber = 10;
		break;
	}
	case 10000: {
		iPulseWidthNumber = 11;
		break;
	}
	case 20000: {
		iPulseWidthNumber = 12;
		break;
	}
	case 40000: {
		iPulseWidthNumber = 13;
		break;
	}
	default: {
		// hb2_dbg(_T("不存在该脉宽!\n"));
		break;
	}
	}
	return iPulseWidthNumber;
}

int GetLowPowerValue(struct _OTDRPipeTestParam sOTDRPipeTestParam)
{
	int iLowPowerValue = 0;
	switch(sOTDRPipeTestParam.iPulseWidth) {
	case 640: {
		iLowPowerValue = 4;
		break;
	}
	case 1280: {
		iLowPowerValue = 3;
		break;
	}
	case 2560: {
		iLowPowerValue = 3;
		break;
	}
	case 5000: {
		iLowPowerValue = 2;
		break;
	}
	case 10000: {
		if (sOTDRPipeTestParam.iWaveLength == 1310) {
			iLowPowerValue = 1;
		}
		else {
			iLowPowerValue = 2;
		}
		break;
	}
	case 20000: {
		if (sOTDRPipeTestParam.iWaveLength == 1310) {
			iLowPowerValue = 1;
		}
		else {
			iLowPowerValue = 2;
		}
		break;
	}
	case 40000: {
		if (sOTDRPipeTestParam.iWaveLength == 1310) {
			iLowPowerValue = 1;
		}
		else {
			iLowPowerValue = 2;
		}
		break;
	}
	}
	return iLowPowerValue;
}

int GetPowerValue(struct _OTDRPipeTestParam sOTDRPipeTestParam, bool bLowPower)
{
	int iPowerValue = 0;
	if (sOTDRPipeTestParam.iRange <= 60000) {
		iPowerValue = 4;
	}
	else {
		if (!bLowPower) {
			iPowerValue = 4;
		}
		else {
			iPowerValue = GetLowPowerValue(sOTDRPipeTestParam);
		}
	}
	return iPowerValue;
}

int GetHighReceiverResistance(struct _OTDRPipeTestParam sOTDRPipeTestParam)
{
	int iHighReceiverResistance = 0;
	switch(sOTDRPipeTestParam.iPulseWidth) {
	case 640: {
		iHighReceiverResistance = 12;
		break;
	}
	case 1280: {
		iHighReceiverResistance = 12;
		break;
	}
	case 2560: {
		iHighReceiverResistance = 12;
		break;
	}
	case 5000: {
		iHighReceiverResistance = 13;
		break;
	}
	case 10000: {
		iHighReceiverResistance = 13;
		break;
	}
	case 20000: {
		iHighReceiverResistance = 13;
		break;
	}
	case 40000: {
		iHighReceiverResistance = 13;
		break;
	}
	}
	return iHighReceiverResistance;
}

int GetLowReceiverResistance(struct _OTDRPipeTestParam sOTDRPipeTestParam)
{
	int iLowReceiverResistance = 0;
	switch(sOTDRPipeTestParam.iPulseWidth) {
	case 640: {
		iLowReceiverResistance = 6;
		break;
	}
	case 1280: {
		iLowReceiverResistance = 6;
		break;
	}
	case 2560: {
		iLowReceiverResistance = 6;
		break;
	}
	case 5000: {
		iLowReceiverResistance = 6;
		break;
	}
	case 10000: {
		iLowReceiverResistance = 6;
		break;
	}
	case 20000: {
		iLowReceiverResistance = 6;
		break;
	}
	case 40000: {
		iLowReceiverResistance = 6;
		break;
	}
	}
	return iLowReceiverResistance;
}

int GetReceiverResistance(struct _OTDRPipeTestParam sOTDRPipeTestParam, bool bLowPower)
{
	int iReceiverResistance = 0;
	if (sOTDRPipeTestParam.iPulseWidth <= 40) {
		iReceiverResistance = 7;
	}
	else if (sOTDRPipeTestParam.iPulseWidth <= 320) {
		iReceiverResistance = 10;
	}
	else {
		if (sOTDRPipeTestParam.iRange <= 60000) {
			iReceiverResistance = 10;
		}
		else {
			if (!bLowPower) {
				iReceiverResistance = GetHighReceiverResistance(sOTDRPipeTestParam);
			}
			else {
				iReceiverResistance = GetLowReceiverResistance(sOTDRPipeTestParam);
			}
		}
	}
	return iReceiverResistance;
}

int GetAPDVoltage(struct _OTDRPipeTestParam sOTDRPipeTestParam)
{
	int iAPDVoltage = 0;
	if (sOTDRPipeTestParam.iPulseWidth <= 80) {
		iAPDVoltage = 1;
	}
	return iAPDVoltage;
}

unsigned long CmdSPITest(unsigned char *pCmdSPI, struct _OTDRPipeTestParam sOTDRPipeTestParam, bool bLowPower)
{
	int iDistanceNumber = GetDistanceNumber(sOTDRPipeTestParam.iRange);
	int iPulseWidthNumber = GetPulseWidthNumber(sOTDRPipeTestParam.iPulseWidth);
	int iPowerValue = GetPowerValue(sOTDRPipeTestParam, bLowPower);
	int iReceiverResistance = GetReceiverResistance(sOTDRPipeTestParam, bLowPower);
	int iAPDVoltage = GetAPDVoltage(sOTDRPipeTestParam);

	pCmdSPI[0] = (1 << 4) + sOTDRPipeTestParam.iPipeSerialNum;
	pCmdSPI[1] = (2 << 4) + iPulseWidthNumber;
	pCmdSPI[2] = (3 << 4) + iDistanceNumber;
	pCmdSPI[3] = (4 << 4) + iPowerValue;  //功率值
	pCmdSPI[4] = (5 << 4) + iReceiverResistance; //接收机跨阻
	pCmdSPI[5] = (6 << 4) + iAPDVoltage; //APD电压
	pCmdSPI[6] = (7 << 4) + 3; //测试

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}

unsigned long CmdSPIRequestPipeData(unsigned char *pCmdSPI, int iPipeNumber, struct _OTDRPipeTestParam sOTDRPipeTestParam, bool bLowPower)
{
	int iDistanceNumber = GetDistanceNumber(sOTDRPipeTestParam.iRange);
	int iPulseWidthNumber = GetPulseWidthNumber(sOTDRPipeTestParam.iPulseWidth);
	int iPowerValue = GetPowerValue(sOTDRPipeTestParam, bLowPower);
	int iReceiverResistance = GetReceiverResistance(sOTDRPipeTestParam, bLowPower);
	int iAPDVoltage = GetAPDVoltage(sOTDRPipeTestParam);

	pCmdSPI[0] = (1 << 4) + iPipeNumber;
	pCmdSPI[1] = (2 << 4) + iPulseWidthNumber;
	pCmdSPI[2] = (3 << 4) + iDistanceNumber;
	pCmdSPI[3] = (4 << 4) + iPowerValue;  //功率值
	pCmdSPI[4] = (5 << 4) + iReceiverResistance; //接收机跨阻
	pCmdSPI[5] = (6 << 4) + iAPDVoltage; //APD电压
	pCmdSPI[6] = (7 << 4) + 4;  //请求通路数据

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}

unsigned long CmdSPIAlarmAppear(unsigned char *pCmdSPI, int iPipeNumber)
{
	pCmdSPI[0] = (1 << 4) + iPipeNumber;
	pCmdSPI[1] = (2 << 4) + 15;
	pCmdSPI[2] = (3 << 4) + 15;
	pCmdSPI[3] = (4 << 4) + 15;
	pCmdSPI[4] = (5 << 4) + 15;
	pCmdSPI[5] = (6 << 4) + 15;
	pCmdSPI[6] = (7 << 4) + 1;  //告警产生

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}

unsigned long CmdSPIAlarmDisappear(unsigned char *pCmdSPI, int iPipeNumber)
{
	pCmdSPI[0] = (1 << 4) + iPipeNumber;
	pCmdSPI[1] = (2 << 4) + 15;
	pCmdSPI[2] = (3 << 4) + 15;
	pCmdSPI[3] = (4 << 4) + 15;
	pCmdSPI[4] = (5 << 4) + 15;
	pCmdSPI[5] = (6 << 4) + 15;
	pCmdSPI[6] = (7 << 4) + 2;  //告警消失

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}

unsigned long CmdSPIRestart(unsigned char *pCmdSPI)
{
	pCmdSPI[0] = (1 << 4) + 0; //通道号此时为0，对应1通道
	pCmdSPI[1] = (2 << 4) + 15;
	pCmdSPI[2] = (3 << 4) + 15;
	pCmdSPI[3] = (4 << 4) + 15;
	pCmdSPI[4] = (5 << 4) + 15;
	pCmdSPI[5] = (6 << 4) + 15;
	pCmdSPI[6] = (7 << 4) + 5;  //复位

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}

unsigned long CmdSPIGetPipeNum(unsigned char *pCmdSPI)
{
	pCmdSPI[0] = (1 << 4) + 0; //通道号此时为0，对应1通道
	pCmdSPI[1] = (2 << 4) + 15;
	pCmdSPI[2] = (3 << 4) + 15;
	pCmdSPI[3] = (4 << 4) + 15;
	pCmdSPI[4] = (5 << 4) + 15;
	pCmdSPI[5] = (6 << 4) + 15;
	pCmdSPI[6] = (7 << 4) + 6;  //获取通道数目

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}

unsigned long CmdSPIGetUnitNumber(unsigned char *pCmdSPI)
{
	pCmdSPI[0] = (1 << 4) + 0; //通道号此时为0，对应1通道
	pCmdSPI[1] = (2 << 4) + 15;
	pCmdSPI[2] = (3 << 4) + 15;
	pCmdSPI[3] = (4 << 4) + 15;
	pCmdSPI[4] = (5 << 4) + 15;
	pCmdSPI[5] = (6 << 4) + 15;
	pCmdSPI[6] = (7 << 4) + 7;  //获取槽位号

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}

unsigned long CmdSPIGetNetSegmentFlag(unsigned char *pCmdSPI)
{
	pCmdSPI[0] = (1 << 4) + 0; //通道号此时为0，对应1通道
	pCmdSPI[1] = (2 << 4) + 15;
	pCmdSPI[2] = (3 << 4) + 15;
	pCmdSPI[3] = (4 << 4) + 15;
	pCmdSPI[4] = (5 << 4) + 15;
	pCmdSPI[5] = (6 << 4) + 15;
	pCmdSPI[6] = (7 << 4) + 8;  //获取网段标志

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}

#ifdef __cplusplus
}
#endif