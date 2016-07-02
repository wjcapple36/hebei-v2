/*
 **************************************************************************************
 *                        ���ֹ�ͨ���ӹ��̹�˾
 *
 *  �ļ������� �����¼������
 *
 *  �ļ���  �� EventParam.c
 *  ������  �� ����
 *  �������ڣ� 2011-12-27 17:19:19
 *  ��ǰ�汾�� v1.0
 * 
 ***** �޸ļ�¼ *****
 *  �޸���  �� 
 *  �޸����ڣ� 
 *  ��    ע�� 
 **************************************************************************************
*/
#include <stdio.h>
#include <math.h>

#include "Otdr.h"
//#include "OtdrEdma.h"
#include "prototypes.h"
#include "protocol.h"

#define     FIBER_PARAM_SEND    0   // �Ƿ��ϴ����˶β���
#define     EVENT_END_SEND      0   // �Ƿ��ϴ��¼�ĩ�����
typedef struct
{
    Int32   *EventGroupStart;
    Int32   EventGroupNum;
}EventGroupStart_t;

// �¼������������˶β�������
#if EVENT_END_SEND
OTDR_UploadEventEnd_t       EventEnd;
#endif

#if FIBER_PARAM_SEND
OTDR_UploadFiberParam_t     FiberParam;
#endif

