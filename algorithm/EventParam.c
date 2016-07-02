/*
 **************************************************************************************
 *                        桂林光通电子工程公司
 *
 *  文件描述： 计算事件点参数
 *
 *  文件名  ： EventParam.c
 *  创建者  ： 彭怀敏
 *  创建日期： 2011-12-27 17:19:19
 *  当前版本： v1.0
 * 
 ***** 修改记录 *****
 *  修改者  ： 
 *  修改日期： 
 *  备    注： 
 **************************************************************************************
*/
#include <stdio.h>
#include <math.h>

#include "Otdr.h"
//#include "OtdrEdma.h"
#include "prototypes.h"
#include "protocol.h"

#define     FIBER_PARAM_SEND    0   // 是否上传光纤段参数
#define     EVENT_END_SEND      0   // 是否上传事件末点参数
typedef struct
{
    Int32   *EventGroupStart;
    Int32   EventGroupNum;
}EventGroupStart_t;

// 事件后点变量，光纤段参数变量
#if EVENT_END_SEND
OTDR_UploadEventEnd_t       EventEnd;
#endif

#if FIBER_PARAM_SEND
OTDR_UploadFiberParam_t     FiberParam;
#endif

/*
 **************************************************************************************************
 *  函数  名： CalculateFiberSectionAttenCoef
 *  函数描述： 按照事件点序号计算其后的光纤段的衰减系数
 *  入口参数： Ai         : 浮点曲线数据
 *             EventTemp  : 事件点信息
 *             Ei         : 事件点序号
 *  返回参数： Ac         : 该事件点之后的光纤段的平均衰减系数
 *             b          : 该光纤段反向延长到0处的初始高度
 *  日期版本： 2013-01-10  16:45:44  v1.0
 **************************************************************************************************
*/
float CalculateFiberSectionAttenCoef(const float Ai[], Event_t* EventTemp, Int32 Ei, float *b)
{
	Int32	m, EventNum, MinEventDis;
	Int32	*FinalEvents, *FinalEventsEnd;
	float	atten_coeff, Ac, bt = RSVD_FLOAT;

    FinalEvents     = EventTemp->FinalEvent;        // 事件起点
    FinalEventsEnd  = EventTemp->FinalEventEnd;     // 事件末点
    EventNum        = EventTemp->FinalEventNum;
    m               = PulseWidthInSampleNum();
/**************************** 计算各个光纤段的衰减系数 *********************************/
    MinEventDis = (Int32)(OtdrState.Points_1m * 5);   // 5米对应的点数，作为两个事件点的最小距离
    MinEventDis = MIN(MinEventDis, 4*m);                // 2011-12-28 9:54:04
    
    // 根据相邻事件点的距离来判断哪些事件可以计算衰减系数
	if(0 == Ei)
	{
	    if(EventNum > 1)    // 判断下一个事件点是否与它相邻
	    {
    		if((FinalEventsEnd[0] + 2*m + MinEventDis) <= FinalEvents[1])    // 如果距离足够远，则可以计算衰减系数
    		{
    		    LeastSquareMethod(Ai, FinalEventsEnd[0]+m, FinalEvents[1]-m, &atten_coeff, &bt);    // 最小二乘法   2011-8-25 16:38:03
                Ac = -atten_coeff;
    		}
    		else
    		{
    		    Ac = RSVD_FLOAT;
    		}
		}
		else
		{
    		Ac = RSVD_FLOAT;
    	}
	}
	else if(EventNum-1 == Ei)
	{
	    Ac = RSVD_FLOAT;
	}
	else
	{
		if((FinalEventsEnd[Ei] + 2*m + MinEventDis) <= FinalEvents[Ei+1])
		{
			LeastSquareMethod(Ai, FinalEventsEnd[Ei]+m, FinalEvents[Ei+1]-m, &atten_coeff, &bt);    // 最小二乘法   2011-8-25 16:38:03
            Ac = -atten_coeff;
    	}
		else    Ac = RSVD_FLOAT;
	}
	
	if(NULL != b)   *b = bt;
	return Ac;
}

