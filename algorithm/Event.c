/*
 **************************************************************************************
 *                        ???ֹ?ͨ???ӹ??̹?˾
 *
 *  ?ļ??????? ?¼??㡣???????????¼??????صĴ??붼???????ļ???
 *
 *  ?ļ???  ?? Event.c
 *  ??????  ?? ?���??
 *  ???????ڣ? 2011-3-7 17:17:40
 *  ??ǰ?汾?? v2.0
 *
 ***** ?޸ļ?¼ *****
 *  ?޸???  ?? ?���??
 *  ?޸????ڣ? 2011-3-29 8:45:32
 *  ??    ע?? ??ʹ??ǿ?˲?Ѱ?Ҵ??¼??????????ڸ????¼?????Ѱ??С?¼???
 **************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>	// for qsort
#include <math.h>

#include "Otdr.h"
#include "prototypes.h"
#include "protocol.h"
#include "DspFpgaReg.h"

/*
 **************************************************************************************
 *  ???????ƣ? AllocEventMem
 *  ?????????? ΪEventTemp??��?????ռ?
 *  ???ڲ????? EventTemp : ָ??Event_t???ͱ?��??ָ??
 *  ???ز????? 0 : ????ʧ??
 *             1 : ?????ɹ?
 *  ???ڰ汾?? 2011-03-31  21:43:06  v1.0
 **************************************************************************************
*/
Int32 AllocEventMem(Event_t* const EventTemp)
{
    Int32 *p;
    
    memset(EventTemp, 0, sizeof(Event_t));
    
    /******* ????EventTemp->TotalEvent *******/
    p = (Int32*)malloc(DATA_LEN*sizeof(Int32));
    if(NULL == p)
	{
		TCPDEBUG_PRINT("EventTemp->TotalEvent alloc fail\n");
		return 0;
	}
    EventTemp->TotalEvent = p;
    
    /******* ????EventTemp->ReflectEvent *******/
    p = (Int32*)malloc(DATA_LEN*sizeof(Int32));
    if(NULL == p)
	{
		TCPDEBUG_PRINT("EventTemp->ReflectEvent alloc fail\n");
		return 0;
	}
    EventTemp->ReflectEvent = p;
    
    /******* ????EventTemp->FinalEvent *******/
    p = (Int32*)malloc(MAX_EVENT_NUM*sizeof(Int32));
    if(NULL == p)
	{
		TCPDEBUG_PRINT("EventTemp->FinalEvent alloc fail\n");
		return 0;
	}
    EventTemp->FinalEvent = p;
    
    /******* ????EventTemp->FinalEventEnd *******/
    p = (Int32*)malloc(MAX_EVENT_NUM*sizeof(Int32));
    if(NULL == p)
	{
		TCPDEBUG_PRINT("EventTemp->FinalEventEnd alloc fail\n");
		return 0;
	}
    EventTemp->FinalEventEnd = p;
    
    /******* ????EventTemp->EventType *******/
    p = (Int32*)malloc(MAX_EVENT_NUM*sizeof(Int32));
    if(NULL == p)
	{
		TCPDEBUG_PRINT("EventTemp->EventType alloc fail\n");
		return 0;
	}
    EventTemp->EventType = p;
    
    /******* ????EventTemp->SaturateEvent *******/
    p = (Int32*)malloc(MAX_EVENT_NUM*sizeof(Int32));
    if(NULL == p)
	{
		TCPDEBUG_PRINT("EventTemp->SaturateEvent alloc fail\n");
		return 0;
	}
    EventTemp->SaturateEvent = p;
    
    return 1;
}

/*
 **************************************************************************************
 *  ???????ƣ? FreeEventMem
 *  ?????????? ?ͷ?EventTemp??��ָ???Ŀռ?
 *  ???ڲ????? EventTemp : ָ??Event_t???ͱ?��??ָ??
 *  ???ز????? 
 *  ???ڰ汾?? 2011-03-31  21:50:04  v1.0
 **************************************************************************************
*/
void FreeEventMem(Event_t* const EventTemp)
{
    if(NULL != EventTemp->TotalEvent   )    free(EventTemp->TotalEvent);
    if(NULL != EventTemp->ReflectEvent )    free(EventTemp->ReflectEvent);
    if(NULL != EventTemp->FinalEvent   )    free(EventTemp->FinalEvent);
    if(NULL != EventTemp->FinalEventEnd)    free(EventTemp->FinalEventEnd);
    if(NULL != EventTemp->EventType    )    free(EventTemp->EventType);
    if(NULL != EventTemp->SaturateEvent)    free(EventTemp->SaturateEvent);
    
    memset(EventTemp, 0, sizeof(Event_t));
}

/*
 **************************************************************************************************
 *  ????  ???? GetLaserParam
 *  ?????????? ???ݲ?????ȡ??????????????˥??ϵ????????ɢ??ϵ??
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-08-24  14:34:58  v1.0
 **************************************************************************************************
*/
void GetLaserParam(Int32 Lambda_nm, float *AttenCoef, float *BackScatterCoeff)
{
    float k0, bsc;
    
    if(1550 == Lambda_nm)	    {   k0  = 0.185;    bsc = -82.0; }
	else if(1310 == Lambda_nm)	{   k0  = 0.321;	bsc = -80.0; }
	else if(1625 == Lambda_nm)	{   k0  = 0.2  ;	bsc = -83.0; }
    else if(1650 == Lambda_nm)	{   k0  = 0.22 ;	bsc = -84.0; }
	else if(850  == Lambda_nm)	{   k0  = 2.16 ;	bsc = -62.0; }
	else if(1300 == Lambda_nm)	{   k0  = 0.465;	bsc = -70.0; }
	else if(1490 == Lambda_nm)	{   k0  = 0.22 ;	bsc = -81.0; }
	else	                    {   k0  = 0.25 ;	bsc = -73.0; }
	
    if(NULL != AttenCoef)           *AttenCoef = k0;
    if(NULL != BackScatterCoeff)    *BackScatterCoeff = bsc;
}

float GetMinAttenCoef(Int32 Lambda_nm)
{
    float k0;
    
    if(1550 == Lambda_nm)	    k0 = 0.17;
	else if(1310 == Lambda_nm)	k0 = 0.3;
	else if(1625 == Lambda_nm)	k0 = 0.17;
	else if(1650 == Lambda_nm)	k0 = 0.18;
	else if(850 == Lambda_nm)	k0 = 1.8;
	else if(1300 == Lambda_nm)	k0 = 0.4;
	else if(1490 == Lambda_nm)	k0 = 0.2;
	else	                    k0 = 0.25;
	
	return k0;
}

/*
 **************************************************************************************
 *  ???????ƣ? GetEventFindingParamBeforeLog
 *  ?????????? ?????Զ?Ѱ???¼??㣬?????㷨Ϊ??OTDR?¼???????-2010-10-26??
 *             Ϊ?????¼??????˺???EventPointsFilter????ʹ??Mֵ????????????????ֵ
 *  ???ڲ????? t1   : ??ֵ????ϵ??1
 *             t2   : ??ֵ????ϵ??2
 *             k    : ˥??б??
 *             M    : ????????
 *             type : ??????????
 *             PowerMode : ????ģʽ????ͬ?Ĺ???ʹ?ò?ͬ??ϵ??
 *  ???ز?????
 *  ???ڰ汾?? 2010-11-01  10:43:33  v1.0	2010-12-8 16:32:18 v2.0
 **************************************************************************************
*/
void GetEventFindingParamBeforeLog(float *t1, float *t2, float *k, Int32 *M, const char type[], Int32 PowerMode)
{
	float k0, tt1, tt2;
	Int32 Lambda_nm, PulseWidth_ns, m;

	Lambda_nm       = OtdrState.MeasureParam.Lambda_nm;
	PulseWidth_ns   = OtdrState.MeasureParam.PulseWidth_ns;
    
/***************************************** ??ֵ???? **********************************************/
	if(strcmp(type, "Large") == 0)          // Ѱ?Ҵ??¼???
	{
        if(PowerMode == POWER_MODE_LOW)
        {
            tt1 = 8;
            tt2 = 0.03;
        }
        else
        {
            tt1 = 8;
            tt2 = 0.04;
        }

        if((Lambda_nm == 850) || (Lambda_nm == 1300))  // 850????   2012-6-27 15:16:53
	    {
	        tt1 = 8;
	        tt2 = 0.04;
	    }
	    else if(TR600_C_CLASS && !TR600_A_CLASS)      // 2012-11-5 17:21:16 24CΪ???弤???????й??ʿ???
	    {
	        if(OtdrState.RealSampleRate_MHz >= CLK_400MHz)
	        {
    	        tt1 = 8;
    	        tt2 = 0.065;
    	    }
	    }
	    
    	if(t1 != NULL)		*t1 = tt1;		// 2011-3-7 17:12:44
    	if(t2 != NULL)		*t2 = tt2;
    }
    else if(strcmp(type, "Small") == 0)     // Ѱ??С?¼???
    {
        tt1 = 7;//8;//
        tt2 = 0.03;//0.025;//
    	if((Lambda_nm == 850) || (Lambda_nm == 1300))  // 850????   2012-6-27 15:16:53
	    {
	        tt1 = 8;
	        tt2 = 0.04;
	    }
	    else if(TR600_C_CLASS && !TR600_A_CLASS)      // 2012-11-5 17:21:16 24CΪ???弤???????й??ʿ???
	    {
	        if(OtdrState.RealSampleRate_MHz >= CLK_200MHz)
	        { 
    	        tt1 = 14;
    	        tt2 = 0.1;
    	    }
	    }

    	if(t1 != NULL)		*t1 = tt1;		// 2011-3-7 17:12:44
    	if(t2 != NULL)		*t2 = tt2;
    }

/******************************************** ˥??б?? *******************************************/
    // ˥??б?ʵ?ȷ???????չ???˥??ϵ??Ϊ D ???㣬????1km??��??????˥??Ϊ D 
    // ???ǰ??յ?????˥??ϵ??Ϊ k = D/N_1km ????????˥??ϵ??????1km??Ӧ?ĵ???
	GetLaserParam(Lambda_nm, &k0, NULL);

	m = PulseWidthInSampleNum();
	if(strcmp(type, "Large") == 0)     // Ѱ?Ҵ??¼???
    {
        // ????????̫С??????20nsΪ???ޣ?????????̫????????5120Ϊ????   2013-3-25 16:38:07
        if(PulseWidth_ns < 20)          m <<= 1;
    	else if(PulseWidth_ns >= 5120)  m = (Int32)(m / (PulseWidth_ns / 5120.0));
    	
    	if(k != NULL)		*k = k0 / (OtdrState.Points_1m * 1000); // ???任??1km??Ӧ?ĵ???
    	if(M != NULL)		*M = m;
    }
    else if(strcmp(type, "Small") == 0)          // Ѱ??С?¼???
	{
    	if(k != NULL)		*k = k0 / (OtdrState.Points_1m * 1000); // ???任??1km??Ӧ?ĵ???
    	if(M != NULL)
    	{
    	    if(PulseWidth_ns <= 20)         *M = 16*m;              // ʹ???????˲?
    	    else if(PulseWidth_ns <= 240)   *M = 8*m;               // ʹ???????˲?
    	    else if(PulseWidth_ns <= 320)   *M = 4*m;               // ʹ???????˲?
    	    else                            *M = 2*m;
    	}
    }
}

