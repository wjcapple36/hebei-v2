/*
 **************************************************************************************
 *                        桂林光通电子工程公司
 *
 *  文件描述： 变量与函数声明
 *
 *
 *  文件名  ： prototypes.h
 *  创建者  ： 彭怀敏
 *  创建日期： 2010-11-25  15:30:34
 *  当前版本： v1.0
 * 
 ***** 修改记录 *****
 *  修改者  ： 
 *  修改日期： 
 *  备    注： 
 **************************************************************************************
*/
#ifndef _PROTOTYPE_H__
#define _PROTOTYPE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"


/*
 **************************************************************************************
 *   调试宏，只在调试状态下存在，记录时间与事件
 *   str        : 要在LOG对象里显示的字符
 *   ElapseTime : 整型变量，用来记录CLK_gethtime()
 **************************************************************************************
*/
// _DEBUG_LOG_TIME		记录时间的调试宏
#ifdef	_DEBUG_LOG_TIME

	#define	LogType	uint32_t			// 使用LogType来统一定义变量
	
	// DEBUG_SET_TIME 是将当前高分辨率时钟记入整型变量ElapseTime
	#define DEBUG_SET_TIME(Time)			Time = CLK_gethtime()
	
	// DEBUG_END_TIME 是将当前时间间隔写入整型变量ElapseTime并输出到 LogObj_Time
	// 这里使用了预处理的技巧：
	// #  是将str变为字符串
	// ## 是将两个字符连接在一起
//	#define DEBUG_END_TIME(Time, str)		LOG_printf(&LogObj_Time, str##" : %fms", (float)(CLK_gethtime() - Time)/56250)  // 450MHz
//	#define DEBUG_END_TIME(Time, str)		LOG_printf(&LogObj_Time, str##" : %fms", (float)(CLK_gethtime() - Time)/37500)  // 300MHz
	#define DEBUG_END_TIME(Time, str)		LOG_printf(&LogObj_Time, str##" : %fms", (float)(CLK_gethtime() - Time)/75000)  // 600MHz
#else
	#define	LogType	uint32_t
	#define	DEBUG_SET_TIME(Time)
	#define DEBUG_END_TIME(Time, str)
#endif	/* _DEBUG_LOG_TIME */

// _DEBUG_LOG_EVENT		记录事件与时间的调试宏
#ifdef	_DEBUG_LOG_EVENT
	// DEBUG_RST_EVENT 是复位当前记录事件的LOG对象
	#define DEBUG_RST_EVENT(str)		LOG_reset(&LogObj_Event)
	// DEBUG_ADD_EVENT 是将当前低分辨率时钟和字符串str记入LOG对象
	#define DEBUG_ADD_EVENT(str)		LOG_printf(&LogObj_Event, "%d : "##str, CLK_getltime())
#else
	#define DEBUG_RST_EVENT(str)
	#define	DEBUG_ADD_EVENT(str)
#endif	/* _DEBUG_LOG_EVENT */

// _DEBUG_LOG_RECORD		记录事件的调试宏
#ifdef	_DEBUG_LOG_RECORD
	// DEBUG_LOG_RST 是复位当前记录事件的LOG对象
	#define DEBUG_LOG_RST(str)		LOG_reset(&LogObj_Event)
	// DEBUG_LOG_PRINT 是将事件与数据str记入LOG对象
	#define DEBUG_LOG_PRINT(str, d)	LOG_printf(&LogObj_Event, str, d)
#else
	#define DEBUG_LOG_RST(str)
	#define	DEBUG_LOG_PRINT(str, d)
#endif	/* _DEBUG_LOG_RECORD */

/*
 **************************************************************************************
 *   外部变量声明	2010-12-22 15:37:36
 **************************************************************************************
*/
// BuildTime.c
extern int bt_year;
extern int bt_mon;
extern int bt_day;
extern int bt_hour;
extern int bt_min;
extern int bt_sec;

// OTDR.c
extern OTDR_ChannelData_t		OtdrData;       // 通道数据变量
extern OTDR_UploadAllData_t     MeasureResult;  // 测量结果变量
extern GroupEvent_t      GroupEvent;

extern OtdrCtrlVariable_t       OtdrCtrl;        // 控制变量
extern OtdrStateVariable_t      OtdrState;       // 状态变量

extern OtdrParam_t      OtdrParam;
extern uint16_t           OtdrLaserMap1550[6];

// Tsk_Stack.c
extern char LocalIPAddr[16];
extern char LocalIPMask[16];
extern char GatewayIP[16];

extern OTDR_MeasureParam_t      NetWorkMeasureParam;    // 网络控制变量