/*
 **************************************************************************************************
 *  函数  名： InitEventGroupStart
 *  函数描述： 根据衰减系数来计算事件组的起始事件索引
 *  入口参数： AttenCoef     : 光纤段的衰减系数
 *             EventNum      : 事件点总数
 *  返回参数： EGS           : 事件组的起始事件序号及事件组的个数
 *             EventGroupNum : 事件组的个数
 *  日期版本： 2013-01-10  17:05:30  v1.0
 **************************************************************************************************
*/
Int32 InitEventGroupStart(const float AttenCoef[], EventGroupStart_t *EGS, Int32 EventNum)
{
    Int32 i, EventGroupNum = 0;
    
    EGS->EventGroupStart[EventGroupNum++] = 0;
    for(i = 1; i < EventNum; i++)
    {
        if(AttenCoef[i-1] != RSVD_FLOAT)    EGS->EventGroupStart[EventGroupNum++] = i;
    }
    EGS->EventGroupNum = EventGroupNum;
    return EventGroupNum;
}

/*
 **************************************************************************************************
 *  函数  名： CalculateEventPoint_IL_RL
 *  函数描述： 根据事件组来计算光纤段的插入损耗和反射损耗
 *  入口参数： Ai        : 浮点曲线数据
 *             EventTemp : 事件点信息
 *             AC        : 光纤段的衰减系数
 *             b         : 光纤段反向延长到0处的初始高度
 *             EGS       : 事件组的起始事件序号及事件组的个数
 *             Gi        : 事件组序号
 *             bsc       : 后向散射系数
 *  返回参数： IL        : 插入损耗
 *             RL        : 反射损耗
 *             insert_loss : 该事件组的插入损耗
 *  日期版本： 2013-01-11  10:59:30  v1.0
 **************************************************************************************************
*/
float CalculateEventPoint_IL_RL(const float Ai[], const Event_t* EventTemp, const float AC[], const float b[], 
                                const EventGroupStart_t *EGS, Int32 Gi, float bsc, float IL[], float RL[])
{
	Int32	i, gs1, gs2, ge0, ge1;
	Int32	*FinalEvents, *FinalEventsEnd, *EventGroupStart;
	float	Lv, Rv, Am, A0, insert_loss;
	
	FinalEvents     = EventTemp->FinalEvent;        // 事件起点
    FinalEventsEnd  = EventTemp->FinalEventEnd;     // 事件末点
    EventGroupStart = EGS->EventGroupStart;         // 事件组起点

    if(0 == Gi)     /******* 起始事件组，只计算反射损耗 *******/
    {
        if(EGS->EventGroupNum > 1)
        {
		    ge1 = EventGroupStart[1]-1;
    	    MaxValue(Ai, FinalEvents[ge1], FinalEventsEnd[ge1], &Am, &i, DATA_TYPE_FLOAT);
    	    A0 = b[ge1] - AC[ge1] * i;   // 用直线拟合出来的事件左值
    		RL[ge1] = -(bsc + 2*(Am - A0));    // 反射损耗赋予事件组里的最后一个事件
		}
		insert_loss = RSVD_FLOAT;   // 没有插损
    }
    else if(EGS->EventGroupNum-1 == Gi)    /******* 结束事件组，只计算反射损耗 *******/
    {
    	gs1 = EventGroupStart[Gi];
    	ge0 = gs1-1;
		MaxValue(Ai, FinalEvents[gs1], FinalEventsEnd[gs1], &Am, &i, DATA_TYPE_FLOAT);
        A0 = b[ge0] - AC[ge0] * i;   // 用直线拟合出来的事件左值
		RL[gs1] = -(bsc + 2*(Am - A0));        // 反射损耗赋予事件组里的第一个事件
		insert_loss = RSVD_FLOAT;   // 没有插损
    }
    else            /******* 中间事件点，计算插入损耗、反射损耗 *******/
   	{
        gs1 = EventGroupStart[Gi];
		gs2 = EventGroupStart[Gi+1];
		ge0 = gs1-1;
		ge1 = gs2-1;
		
		/******* 计算反射损耗 *******/
		MaxValue(Ai, FinalEvents[gs1], FinalEventsEnd[gs1], &Am, &i, DATA_TYPE_FLOAT);
        A0 = b[ge0] - AC[ge0] * i;   // 用直线拟合出来的事件左值
		RL[gs1] = -(bsc + 2*(Am - A0));    // 反射损耗赋予事件组里的第一个事件
		
		// 如果事件组里的事件多于1个，则最后一个事件也可以拥有反射损耗
		if(gs1 < ge1)
		{
		    MaxValue(Ai, FinalEvents[ge1], FinalEventsEnd[ge1], &Am, &i, DATA_TYPE_FLOAT);
    	    A0 = b[ge1] - AC[ge1] * i;   // 用直线拟合出来的事件左值
    		RL[ge1] = -(bsc + 2*(Am - A0));    // 反射损耗赋予事件组里的第一个事件
		}

		/******* 计算插入损耗 *******/
		Lv = b[ge0] - AC[ge0] * FinalEvents[gs1];   // 用直线拟合出来的事件左值
	    Rv = b[ge1] - AC[ge1] * FinalEvents[gs1];   // 用直线拟合出来的事件右值，和左值在同一个位置
        insert_loss = Lv - Rv;
        IL[gs1]  = insert_loss;             // 插入损耗赋予事件组里的第一个事件
    }
    return insert_loss;
}