#define KD  2   // ʹ????��????��?????¼?ת?۵?ʱ??��?????½?????
/*
 **************************************************************************************
 *  ???????ƣ? FindLargeEventsBeforeLog
 *  ?????????? ȡ????֮ǰѰ?Ҵ??¼??㣬?Ƿ???????Ϊ?Զ????á?
 *  ???ڲ????? OtdrData  : ?????˲???????
 *             DataLen   : ???????ݳ???
 *             EventTemp : ?????¼??????飬?洢?¼?????Ϣ
 *             State     : ״̬??��
 *  ???ز????? EventNum  : ?¼???????
 *  ???ڰ汾?? 2013-8-6 11:19:22
 **************************************************************************************
*/
Int32 FindLargeEventsBeforeLog(OTDR_ChannelData_t *OtdrData, Int32 DataLen, Event_t* EventTemp, OtdrStateVariable_t *State)
{
	Int32   i, j, t, M, Threshold, RefineP = 0, RefineN = 0;
	Int32   EndPoint, nEventPt, EventNum = 0, rEventNum = 0;
	Int32   *An, *Dn, *TotalEvent;
	Int32   sigma;
	float   t1, t2, k, v;
    
/*************************************** ????Delta ***********************************************/
    An = OtdrData->ChanData;
    Dn = OtdrData->Delta;
	TotalEvent = EventTemp->TotalEvent;
	sigma      = State->sigma;

/*************************************** ??ȡ???? ************************************************/
	GetEventFindingParamBeforeLog(&t1, &t2, &k, &M, "Large", OtdrCtrl.PowerMode);
	EndPoint = MIN(State->MeasureLengthPoint, DataLen - NOISE_LEN-2*M-1);

/************************************* ??????��???? **********************************************/
	// ??????��????Delta(n) = An(n) ?C An(n+M) ?C k*M * An
    v = Tenth_div5(-k*M);       // ?? v = 10^(-k*M/5)

	for(i = 0; i < DataLen-M; i++)      // Ϊʲô???? Dn[i] = An[i-M] - An[i+M]
	{
		Dn[i] = (Int32)(v * An[i] - An[i+M]);
	}
	for(i = DataLen-M; i < DataLen; i++)
	{
		Dn[i] = 0;
	}

/************************************** Ѱ???¼??? ***********************************************/
    // ?ߵ͹??????߷ֿ????ԣ??ȴ????߹???????
	for(i = State->CurveConcatPoint; i <= EndPoint; i++)
	{
		{
/********************** ??????ֵ??????T(n) = (t1 * ?? + t2 * A(n)) *******************************/
    		Threshold = t1*sigma + t2*An[i];
    	    if(abs(Dn[i]) > Threshold)
    	    {
    	        nEventPt = i+M;
    	        if(Dn[i] > Threshold)       // ??��Ϊ??????????Ϊ?½???
    	        {
        	        if(!RefineP)    // ?ñ?��ָ????һ???Ƿ??Ѿ?ϸ?????¼???ʼλ??
        	        {
            	        // ?ӵ?ǰλ????ǰ??????��????ͻ????λ??
            	        t = MAX(0, i-M);
            	        for(j = i; j > t; j--)
            	        {
            	            // 2??????????��????ƽ??????��????
            	            if(((Dn[j+1] + KD*Dn[j-1]) > (1+KD)*Dn[j]) || (Dn[j-1] <= 0))   break;
            	        }
            	        nEventPt = j+M;
            	        
            	        for(j = nEventPt; j <= i+M; j++)
        			    {
        			        TotalEvent[EventNum++] = j;
    		            }
    		            RefineP = 1;
            	    }
            	    else
            	    {
                	    TotalEvent[EventNum++] = nEventPt;
        		        if(EventNum >= DataLen)     break;  // 2011-5-28 10:25:04
        		    }
        		}
        		else       // ??��Ϊ??????????Ϊ??????
        	    {
            	    if(!RefineN)    // ?ñ?��ָ????һ???Ƿ??Ѿ?ϸ?????¼???ʼλ??
        	        {
            	        // ?ӵ?ǰλ????ǰ??????��????ͻ????λ??
            	        t = MAX(0, i-M);
            	        for(j = i; j > t; j--)
            	        {
            	            // 2??????????��????ƽ??????��????
            	            if(((Dn[j+1] + KD*Dn[j-1]) < (1+KD)*Dn[j]) || (Dn[j-1] >= 0))   break;
            	        }
            	        nEventPt = j+M;
            	        
            	        for(j = nEventPt; j <= i+M; j++)
        			    {
        			        TotalEvent[EventNum++] = j;
    		            }
    		            RefineN = 1;
            	    }
            	    else
            	    {
                	    TotalEvent[EventNum++] = nEventPt;
        		        if(EventNum >= DataLen)     break;  // 2011-5-28 10:25:04
        		    }
    		    }
    		}
    		else
    		{
    		    RefineP = 0;
    		    RefineN = 0;
    		}
    	}
	}
	
/******************************** ?ٴ????͹???????ǰ?˵??¼? *************************************/
	if(OtdrCtrl.CurveConcat)
	{
	    Int32 lpdsat, lastSat = State->SatThreshold;
	    
	    RefineP = 0;
    	RefineN = 0;
    	EndPoint = State->CurveConcatPoint - M;
    	An    = OtdrData->LowPowerData;
    	
    	// ?????͹??????ߵ?ƽ????ֵ
    	GetSaturateThreshold(An);
    	lpdsat = State->SatThreshold;
    	State->SatThreshold = lastSat;
    	
    	sigma = State->sigmaLp;
    	for(i = 0; i < EndPoint; i++)
        {
            Dn[i] = (Int32)(v * An[i] - An[i+M]);
        }
        
        GetEventFindingParamBeforeLog(&t1, &t2, NULL, NULL, "Large", POWER_MODE_LOW);
    	for(i = 0; i < EndPoint; i++)
        {
            if(An[i] >= lpdsat)     TotalEvent[EventNum++] = nEventPt;
            else
            {
/********************** ??????ֵ??????T(n) = (t1 * ?? + t2 * A(n)) *******************************/
        		Threshold = t1*sigma + t2*An[i];
        	    if(abs(Dn[i]) > Threshold)
        	    {
        	        nEventPt = i+M;
        	        
        	        if(Dn[i] > Threshold)       // ??��Ϊ??????????Ϊ?½???
        	        {
            	        if(!RefineP)    // ?ñ?��ָ????һ???Ƿ??Ѿ?ϸ?????¼???ʼλ??
            	        {
                	        // ?ӵ?ǰλ????ǰ??????��????ͻ????λ??
                	        t = MAX(0, i-M);
                	        for(j = i; j > t; j--)
                	        {
                	            // 2??????????��????ƽ??????��????
                	            if(((Dn[j+1] + KD*Dn[j-1]) > (1+KD)*Dn[j]) || (Dn[j-1] <= 0))   break;
                	        }
                	        nEventPt = j+M;
                	        
                	        for(j = nEventPt; j <= i+M; j++)
            			    {
            			        TotalEvent[EventNum++] = j;
        		            }
        		            RefineP = 1;
                	    }
                	    else
                	    {
                    	    TotalEvent[EventNum++] = nEventPt;
            		        if(EventNum >= DataLen)     break;  // 2011-5-28 10:25:04
            		    }
            		}
            		else       // ??��Ϊ??????????Ϊ??????  2012-12-18 10:00:06
            	    {
                	    if(!RefineN)    // ?ñ?��ָ????һ???Ƿ??Ѿ?ϸ?????¼???ʼλ??
            	        {
                	        // ?ӵ?ǰλ????ǰ??????��????ͻ????λ??
                	        t = MAX(0, i-M);
                	        for(j = i; j > t; j--)
                	        {
                	            // 2??????????��????ƽ??????��????
                	            if(((Dn[j+1] + KD*Dn[j-1]) < (1+KD)*Dn[j]) || (Dn[j-1] >= 0))   break;
                	        }
                	        nEventPt = j+M;
                	        
                	        for(j = nEventPt; j <= i+M; j++)
            			    {
            			        TotalEvent[EventNum++] = j;
        		            }
        		            RefineN = 1;
                	    }
                	    else
                	    {
                    	    TotalEvent[EventNum++] = nEventPt;
            		        if(EventNum >= DataLen)     break;  // 2011-5-28 10:25:04
            		    }
        		    }
        		}
        		else
        		{
        		    RefineP = 0;
        		    RefineN = 0;
        		}
        	}
    	}
    }
    
	EventTemp->TotalEventNum   = EventNum;
	EventTemp->ReflectEventNum = rEventNum;

/******************************** ?ͷ??ڴ? *******************************************************/
	return EventNum;
}

#define FLOAT_ENLARGE_FACTOR  10000   // ?????????Ŵ??ı???
#define U_LIMIT         4       // ??Ϊ?????񵴷???Ϊ???ٸ?sigma
#define USE_ADAPTIVE    1       // ?Ƿ?ʹ?ø?????Ӧ????