// OtdrTable.c
extern const uint32_t OtdrMeasureLength[MEASURE_LENGTH_NUM];
extern const float  OtdrPulsePeriod[MEASURE_LENGTH_NUM];
extern const uint32_t OtdrSampleRate[MEASURE_LENGTH_NUM];
extern const uint32_t OtdrSampleRateHz[MEASURE_LENGTH_NUM];
extern const uint16_t OtdrMinAllowPulseWidth[MEASURE_LENGTH_NUM];
extern const uint16_t OtdrMaxAllowPulseWidth[MEASURE_LENGTH_NUM];
extern const uint16_t OtdrMinPulseWidth[MEASURE_LENGTH_NUM];
extern const uint16_t OtdrMaxPulseWidth[MEASURE_LENGTH_NUM];
extern const uint16_t OtdrDefaultPulseWidth[MEASURE_LENGTH_NUM];
extern const uint16_t OtdrDefaultPulseWidth_NMF[MEASURE_LENGTH_NUM];
extern const uint16_t OtdrDefaultPulseWidth_AUTO[MEASURE_LENGTH_NUM][6];
extern const uint16_t OtdrLambdaIndex[LAMBDA_NUM];
extern const uint16_t OtdrDefaultLaserMap1550[6];
extern const uint32_t OtdrBlindZone[2][6];
extern const float  OtdrEventBlindZone[PULSE_WIDTH_NUM];
extern const uint16_t OtdrStartPoint[MEASURE_LENGTH_NUM];
extern const uint16_t	OtdrPowerLevel1310[MEASURE_LENGTH_NUM][PULSE_WIDTH_NUM];
extern const uint16_t	OtdrPowerLevel1550[MEASURE_LENGTH_NUM][PULSE_WIDTH_NUM];
extern const uint16_t OtdrPowerLevel1625[MEASURE_LENGTH_NUM][PULSE_WIDTH_NUM];
extern const uint16_t OtdrPowerLevel1490[MEASURE_LENGTH_NUM][PULSE_WIDTH_NUM];
extern const uint16_t OtdrPowerLevel1610[MEASURE_LENGTH_NUM][PULSE_WIDTH_NUM];
extern const uint16_t OtdrPowerLevel1590[MEASURE_LENGTH_NUM][PULSE_WIDTH_NUM];
extern const uint32_t OtdrEventAlgoTimeTick[9][MAX_GROUP_NUM-1];
extern const uint32_t OtdrReceiver[MEASURE_LENGTH_NUM][PULSE_WIDTH_NUM];

/*
 **************************************************************************************
 *   外部函数声明	2010-12-22 15:37:39
 **************************************************************************************
*/
// OtdrEdma.c
extern void EDMA_Chan_ClrData(void);
extern uint32_t CheckDataAvailable(void);
extern int32_t EDMA_Chan_GetData(void);
extern void EDMA_Chan_CopyData(void);

// Otdr.c
extern int32_t SaveOtdrFile(char *filename, void *buf, uint32_t len);
extern int32_t SaveOtdrParam(void);
extern void OtdrDataInit(void);
extern void AdaptSampleFreq_PulsePeriod(void);
extern void AdaptMeasureLength_PulseWidth(uint32_t FiberLen);
extern void ModifyMeasureLength_PulseWidth(void);
extern void GetPowerLevel(uint32_t index);
extern void GetReceiver(uint32_t index);
extern void DecideReceiver(void);
extern void InitApdV(void);
extern void GetReceiverAndPowerLevel(uint32_t index);
extern void OtdrUpdateParam(void);
extern void OtdrStateInit(void);
extern int32_t CheckIfCurveConnect(void);
extern int32_t CheckIfStartEventAlgo(void);
extern int32_t CheckIfFindSmallEvent(void);
extern int32_t CheckRunParamValid(uint32_t MeasureLength_m, uint32_t PulseWidth_ns);
extern void OtdrUpdateRecorrectParam(void);
extern uint32_t GetNfirWidth(uint32_t PowerMode);
extern void PerformOtdrEventAlgorithm(void);
extern void ProcessRefreshData(uint32_t RefreshCount);
extern void ProcessFinalData  (uint32_t AlgoPurpose);
extern float EstimateFiberLen(void);
extern void EstimateCurveConnect(void);