/*
 **************************************************************************************************
 *  函数  名： CalculateFSL
 *  函数描述： 根据事件组来计算光纤段的损耗
 *  入口参数： EventTemp : 事件点信息
 *             AC        : 光纤段的衰减系数
 *             EGS       : 事件组的起始事件序号及事件组的个数
 *  返回参数： FSL       : 光纤段损耗
 *             TotalLoss : 光纤段引起的总损耗
 *  日期版本： 2013-1-18 9:25:22  v1.0
 **************************************************************************************************
*/
float CalculateFSL(const Event_t* EventTemp, const float AC[], 
                   const EventGroupStart_t *EGS, float FSL[])
{
	Int32	i, gs0, gs1;
	Int32	*FinalEvents, *EventGroupStart;
	float	TotalLoss = 0;

	FinalEvents     = EventTemp->FinalEvent;        // 事件起点
    EventGroupStart = EGS->EventGroupStart;         // 事件组起点
    
    /******* 初始化各个光纤段的损耗 *******/
    for(i = 0; i < EventTemp->FinalEventNum-1; i++)     FSL[i] = RSVD_FLOAT;
    
    /******* 计算各个光纤段的损耗 *******/
    for(i = 1; i < EGS->EventGroupNum; i++)
    {
        gs1 = EventGroupStart[i];
        gs0 = EventGroupStart[i-1];
        FSL[gs1-1] = AC[gs1-1] * (FinalEvents[gs1]-FinalEvents[gs0]);
        TotalLoss += FSL[gs1-1];
    }
    return TotalLoss;
}

/*
 **************************************************************************************************
 *  函数  名： CalculateEventPointTL
 *  函数描述： 根据事件组来计算事件点的累计损耗及光纤段的损耗
 *  入口参数： EventTemp : 事件点信息
 *             AC        : 光纤段的衰减系数
 *             IL        : 插入损耗
 *             EGS       : 事件组的起始事件序号及事件组的个数
 *  返回参数： FSL       : 光纤段损耗
 *             TL        : 累计损耗
 *             TotalLoss : 光纤链的总累计损耗
 *  日期版本： 2013-01-11  11:44:33  v1.0
 **************************************************************************************************
*/
float CalculateEventPointTL(const Event_t* EventTemp, const float AC[], const float IL[], 
                            const EventGroupStart_t *EGS, float FSL[], float TL[])
{
	Int32	i, gs, *EventGroupStart = EGS->EventGroupStart;         // 事件组起点
	float	TotalLoss;

    /******* 初始化各个事件点的累计损耗，并计算各个光纤段的损耗 *******/
    for(i = 0; i < EventTemp->FinalEventNum; i++)     TL[i] = RSVD_FLOAT;
    CalculateFSL(EventTemp, AC, EGS, FSL);
    
    /******* 计算各个事件点的累计损耗 *******/
	TotalLoss = 0;
	TL[0]     = 0;//RSVD_FLOAT;
    for(i = 1; i < EGS->EventGroupNum; i++)
    {
        gs = EventGroupStart[i];
        TotalLoss += FSL[gs-1];
        
    #if 1    // 如果该事件点存在插入损耗，则计入累计损耗
        if(IL[gs] != RSVD_FLOAT)    TotalLoss += IL[gs];
    	TL[gs] = TotalLoss;
    #else    // 即使该事件点存在插入损耗，并不计入累计损耗，而是计算下一个事件点的累计损耗
        TL[gs] = TotalLoss;
        if(IL[gs] != RSVD_FLOAT)    TotalLoss += IL[gs];
    #endif
    }

    return TotalLoss;
}