/*
 **************************************************************************************
 *  ???????ƣ? EventPointsFilter
 *  ?????????? ?¼???????
 *  ???ڲ????? AiLog     : ????˥??????????(dB)
 *             EventTemp : ?????¼?????Ϣ
 *             type      : ??????????
 *  ???ز????? FinalEventNum : ʵ???????¼???????
 *  ???ڰ汾?? 2011-3-7 17:56:23  v2.0
 **************************************************************************************
*/
Int32 EventPointsFilter(OTDR_ChannelData_t *OtdrData, Event_t *EventTemp, const char type[], OtdrStateVariable_t *State)
{
	Int32   i, j, temp, M, MinDis, EventDis;
	Int32   PulseWidth_ns, TotalEventNum, FinalEventNum = 0;
	Int32   *An, *TotalEvents, *EventType, *FinalEvent, *FinalEventEnd;
    
	// ??ȡMֵ
	An            = OtdrData->ChanData;
	PulseWidth_ns = State->MeasureParam.PulseWidth_ns;
	GetEventFindingParamBeforeLog(NULL, NULL, NULL, &M, type, 0);

/************************************** ??ȡ???? *************************************************/
	// ????4?׶?Ӧ?Ĳ?????????????????????12.5 MHz??clk_MHz??ʵ??ֵ??0.5????΢????
    MinDis = (Int32)(5*State->Points_1m + 0.5);       //5?׶?Ӧ?ĵ???????Ϊ??С????
    if(PulseWidth_ns <= 20)             EventDis = 1.2*M;
    else if(PulseWidth_ns >= 5120)      EventDis = State->M;
	else                                EventDis = M;

	EventDis   = MAX(EventDis, MinDis);
	EventDis   = MIN(EventDis, 200);        // 2012-12-13 15:09:33

	TotalEvents   = EventTemp->TotalEvent;
	TotalEventNum = EventTemp->TotalEventNum;
	EventType     = EventTemp->EventType;
	FinalEvent    = EventTemp->FinalEvent;
	FinalEventEnd = EventTemp->FinalEventEnd;

	// 2016-05-02 17:44:31
	if(TotalEventNum < 1){
		FinalEventNum = 1;
		FinalEvent[0] = 0;
		FinalEventEnd[0] = 0;
		EventType[0] = EVENT_TYPE_START ;
		EventTemp->FinalEventNum = 1;
		EventTemp->TotalEventNum = 1;
		return FinalEventNum;
	}

/************************************** ??ʼ?????¼??? *******************************************/
    // ??ƽ?????ͺ???β?ĵ㶼???ӽ?ȥ
    for(i = 0; i < State->OtdrCurveSaturate.SatNum; i++)
    {
        for(j = State->OtdrCurveSaturate.SatStart[i]; j <= State->OtdrCurveSaturate.TailEnd[i]; j++)
        {
            if(TotalEventNum >= DATA_LEN)   break;
            TotalEvents[TotalEventNum++] = j;
        }
    }

/************************************** ??TotalEvents???????? ************************************/
    // ??????Сֵ????0???????Ӵ?0??????Сֵ
    MinValue(TotalEvents, 0, TotalEventNum-1, &temp, NULL, DATA_TYPE_INT);
    for(i = 0; i < temp; i++)      TotalEvents[TotalEventNum++] = i;
    
    qsort(TotalEvents, TotalEventNum, sizeof(TotalEvents[0]), qsort_cmp);
    
/**************************************** ??ʼȷ???¼??? *****************************************/
	// ??һ??ʱ????Ϊ??ʼ??
	FinalEventNum = 0;
	FinalEvent[FinalEventNum] = TotalEvents[0];

/********************************* ?޳?ͬһ?????????ڵ??¼??? ************************************/
	for(i = 1; i < TotalEventNum; i++)
	{
		if(TotalEvents[i] - TotalEvents[i-1] > EventDis)    //???ڵ?��?????ݱȽ?
		{
			FinalEventEnd[FinalEventNum++] = TotalEvents[i-1];	// ?洢ǰһ??????Ϊ??һ?¼?????ĩ?㣬ͬʱ????M
			if(FinalEventNum >= MAX_EVENT_NUM)   break;         // 2011-5-28 10:41:06 ?ŵ????????˳????ƶ???
			
			FinalEvent[FinalEventNum]      = TotalEvents[i];    // ??һ?¼???????
		}
	}

	// ?¼???ĩ??       2011-10-12 9:11:50
	if(FinalEventNum < MAX_EVENT_NUM)
	{
    	if(TotalEventNum > 1)	FinalEventEnd[FinalEventNum++] = TotalEvents[TotalEventNum-1];// - M;
    	else                    FinalEventEnd[FinalEventNum++] = TotalEvents[0];
    }

#if 1
/******************************** ?˳????Ȳ??????????¼??? ***************************************/
/*	????һ???¼???????????ĩ??֮???ľ??벻????N????????ɾ??????ֹ????̫????????	2010-12-9 21:36:30
	???ڴ?ʱTotalEvents?Ѿ??????ˣ????Կ???????????ʱ?洢?????洢???????¼?????FinalEvent?е?λ??   */
	// ?ȱ?????һ???¼???
	EventDis = MIN(M, 5);
	TotalEventNum = 0;
	TotalEvents[TotalEventNum++] = 0;
	for(i = 1; i < FinalEventNum; i++)
	{
	    // ?????㹻????ֱ??????
		if(FinalEventEnd[i] - FinalEvent[i] >= EventDis)
		{
			TotalEvents[TotalEventNum++] = i;   // ???????? 2011-3-9 16:42:44
		}
	}
	
/***************************************** ???¼????ƶ? ******************************************/
	if(TotalEventNum < FinalEventNum)
	{
    	for(i = 0; i < TotalEventNum; i++)
    	{
    		FinalEvent[i]    = FinalEvent[TotalEvents[i]];
    		FinalEventEnd[i] = FinalEventEnd[TotalEvents[i]];
    	}
    
    	FinalEventNum = TotalEventNum;
    }
#endif

/************************************* ?????¼??????? ********************************************/
	// ???ں??滹???¼????ϲ??????Բ???ȷ???????¼??㣬??Ӧ??ʹ??FindEndEventPoint????��ȷ??
    // ?Զ??巴????ֵΪ1dB??ֻ?и??ڸ???ֵ???¼????Ƿ????¼??????????ǷǷ????¼?    2012-9-21 8:52:37
    // ??ԭ???ǿ??????¼????ķ?ֵ?Ƿ????????Ҷ??߳?һ????????ֵ     ????????
#define     REFLEX_THRESHOLD    1
    {
        Int32   AL, AR, AM, from, to;
        float   t = Tenth_div5(REFLEX_THRESHOLD);   // ??????ֵ??Ӧ?ı???
        Int32   *AnTemp = An;
        
        if(FinalEvent[0] >= State->CurveConcatPoint) An = OtdrData->ChanData;
        else    An = OtdrData->LowPowerData;
        
        // ??һ???¼??????????ж?
        #if 0
        AL = An[0];
        MeanValue(An, FinalEventEnd[0], FinalEventEnd[0] + M, &AR, DATA_TYPE_INT);
        MaxValue(An, 0, FinalEventEnd[0], &AM, NULL, DATA_TYPE_INT);
        AL = MAX(AL, AR);
        if(AM >= AL*t)      EventType[0] = EVENT_TYPE_REFLECT;  // ??????????ֵ??Ϊ?????¼?
        else			    EventType[0] = EVENT_TYPE_NONREFLECT;
    #else   // 2014-4-11 15:20:52 ??ʼ?¼??̶?Ϊ?????¼?
        EventType[0] = EVENT_TYPE_REFLECT;
    #endif
        
        // ?м??¼?????????
        for(i = 1; i < FinalEventNum; i++)
    	{
    	    if(FinalEvent[i] >= State->CurveConcatPoint) An = OtdrData->ChanData;
            else    An = OtdrData->LowPowerData;
    	    
    	    from = MAX(FinalEventEnd[i-1], FinalEvent[i] - M);
    	    if(i == FinalEventNum-1)    to = MIN(DATA_LEN-1, FinalEventEnd[i] + M);
    	    else                        to = MIN(FinalEvent[i+1], FinalEventEnd[i] + M);
    	    
    	    MeanValue(An, from, FinalEvent[i], &AL, DATA_TYPE_INT);
    	    MeanValue(An, FinalEventEnd[i], to, &AR, DATA_TYPE_INT);
    	    MaxValue(An, FinalEvent[i], FinalEventEnd[i], &AM, NULL, DATA_TYPE_INT);
    		AL = MAX(AL, State->sigma);
    		AR = MAX(AR, State->sigma);
    		AL = MAX(AL, AR);
            if(AM >= AL*t)      EventType[i] = EVENT_TYPE_REFLECT;  // ??????????ֵ??Ϊ?????¼?
            else			    EventType[i] = EVENT_TYPE_NONREFLECT;
    	}
    	An = AnTemp;
    	
    	// 2014-2-26 10:27:35 debug
    	TCPDEBUG_PRINT("Evt : ");
    	for(i = 0; i < FinalEventNum; i++)
    	{
    	    TCPDEBUG_PRINT("%d  ", EventType[i]);
    	}
    	TCPDEBUG_PRINT("\n");
    }
	
    // ?????¼???????
    EventTemp->FinalEventNum = FinalEventNum;
    
/************************************* ?ͷŻ????? ************************************************/
	return FinalEventNum;
}

/*
 **************************************************************************************
 *  ???????ƣ? FindProbableEndEvent
 *  ?????????? ʹ??3.5dBΪ??ֵѰ??û???ҵ??ķǷ????????¼?
 *  ???ڲ????? OtdrData  : ????????????
 *             EventTemp : ?????¼?????Ϣ
 *             State     : OTDR״̬??��
 *  ???ز????? ?Ƿ??ҵ?ֱ?????Ϻ????¼?
 *  ???ڰ汾?? 2013-11-6 11:30:51
 **************************************************************************************
*/
Int32 FindProbableEndEvent(OTDR_ChannelData_t *OtdrData, Event_t *EventTemp, OtdrStateVariable_t *State)
{
    Int32   i, X0, X1, M, L, st, et, FinalEventNum, FindEnd = 1, ok = 0;
	Int32   *EventType, *FinalEvent, *FinalEventEnd;
	float   *Ai, v, k, ac;
    
	Ai = OtdrData->Ai;
	FinalEventNum = EventTemp->FinalEventNum;
	EventType     = EventTemp->EventType;
	FinalEvent    = EventTemp->FinalEvent;
	FinalEventEnd = EventTemp->FinalEventEnd;
	
	// ֻ????????��?̱Ƚϴ???ʱ??????
	if((State->MeasureParam.PulseWidth_ns > 2560) && (State->MeasureParam.MeasureLength_m >= 100000))
    {
        // ????????Ϊ?????¼?ĩ?㼰????8km?ĳ???֮??
        X0 = FinalEventEnd[FinalEventNum-1];
        X1 = X0 + State->Points_1m * 8000;
        for(i = X0+1; i <= X1; i++)
        {
            if(Ai[i] < 3.5)
            {
                FindEnd = 0;
                break;
            }
        }
    }
    else        return 0;
    
    if(FindEnd)
    {
        // Ѱ?ҵ???3.5dB?ĵ?
        for(i = X1+1; i < DATA_LEN; i++)
        {
            if(Ai[i] < 3.5)     break;
        }
        if(i > 30000)   return 0;
        X1 = i;
    }
    else        return 0;
    
    // Ѱ???¼?ǰ?㣬????��????С??0.2dB
    GetEventFindingParamBeforeLog(NULL, NULL, &k, &M, "Large", 0);
    st = 0;
    L = 64;
    for(i = X1; i > X0+L; i--)
    {
        v = Ai[i-L] - Ai[i] - k*L;
        if(v < 0.2)
        {
            st = i;
            break;
        }
    }
    // Ѱ???¼?ǰ?㣬??????????С??1????????????????1
    et = 0;
    for(i = X1; i < DATA_LEN; i++)
    {
        v = Ai[i] - Ai[i+1];
        if((Ai[i] < 1) && (v > 1))
        {
            et = i;
            break;
        }
    }
    
    // ?жϸ??¼??Ƿ?????ǰһ???¼?????̫??
    if(st - X0 > 1.5*M)
    {
        GetLaserParam(State->MeasureParam.Lambda_nm, &v, NULL);
        LeastSquareMethod(Ai, X0, st, &ac, NULL);
        ac = -ac * State->Points_1m * 1000;
        if((ac >= v*0.8) && (ac < v*1.2))
        {
            // ȷ?????¼?????????
            FinalEvent[FinalEventNum] = st;
            FinalEventEnd[FinalEventNum] = et;
            EventType[FinalEventNum] = EVENT_TYPE_PROBABLE;//EVENT_TYPE_NONREFLECT;//
            EventTemp->FinalEventNum = FinalEventNum+1;
            ok = 1;
            
            TCPDEBUG_PRINT("FindProbableEndEvent Add Event at %.2fm\n", st/State->Points_1m);
        }
        else    TCPDEBUG_PRINT("FindProbableEndEvent Find Event at %.2fm, but AC = %.3f\n", st/State->Points_1m, ac);
    }
    else
    {
        TCPDEBUG_PRINT("FindProbableEndEvent failed\n");
    }
    return ok;
}

/*
 **************************************************************************************
 *  ???????ƣ? MergeFinalEvents
 *  ?????????? ??��??Ѱ?ҵ????¼????ϲ???��
 *  ???ڲ????? EventTempLarge : ???¼??㣬??FindLargeEventsxxxx????ȷ????ͬʱ?洢?????¼???
 *             EventTempSmall : С?¼??㣬??FindSmallEventsxxxx????ȷ??
 *  ???ز????? FinalEventNum  : ?????¼???????
 *  ???ڰ汾?? 2011-4-19 8:11:30  v1.0
 **************************************************************************************
*/
Int32 MergeFinalEvents(Event_t *EventTempLarge, Event_t *EventTempSmall)
{
    Int32   i, j, temp, M, EventDis;
	Int32   *TotalEvents_L, TotalEventNum_L, FinalEventNum_L = 0, FinalEventNum_S = 0;
	Int32   *EventType_L, *FinalEvent_L, *FinalEventEnd_L;
	Int32   *EventType_S, *FinalEvent_S, *FinalEventEnd_S;

/************************************ ??ȡ???? ***************************************************/
    GetEventFindingParamBeforeLog(NULL, NULL, NULL, &M, "Small", 0);    // С?¼????????¼???֮???ļ???
	EventDis = (Int32)(32 * OtdrState.Points_1m);       //  32?׶?Ӧ?ĵ???????Ϊ??С????    ????????????????
	EventDis = MAX(EventDis, M);
	
/************************************ ????ָ?? ***************************************************/
    // ???¼?????��
    TotalEvents_L   = EventTempLarge->TotalEvent;
	TotalEventNum_L = 0;
	EventType_L     = EventTempLarge->EventType;
	FinalEvent_L    = EventTempLarge->FinalEvent;
	FinalEventEnd_L = EventTempLarge->FinalEventEnd;
	FinalEventNum_L = EventTempLarge->FinalEventNum;
    // С?¼?????��
    EventType_S     = EventTempSmall->EventType;
	FinalEvent_S    = EventTempSmall->FinalEvent;
	FinalEventEnd_S = EventTempSmall->FinalEventEnd;
	FinalEventNum_S = EventTempSmall->FinalEventNum;

/*********************************** ?ϲ????¼?????С?¼??? **************************************/
	for(i = 0; i < FinalEventNum_S; i++)
	{
	    // ?ϲ????��??Ǵ?С?¼????????н??棬??????С?¼??㣬???????ӵ????¼?????
	    if(FinalEvent_S[i] > FinalEventEnd_L[FinalEventNum_L-1]+EventDis)   // ???ڴ??¼?????ĩ??֮?⣬Ϊ???¼???   2011-6-20 16:33:55
	    {
	        // ??Ҫ???ӵ?С?¼?????????ֵ???ŵ? TotalEvents_L ??
	        TotalEvents_L[TotalEventNum_L++] = i;
	    }
	    else        // ?????ڴ??¼?????ĩ??֮?⣬?????Ƿ??????м????¼???֮??
	    {
    	    for(j = 0; j < FinalEventNum_L-1; j++)
    	    {
    	        // ????С?¼??㴦??��?????¼???֮?䣬????һ?????¼??㣬??????λ????ʱ???ӵ? TotalEvents_L ??
    	        if(FinalEvent_S[i] > FinalEventEnd_L[j]+EventDis)   // 2011-6-20 16:34:29
    	        {
    	            if(FinalEventEnd_S[i]+EventDis < FinalEvent_L[j+1])    // 2011-6-8 21:57:45֮??????Ϊ?����??ܲ???????   2011-6-20 16:34:45
        	        {
        	            TotalEvents_L[TotalEventNum_L++] = i;
        	            break;
        	        }
        	    }
        	    else    break;
    	    }
    	}
	}

/********************* ?ٽ????¼????ϲ??????¼????? FinalEvent ?? ********************************/
	if(TotalEventNum_L > 0)
	{
	    for(i = 0; i < TotalEventNum_L; i++)
	    {
	        FinalEvent_L   [FinalEventNum_L] = FinalEvent_S[TotalEvents_L[i]];
	        FinalEventEnd_L[FinalEventNum_L] = FinalEventEnd_S[TotalEvents_L[i]];
	        EventType_L    [FinalEventNum_L] = EventType_S[TotalEvents_L[i]];
	        FinalEventNum_L++;
	        if(FinalEventNum_L >= MAX_EVENT_NUM)     break;
	    }
	    
/************************************* ?ٶ??¼??????????? ****************************************/
	    for(i = 0; i < FinalEventNum_L; i++)
	    {
	        for(j = i+1; j < FinalEventNum_L; j++)
	        {
	            if(FinalEvent_L[i] > FinalEvent_L[j])           // i??jҪ????λ??
	            {
	                temp               = FinalEvent_L[i];       // ?????¼???????
	                FinalEvent_L[i]    = FinalEvent_L[j];
	                FinalEvent_L[j]    = temp;
	                
	                temp               = FinalEventEnd_L[i];    // ?????¼???ĩ??
	                FinalEventEnd_L[i] = FinalEventEnd_L[j];
	                FinalEventEnd_L[j] = temp;
	                
	                temp               = EventType_L[i];        // ?????¼???????
	                EventType_L[i]     = EventType_L[j];
	                EventType_L[j]     = temp;
	            }
	        }
	    }
	    // ???´??¼???????
	    EventTempLarge->FinalEventNum = FinalEventNum_L;
	}
	
	return FinalEventNum_L;
}