/*
 **************************************************************************************************
 *  ����  ���� CalculateFiberSectionAttenCoef
 *  ���������� �����¼�����ż������Ĺ��˶ε�˥��ϵ��
 *  ��ڲ����� Ai         : ������������
 *             EventTemp  : �¼�����Ϣ
 *             Ei         : �¼������
 *  ���ز����� Ac         : ���¼���֮��Ĺ��˶ε�ƽ��˥��ϵ��
 *             b          : �ù��˶η����ӳ���0���ĳ�ʼ�߶�
 *  ���ڰ汾�� 2013-01-10  16:45:44  v1.0
 **************************************************************************************************
*/
float CalculateFiberSectionAttenCoef(const float Ai[], Event_t* EventTemp, Int32 Ei, float *b)
{
	Int32	m, EventNum, MinEventDis;
	Int32	*FinalEvents, *FinalEventsEnd;
	float	atten_coeff, Ac, bt = RSVD_FLOAT;

    FinalEvents     = EventTemp->FinalEvent;        // �¼����
    FinalEventsEnd  = EventTemp->FinalEventEnd;     // �¼�ĩ��
    EventNum        = EventTemp->FinalEventNum;
    m               = PulseWidthInSampleNum();
/**************************** ����������˶ε�˥��ϵ�� *********************************/
    MinEventDis = (Int32)(OtdrState.Points_1m * 5);   // 5�׶�Ӧ�ĵ�������Ϊ�����¼������С����
    MinEventDis = MIN(MinEventDis, 4*m);                // 2011-12-28 9:54:04
    
    // ���������¼���ľ������ж���Щ�¼����Լ���˥��ϵ��
	if(0 == Ei)
	{
	    if(EventNum > 1)    // �ж���һ���¼����Ƿ���������
	    {
    		if((FinalEventsEnd[0] + 2*m + MinEventDis) <= FinalEvents[1])    // ��������㹻Զ������Լ���˥��ϵ��
    		{
    		    LeastSquareMethod(Ai, FinalEventsEnd[0]+m, FinalEvents[1]-m, &atten_coeff, &bt);    // ��С���˷�   2011-8-25 16:38:03
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
			LeastSquareMethod(Ai, FinalEventsEnd[Ei]+m, FinalEvents[Ei+1]-m, &atten_coeff, &bt);    // ��С���˷�   2011-8-25 16:38:03
            Ac = -atten_coeff;
    	}
		else    Ac = RSVD_FLOAT;
	}
	
	if(NULL != b)   *b = bt;
	return Ac;
}

/*
 **************************************************************************************************
 *  ����  ���� InitEventGroupStart
 *  ���������� ����˥��ϵ���������¼������ʼ�¼�����
 *  ��ڲ����� AttenCoef     : ���˶ε�˥��ϵ��
 *             EventNum      : �¼�������
 *  ���ز����� EGS           : �¼������ʼ�¼���ż��¼���ĸ���
 *             EventGroupNum : �¼���ĸ���
 *  ���ڰ汾�� 2013-01-10  17:05:30  v1.0
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
 *  ����  ���� CalculateEventPoint_IL_RL
 *  ���������� �����¼�����������˶εĲ�����ĺͷ������
 *  ��ڲ����� Ai        : ������������
 *             EventTemp : �¼�����Ϣ
 *             AC        : ���˶ε�˥��ϵ��
 *             b         : ���˶η����ӳ���0���ĳ�ʼ�߶�
 *             EGS       : �¼������ʼ�¼���ż��¼���ĸ���
 *             Gi        : �¼������
 *             bsc       : ����ɢ��ϵ��
 *  ���ز����� IL        : �������
 *             RL        : �������
 *             insert_loss : ���¼���Ĳ������
 *  ���ڰ汾�� 2013-01-11  10:59:30  v1.0
 **************************************************************************************************
*/
float CalculateEventPoint_IL_RL(const float Ai[], const Event_t* EventTemp, const float AC[], const float b[], 
                                const EventGroupStart_t *EGS, Int32 Gi, float bsc, float IL[], float RL[])
{
	Int32	i, gs1, gs2, ge0, ge1;
	Int32	*FinalEvents, *FinalEventsEnd, *EventGroupStart;
	float	Lv, Rv, Am, A0, insert_loss;
	
	FinalEvents     = EventTemp->FinalEvent;        // �¼����
    FinalEventsEnd  = EventTemp->FinalEventEnd;     // �¼�ĩ��
    EventGroupStart = EGS->EventGroupStart;         // �¼������

    if(0 == Gi)     /******* ��ʼ�¼��飬ֻ���㷴����� *******/
    {
        if(EGS->EventGroupNum > 1)
        {
		    ge1 = EventGroupStart[1]-1;
    	    MaxValue(Ai, FinalEvents[ge1], FinalEventsEnd[ge1], &Am, &i, DATA_TYPE_FLOAT);
    	    A0 = b[ge1] - AC[ge1] * i;   // ��ֱ����ϳ������¼���ֵ
    		RL[ge1] = -(bsc + 2*(Am - A0));    // ������ĸ����¼���������һ���¼�
		}
		insert_loss = RSVD_FLOAT;   // û�в���
    }
    else if(EGS->EventGroupNum-1 == Gi)    /******* �����¼��飬ֻ���㷴����� *******/
    {
    	gs1 = EventGroupStart[Gi];
    	ge0 = gs1-1;
		MaxValue(Ai, FinalEvents[gs1], FinalEventsEnd[gs1], &Am, &i, DATA_TYPE_FLOAT);
        A0 = b[ge0] - AC[ge0] * i;   // ��ֱ����ϳ������¼���ֵ
		RL[gs1] = -(bsc + 2*(Am - A0));        // ������ĸ����¼�����ĵ�һ���¼�
		insert_loss = RSVD_FLOAT;   // û�в���
    }
    else            /******* �м��¼��㣬���������ġ�������� *******/
   	{
        gs1 = EventGroupStart[Gi];
		gs2 = EventGroupStart[Gi+1];
		ge0 = gs1-1;
		ge1 = gs2-1;
		
		/******* ���㷴����� *******/
		MaxValue(Ai, FinalEvents[gs1], FinalEventsEnd[gs1], &Am, &i, DATA_TYPE_FLOAT);
        A0 = b[ge0] - AC[ge0] * i;   // ��ֱ����ϳ������¼���ֵ
		RL[gs1] = -(bsc + 2*(Am - A0));    // ������ĸ����¼�����ĵ�һ���¼�
		
		// ����¼�������¼�����1���������һ���¼�Ҳ����ӵ�з������
		if(gs1 < ge1)
		{
		    MaxValue(Ai, FinalEvents[ge1], FinalEventsEnd[ge1], &Am, &i, DATA_TYPE_FLOAT);
    	    A0 = b[ge1] - AC[ge1] * i;   // ��ֱ����ϳ������¼���ֵ
    		RL[ge1] = -(bsc + 2*(Am - A0));    // ������ĸ����¼�����ĵ�һ���¼�
		}

		/******* ���������� *******/
		Lv = b[ge0] - AC[ge0] * FinalEvents[gs1];   // ��ֱ����ϳ������¼���ֵ
	    Rv = b[ge1] - AC[ge1] * FinalEvents[gs1];   // ��ֱ����ϳ������¼���ֵ������ֵ��ͬһ��λ��
        insert_loss = Lv - Rv;
        IL[gs1]  = insert_loss;             // ������ĸ����¼�����ĵ�һ���¼�
    }
    return insert_loss;
}

/*
 **************************************************************************************************
 *  ����  ���� CalculateFSL
 *  ���������� �����¼�����������˶ε����
 *  ��ڲ����� EventTemp : �¼�����Ϣ
 *             AC        : ���˶ε�˥��ϵ��
 *             EGS       : �¼������ʼ�¼���ż��¼���ĸ���
 *  ���ز����� FSL       : ���˶����
 *             TotalLoss : ���˶�����������
 *  ���ڰ汾�� 2013-1-18 9:25:22  v1.0
 **************************************************************************************************
*/
float CalculateFSL(const Event_t* EventTemp, const float AC[], 
                   const EventGroupStart_t *EGS, float FSL[])
{
	Int32	i, gs0, gs1;
	Int32	*FinalEvents, *EventGroupStart;
	float	TotalLoss = 0;

	FinalEvents     = EventTemp->FinalEvent;        // �¼����
    EventGroupStart = EGS->EventGroupStart;         // �¼������
    
    /******* ��ʼ���������˶ε���� *******/
    for(i = 0; i < EventTemp->FinalEventNum-1; i++)     FSL[i] = RSVD_FLOAT;
    
    /******* ����������˶ε���� *******/
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
 *  ����  ���� CalculateEventPointTL
 *  ���������� �����¼����������¼�����ۼ���ļ����˶ε����
 *  ��ڲ����� EventTemp : �¼�����Ϣ
 *             AC        : ���˶ε�˥��ϵ��
 *             IL        : �������
 *             EGS       : �¼������ʼ�¼���ż��¼���ĸ���
 *  ���ز����� FSL       : ���˶����
 *             TL        : �ۼ����
 *             TotalLoss : �����������ۼ����
 *  ���ڰ汾�� 2013-01-11  11:44:33  v1.0
 **************************************************************************************************
*/
float CalculateEventPointTL(const Event_t* EventTemp, const float AC[], const float IL[], 
                            const EventGroupStart_t *EGS, float FSL[], float TL[])
{
	Int32	i, gs, *EventGroupStart = EGS->EventGroupStart;         // �¼������
	float	TotalLoss;

    /******* ��ʼ�������¼�����ۼ���ģ�������������˶ε���� *******/
    for(i = 0; i < EventTemp->FinalEventNum; i++)     TL[i] = RSVD_FLOAT;
    CalculateFSL(EventTemp, AC, EGS, FSL);
    
    /******* ��������¼�����ۼ���� *******/
	TotalLoss = 0;
	TL[0]     = 0;//RSVD_FLOAT;
    for(i = 1; i < EGS->EventGroupNum; i++)
    {
        gs = EventGroupStart[i];
        TotalLoss += FSL[gs-1];
        
    #if 1    // ������¼�����ڲ�����ģ�������ۼ����
        if(IL[gs] != RSVD_FLOAT)    TotalLoss += IL[gs];
    	TL[gs] = TotalLoss;
    #else    // ��ʹ���¼�����ڲ�����ģ����������ۼ���ģ����Ǽ�����һ���¼�����ۼ����
        TL[gs] = TotalLoss;
        if(IL[gs] != RSVD_FLOAT)    TotalLoss += IL[gs];
    #endif
    }

    return TotalLoss;
}

/*
 **************************************************************************************************
 *  ����  ���� NormalizeAC_RL
 *  ���������� �淶���ز���ĺ�˥��ϵ��
 *  ��ڲ����� State    : ״̬����
 *             EventNum : �¼�����
 *  ���ز����� AC       : ˥��ϵ��
 *             RL       : �ز����
 *  ���ڰ汾�� 2013-01-18  10:16:50  v1.0
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
 *  �������ƣ� AnalyseMeasureResult
 *  ���������� �����������
 *  ��ڲ����� Ai        : ����˥������
 *             EventTemp : Я���¼�����Ϣ���м����
 *             DataTemp  : ��ʱ���棬���뱣֤�� 4*MAX_EVENT_NUM �� float �Ŀռ��С
 *             hasEvent  : �Ƿ�����¼��㣬ˢ�����ݺ�ʵʱģʽ�����ݲ����¼���
 *  ���ز����� 
 *  ���ڰ汾�� 2010-11-02  10:07:40  v1.0	2010-12-10 9:18:54 v2.0	2011-1-6 21:21:56
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
	
	OtdrStateVariable_t     *State     = NULL;  // ����״̬����
	OTDR_UploadAllData_t    *AllResult = NULL;  // ȫ����������
	OTDR_UploadAllData_t    *AllEvent  = NULL;  // ȫ���������� �¼��㲿��  2011-6-28 14:47:20
	OTDR_UploadRefData_t    *RefResult = NULL;  // ˢ������
	
	State     = (OtdrStateVariable_t *)&OtdrState;
	
	// �� UploadLen �����2�ı���������Ϊ�˶���
	UploadLen = MIN(OtdrState.MeasureLengthPoint+1, DATA_LEN-NOISE_LEN);
	if(UploadLen & 1)   UploadLen++;

	if(hasEvent)
	{
    	FinalEvents     = EventTemp->FinalEvent;        // �¼����
    	FinalEventsEnd  = EventTemp->FinalEventEnd;     // �¼�ĩ��
    	EventType       = EventTemp->EventType;         // �¼�����
    	EventGroupStart = EventTemp->TotalEvent;        // �¼������
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
#error  AnalyseMeasureResult �ռ���䲻��
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
	if(hasEvent)      // ȫ����������
	{
	    // ��ȡʵ��ʹ�õĲ����ʺ������ʣ�������ϵ��
	    ppm = State->Points_1m;
    	mpp = 1/ppm;
    	
    	// ����ɢ��ϵ��
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
	if(!hasEvent)       // ˢ������
	{
	    RefResult->OtdrData.DataNum = UploadLen;
	    pdB                         = (Uint16*)(RefResult->OtdrData.dB_x1000);

	    // update PacketLength
	    PacketLength               += 4 + UploadLen*sizeof(Uint16);
	}
	else                // ȫ����������
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
    if(hasEvent)      // ȫ����������
    {
        // Ϊ���ı����ݽṹ���� AllEvent ƫ���������¼��㲿��ָ�� NextAddr
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
        /******* ���������¼���ľ������ж���Щ�¼����Լ���˥��ϵ�� *******/
        EventNum = EventTemp->FinalEventNum;
    	for(i = LastValidEventIndex; (i < EventNum) && (i < MAX_EVENT_NUM); i++)  // 2011-5-7 11:19:14
        {
    		AttenCoef[i] = CalculateFiberSectionAttenCoef(Ai, EventTemp, i, &b0[i]);
    	}
    	
        // Ѱ���¼�������顣 ����EventGroupStart����ÿ���¼������ʼ�¼������
        // ���ĳ���¼��Ƕ����¼��������¼��飬������������ȡֵ��������������
        EventGroupNum = InitEventGroupStart(AttenCoef, &EGS, EventNum);
        
        /******* �����¼�������������� *******/
    	EventNum = EventTemp->FinalEventNum;
    	FinalEventNum = EventNum;
    	FuckEventNum  = 0;
    	for(i = LastValidGroupIndex; i < EventGroupNum; i++)  // 2012-9-24 15:08:39
        {
            gs1 = EventGroupStart[i];
		    gs2 = EventGroupStart[i+1];
            insert_loss = CalculateEventPoint_IL_RL(Ai, EventTemp, AttenCoef, b0, &EGS, 
                            i, BackwardScatterCoeff, EventInsertLoss, EventReflectLoss);
    
    		// ���˵��������С���¼���
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
                    for(j = gs1; j < gs2; j++)      // �����¼���һ���˵�
                    {
                        EventType[j]  = EVENT_TYPE_FUCK;
                        FuckEventNum++;
                    }
                }
                
                // �������FUCK�¼������˳�����
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

    	/******* ��������¼�����ۼ���ļ����˶ε���� *******/
    	TotalLoss = CalculateEventPointTL(EventTemp, AttenCoef, EventInsertLoss, &EGS, FiberSectionLoss, EventTotalLoss);
    	
    	/******* �淶���ز���ĺ�˥��ϵ�� *******/
    	NormalizeAC_RL(State, AttenCoef, EventReflectLoss, EventType, EventNum);
        
        /******* ��ȷ�������¼�����ۼ���ģ�������ݽṹ *******/
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
        
    	/******* ��ӡ�¼�����Ϣ *******/
        TCPDEBUG_PRINT("\n******** EVENT LIST ********\n");
        TCPDEBUG_PRINT("Event  Position  InsertLoss  ReflectLoss  AttenCoef  TotalLoss\n");
        for(i = 0; i < FinalEventNum; i++)
        {
            TCPDEBUG_PRINT(" %-4d  %-8d  %-10.3f  %-11.3f  %-9.4f  %-9.3f\n", i, AllEvent->Event.EventPoint[i].EventXlabel, 
                AllEvent->Event.EventPoint[i].EventInsertLoss, AllEvent->Event.EventPoint[i].EventReflectLoss, 
                AllEvent->Event.EventPoint[i].AttenCoef, AllEvent->Event.EventPoint[i].EventTotalLoss);
        }
    #if 1
        // ��ʵ������֮��ƽ��˥��ϵ������ʼ�¼�����ڣ������¼��㲻���ڣ�ʵ����ϣ��
        // ��ʼ�¼��㲻����˥��ϵ�����������¼�����ڣ����Ա�������Ųһλ
        for(j = FinalEventNum-1; j > 0; j--)
        {
            AllEvent->Event.EventPoint[j].AttenCoef = AllEvent->Event.EventPoint[j-1].AttenCoef;
        }
        AllEvent->Event.EventPoint[0].AttenCoef = RSVD_FLOAT;   // ��ʼ�¼��㣬˥��ϵ����Ϊ0
    #endif
    
    #if EVENT_END_SEND
        /******* ����¼���� *******/
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
        /******* �����˶β��� *******/
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
        
        /******* ��ӡ���˶���Ϣ *******/
        TCPDEBUG_PRINT("\n******** ���˶���Ϣ ********\n");
        TCPDEBUG_PRINT("Section  Length(m)  Loss(dB)  AttenCoef\n");
        for(i = 0; i < FinalEventNum-1; i++)
        {
            TCPDEBUG_PRINT(" %2d~%2d   %-9.2f  %-8.3f  %-8.3f\n", i, i+1, 
                            mpp * FiberParam.Fiber[i].FiberLen, FiberSectionLoss[i], AttenCoef[i]);
        }
    #endif
    }

/************************************ ������ı���ֵ������ *************************************/
	if(!hasEvent)       // ˢ������
	{
	    *(Uint32*)NextAddr      = RSVD_VALUE;
	    RefResult->PacketLength = PacketLength;
	}
	else                // ȫ����������
	{
	    *(Uint32*)&(AllEvent->Event.EventPoint[FinalEventNum]) = RSVD_VALUE;
	    AllResult->PacketLength = PacketLength;
	}
	
	// ����ǰ���ȫ������ĺ���ƽ��˥��ϵ��
	if(hasEvent)
	{
    	AllResult = (OTDR_UploadAllData_t*)&MeasureResult;
    	AllResult->MeasureParam.FiberLoss = (TotalLoss == 0) ? RSVD_FLOAT : TotalLoss;
    	if(TotalLoss == RSVD_FLOAT)     AllResult->MeasureParam.FiberAttenCoef  = RSVD_FLOAT;
    	else
    	{
    	    // ������˵�����
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
 *  ����  ���� PutRawData
 *  ���������� �ϴ�ԭʼ����
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2012-05-14  15:46:38  v1.0
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
	
	// ���������
	PacketLength                         = sizeof(result->MeasureParam) + 4 + DATA_LEN*sizeof(Int32);
	result->Cmd                          = CMD_DSP_UPLOAD_RAW;
	result->PacketLength                 = PacketLength;
	
	// ������
	result->MeasureParam.SampleRate_Hz   = OtdrState.RealSampleRate_Hz;
	result->MeasureParam.MeasureLength_m = OtdrState.MeasureParam.MeasureLength_m;
	result->MeasureParam.PulseWidth_ns   = OtdrState.MeasureParam.PulseWidth_ns;
	result->MeasureParam.Lambda_nm       = OtdrState.MeasureParam.Lambda_nm;
	result->MeasureParam.MeasureTime_ms  = OtdrState.MeasureParam.MeasureTime_ms;
	result->MeasureParam.n               = OtdrState.MeasureParam.n;
	
	// �������
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
 *  ����  ���� PutLowPowerRawData
 *  ���������� �ϴ��͹���ԭʼ����
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2012-05-14  15:46:38  v1.0
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
	
	// ���������
	PacketLength                         = DATA_LEN*sizeof(Int32);
	result->Cmd                          = 0x90abcdef;
	result->PacketLength                 = PacketLength;
	
	// �������
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