/*
 **************************************************************************************************
 *  函数  名： NormalizeAC_RL
 *  函数描述： 规范化回波损耗和衰减系数
 *  入口参数： State    : 状态变量
 *             EventNum : 事件点数
 *  返回参数： AC       : 衰减系数
 *             RL       : 回波损耗
 *  日期版本： 2013-01-18  10:16:50  v1.0
 **************************************************************************************************
*/
void NormalizeAC_RL(OtdrStateVariable_t *State, float AC[], float RL[], Int32 EventType[], Int32 EventNum)
{
    Int32 i;
    for(i = 0; i < EventNum; i++)
    {
        AC[i] = (AC[i] == RSVD_FLOAT) ? RSVD_FLOAT : (AC[i]*State->Points_1m * 1000);
        
        if(((i < EventNum-1) && (EventType[i] == EVENT_TYPE_NONREFLECT)) || 
    	   ((i == EventNum-1) && !State->EndEventIsReflect))
        {
            RL[i] = RSVD_FLOAT;
        }
    }
}

/*
 **************************************************************************************
 *  函数名称： AnalyseMeasureResult
 *  函数描述： 分析测量结果
 *  入口参数： Ai        : 对数衰减曲线
 *             EventTemp : 携带事件点信息的中间变量
 *             DataTemp  : 临时缓存，必须保证有 4*MAX_EVENT_NUM 个 float 的空间大小
 *             hasEvent  : 是否存在事件点，刷新数据和实时模式的数据不带事件点
 *  返回参数： 
 *  日期版本： 2010-11-02  10:07:40  v1.0	2010-12-10 9:18:54 v2.0	2011-1-6 21:21:56
 **************************************************************************************
*/
#define LITTLE_INSERT_LOSS_LIMIT    0.03