/*
 **************************************************************************************************
 *  ????  ???? SetEndEvent
 *  ?????????? ȷ????ʼ?ͽ????¼?
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2013-04-01  16:44:00  v1.0
 **************************************************************************************************
*/
void SetEndEvent(Event_t* EventTemp, OtdrStateVariable_t *State)
{
    Int32 FinalEventNum = EventTemp->FinalEventNum;
    
    State->EndEventIsReflect = 0;
    if(EVENT_TYPE_REFLECT == EventTemp->EventType[FinalEventNum-1])    State->EndEventIsReflect = 1;
    EventTemp->EventType[FinalEventNum-1] = EVENT_TYPE_END;
    EventTemp->EventType[0] = EVENT_TYPE_START;
}

/*
 **************************************************************************************************
 *  ????  ???? MergeGroupEvents
 *  ?????????? ???????յ??¼?
 *  ???ڲ????? 
 *  ???ز????? ?????˵??¼??ĸ???
 *  ???ڰ汾?? 2012-09-25  16:02:53  v1.0
 **************************************************************************************************
*/
Int32 MergeGroupEvents(Event_t *EventTemp, GroupEvent_t *GroupEvent)
{
    Int32 i, j, k, M, X1, X2, X, count, exist, FinalEventNum, EventNum, FuckEventNum;
    Int32 *FinalEvents, *FinalEventsEnd, *EventType;
    
    if(GroupEvent->ValidGroupNum == 0)      return 0;
    
    FinalEvents     = EventTemp->FinalEvent;        // ?¼?????
    FinalEventsEnd  = EventTemp->FinalEventEnd;     // ?¼?ĩ??
    EventType       = EventTemp->EventType;         // ?¼?????
    EventNum        = EventTemp->FinalEventNum;
    exist           = (GroupEvent->ValidGroupNum + 1) >> 1;     // ????????ֵ????Ϊ??????ʵ???ڵ??¼?
    M = PulseWidthInSampleNum();
//    M = MIN(M, 10);
    
    FuckEventNum  = 0;
    FinalEventNum = EventNum;
    for(i = 1; i < EventNum; i++)   // ?????¼?Ϊ??Ҫ?��?
    {
        count = 1;
        if(FinalEvents[i] <= FinalEventsEnd[i-1])   X1 = MAX(FinalEvents[i] - M, (FinalEvents[i] + FinalEvents[i-1])/2);
        else    X1 = MAX(FinalEvents[i] - M, FinalEventsEnd[i-1]);
        X2 = FinalEvents[i]+M;
        
        // ͳ???ж??ٸ??¼????????? [X1, X2]
        for(j = 0; j < GroupEvent->ValidGroupNum; j++)
        {
            for(k = 1; k < GroupEvent->GroupEventArray[j].EventNum; k++)
            {
                X = GroupEvent->GroupEventArray[j].FinalEvents[k];
                
//                if(EventType[i] == GroupEvent->GroupEventArray[j].EventType[k])
                {
                    if(X >= X2)  break;
                    else if(X > X1)
                    {
                        count++;
                        break;
                    }
                }
            }
        }
        
        // ????????һ????��??????Ϊ???¼???ʵ???ڣ????????????˵?
        if(count < exist)
        {
            TCPDEBUG_PRINT("MergeGroupEvents : Event %d count %d\n", FinalEvents[i], count);
            EventType[i]  = EVENT_TYPE_FUCK;
            FuckEventNum++;
        }
    }
    
    // ????????FUCK?¼??????˳?????
    if(FuckEventNum > 0)
    {
        EventNum = KickOutOfArray(EventTemp->EventType, EVENT_TYPE_FUCK, 
                                       EventTemp->FinalEvent, EventTemp->FinalEventEnd, EventNum);

        EventTemp->FinalEventNum = EventNum;
        SetEndEvent(EventTemp, &OtdrState);
    }

    TCPDEBUG_PRINT("MergeGroupEvents : %d-%d\n", FinalEventNum, FuckEventNum);
    return FuckEventNum;
}

/*
 **************************************************************************************************
 *  ????  ???? EventArraySort
 *  ?????????? ?¼???????????
 *  ???ڲ????? EventTemp : Я???¼?????Ϣ??ָ??
 *  ???ز????? 
 *  ???ڰ汾?? 2011-06-21  16:36:19  v1.0
 **************************************************************************************************
*/
// ???????????麯??????bufΪ????a1??a2????һ???䶯
static Int32 EventPartitions(Int32 buf[], Int32 a1[], Int32 a2[], Int32 low, Int32 high)
{
	Int32 p1, p2, PivotKey;

	PivotKey = buf[low];
	p1 = a1[low];
    p2 = a2[low];

	while(low < high)
	{
		// ?Ӻ???ǰ????
		while((buf[high] >= PivotKey) && (low < high))   --high;
		buf[low] = buf[high];
		a1[low] = a1[high];
        a2[low] = a2[high];

		// ??ǰ????????
		while((buf[low] <= PivotKey) && (low < high))    ++low;
		buf[high] = buf[low];
		a1[high] = a1[low];
        a2[high] = a2[low];
	}
	buf[low] = PivotKey;
	a1[low] = p1;
    a2[low] = p2;
	return low;
}
// ?????????ݹ麯??
void EventQsort(Int32 buf[], Int32 a1[], Int32 a2[], Int32 low, Int32 high)
{
	Int32 mid;
	if(low < high)
	{
		mid = EventPartitions(buf, a1, a2, low, high);
		EventQsort(buf, a1, a2, low, mid-1);
		EventQsort(buf, a1, a2, mid+1, high);
	}
}

static Int32 EventArraySort(Event_t *EventTemp)
{
    Int32   i, j, temp, FinalEventNum;
    Int32   MinEventIndex, MinEventStart;
    Int32   *FinalEvent, *FinalEventEnd, *EventType;
    
    FinalEvent    = EventTemp->FinalEvent;
	FinalEventEnd = EventTemp->FinalEventEnd;
	EventType     = EventTemp->EventType;
	FinalEventNum = EventTemp->FinalEventNum;
	
	if(FinalEventNum < 40)
	{
	    // ??ѡ???????ķ????????¼?????????????
        for(i = 0; i < FinalEventNum - 1; i++)
        {
            MinEventIndex = i;
            MinEventStart = FinalEvent[i];
            for(j = i+1; j < EventTemp->FinalEventNum; j++)
            {
                if(FinalEvent[j] < MinEventStart)   // ??λ?ø???ǰ
                {
                    MinEventIndex = j;
                    MinEventStart = FinalEvent[j];
                }
            }
            
            // ??????С?¼??㲻??i???򽻻?????
            if(MinEventIndex != i)
            {
                temp                         = FinalEvent[i];
                FinalEvent[i]                = FinalEvent[MinEventIndex];
                FinalEvent[MinEventIndex]    = temp;
                
                temp                         = FinalEventEnd[i];
                FinalEventEnd[i]             = FinalEventEnd[MinEventIndex];
                FinalEventEnd[MinEventIndex] = temp;
                
                temp                         = EventType[i];
                EventType[i]                 = EventType[MinEventIndex];
                EventType[MinEventIndex]     = temp;
            }
        }
    }
    else    EventQsort(FinalEvent, FinalEventEnd, EventType, 0, FinalEventNum-1);
    
    return 1;
}

/*
 **************************************************************************************************
 *  ????  ???? CrossZerosCount
 *  ?????????? ????????????????ͳ?ƴ????򵽸????Ĵ?Խ?????Ҳ??Ƶ?һ?????ݼ?Ϊ0??????
 *  ???ڲ????? input      : ????????
 *             from       : ????????
 *             to         : ?????յ?
 *             ZerosPoint : ??????λ?ã??̶?fromΪ??1?????????��?
 *  ???ز????? czc        : ??????????
 *  ???ڰ汾?? 2011-06-21  21:15:53  v1.0   2011-7-7 15:37:09 v2.0
 *             ͳ?ƴ?˫???Ĵ?Խ????????ZerosPoint?ṹΪ 0 + - + - ... ????1??ֵΪfrom???????��?Ϊ
 *             ?????????㣬?????????㣬...??????Ҫ?????????ݵĵ?1??ֵinput[from]????Ϊ??
 **************************************************************************************************
*/
static Int32 CrossZerosCount(const Int32 input[], Int32 from, Int32 to, Int32 ZerosPoint[])
{
    Int32 i, LastZeroPoint, TotalNum = 0;
    
    // Ҫ????1??ֵ????Ϊ??
    if(input[from] >= 0)    return 1;
    
/*************************************** ??ʼѰ?ҹ????? ******************************************/
    ZerosPoint[TotalNum++] = from;
    LastZeroPoint = 1;  // ָ?����????ӵ??Ǹ?????????
    for(i = from; i < to; i++)
    {
        if((input[i] < 0) && (input[i+1] >= 0))         // ???????ϴ?????????????
        {
            // ?????ϴ????ӵ????????????㣬?????????ϴ??????????㵽???ڻ?û?и????????㣬
            // ??????????һ??????????????Ϊ?????????㣬Ȼ???????ӱ?????????????
            if(0 == LastZeroPoint)
            {
                ZerosPoint[TotalNum] = ZerosPoint[TotalNum-1];
                TotalNum++;
            }
            
            ZerosPoint[TotalNum++] = i;
            LastZeroPoint = 0;
        }
        else if((input[i] > 0) && (input[i+1] <= 0))    // ???????´?????????????
        {
            // ?????ϴ????ӵ??Ǹ????????㣬?????????ϴθ????????㵽???ڻ?û???????????㣬
            // ??????????һ??????????????Ϊ?????????㣬Ȼ???????ӱ??θ?????????
            if(1 == LastZeroPoint)
            {
                ZerosPoint[TotalNum] = ZerosPoint[TotalNum-1];
                TotalNum++;
            }
            
            ZerosPoint[TotalNum++] = i;
            LastZeroPoint = 1;
        }
    }

	// ʵ?ʶ?Ӧ???????????????͸?????????????ͬ????????ͬ????ȥ???????Ǹ???????
    return (TotalNum & 1) ? (TotalNum>>1) : ((TotalNum-1) >> 1);
}