// OtdrAlgo.c
extern void RemoveBaseLine(int32_t input[], int32_t DataLen, int32_t NoiseLen);
extern void DeleteOsc(int32_t input[], int32_t DataLen, int32_t sat);
extern void EnlargeData(int32_t input[], int32_t DataLen, int32_t ratio);
extern void AdjustCurve(int32_t input[], int32_t DataLen);
extern void InterleaveAverage(int32_t input[], int32_t DataLen, int32_t space);
extern int32_t NoiseMean(const int32_t input[], int32_t DataLen, int32_t NoiseLen);
extern int32_t RootMeanSquare(const int32_t input[], int32_t DataLen, int32_t NoiseLen);
extern void AbsTooLowData(int32_t input[], int32_t DataLen, int32_t sigma);
extern int32_t PulseWidthInSampleNum(void);
extern int32_t GetSaturateThreshold(const int32_t input[]);
extern uint32_t GetMeasureLengthIndex(uint32_t MeasureLength_m);
extern uint32_t GetPulseWidthIndex(uint32_t PulseWidth_ns);
extern uint32_t GetMeasureTimeIndex(uint32_t MeasureTime_ms);
extern uint32_t GetLambdaIndex(uint32_t Lambda);
extern OtdrPowerLevel_t GetPowerLevelIndex(uint32_t Lambda);
extern uint32_t GetPulseWidthSaturateBlindZone(uint32_t Lambda_nm, uint32_t PulseWidth_ns);
extern uint32_t GetPulseWidthLowPowerCount(uint32_t Lambda_nm, uint32_t PulseWidth_ns);
extern uint32_t GetPulseWidthEventBlindZone(uint32_t PulseWidth_ns);
extern int32_t CapacityLinearCompensate(int32_t input[], int32_t DataLen, int32_t FilterLen, int32_t sigma);
extern void RaiseLowPowerCurve(int32_t input[], int32_t sigma);
extern void CombineCurve(int32_t Hsigma);
extern void InsertNonReflexEvent(int32_t input[], int32_t where, float insertloss);
extern void InsertNormalReflexEvent(int32_t input[], int32_t where, float insertloss, float high);
extern void InsertSaturateEvent(int32_t input[], int32_t where, float insertloss, int32_t duration);
extern uint32_t FpgaEchoTest(void);
extern void FpgaStart(uint32_t powerlevel, uint32_t rcv, uint32_t);

// EventParam.c
extern void AnalyseMeasureResult(const float *Ai, Event_t* EventTemp, float DataTemp[], int32_t hasEvent);
extern void AnalyseMeasureResultEvent(const float *Ai, Event_t* EventTemp, float DataTemp[], int32_t EventGroupIndex);
extern int32_t CheckStartPoint(const int32_t *An, Event_t* EventTemp);
extern void PutRawData(int32_t data[]);
extern void PutDeltaThreshold(float t1, float t2, int32_t sigma, int32_t Delta[], int32_t Rn[]);
extern void PutLowPowerRawData(int32_t data[]);

// Filter.c
extern void nfir(int32_t *const input, int32_t *const output, int32_t DataLen, int32_t FilterLen);
extern void NonLinearFilterNFir(int32_t *const input, int32_t *const output, int32_t DataLen, int32_t m);
extern void GetIIRFilterThreshold(int32_t *K, int32_t *NoiseWidth);
extern void IIR_AdaptiveFilter(int32_t input[], int32_t output[], int32_t DataLen, int32_t sigmaIn, int32_t Threshold, int32_t NoiseWidth);
extern void IIR_AdaptiveFilter_Pretty(int32_t input[], int32_t output[], int32_t DataLen, int32_t sigmaIn, int32_t Threshold, int32_t NoiseWidth);
extern void FilterPulse(int32_t input[], int32_t output[], int32_t DataLen);