void AnalyseMeasureResult(const float *Ai, Event_t* EventTemp, float DataTemp[], Int32 hasEvent)
{
	Int32	i, j, UploadLen, PacketLength = 0;
	Int32	gs1, gs2, DoesItMerged = 0;
	Int32	EventNum, LastValidGroupIndex, LastValidEventIndex;
	Uint16	*pdB;
	
	Int32	*FinalEvents, *FinalEventsEnd, *EventType, *EventGroupStart, FuckEventNum, FinalEventNum, EventGroupNum;
	float	Distance_m, TotalLoss, insert_loss;
	float	mpp, ppm, BackwardScatterCoeff;
	char	*NextAddr, hasCurve = 1;
	float	*EventReflectLoss, *EventInsertLoss, *AttenCoef, *EventTotalLoss, *FiberSectionLoss, *b0;
	EventGroupStart_t   EGS;
	
	OtdrStateVariable_t     *State     = NULL;  // 测试状态变量
	OTDR_UploadAllData_t    *AllResult = NULL;  // 全部测试数据
	OTDR_UploadAllData_t    *AllEvent  = NULL;  // 全部测试数据 事件点部分  2011-6-28 14:47:20
	OTDR_UploadRefData_t    *RefResult = NULL;  // 刷新数据
	
	State     = (OtdrStateVariable_t *)&OtdrState;
	
	// 把 UploadLen 定义成2的倍数，这是为了对齐
	UploadLen = MIN(OtdrState.MeasureLengthPoint+1, DATA_LEN-NOISE_LEN);
	if(UploadLen & 1)   UploadLen++;

	if(hasEvent)
	{
    	FinalEvents     = EventTemp->FinalEvent;        // 事件起点
    	FinalEventsEnd  = EventTemp->FinalEventEnd;     // 事件末点
    	EventType       = EventTemp->EventType;         // 事件类型
    	EventGroupStart = EventTemp->TotalEvent;        // 事件组起点
    	EventNum        = EventTemp->FinalEventNum;
    	AllResult       = (OTDR_UploadAllData_t*)&MeasureResult;
    	EGS.EventGroupStart = EventGroupStart;
    	
    	EventReflectLoss = (float*)(DataTemp);
    	EventInsertLoss  = (float*)(EventReflectLoss + MAX_EVENT_NUM);
    	AttenCoef        = (float*)(EventInsertLoss  + MAX_EVENT_NUM);
    	EventTotalLoss   = (float*)(AttenCoef        + MAX_EVENT_NUM);
    	b0               = (float*)(EventTotalLoss   + MAX_EVENT_NUM);
    	FiberSectionLoss = (float*)(b0               + MAX_EVENT_NUM);
        
        if(OtdrCtrl.MeasurePurpose == CMD_HOST_MEASURE_FIBER)   hasCurve = 0;
        
#if (MAX_EVENT_NUM * 6 > DATA_LEN)
#error  AnalyseMeasureResult 空间分配不足
#endif
    	
    	for(i = 0; i < MAX_EVENT_NUM; i++)
    	{
    	    EventReflectLoss[i] = RSVD_FLOAT;
    	    EventInsertLoss[i]  = RSVD_FLOAT;
    	    AttenCoef[i]        = RSVD_FLOAT;
    	    EventTotalLoss[i]   = RSVD_FLOAT;
    	    b0[i]               = RSVD_FLOAT;
    	    FiberSectionLoss[i] = RSVD_FLOAT;
    	}
    }
	else
	{
	    RefResult       = (OTDR_UploadRefData_t*)&MeasureResult;
	}

/*************************** fill the OTDR_UploadAllData_t structure ********************************/
	if(!hasEvent)	    RefResult->Cmd = CMD_DSP_UPLOAD_REF_DATA;
	else
	{
	    AllResult->Cmd = (hasCurve ? CMD_DSP_UPLOAD_ALL_DATA : CMD_DSP_UPLOAD_PARAM_DATA);
	#if EVENT_END_SEND
	    EventEnd.Cmd   = CMD_DSP_UPLOAD_EVENT_END;
	#endif
	#if FIBER_PARAM_SEND
	    FiberParam.Cmd = CMD_DSP_UPLOAD_FIBER_PARAM;
	#endif
	}

/************************** fill the MeasureParam substructure ***********************************/
	if(hasEvent)      // 全部测试数据
	{
	    // 获取实际使用的采样率和折射率，并计算系数
	    ppm = State->Points_1m;
    	mpp = 1/ppm;
    	
    	// 后向散射系数
        GetLaserParam(State->MeasureParam.Lambda_nm, NULL, &BackwardScatterCoeff);
        BackwardScatterCoeff += 10 * log10(State->MeasureParam.PulseWidth_ns);      // 2012-3-22 17:37:44
	    
    	AllResult->MeasureParam.SampleRate_Hz   = State->RealSampleRate_Hz;
    	AllResult->MeasureParam.MeasureLength_m = State->MeasureParam.MeasureLength_m;
    	AllResult->MeasureParam.PulseWidth_ns   = State->RcvPw;
    	AllResult->MeasureParam.Lambda_nm       = State->MeasureParam.Lambda_nm;
    	AllResult->MeasureParam.MeasureTime_ms  = State->TotalMeasureTime;
    	AllResult->MeasureParam.n               = State->MeasureParam.n;
//    	AllResult->MeasureParam.FiberLength     = Distance_m;
    	AllResult->MeasureParam.NonRelectThreshold  = State->MeasureParam.NonRelectThreshold;
    	AllResult->MeasureParam.EndThreshold    = State->MeasureParam.EndThreshold;
    	AllResult->MeasureParam.OtdrMode        = OtdrCtrl.OtdrMode;
    	AllResult->MeasureParam.MeasureMode     = OtdrCtrl.MeasureMode;
    	
    	// update PacketLength
    	PacketLength += sizeof(AllResult->MeasureParam);
    }
/**************************** fill the OtdrData substructure *************************************/
	if(!hasEvent)       // 刷新数据
	{
	    RefResult->OtdrData.DataNum = UploadLen;
	    pdB                         = (Uint16*)(RefResult->OtdrData.dB_x1000);

	    // update PacketLength
	    PacketLength               += 4 + UploadLen*sizeof(Uint16);
	}
	else                // 全部测试数据
	{
	    if(hasCurve)
	    {
    	    AllResult->OtdrData.DataNum = UploadLen;
    	    pdB                         = (Uint16*)(AllResult->OtdrData.dB_x1000);
    
    	    // update PacketLength
    	    PacketLength               += 4 + UploadLen*sizeof(Uint16);
    	}
	}
    
    if(hasCurve)
    {
        for(i = 0; i < UploadLen; i++)
    	{
    	    pdB[i]   = (Uint16)(1000 * (Ai[i] - OtdrState.MinNoiseAmp));
    	}
    	NextAddr = (char*)&pdB[i];
    }

/****************************** fill the Event substructure **************************************/
    if(hasEvent)      // 全部测试数据
    {
        // 为不改变数据结构，把 AllEvent 偏移让它的事件点部分指向 NextAddr
    	if(hasCurve)
        {
            AllEvent = (OTDR_UploadAllData_t*)(NextAddr - 8 - sizeof(AllEvent->MeasureParam) - sizeof(AllEvent->OtdrData));
        }
        else
    	{
            AllEvent = (OTDR_UploadAllData_t*)((Uint32)&MeasureResult - sizeof(AllEvent->OtdrData));
        }
    	// update PacketLength
    	PacketLength += 4;
        
GroupEventRestart:
        LastValidGroupIndex = 0;
        LastValidEventIndex = 0;
EventRestart:
        /******* 根据相邻事件点的距离来判断哪些事件可以计算衰减系数 *******/
        EventNum = EventTemp->FinalEventNum;
    	for(i = LastValidEventIndex; (i < EventNum) && (i < MAX_EVENT_NUM); i++)  // 2011-5-7 11:19:14
        {
    		AttenCoef[i] = CalculateFiberSectionAttenCoef(Ai, EventTemp, i, &b0[i]);
    	}
    	
        // 寻找事件点里的组。 数组EventGroupStart保存每个事件组的起始事件的序号
        // 如果某个事件是独立事件而不是事件组，则数组在这里取值连续，否则不连续
        EventGroupNum = InitEventGroupStart(AttenCoef, &EGS, EventNum);
        
        /******* 根据事件组来计算其参数 *******/
    	EventNum = EventTemp->FinalEventNum;
    	FinalEventNum = EventNum;
    	FuckEventNum  = 0;
    	for(i = LastValidGroupIndex; i < EventGroupNum; i++)  // 2012-9-24 15:08:39
        {
            gs1 = EventGroupStart[i];
		    gs2 = EventGroupStart[i+1];
            insert_loss = CalculateEventPoint_IL_RL(Ai, EventTemp, AttenCoef, b0, &EGS, 
                            i, BackwardScatterCoeff, EventInsertLoss, EventReflectLoss);
    
    		// 过滤掉插入损耗小的事件点
            if((EVENT_TYPE_NONREFLECT == EventType[gs1]) && (insert_loss != RSVD_FLOAT))
            {
                float lim;
                
            	if(NR_MODE_MANUAL == OtdrCtrl.NonReflectThreMode)
                {
                        lim = OtdrState.MeasureParam.NonRelectThreshold;
                }
                else    lim = LITTLE_INSERT_LOSS_LIMIT;

                if(fabs(insert_loss) < lim)
                {
                    for(j = gs1; j < gs2; j++)      // 将该事件组一并滤掉
                    {
                        EventType[j]  = EVENT_TYPE_FUCK;
                        FuckEventNum++;
                    }
                }
                
                // 如果存在FUCK事件，则滤除它们
                if(FuckEventNum > 0)
                {
                    FinalEventNum = KickOutOfArray(EventTemp->EventType, EVENT_TYPE_FUCK, 
                                                   EventTemp->FinalEvent, EventTemp->FinalEventEnd, FinalEventNum);
                    EventTemp->FinalEventNum = FinalEventNum;
                    
                    LastValidGroupIndex = i-1;      // 2011-11-2 14:55:26
                    LastValidEventIndex = EventGroupStart[gs1-1];
                    goto EventRestart;
                }
            }
    	}

    	/******* 计算各个事件点的累计损耗及光纤段的损耗 *******/
    	TotalLoss = CalculateEventPointTL(EventTemp, AttenCoef, EventInsertLoss, &EGS, FiberSectionLoss, EventTotalLoss);
    	
    	/******* 规范化回波损耗和衰减系数 *******/
    	NormalizeAC_RL(State, AttenCoef, EventReflectLoss, EventType, EventNum);
        
        /******* 已确定各个事件点的累计损耗，填充数据结构 *******/
        for(i = 0; (i < EventNum) && (i < MAX_EVENT_NUM); i++)
        {
    		AllEvent->Event.EventPoint[i].EventXlabel	   = FinalEvents[i];//FinalEventsEnd[i];//
    		AllEvent->Event.EventPoint[i].EventType		   = EventType[i];
    		AllEvent->Event.EventPoint[i].EventReflectLoss = EventReflectLoss[i];
    		AllEvent->Event.EventPoint[i].EventInsertLoss  = EventInsertLoss[i];
    		AllEvent->Event.EventPoint[i].AttenCoef        = AttenCoef[i];
    		AllEvent->Event.EventPoint[i].EventTotalLoss   = EventTotalLoss[i];
    	}
    	AllEvent->Event.EventNum = i;
        FinalEventNum            = i;
        // update PacketLength
    	PacketLength += FinalEventNum*sizeof(AllEvent->Event.EventPoint[0]);
        
    	/******* 打印事件点信息 *******/
        TCPDEBUG_PRINT("\n******** EVENT LIST ********\n");
        TCPDEBUG_PRINT("Event  Position  InsertLoss  ReflectLoss  AttenCoef  TotalLoss\n");
        for(i = 0; i < FinalEventNum; i++)
        {
            TCPDEBUG_PRINT(" %-4d  %-8d  %-10.3f  %-11.3f  %-9.4f  %-9.3f\n", i, AllEvent->Event.EventPoint[i].EventXlabel, 
                AllEvent->Event.EventPoint[i].EventInsertLoss, AllEvent->Event.EventPoint[i].EventReflectLoss, 
                AllEvent->Event.EventPoint[i].AttenCoef, AllEvent->Event.EventPoint[i].EventTotalLoss);
        }
    #if 1
        // 其实这样做之后，平均衰减系数是起始事件点存在，结束事件点不存在，实际是希望
        // 起始事件点不存在衰减系数，而结束事件点存在，所以必须往后挪一位
        for(j = FinalEventNum-1; j > 0; j--)
        {
            AllEvent->Event.EventPoint[j].AttenCoef = AllEvent->Event.EventPoint[j-1].AttenCoef;
        }
        AllEvent->Event.EventPoint[0].AttenCoef = RSVD_FLOAT;   // 起始事件点，衰减系数定为0
    #endif
    
    #if EVENT_END_SEND
        /******* 填充事件后点 *******/
        EventEnd.EventNum = FinalEventNum;
        for(i = 0; i < FinalEventNum; i++)
        {
            EventEnd.FinalEventsEnd[i] = FinalEventsEnd[i];
        }
        EventEnd.PacketLength = 4+FinalEventNum*sizeof(Uint32);
        EventEnd.FinalEventsEnd[i] = RSVD_VALUE;
        PutDataFrame((char *)&EventEnd, 12+EventEnd.PacketLength);
    #endif
    
    #if FIBER_PARAM_SEND
        /******* 填充光纤段参数 *******/
        FiberParam.FiberNum = FinalEventNum-1;
        for(i = 0; i < FinalEventNum-1; i++)
        {
            FiberParam.Fiber[i].FiberLen         = FinalEvents[i+1] - FinalEvents[i];
            FiberParam.Fiber[i].FiberType        = EVENT_TYPE_FIBER;
            FiberParam.Fiber[i].FiberReflectLoss = RSVD_FLOAT;
            FiberParam.Fiber[i].FiberInsertLoss  = FiberSectionLoss[i];
            FiberParam.Fiber[i].AttenCoef        = AttenCoef[i];
            FiberParam.Fiber[i].FiberTotalLoss   = RSVD_FLOAT;
        }
        FiberParam.PacketLength = 4+FiberParam.FiberNum*sizeof(FiberParam.Fiber[0]);
        *(Uint32*)&(FiberParam.Fiber[i]) = RSVD_VALUE;
        PutDataFrame((char *)&FiberParam, 12+FiberParam.PacketLength);
        
        /******* 打印光纤段信息 *******/
        TCPDEBUG_PRINT("\n******** 光纤段信息 ********\n");
        TCPDEBUG_PRINT("Section  Length(m)  Loss(dB)  AttenCoef\n");
        for(i = 0; i < FinalEventNum-1; i++)
        {
            TCPDEBUG_PRINT(" %2d~%2d   %-9.2f  %-8.3f  %-8.3f\n", i, i+1, 
                            mpp * FiberParam.Fiber[i].FiberLen, FiberSectionLoss[i], AttenCoef[i]);
        }
    #endif
    }

/************************************ 添加最后的保留值及包长 *************************************/
	if(!hasEvent)       // 刷新数据
	{
	    *(Uint32*)NextAddr      = RSVD_VALUE;
	    RefResult->PacketLength = PacketLength;
	}
	else                // 全部测试数据
	{
	    *(Uint32*)&(AllEvent->Event.EventPoint[FinalEventNum]) = RSVD_VALUE;
	    AllResult->PacketLength = PacketLength;
	}
	
	// 更新前面的全局链损耗和链平均衰减系数
	if(hasEvent)
	{
    	AllResult = (OTDR_UploadAllData_t*)&MeasureResult;
    	AllResult->MeasureParam.FiberLoss = (TotalLoss == 0) ? RSVD_FLOAT : TotalLoss;
    	if(TotalLoss == RSVD_FLOAT)     AllResult->MeasureParam.FiberAttenCoef  = RSVD_FLOAT;
    	else
    	{
    	    // 计算光纤的链长
    	    if(EventNum > 1)    Distance_m = mpp * FinalEvents[FinalEventNum-1];
        	else                Distance_m = 0;
        	AllResult->MeasureParam.FiberLength = Distance_m;
        	
    	    gs1 = EventGroupStart[EventGroupNum-1];
    	    Distance_m = mpp * FinalEvents[gs1];
    	    if(Distance_m > 0)      AllResult->MeasureParam.FiberAttenCoef = TotalLoss/Distance_m * 1000;
    	    else                    AllResult->MeasureParam.FiberAttenCoef = RSVD_FLOAT;
    	}
    	TCPDEBUG_PRINT("FiberLength = %.2fm, FiberLoss = %.3fdB, FiberAttenCoef = %.3fdB/km\n", AllResult->MeasureParam.FiberLength, AllResult->MeasureParam.FiberLoss, AllResult->MeasureParam.FiberAttenCoef);
	}
	PacketLength += 12;
	PutDataFrame((char *)&MeasureResult, PacketLength);
	TCPDEBUG_PRINT("Please, Send my data : %d\n", PacketLength);
}