/*
 **************************************************************************************************
 *  ????  ???? SplitThreshold
 *  ?????????? ?¼??ָ?????ֵ
 *  ???ڲ????? ??dB???Ƶ?????ֵ
 *  ???ز????? ??dB???Ƶĸ?????
 *  ???ڰ汾?? 2013-8-5 15:17:59
 **************************************************************************************************
*/
#define USE_DYNAMIC_SPLIT   1

#define BASE_THRESHOLD      0.8     // ????????ֵ
#define STA1    15          // ??????ֵΪ STA1(dB) ʱ??ʼʹ????ֵ
#define STG1    1.2         // ??????ֵΪ STA1(dB) ʱ????ֵ??��
#define STA2    5           // ??????ֵΪ STA2(dB) ʱ????ʹ????ֵ
#define STG2    2.4         // ??????ֵΪ STA2(dB) ʱ????ֵ??��
#define ST_K    ((STG2 - STG1) / (STA2 - STA1))

#if USE_DYNAMIC_SPLIT
// ʹ?????Բ?ֵ
static float SplitThreshold_Linear(float Ai)
{
    float gain, b = BASE_THRESHOLD;
    
    if((Ai >= STA1) || (Ai <= STA2))        gain = 0.0;
    else            gain = (Ai - STA1) * ST_K + STG1;
    
    if(OtdrState.MeasureParam.PulseWidth_ns < 10)   b = 1.2;
    return (b + gain);
}

static float ST_Exp_C, ST_Exp_K;
// ʹ??ָ????ֵ
static float SplitThreshold_Exp(float Ai)
{
    float gain, b = BASE_THRESHOLD;
    
    if((Ai >= STA1) || (Ai <= STA2))        gain = 0.0;
    else            gain = ST_Exp_C * Tenth(ST_Exp_K * Ai);
    
    if(OtdrState.MeasureParam.PulseWidth_ns < 10)   b = 1.2;
    return (b + gain);
}
    
#endif

/*
 **************************************************************************************************
 *  ????  ???? SplitFinalEvents
 *  ?????????? ?????¼??㻮?֣?Ѱ?Ҹ?ϸ???¼???
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2011-06-20  21:27:49  v1.0
 **************************************************************************************************
*/
Int32 SplitFinalEvents(float *AiLog, Int32 DataLen, Event_t *EventTemp)
{
    extern void MinValueEQ(const void *input, int from, int to, void *MV, int *pos, int type);
    Int32   i, j, k, M, EventWidth, temp, czc, s, GetsEvent, SatEnd, EndAndSat = 0;
	Int32   EventStart, EventEnd, FinalEventNum, EventNum, EnOld, NewEventNum = 0;
	Int32   *AnLog, *Delta, *An, *Dn;
	Int32   *FinalEvent, *FinalEventEnd, *EventType, *NewEvent, *ZerosPoint;
    float   *Ai, v;

/********************************** ????AnLog??Delta *********************************************/
    AnLog = NULL;
    AnLog = (Int32*)malloc(DataLen*sizeof(Int32));
	if(NULL == AnLog)
	{
		TCPDEBUG_PRINT("AnLog alloc fail in FindSmallEventsAfterLog\n");
		return 0;
	}
	
	Delta = NULL;
	Delta = (Int32*)malloc(DataLen*sizeof(Int32));
	if(NULL == Delta)
	{
		TCPDEBUG_PRINT("Delta alloc fail in FindSmallEventsAfterLog\n");
		return 0;
	}

/************************ ??ȡ???????Ŵ? FLOAT_ENLARGE_FACTOR ?? ***************************************/
	GetEventFindingParamBeforeLog(NULL, NULL, NULL, &M, "Large", 0);
	if(OtdrState.MeasureParam.PulseWidth_ns < 20)   M *= 2;      // 2014-2-28 16:09:06   ??Ϊ10ns????????ƽ???ķ?ֵ?϶?????һ???¼?????????????С??20ns?Ŀ???
	M = MAX(M, 4);      // 2011-11-29 9:22:28
	M = MIN(M, 128);    // 2012-12-13 15:31:49

/******************* ?????????߸???ֵ???? FLOAT_ENLARGE_FACTOR ??ȡ?? **********************************/
	An = AnLog;
	Ai = AiLog;
	for(i = 0; i < DataLen; i++)
	{
	    An[i] = (Int32)(Ai[i] * FLOAT_ENLARGE_FACTOR);
	}
	k = (Int32)(OtdrState.MeasureParam.EndThreshold * FLOAT_ENLARGE_FACTOR);
	for(i = 0; i < DataLen; i++)
	{
	    if(An[i] < k)      An[i] = k;
	}
	
#if USE_DYNAMIC_SPLIT
    // ??ʼ?? ST_Exp_C, ST_Exp_K
    v = STG1 / STG2;
    v = FastLog10(v);
    ST_Exp_K = v / (STA1 - STA2);
    
    v = Tenth(ST_Exp_K * STA1);
    ST_Exp_C = STG1 / v;
#endif
/********************************** ?????¼??? ***************************************************/
	FinalEvent    = EventTemp->FinalEvent;
	FinalEventEnd = EventTemp->FinalEventEnd;
	EventType     = EventTemp->EventType;
	FinalEventNum = EventTemp->FinalEventNum;
	EnOld         = FinalEventNum;
	
	NewEvent      = EventTemp->TotalEvent;
    ZerosPoint    = EventTemp->ReflectEvent;
#if !USE_DYNAMIC_SPLIT
	for(EventNum = 0; EventNum < EventTemp->FinalEventNum; EventNum++)
	{
	    // ͨ???????????ҵ????¼???????
        if(EventType[EventNum] == EVENT_TYPE_PROBABLE)    continue;
	    
	    EndAndSat  = 0;
	    EventStart = FinalEvent[EventNum];
	    EventEnd   = FinalEventEnd[EventNum];

	    // ????????һ??ƽ???¼?????Ȼ?ָ??ֻ???���ƽ?????????�����????β
        if(EventNum == EventTemp->FinalEventNum-1)
        {
    	    for(i = EventStart; i <= EventEnd; i++)
            {
                if(OtdrData.ChanData[i] >= OtdrState.SatThreshold)
                {
                    // Ѱ??ƽ??????
                    Int32 temp = OtdrState.SatThreshold * Tenth_div5(-0.1);
                    
                    for(j = EventEnd; j > i; j--)    // Ѱ??ƽ??????
                    {
                        if(OtdrData.ChanData[j] >= temp)   break;
                    }
                    SatEnd = j;
                    EndAndSat = 1;
                
                    // ????ǰ?¼?ĩ?㱣?浽SatEnd
                    temp = EventEnd;
                    EventEnd = SatEnd;
                    SatEnd = temp;
                }
            }
        }

	    NewEventNum = 0;
	    EventWidth = EventEnd - EventStart + 1;
	    if(EventWidth <= M)      continue;      // ????̫С?򲻿??ܴ??ڶ????¼???
	    if(EventStart >= OtdrState.MeasureLengthPoint)    break;
	    
/**************************** ??????��???? *******************************************************/
        // Delta(n) = AnLog(n) ?C AnLog(n+M)
    	Dn = Delta;
    	for(i = EventStart; i <= EventEnd-M; i++)
    	{
    		*Dn++ = (Int32)(An[i] - An[i+M]);
    	}
    	for(i = EventEnd-M+1; i <= EventEnd; i++)
    	{
    		*Dn++ = 0;
    	}

/**************************** ??????��??????????????̬ *******************************************/
        czc = CrossZerosCount(Delta, 0, EventWidth-1, ZerosPoint);
        if(czc > 1)     // ????2?????Ϲ?????
        {
            // ??Ϊ???????¼???ĩ?㵽ZerosPoint??????
            ZerosPoint[2*czc+1] = EventWidth-1;
            
            s = EventStart + ZerosPoint[0];
            
            // ????ÿ?????????Ƿ????ڲ????? 1.5dB ??????
            for(i = 1; i <= czc; i++)
            {
                GetsEvent = 0;
                
                // ??��????????????֮????????????
                for(j = ZerosPoint[2*i-2]; j <= ZerosPoint[2*i]; j++)
                {
                    if(abs(Delta[j]) >= (BASE_THRESHOLD * FLOAT_ENLARGE_FACTOR))  // ?Բ??? 0.8dB Ϊ????
                    {
                        GetsEvent = 1;
                        
                        // Ѱ???¼????????㣬?ӵ?ǰ?????????㴦??ʼ????һ????????????????ƽ̹????
                        for(k = EventStart + ZerosPoint[2*i]; k <= EventStart + ZerosPoint[2*i+1]; k++)
                        {
                            if((An[k-1] + KD*An[k+1]) > (1+KD)*An[k])      break;
                        }
                        if(k > EventEnd)    k = EventEnd;
                        
                        if(k-s >= 2)                         // ????Ҫ??2???? 2011-7-14 11:51:31
                        {
                            NewEvent[2*NewEventNum+0] = s;      // ?Դ?Ϊ?¼?????????
                            NewEvent[2*NewEventNum+1] = k;      // ?Դ?Ϊ?¼?????ĩ??
                            NewEventNum++;
                        }
                        
                        break;
                    }
                }
                
                // ??????һ???¼????????㣬?ӵ?ǰ?????????㴦??ʼ????һ????????????????ƽ̹????
                if(i < czc)
                {
                    // ?????Ѿ?ȷ???¼??㣬??kֵ??Ч????????Ч
                    s = GetsEvent ? k : (EventStart + ZerosPoint[2*i]);
                    MinValue(An, s, EventStart + ZerosPoint[2*i+1], NULL, &s, DATA_TYPE_INT);
                }
            }
        }
        
/****************************** ???ݽ????????¼??????? *******************************************/
        if(NewEventNum > 1)
        {
            FinalEvent   [EventNum] = NewEvent[0];
            FinalEventEnd[EventNum] = NewEvent[1];
            
            for(i = 1; i < NewEventNum; i++)
            {
                FinalEvent   [FinalEventNum] = NewEvent[2*i+0];
                FinalEventEnd[FinalEventNum] = NewEvent[2*i+1];
                EventType    [FinalEventNum] = EventType[EventNum];
                FinalEventNum++;
            }
            if(EndAndSat)   FinalEventEnd[FinalEventNum-1] = SatEnd;
        }
    }
#else   // USE_DYNAMIC_SPLIT ʹ?ö?̬?仯????ֵ
    for(EventNum = 0; EventNum < EventTemp->FinalEventNum; EventNum++)
	{
	    EventStart = FinalEvent[EventNum];
	    EventEnd   = FinalEventEnd[EventNum];

	    NewEventNum = 0;
	    EventWidth = EventEnd - EventStart + 1;
	    if(EventWidth <= M)      continue;      // ????̫С?򲻿??ܴ??ڶ????¼???
	    if(EventStart >= OtdrState.MeasureLengthPoint)    break;
	    
/**************************** ??????��???? *******************************************************/
        // Delta(n) = AnLog(n) ?C AnLog(n+M)
    	Dn = Delta;
    	for(i = EventStart; i <= EventEnd-M; i++)
    	{
    		*Dn++ = (Int32)(An[i] - An[i+M]);
    	}
    	for(i = EventEnd-M+1; i <= EventEnd; i++)
    	{
    		*Dn++ = 0;
    	}
    	
    	// 2014-2-28 17:40:52 ????��????̫С???޷?
    	temp = BASE_THRESHOLD * FLOAT_ENLARGE_FACTOR;
    	Dn = Delta;
    	for(i = 0; i < EventWidth; i++)
    	{
    	    if((Delta[i] > 0) && (Delta[i] < temp))     Delta[i] = temp;
    		if((Delta[i] < 0) && (Delta[i] > -temp))     Delta[i] = -temp;
//    		if(abs(Delta[i]) < temp)     Delta[i] = 0;
    	}
    	
/**************************** ??????��??????????????̬ *******************************************/
        czc = CrossZerosCount(Delta, 0, EventWidth-1, ZerosPoint);
        if(czc > 1)     // ????2?????Ϲ?????
        {
            // ??Ϊ???????¼???ĩ?㵽ZerosPoint??????
            ZerosPoint[2*czc+1] = EventWidth-1;
            
            s = EventStart + ZerosPoint[0];
            
            // ????ÿ?????????Ƿ????ڲ????? 1.5dB ??????
            for(i = 1; i <= czc; i++)
            {
                GetsEvent = 0;
                
                // ??��????????????֮????????????
//                TCPDEBUG_PRINT("Event %d Splitting, Threshold as below:\n", EventNum);
                for(j = ZerosPoint[2*i-2]; j <= ZerosPoint[2*i]; j++)
                {
                    // ʹ?ñ仯????ֵ???涯̬?½???????
                    v = SplitThreshold_Linear(Ai[EventStart + j]);
//                    TCPDEBUG_PRINT("%.2f ", v);
                    if(abs(Delta[j]) > (v * FLOAT_ENLARGE_FACTOR))  // ?Բ??? 0.8dB Ϊ????
                    {
                        GetsEvent = 1;
                        
                        // Ѱ???¼????????㣬?ӵ?ǰ?????????㴦??ʼ????һ????????????????ƽ̹????
                        for(k = EventStart + ZerosPoint[2*i]; k <= EventStart + ZerosPoint[2*i+1]; k++)
                        {
                            if((An[k-1] + KD*An[k+1]) > (1+KD)*An[k])      break;
                        }
                        if(k > EventEnd)    k = EventEnd;
                        
                        if(k-s >= 2)                         // ????Ҫ??2???? 2011-7-14 11:51:31
                        {
                            NewEvent[2*NewEventNum+0] = s;      // ?Դ?Ϊ?¼?????????
                            NewEvent[2*NewEventNum+1] = k;      // ?Դ?Ϊ?¼?????ĩ??
                            NewEventNum++;
                        }
                        
                        break;
                    }
                }
//                TCPDEBUG_PRINT("\n");
                
                // ??????һ???¼????????㣬?ӵ?ǰ?????????㴦??ʼ????һ????????????????ƽ̹????
                if(i < czc)
                {
                    // ?????Ѿ?ȷ???¼??㣬??kֵ??Ч????????Ч
                    s = GetsEvent ? k : (EventStart + ZerosPoint[2*i]);
                    MinValueEQ(An, s, EventStart + ZerosPoint[2*i+1], NULL, &s, DATA_TYPE_INT);
                }
            }
        }
        
/****************************** ???ݽ????????¼??????? *******************************************/
        if(NewEventNum > 1)
        {
            FinalEvent   [EventNum] = NewEvent[0];
            FinalEventEnd[EventNum] = NewEvent[1];
            
            for(i = 1; i < NewEventNum; i++)
            {
                FinalEvent   [FinalEventNum] = NewEvent[2*i+0];
                FinalEventEnd[FinalEventNum] = NewEvent[2*i+1];
                EventType    [FinalEventNum] = EventType[EventNum];
                
                TCPDEBUG_PRINT("new event (at %d) added while Split event %d\n", FinalEvent[FinalEventNum], FinalEvent[EventNum]);
                FinalEventNum++;
            }
        }
    }
#endif
    if(FinalEventNum > EventTemp->FinalEventNum)
    {
        EventTemp->FinalEventNum = FinalEventNum;
        EventArraySort(EventTemp);
    }
/******************************** ?ͷ??ڴ? *******************************************************/
	TCPDEBUG_PRINT("After SplitFinalEvents, FinalEventNum = %d, Add %d\n", FinalEventNum, FinalEventNum-EnOld);
	free(AnLog);
	free(Delta);
	
	return 1;
}

