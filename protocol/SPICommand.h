// SPICommand.h: interface for the CSPICommand class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _SPICOMMAND_H_
#define _SPICOMMAND_H_
#include "protocol/struct.h"


#ifdef __cplusplus
extern "C" {
#endif

#define FPGA_COMMAND_FRAME_BYTE_NUM  7

int GetDistanceNumber(int iRange);
int GetPulseWidthNumber(int iPulseWidth);
int GetLowPowerValue(struct _OTDRPipeTestParam sOTDRPipeTestParam);
int GetPowerValue(struct _OTDRPipeTestParam sOTDRPipeTestParam, bool bLowPower);
int GetLowReceiverResistance(struct _OTDRPipeTestParam sOTDRPipeTestParam);
int GetHighReceiverResistance(struct _OTDRPipeTestParam sOTDRPipeTestParam);
int GetReceiverResistance(struct _OTDRPipeTestParam sOTDRPipeTestParam, bool bLowPower);
int GetAPDVoltage(struct _OTDRPipeTestParam sOTDRPipeTestParam);
unsigned long CmdSPITest(unsigned char *pCmdSPI, struct _OTDRPipeTestParam sOTDRPipeTestParam, bool bLowPower);
unsigned long CmdSPIRequestPipeData(unsigned char *pCmdSPI, int iPipeNumber, struct _OTDRPipeTestParam sOTDRPipeTestParam, bool bLowPower);
unsigned long CmdSPIAlarmAppear(unsigned char *pCmdSPI, int iPipeNumber);
unsigned long CmdSPIAlarmDisappear(unsigned char *pCmdSPI, int iPipeNumber);
unsigned long CmdSPIRestart(unsigned char *pCmdSPI);

unsigned long CmdSPIGetPipeNum(unsigned char *pCmdSPI);
unsigned long CmdSPIGetUnitNumber(unsigned char *pCmdSPI);
unsigned long CmdSPIGetNetSegmentFlag(unsigned char *pCmdSPI);



// #if 0
// };
// #endif

#ifdef __cplusplus
}
#endif


#endif