/*
 **************************************************************************************************
 *  函数  名： PutRawData
 *  函数描述： 上传原始数据
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2012-05-14  15:46:38  v1.0
 **************************************************************************************************
*/
void PutRawData(Int32 data[])
{
    OTDR_UploadRawData_t    *result;
    Int32   i, *src, *dst, PacketLength = 0;
    
    result = (OTDR_UploadRawData_t*)malloc(1*sizeof(OTDR_UploadRawData_t));
    if(NULL == result)
	{
		TCPDEBUG_PRINT("result alloc fail in PutRawData\n");
		return;
	}
	
	// 填充命令码
	PacketLength                         = sizeof(result->MeasureParam) + 4 + DATA_LEN*sizeof(Int32);
	result->Cmd                          = CMD_DSP_UPLOAD_RAW;
	result->PacketLength                 = PacketLength;
	
	// 填充参数
	result->MeasureParam.SampleRate_Hz   = OtdrState.RealSampleRate_Hz;
	result->MeasureParam.MeasureLength_m = OtdrState.MeasureParam.MeasureLength_m;
	result->MeasureParam.PulseWidth_ns   = OtdrState.MeasureParam.PulseWidth_ns;
	result->MeasureParam.Lambda_nm       = OtdrState.MeasureParam.Lambda_nm;
	result->MeasureParam.MeasureTime_ms  = OtdrState.MeasureParam.MeasureTime_ms;
	result->MeasureParam.n               = OtdrState.MeasureParam.n;
	
	// 填充数据
//	src = OtdrData.ChanData;
	src = data;
	dst = result->RawData;
	result->DataNum = DATA_LEN;
	for(i = 0; i < DATA_LEN; i++)
	{
	    *dst++ = *src++;
	}
	
	PutDataFrame((char *)result, PacketLength + 12);
	TCPDEBUG_PRINT("Please Send Raw Data : %d\n", PacketLength + 12);
	
	free(result);
}