/*
 **************************************************************************************
 *  ???????ƣ? FindSaturateEvents       CountSaturateEventsDuration
 *  ?????????? Ѱ?ұ???ƽ???¼?????????λ??     ͳ??һ??ƽ???ĳ???????
 *  ???ڲ????? input        : ???????ݣ?ȡ????ǰ
 *             EventTemp    : ?????¼?????Ϣ???????汥??ƽ???¼?????Ϣ
 *             SatThreshold : ????ƽ????ֵ
 *  ???ز????? SatEventNum  : ʵ?????����?ƽ???¼???????
 *  ???ڰ汾?? 2011-03-09  17:02:50  v1.0
 **************************************************************************************
*/
Int32 FindSaturateEvents(const Int32 input[], Event_t* EventTemp, Int32 SatThreshold)
{
    Int32   *SaturateEvent, *FinalEvent, *FinalEventEnd, SatEventNum = 0;
    Int32   i, n;

/************************************** ??��??ʼ?? ***********************************************/
    SatEventNum = 0;
    SaturateEvent = EventTemp->SaturateEvent;
    FinalEvent    = EventTemp->FinalEvent;
    FinalEventEnd = EventTemp->FinalEventEnd;

/**************** ?Ƚ?һ???¼?????????ĩ??֮???Ƿ????ڲ?????SatThreshold??ֵ *********************/
    for(n = 0; n < EventTemp->FinalEventNum; n++)
    {
        for(i = FinalEvent[n]; i <= FinalEventEnd[n]; i++)
        {
            if(input[i] >= SatThreshold)
            {
                SaturateEvent[SatEventNum++] = n;
                break;
            }
        }
    }
    EventTemp->SatEventNum = SatEventNum;

    return SatEventNum;
}

Int32 CountSaturateEventsDuration(const Int32 input[], Event_t* EventTemp, Int32 ei)
{
    Int32   *SaturateEvent, SatEventNum;
    Int32   i, n, from, to, SatThreshold, exist = 0;

/************************************** ??��??ʼ?? ***********************************************/
    SaturateEvent = EventTemp->SaturateEvent;
    SatEventNum   = EventTemp->SatEventNum;

    // ?ж??¼?ei?Ƿ???ƽ???¼?
    for(n = 0; n < SatEventNum; n++)
    {
        if(SaturateEvent[n] == ei)
        {
            exist = 1;
            break;
        }
    }
    if(!exist)      return 0;       // ????????ƽ???¼????򷵻?0
    
/**************** ?Ƚ?һ???¼?????????ĩ??֮???Ƿ????ڲ?????SatThreshold??ֵ *********************/
    MaxValue(input, EventTemp->FinalEvent[ei], EventTemp->FinalEventEnd[ei], &SatThreshold, &i, DATA_TYPE_INT);
    SatThreshold *= 0.98;
    
    from = i-1;   // ȷ????ʼ??
    for(n = i-1; n > OtdrState.M; n--)
    {
        if(input[n] <= SatThreshold)
        {
            from = n;
            break;
        }
    }
    to = i+1; // ȷ????????
    for(n = i+1; n < OtdrState.MeasureLengthPoint; n++)
    {
        if(input[n] <= SatThreshold)
        {
            to = n;
            break;
        }
    }
    
    return (to-from);
}

/*
 **************************************************************************************************
 *  ????  ???? GetCurveSaturatePoint
 *  ?????????? ??ȡ???߱??͵㡣ֻ?????߹??????ߣ??͹??????߲?????
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2011-08-22  21:36:12  v1.0
 **************************************************************************************************
*/
Int32 GetCurveSaturatePoint(const OTDR_ChannelData_t *OtdrData, Int32 DataLen)
{
    const Int32 *An;
    Int32   i, j, M, SatNum, sigma3, temp, Threshold, TailEventNum, TailEndPoint;
	Int32   maxcount, count = 0;
	Int32   LastSaturate, SaturatePos, ConsecutiveSat;
	
/************************************** ??��??ʼ?? ***********************************************/
	SatNum = 0;
	OtdrState.OtdrCurveSaturate.SatNum = 0;
	for(i = 0; i < MAX_SAT_POINTS; i++)
	{
	    OtdrState.OtdrCurveSaturate.SatStart[i]  = DataLen;
	    OtdrState.OtdrCurveSaturate.TailStart[i] = DataLen;
	    OtdrState.OtdrCurveSaturate.TailEnd[i]   = DataLen;
	}
	
	// ƽ????ֵ
	An        = OtdrData->ChanData;
	Threshold = OtdrState.SatThreshold;
	M         = OtdrState.M;
	maxcount  = MAX(M, 5);
	
	// ??β???ȶ?Ӧ?Ĳ???????
	TailEventNum = (Int32)(OtdrState.Points_1m*OtdrState.TailLength);
	sigma3       = 3*OtdrState.sigma;

/************************************** ??ʼѰ??ƽ?????͵? ***************************************/
	LastSaturate = 0;
	SaturatePos  = 0;
	i = OtdrState.CurveConcatPoint;    // ??Ϊƴ?ӵ?ǰ?ĵ͹??????߲???ƽ??
    while(i < DataLen-NOISE_LEN)
    {
        if(An[i] >= Threshold)
        {
            // Ѱ???½???Threshold?ĵ?
            for(j = i+1; j < DataLen-10*M; j++)
            {
                if(An[j] < Threshold)  break;
            }
            MaxValue(An, i, j, &temp, NULL, DATA_TYPE_INT);
            count = GetCount(An, i, j, temp, 1);
            
            if(count >= maxcount)
            {
                LastSaturate = 1;
                SaturatePos  = j;
                
                // ????ƽ????ʼ??
                OtdrState.OtdrCurveSaturate.SatStart[SatNum] = i;
            }
            i = j;
        }
        else
        {
            if(LastSaturate == 1)
            {
    			LastSaturate = 0;
    			OtdrState.OtdrCurveSaturate.TailStart[SatNum] = SaturatePos;
    		
    		#if 0   // ?÷???Ѱ???Ƿ???��????ƽ?????????У?????��??ƽ???????͵?Ϊ??β???????????Թ̶?????β????Ϊ????
    		        // ?÷??????Ǻܺã???Ϊ???Ḳ????β?ϵķ?ƽ???????¼?
    			// ?ȼ???????һ?����͵?֮????TailEventNum???????Ƿ???????һ?����?
                ConsecutiveSat = 0;
                for(j = i+1; j <= TailEventNum + SaturatePos; j++)
                {
                    if(j >= DataLen)     break;
                    if(An[j] >= Threshold)
                    {
                        ConsecutiveSat = 1;
                        break;
                    }
                }

                // ?÷???Ѱ??��????��????β֮???????͵㣬????Ϊ��??ƽ????ǰһ??β??????
                // ????û??��??ƽ??????????β????Ϊ????β??????
                if(ConsecutiveSat)   // ??��??ƽ??
                {
                    MinValue(An, SaturatePos, j, NULL, &TailEndPoint, DATA_TYPE_INT);
                }
                else
                {
                    TailEndPoint = MAX(TailEndPoint, TailEventNum + SaturatePos);
                    TailEndPoint = MIN(TailEndPoint, DataLen-NOISE_LEN-M);
                }
            
            #else   // ?÷?????Ѱ????β?????ϵĹյ㣨ת?۵㣩??Ϊ??β??????
                TailEndPoint = MIN(TailEventNum + SaturatePos, DataLen - NOISE_LEN);
                for(j = i+1+M; j <= TailEndPoint; j++)
                {
                    if(An[j] <= sigma3)
                    {
                        TailEndPoint = j;
                        break;
                    }
                    
                    // ??Ϊ?????0.88dB(1.5??)??Ϊ?յ?
                    if(An[j] >= (An[j-M]*1.5))
                    {
                        MinValue(An, SaturatePos, j, NULL, &TailEndPoint, DATA_TYPE_INT);
                        break;
                    }
                }
            #endif
                OtdrState.OtdrCurveSaturate.TailEnd[SatNum++] = TailEndPoint;
                if(SatNum >= MAX_SAT_POINTS)    break;
                i = TailEndPoint;
            }
        }
        
        i++;
    }
    
    OtdrState.OtdrCurveSaturate.SatNum = SatNum;
    
    if(SatNum > 0)
    {
        Int32   ts, te;
        float   mpp = 1/OtdrState.Points_1m;
        TCPDEBUG_PRINT("SatPoint : ");
        for(i = 0; i < SatNum; i++)
        {
            ts = OtdrState.OtdrCurveSaturate.SatStart[i];
            te = OtdrState.OtdrCurveSaturate.TailEnd[i];
            TCPDEBUG_PRINT("{%d(%.2fm), %d(%.2fm)}  ", ts, ts*mpp, te, te*mpp);
        }
        TCPDEBUG_PRINT("\n");
    }
    return SatNum;
}