// Event.c
extern int32_t AllocEventMem(Event_t* const EventTemp);
extern void FreeEventMem(Event_t* const EventTemp);
extern int32_t FindSignalEndPoint(const int32_t input[], int32_t DataLen);
extern void GetLaserParam(int32_t Lambda_nm, float *AttenCoef, float *BackScatterCoeff);
extern float GetMinAttenCoef(int32_t Lambda_nm);
extern void GetEventFindingParamBeforeLog(float *t1, float *t2, float *k, int32_t *M, const char type[], int32_t PowerMode);
extern int32_t FindLargeEventsBeforeLog(OTDR_ChannelData_t *OtdrData, int32_t DataLen, Event_t* EventTemp, OtdrStateVariable_t *State);
extern int32_t FindSmallEventsBeforeLog(OTDR_ChannelData_t *OtdrData, int32_t DataLen, Event_t* EventTemp, OtdrStateVariable_t *State);
extern void GetEventFindingParamAfterLog(float *a, float *b, float *k, int32_t *M, const char type[]);
extern int32_t FindLargeEventsAfterLog(OTDR_ChannelData_t *OtdrData, int32_t DataLen, Event_t* EventTemp, OtdrStateVariable_t *State);
extern int32_t FindSmallEventsAfterLog(OTDR_ChannelData_t *OtdrData, int32_t DataLen, Event_t* EventTemp, OtdrStateVariable_t *State);
extern int32_t FindProbableEndEvent(OTDR_ChannelData_t *OtdrData, Event_t *EventTemp, OtdrStateVariable_t *State);
extern int32_t EventPointsFilter(OTDR_ChannelData_t *OtdrData, Event_t *EventTemp, const char type[], OtdrStateVariable_t *State);
extern int32_t MergeFinalEvents(Event_t *EventTempLarge, Event_t *EventTempSmall);
extern int32_t MergeGroupEvents(Event_t *EventTemp, GroupEvent_t *GroupEvent);
extern int32_t SplitFinalEvents(float *AiLog, int32_t DataLen, Event_t *EventTemp);
extern int32_t FindSaturateEvents(const int32_t input[], Event_t* EventTemp, int32_t SatThreshold);
extern int32_t GetCurveSaturatePoint(const OTDR_ChannelData_t *OtdrData, int32_t DataLen);
extern void FindEndEventPoint(OTDR_ChannelData_t *OtdrData, Event_t* EventTemp, OtdrStateVariable_t *State);
extern void RemoveTailNonReflexEventPoint(OTDR_ChannelData_t *OtdrData, Event_t* EventTemp);
extern void RemoveReflectShadowEventPoint(const OTDR_ChannelData_t *OtdrData, Event_t* EventTemp);
extern void RemoveLowLevelEventPoint(const OTDR_ChannelData_t *OtdrData, Event_t* EventTemp);
extern void RemoveConnectPointEventPoint(const OTDR_ChannelData_t *OtdrData, Event_t* EventTemp);
extern void RemoveShortDistanceEventPoint(const OTDR_ChannelData_t *OtdrData, Event_t* EventTemp);
extern int32_t FastEstimateFiber(OTDR_ChannelData_t *OtdrData, int32_t DataLen, int32_t sigma);

// TskFile_Socket.c
extern void PutDataFrame(char *buf, uint32_t len);

// Utility.c
extern void LeastSquareMethod(const float di[], int from, int to, float *k, float *b);
extern float FastLog10(float x);
extern float FastLog10_Addr(float *xa);
extern void FastLog10Vector(const int32_t input[], float output[], int32_t DataLen, int32_t sigma, float minLevel);
extern void FastLog10Vector2(const OTDR_ChannelData_t *data, float output[], int32_t DataLen, OtdrStateVariable_t *state);
extern double dFastLog10(double x);
extern int StringSearch(const char *text, int TextSize, const char *pattern);
extern int num2str(char *str, unsigned int num, int isitHex);
extern unsigned int str2num(char *str, int *num, int isitHex);
extern void SetFlashPage(int page);

extern void MaxValue (const void *input, int from, int to, void *MV, int *pos, int type);
extern void MinValue (const void *input, int from, int to, void *MV, int *pos, int type);
extern void MeanValue(const void *input, int from, int to, void *MV, int type);
extern int TurningPoint(const int input[], int from, int to, char *direction);
extern int GetCount(const int input[], int from, int to, int dst, int tol);

extern int32_t qsort_cmp(const void *a , const void *b);
extern float HostFloat2NetworkFloat(float f);
extern float NetworkFloat2HostFloat(float f);
extern uint32_t DspCodeCheckSum(const uint8_t *pdata, uint32_t length);
extern int32_t KickOutOfArray(int32_t *BaseArray, int32_t BadEgg, int32_t *Array1, int32_t *Array2, int32_t ArrayLen);
extern int32_t qsort_partitions(int32_t buf[], int32_t low, int32_t high);
extern int32_t kth_smallest(int32_t buf[], int n, int k);

// TskFile_TcpDebug.c
#if ENABLE_TCPDEBUG     // 如果使能TCP调试，则声明并定义宏
    extern int TcpDebugPrintf(const char *format, ...);
	extern void TcpDebugPrintMem(int32_t segid);

    #define TCPDEBUG_PRINT                              printf
#else       // 否则定义宏为空
    #define TCPDEBUG_PRINT	printf
#endif

// TskFile_TcpConsole.c
extern int mConPrintf(char *format, ...);


#ifdef __cplusplus
}
#endif

#endif	// _PROTOTYPE_H__

/*
 ********************************************************************
 *    End    of    File
 ********************************************************************
*/