/*
 **************************************************************************************************
 *  函数  名： PutLowPowerRawData
 *  函数描述： 上传低功率原始数据
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2012-05-14  15:46:38  v1.0
 **************************************************************************************************
*/
void PutLowPowerRawData(Int32 data[])
{
    OTDR_UploadRawData_t    *result;
    Int32   i, *src, *dst, PacketLength = 0;
    
    result = (OTDR_UploadRawData_t*)malloc(1*sizeof(OTDR_UploadRawData_t));
    if(NULL == result)
	{
		TCPDEBUG_PRINT("result alloc fail in PutLowPowerRawData\n");
		return;
	}
	
	// 填充命令码
	PacketLength                         = DATA_LEN*sizeof(Int32);
	result->Cmd                          = 0x90abcdef;
	result->PacketLength                 = PacketLength;
	
	// 填充数据
	src = data;
	dst = (Int32*)result + 2;
	for(i = 0; i < DATA_LEN; i++)
	{
	    *dst++ = *src++;
	}
	
	PutDataFrame((char *)result, PacketLength + 12);
	TCPDEBUG_PRINT("Please Send Raw Data : %d\n", PacketLength + 12);
	
	free(result);
}

/*
 **************************************************************************************
 *    End    of    File
 **************************************************************************************
*/