/*
 **************************************************************************************
 *  ???????ƣ? FindEndEventPoint
 *  ?????????? ?����????¼??㡣ֻ?????߹???????
 *  ???ڲ????? input       : ???????ݣ?ȡ????ǰ
 *             EventTemp   : ?????¼?????Ϣ?????????????¼?????Ϣ
 *             EndThreshod : ?û??趨?Ľ??????ޣ?Ŀǰ???ã?
 *  ???ز?????
 *  ???ڰ汾?? 2011-03-09  17:27:58  v1.0
 **************************************************************************************
*/
void FindEndEventPoint(OTDR_ChannelData_t *OtdrData, Event_t* EventTemp, OtdrStateVariable_t *State)
{
    Int32   i, j, n, m, si, X1, X2, Evt2Peak, Evt3Peak, hitsGhost = 0;
    Int32   SatEventNum, FinalEventNum, GhostEventNum;
    Int32   AnEvt, XSat, XEvt, XEvt2, XTemp, EndThreshold;
    Int32   *An, *hitsGhostX;
    float   k;
    
    An            = OtdrData->ChanData;
    GhostEventNum = 0;
    SatEventNum   = EventTemp->SatEventNum;
    FinalEventNum = EventTemp->FinalEventNum;
    hitsGhostX    = EventTemp->TotalEvent;  // ???øõ???��???????????η??????¼???????
    
    m = PulseWidthInSampleNum();
    
    EndThreshold = State->sigma * Tenth_div5(State->MeasureParam.EndThreshold);
    
    State->EndEventIsReflect = 0;
    if((SatEventNum > 0) && (EventTemp->SaturateEvent[SatEventNum-1] >= 1)) // ֻ???ڴ??ڱ????¼??Ž??к????ļ???
    {
        // ???㱥?ʹ??Ĳ???
        j = SatEventNum-1;
__GHOST_AGAIN__:
        si    = EventTemp->SaturateEvent[j];
        XSat  = EventTemp->FinalEvent[si];
        
        // ??ʶ??ƽ???¼????????????¼???δ?????????ڸ?ƽ???¼??Ķ??η???
        hitsGhost = 0;
        for(n = FinalEventNum-1; n > si; n--)   hitsGhostX[n] = 0;

        // ??????һ???¼?????ǰ??????ƽ???¼?
        for(n = FinalEventNum-1; n > si; n--)
        {
            // ֻ?з????¼?????Ҫ????
            if(EventTemp->EventType[n] != EVENT_TYPE_REFLECT)       continue;
            
            XEvt  = EventTemp->FinalEvent[n];
            X1    = XEvt - m;
            X1    = MAX(X1, EventTemp->FinalEventEnd[n-1]);
            X2    = XEvt;
            MeanValue(An, X1, X2, &AnEvt, DATA_TYPE_INT);
            
            // ???????¼????ڽ??????ޣ????????ǹ?Ӱ??????????????ϵ
            if(AnEvt < EndThreshold)
            {
                k = (float)XEvt / XSat;
                if((k > 1.95) && (k < 2.05))    // ????2????????ϵ
                {
                    if(!hitsGhost)
                    {
                        hitsGhost = 1;  // һ??ƽ???¼?ֻ?ܲ???һ?????η???
                        EventTemp->EventType[n] = EVENT_TYPE_GHOST;
                        GhostEventNum++;
                    }
                }
                else    // ???벻???㣬?????Ƿ???ƽ????һ???¼????Ķ??η???
                {
                    if(n > si + 1)
                    {
                        XEvt2 = (XSat + XEvt) / 2;
                        for(i = n-1; i > si; i--)
                        {
                            if(EventTemp->EventType[i] != EVENT_TYPE_REFLECT)       continue;
                            if(hitsGhostX[i])       continue;
                            
                            XTemp = EventTemp->FinalEvent[i];
                            if((XTemp > 0.97*XEvt2) && (XTemp < 1.03*XEvt2))    // ????2?η?????ϵ???¼???
                            {
                                // ???????????ͣ????Ҹ??¼????ڽ??????ޣ?????Ϊ?ǹ?Ӱ
                                MaxValue(An, EventTemp->FinalEvent[i], EventTemp->FinalEventEnd[i], &Evt2Peak, NULL, DATA_TYPE_INT);
                                MaxValue(An, EventTemp->FinalEvent[n], EventTemp->FinalEventEnd[n], &Evt3Peak, NULL, DATA_TYPE_INT);
                                if(Evt3Peak < Evt2Peak)
                                {
                                    hitsGhostX[i] = 1;
                                    EventTemp->EventType[n] = EVENT_TYPE_GHOST;
                                    GhostEventNum++;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        // ????????��??ƽ???¼?
        if((j > 0) && (SatEventNum-j < 2))
        {
            j--;    // ?˵?ǰһ??ƽ???¼?
            if(EventTemp->SaturateEvent[j] >= 1)    goto __GHOST_AGAIN__;   // ???????¼???Ȼ??????ʼ?¼??Ļ?
        }
        
        // ???????ڹ?Ӱ?????˳?????
        if(GhostEventNum > 0)
        {
            FinalEventNum = KickOutOfArray(EventTemp->EventType, EVENT_TYPE_GHOST, 
                                           EventTemp->FinalEvent, EventTemp->FinalEventEnd, FinalEventNum);
            EventTemp->FinalEventNum = FinalEventNum;
        }
    }
    
    TCPDEBUG_PRINT("After FindEndEventPoint, FinalEventNum = %d\n", FinalEventNum);
    SetEndEvent(EventTemp, State);
}

/*
 **************************************************************************************************
 *  ????  ???? RemoveTailNonReflexEventPoint
 *  ?????????? ??????һ??ƽ???¼????????????????¼?
 *             ????ĳһ?¼??ǷǷ????¼?????????ƽ???ľ???С??5km????????ֵλ??5dB ~ 12dB֮??
 *             ??????ǰ??????????˥?????????????ģ????????𳬹?3dB??????Ϊ???¼?????ʵ???ڵģ?
 *             ??????????ǰ??????һ?¼??????ľ??루????ǰ???룩???ͺ???????һ?¼?ǰ???ľ??루???ƺ????룩
 *             ??????��??????֮һС??8M??????Ϊ???¼????󱨵ģ??????˵?
 *  ???ڲ????? OtdrData    : ????ͨ?????ݣ?????ԭʼ??????????????
 *             EventTemp   : ?????¼?????Ϣ?????????????¼?????Ϣ
 *  ???ز????? 
 *  ???ڰ汾?? 2011-07-26  15:27:31  v1.0
 **************************************************************************************************
*/
int remove_non = 0;
void RemoveTailNonReflexEventPoint(OTDR_ChannelData_t *OtdrData, Event_t* EventTemp)
{
    Int32   m, dl, dr, N5km, EventDis;
    Int32   smax, ei, FinalEventNum, SatEventNum, Fuck = 0;
    Int32   *An, *FinalEvent, *FinalEventEnd, *EventType;
	float	il, *Ai;

	if(remove_non)	return;
	
    // ????100km??????��?̽???
    if(OtdrState.MeasureParam.MeasureLength_m >= 100000)    return;
    
    FinalEvent    = EventTemp->FinalEvent;
    FinalEventEnd = EventTemp->FinalEventEnd;
    FinalEventNum = EventTemp->FinalEventNum;
    EventType     = EventTemp->EventType;
	SatEventNum   = EventTemp->SatEventNum;
    
    An            = OtdrData->ChanData;
	Ai            = OtdrData->Ai;
    m             = PulseWidthInSampleNum();
    N5km          = (Int32)(OtdrState.Points_1m * 5000);
    EventDis      = 8*m;
    
	if(SatEventNum == 0)        // 2016-4-13 10:27:31 ????û??ƽ???????ٴ???
    {
        TCPDEBUG_PRINT("RemoveTailNonReflexEventPoint : No SatEvent, do NOTHING\n");
        return;
    }
	
	smax = EventTemp->SaturateEvent[SatEventNum-1];
	if(smax == FinalEventNum - 1)		// 2016-4-13 10:37:31 ????????һ??ƽ????????һ???¼????????ٴ???
	{
		TCPDEBUG_PRINT("RemoveTailNonReflexEventPoint : LastEvent is SatEvent\n");
		return;
	}
	
    for(ei = smax+1; ei < FinalEventNum; ei++)
    {
        // ???????ڷǷ????¼????????봦??????????
        if(EVENT_TYPE_NONREFLECT == EventType[ei])
        {
			if(FinalEvent[ei] < FinalEvent[smax]+N5km)  // ??ƽ???ľ???С??5km
			{
				if(Ai[FinalEvent[ei]] < 5.0)
				{
					EventType[ei] = EVENT_TYPE_FUCK;
					Fuck++;
				}
				else if(Ai[FinalEvent[ei]] < 12.0)
				{
					il = Ai[FinalEvent[ei]] - Ai[FinalEventEnd[ei]];
					if(ei == FinalEventNum-1){	// ????һ???Ƿ????¼???Ҫ??????????
						if(il <= 3.0){
							EventType[ei] = EVENT_TYPE_FUCK;
							Fuck++;
						}
					}
					else{	// ?м??ķǷ????¼???Ҫ???????빻??
						dl = FinalEvent[ei] - FinalEventEnd[ei-1];
						dr = FinalEvent[ei+1] - FinalEventEnd[ei];
						if((dl < EventDis) || (dr < EventDis)){		// ??????��??????֮һС??8M??????Ϊ???¼????󱨵ģ??????˵?
							EventType[ei] = EVENT_TYPE_FUCK;
							Fuck++;
						}
					}
				}
			}
		}
    }
    
    // ????????FUCK?????˳?????
    if(Fuck> 0)
    {
        FinalEventNum = KickOutOfArray(EventTemp->EventType, EVENT_TYPE_FUCK, 
                                       EventTemp->FinalEvent, EventTemp->FinalEventEnd, FinalEventNum);

        EventTemp->FinalEventNum = FinalEventNum;
    }

    TCPDEBUG_PRINT("RemoveTailNonReflexEventPoint : %d\n", Fuck);
}

/*
 **************************************************************************************************
 *  ????  ???? RemoveConcatPointEventPoint
 *  ?????????? ȥ??ƴ?ӵ㴦???¼???
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2011-11-01  20:55:06  v1.0
 **************************************************************************************************
*/
void RemoveConcatPointEventPoint(const OTDR_ChannelData_t *OtdrData, Event_t* EventTemp)
{
    Int32   i, left, right;
    Int32   FinalEventNum, FuckEventNum;
    Int32   *FinalEvent, *FinalEventEnd, *EventType;
    
    if(!OtdrCtrl.CurveConcat)    return;
    if(NR_MODE_AUTO == OtdrCtrl.NonReflectThreMode)     return;
    
    // ????????ƴ?ӵ????Ҹ?300?׵ķ?Χ
    i     = (Int32)(300 * OtdrState.Points_1m);
    left  = OtdrState.CurveConcatPoint - i;
    right = OtdrState.CurveConcatPoint + i;

    FinalEvent    = EventTemp->FinalEvent;
    FinalEventEnd = EventTemp->FinalEventEnd;
    EventType     = EventTemp->EventType;
    FuckEventNum  = 0;
    
    // ?ӵ?2???¼??????󣬼????¼????Ĳ???????
    FinalEventNum = EventTemp->FinalEventNum;
    for(i = 1; i < FinalEventNum; i++)
    {
        if((FinalEvent[i] >= left) && (FinalEvent[i] <= right))
        {
            EventType[i]  = EVENT_TYPE_FUCK;
            FuckEventNum++;
            TCPDEBUG_PRINT("ConcatPointEvent %d\n", i);
        }
        
        if((FinalEventEnd[i] >= left) && (FinalEventEnd[i] <= right))
        {
            EventType[i]  = EVENT_TYPE_FUCK;
            FuckEventNum++;
            TCPDEBUG_PRINT("ConcatPointEvent %d\n", i);
        }
    }

    // ????????FUCK?¼??????˳?????
    if(FuckEventNum > 0)
    {
        FinalEventNum = KickOutOfArray(EventTemp->EventType, EVENT_TYPE_FUCK, 
                                       EventTemp->FinalEvent, EventTemp->FinalEventEnd, FinalEventNum);

        EventTemp->FinalEventNum = FinalEventNum;
    }
    TCPDEBUG_PRINT("RemoveConcatPointEventPoints %d\n", FuckEventNum);
}

/*
 **************************************************************************************************
 *  ????  ???? RemoveLowLevelEventPoint
 *  ?????????? ȥ?????ڽ??????޵??¼???
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-10-22  14:43:12  v1.0
 **************************************************************************************************
*/
void RemoveLowLevelEventPoint(const OTDR_ChannelData_t *OtdrData, Event_t* EventTemp)
{
    Int32   EventIndex, FinalEventNum, FuckEventNum = 0;
    Int32   m, M, X1, X2, MeanAn;
    Int32   *FinalEvent, *FinalEventEnd, *EventType;
    float   MeanAi, ET;
    const float *Ai;
    const Int32 *An;

    FinalEvent    = EventTemp->FinalEvent;
    FinalEventEnd = EventTemp->FinalEventEnd;
    FinalEventNum = EventTemp->FinalEventNum;
    EventType     = EventTemp->EventType;
    
    An = OtdrData->ChanData;    // Ĭ??ֻ?????߹??????ߣ?????Ϊ?͹?????????ǰ?????¼????????ڽ???????
    Ai = OtdrData->Ai;
    ET = OtdrState.MeasureParam.EndThreshold;
    m  = OtdrState.M;
    M  = OtdrState.sigma * Tenth_div5(ET);

    // ??????һ???¼?????ǰ???ݵ???ʼ?¼???ǰ
    for(EventIndex = FinalEventNum-1; EventIndex > 0; EventIndex--)
    {
        // ͨ???????????ҵ????¼???????
        if(EventType[EventIndex] == EVENT_TYPE_PROBABLE)    continue;

#if 0   // ????????ǰ?????ڽ??????޵??¼???
        X1 = FinalEvent[EventIndex]-m;
        X1 = MAX(X1, FinalEventEnd[EventIndex-1]+m);
        X2 = FinalEvent[EventIndex];
        MeanValue(Ai, X1, X2, &MeanAi, DATA_TYPE_FLOAT);
        MeanValue(An, X1, X2, &MeanAn, DATA_TYPE_INT);
#else   // ???????з?ֵ???ڽ??????޵??¼???
        X1 = FinalEvent[EventIndex];
        X2 = FinalEventEnd[EventIndex];
        MaxValue(Ai, X1, X2, &MeanAi, NULL, DATA_TYPE_FLOAT);
        MaxValue(An, X1, X2, &MeanAn, NULL, DATA_TYPE_INT);
#endif
        if((MeanAi < ET) || (MeanAn < M))
        {
            EventType[EventIndex] = EVENT_TYPE_FUCK;
    	    FuckEventNum++;
        }
    }
    
    // ???????ڹ?Ӱ?????˳?????
    if(FuckEventNum > 0)
    {
        FinalEventNum = KickOutOfArray(EventTemp->EventType, EVENT_TYPE_FUCK, 
                                       EventTemp->FinalEvent, EventTemp->FinalEventEnd, FinalEventNum);

        EventTemp->FinalEventNum = FinalEventNum;
    }

    TCPDEBUG_PRINT("RemoveLowLevelEventPoint %d \n", FuckEventNum);
}

/*
 **************************************************************************************************
 *  ????  ???? RemoveLowAttenCoefEventPoint
 *  ?????????? ȥ??˥??ϵ??̫?͵??¼??㣬??????FindEndEventPoint????֮ǰ????
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2013-12-11 10:20:57
 **************************************************************************************************
*/
void RemoveLowAttenCoefEventPoint(const OTDR_ChannelData_t *OtdrData, Event_t* EventTemp)
{
    Int32   EventIndex, FinalEventNum, FuckEventNum;
    Int32   i, m, X1, X2;
    Int32   *FinalEvent, *FinalEventEnd, *EventType;
    float   v, ac, minAi;
    const float *Ai;

    FinalEvent    = EventTemp->FinalEvent;
    FinalEventEnd = EventTemp->FinalEventEnd;
    FinalEventNum = EventTemp->FinalEventNum;
    EventType     = EventTemp->EventType;
    
    Ai = OtdrData->Ai;
    m  = OtdrState.M;

    // ??????һ???¼?????ǰ???ݵ???ʼ?¼???ǰ
    FuckEventNum = 0;
    GetLaserParam(OtdrState.MeasureParam.Lambda_nm, &v, NULL);
    for(EventIndex = FinalEventNum-1; EventIndex > 0; EventIndex--)
    {
        // ͨ???????????ҵ????¼???????
        if(EventType[EventIndex] == EVENT_TYPE_PROBABLE)    break;

        X1 = FinalEventEnd[EventIndex-1];
        X2 = FinalEvent[EventIndex];
        
        if(X2 - X1 > 2*m)
        {
            MinValue(Ai, X1, X2, &minAi, NULL, DATA_TYPE_FLOAT);
            if(minAi <= 3.5)    break;
            
            LeastSquareMethod(Ai, X1, X2, &ac, NULL);
            ac = -ac * OtdrState.Points_1m * 1000;
            if(ac < v/2)    // ˥??ϵ??̫С??ȷ???зǷ??¼?????ɾ??
            {
                TCPDEBUG_PRINT("RemoveLowAttenCoefEventPoint at %.2fm, AC = %.3f\n", X2/OtdrState.Points_1m, ac);
                EventType[EventIndex] = EVENT_TYPE_FUCK;
    	        FuckEventNum++;
            }
            else    break;  // ???¼??Ϸ???ǰ?治??ɾ??
        }
        else    break;      // ????̫?̣?ǰ?治??ɾ??
    }
    
    // ???????ڹ?Ӱ?????˳?????
    if(FuckEventNum > 0)
    {
        FinalEventNum = KickOutOfArray(EventTemp->EventType, EVENT_TYPE_FUCK, 
                                       EventTemp->FinalEvent, EventTemp->FinalEventEnd, FinalEventNum);

        EventTemp->FinalEventNum = FinalEventNum;
    }
}

/*
 **************************************************************************************************
 *  ????  ???? FastEstimateFiber
 *  ?????????? ???ٹ��ƹ???��??
 *  ???ڲ????? input   : ????????
 *             DataLen : ???ݳ???
 *             sigma   : ??????????????
 *  ???ز????? ?????ź??յ?
 *  ???ڰ汾?? 2011-06-23  21:38:11  v1.0
 **************************************************************************************************
*/
Int32 FastEstimateFiber(OTDR_ChannelData_t *OtdrData, Int32 DataLen, Int32 sigma)
{
    // ?㷨?Թ��?????˥????5dBΪ?�����?��Ƴ?˥?����ȣ????㷨????Ҫ?????¼???        2012-6-25 9:38:39
    // Ѱ??????һ??ƽ????????ƽ????3km?ĵط???ƽ???¼?????˥???˳???6dB??????Ϊƽ?????ǽ?????
    Int32   i, j, M, Pos5dB, Threshold, *An;
    Int32   sigma3, SatFront = 0, SatEnd = 0;
    
    // Ѱ??????˥????5dB?ĵط?
    GetEventFindingParamBeforeLog(NULL, NULL, NULL, &M, "Large", 0);
    Threshold = sigma * Tenth_div5(5.0);
    sigma3    = sigma * 2.5;
    An = OtdrData->ChanData;
#if 1   // ?Ӻ???ǰѰ??5dB?㣬????һ???ᶨλ?ڶ??η???????
    for(i = DataLen-NOISE_LEN; i > M; i--)
	{
        if(An[i] >= Threshold)      break;
    }
#elif 0   // ??ǰ????Ѱ??5dB?㣬????????ȷʵ̫????????̬?ֲ????????ܿ??ܻᶨλ?ڹ???ĩ??֮ǰ
    for(i = M; i < DataLen-NOISE_LEN; i++)
	{
        if(An[i] <= Threshold)      break;
    }
#else   // ʹ???????ͷ???5dB????ƽ??ֵ
    for(i = M; i < DataLen-NOISE_LEN; i++)  // ????5dB
	{
        if(An[i] <= Threshold)      break;
    }
    Pos5dB = i;
    
    for(i = DataLen-NOISE_LEN; i > 0; i--)  // ????5dB
	{
        if(An[i] >= Threshold)      break;
    }
    i = (i+Pos5dB) >> 1;    // ȡƽ??
#endif
    Pos5dB = i;
    TCPDEBUG_PRINT("Pos5dB = %d(%dm)\n", Pos5dB, (Int32)(Pos5dB/OtdrState.Points_1m));
    
    // Ѱ???????????ı??͵?
    Threshold = OtdrState.SatThreshold;
    for(i = DataLen-NOISE_LEN; i > M; i--)
	{
        if(An[i] >= Threshold)      // ????
        {
            for(j = i-1; j >= M; j--)
            {
                if(An[j] < Threshold)
                {
                    // ????j??Ӧ?ĳ???С??1km???򷵻?Pos5dB     2013-6-26 17:40:09
                    if(j * 1.5 < (1000 * OtdrState.Points_1m))
                    {
                        TCPDEBUG_PRINT("Fall from Sat, Jx1.5<1km, Use Pos5dB\n");
                        return Pos5dB;
                    }
                    
                    SatEnd = MIN(i + 3000 * OtdrState.Points_1m, DataLen-NOISE_LEN-M);    // ????3km֮???ĵ?
                    TCPDEBUG_PRINT("SatEnd+3km = %d(%dm)\n", SatEnd, (Int32)(SatEnd/OtdrState.Points_1m));
                    
                    // SatFront ???? j ??ǰ M ?????????͵?
                    MinValue(An, j-M, j, NULL, &SatFront, DATA_TYPE_INT);
                    TCPDEBUG_PRINT("SatFront = %d(%dm)\n", SatFront, (Int32)(SatFront/OtdrState.Points_1m));
                    
                    // ??????��??֮????????
                    if((An[SatFront] <= sigma3) || (An[SatEnd] <= sigma3))
                    {
                        TCPDEBUG_PRINT("SatFront or SatEnd fall into noise, Use MIN(SatFront, Pos5dB)\n");
                        return MIN(SatFront, Pos5dB);      // ̫?ͣ?ֻ??????????
                    }
                    
                    if(An[SatFront] >= An[SatEnd] * Tenth_div5(6.0))
                    {
                        TCPDEBUG_PRINT("SatFront - SatEnd >= 6dB, Use SatFront\n");
                        return SatFront;    // ?????6dB
                    }
                    else
                    {
                        TCPDEBUG_PRINT("SatFront - SatEnd < 6dB, Use Pos5dB\n");
                        return Pos5dB;
                    }
                }
            }
            // ???????????????????ȫ??Ϊƽ??ֵ?????Գ??ȱض?С??ä??
            if(j * 1.5 < (1000 * OtdrState.Points_1m))
            {
                TCPDEBUG_PRINT("No Fall from Sat, Jx1.5<1km, Use Pos5dB\n");
                return Pos5dB;
            }
        }
    }
    
    TCPDEBUG_PRINT("No SatEvent, Use Pos5dB\n");
    return Pos5dB;
}

/*
 ********************************************************************
 *    End    of    File
 ********************************************************************
*/
