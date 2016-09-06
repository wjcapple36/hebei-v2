/*
 **************************************************************************************************
 *                        ???־?��?Ƽ????޹?˾
 *
 *  ?ļ??????? OTDR?????㷨?ļ?
 *
 *
 *  ?ļ???  ?? OtdrAlgo.c
 *  ??????  ?? ?���??
 *  ???????ڣ? 2012-12-28  11:29:03
 *  ??ǰ?汾?? v1.0
 * 
 ***** ?޸ļ?¼ *****
 *  ?޸???  ?? 
 *  ?޸????ڣ? 
 *  ??    ע?? 
 **************************************************************************************************
*/
#include <stdio.h>
#include <math.h>
#include <semaphore.h>

#include "Otdr.h"
#include "prototypes.h"
#include "DspFpgaReg.h"

static Int32 RT[DATA_LEN];
static Int32 TC[DATA_LEN];

/*
 **************************************************************************************************
 *  ????  ???? RemoveBaseLine
 *  ?????????? ȥ????????
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-12-28  11:29:26  v1.0
 **************************************************************************************************
*/
void RemoveBaseLine(Int32 input[], Int32 DataLen, Int32 NoiseLen)
{
    Int32 i, avg, temp, *An = input;
    

    MaxValue(input, 20, DataLen-NoiseLen, &OtdrState.MaxRawData, NULL, DATA_TYPE_INT);
    for(i = DataLen-NoiseLen-1; i > DataLen-NoiseLen-1-OtdrCtrl.NullReadCount; i--)
    {
        An[i] = An[i-OtdrCtrl.NullReadCount];
    }
    

    if(OtdrState.MeasureParam.MeasureLength_m > 100000)
    {
        for(i = DataLen-NoiseLen; i < DataLen; i++)
        {
            An[i] = An[i-NoiseLen];
        }
    }
    

	avg   = NoiseMean(An, DataLen, NoiseLen);
	temp  = NoiseMean(An, DataLen-NoiseLen, NoiseLen);
	if(temp > avg)
	{
    	for(i = DataLen-NoiseLen; i < DataLen; i++)
    	{
    	    An[i] += temp - avg;
    	}
    	avg  = temp;
    }
/*********************** Step 2 : ԭʼ????ȥ????????ƽ??ֵ ***************************************/
    for(i = 0; i < DataLen; i++)
    {
    	An[i] -= avg;
    }
}

/*
 **************************************************************************************************
 *  ????  ???? DeleteOsc
 *  ?????????? ȥ??ƽ??֮????β?ϵ??????񵴣?ֻ?ڵ???20ns?????²Ŵ???
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2011-07-14  11:56:20  v1.0
 **************************************************************************************************
*/
void DeleteOsc(Int32 input[], Int32 DataLen, Int32 sat)
{
    Int32 i, k, interval, exit = 1;
    Int32 j, t, peakValue, osc;
    Int32 SatStartPos, SatEndPos;
    
#if	1 

    
/************************************* ????��??ȷ?????????? **************************************/
    k = OtdrState.RealSampleRate_MHz / CLK_100MHz;
    interval = MAX(OtdrState.M, 4*k);
    
/************************************** ??ƽ???????񵴲?ƽ ***************************************/
    printf("smoothing sat");
    i = 1;
    while(i < DataLen-NOISE_LEN)
    {
        if(input[i] >= sat)
        {
            SatStartPos = i;
            SatEndPos   = DataLen+1;
            

            for(j = i+1; j < DataLen-interval; j++)
            {
                if(input[j] < sat)
                {
					osc = 0;
					for(t = j+1; t < j+interval; t++)
                    {
						if(input[t] >= sat){
							osc = 1;
							j = t;
							break;
						}
                    }
					
					if(osc == 0)
                    {
						SatEndPos = j-1;
						i = j;
						break;
					}
                }
            }
            

            MaxValue(input, SatStartPos, SatEndPos, &peakValue, NULL, DATA_TYPE_INT);
            printf(" : %d ~ %d", SatStartPos, SatEndPos);
            for(j = SatStartPos; j <= SatEndPos; j++)       input[j] = peakValue;
        }
        else    i++;
    }
    printf("\n");
#endif
}

/*
 **************************************************************************************************
 *  ????  ???? EnlargeData
 *  ?????????? ?????????ݷŴ?һ??????
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-12-28  11:37:28  v1.0
 **************************************************************************************************
*/
void EnlargeData(Int32 input[], Int32 DataLen, Int32 ratio)
{
    Int32 i;
    


    




    for(i = 0; i < DataLen; i++)
    {
    	input[i] *= ratio;
    }
}

/*
 **************************************************************************************************
 *  ????  ???? AdjustCurve
 *  ?????????? ???????ߣ???????????ǰ?ƶ???ȷ????һ?????ݼ?Ϊ??һ???¼?
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-12-28  11:51:07  v1.0
 **************************************************************************************************
*/
void AdjustCurve(Int32 input[], Int32 DataLen)
{
    Int32 i, m = OtdrState.CurveStartPoint;
    for(i = 0; i < DataLen - m; i++)
    {
    	input[i] = input[i+m];
    }
}

/*
 **************************************************************************************************
 *  ????  ???? InterleaveAverage
 *  ?????????? ????ƽ??
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-12-31  16:30:14  v1.0
 **************************************************************************************************
*/
void InterleaveAverage(Int32 input[], Int32 DataLen, Int32 space)
{
    int i, j;
    for(i = 0, j = space; i < DataLen-space; i++, j++)
    {
        input[i] = (input[i] + input[j]) >> 1;
    }
}

/*
 **************************************************************************************
 *  ???????ƣ? NoiseMean
 *  ?????????? ????????????????ƽ??ֵ??ADC????????Ϊ1023???ۼ?3???ӹ?180000?Σ?
 *		        ????ֵ????Ϊ1023*180000 = 184140000??ֻ??28λ???ɱ?ʾ??????ȡ1000
 *		        ?????ۼӺ????Գ?10λ????38λ???ɱ?ʾ??????ʹ??Int40��?ۼӡ?
 *  ???ڲ????? input   : ?????ź?
 *             DataLen : ?źų???
 *             NoiseLen   : ??β??????????
 *  ???ز????? ????ƽ??ֵ
 *  ???ڰ汾?? 2010-08-13  11:43:09	v1.0	2010-10-26 10:40:53 v2.0 ???? pOTDRParam
 **************************************************************************************
*/
Int32 NoiseMean(const Int32 input[], Int32 DataLen, Int32 NoiseLen)
{
	Int32 i;
	const Int32 *p;
	double sum = 0;
	
	p = &input[DataLen-1];
	for(i = 0; i < NoiseLen; i++)
		sum += *p--;
	
	return (Int32)(sum / NoiseLen);
}

/*
 **************************************************************************************
 *  ???????ƣ? RootMeanSquare
 *  ?????????? ??????????input?????????ݵľ?????ֵ
 *  ???ڲ????input   : ?????ź?
 *             DataLen : ?źų???
 *             NoiseLen   : ??β??????????
 *  ???ز????? 
 *  ????ʱ?䣺 2010-10-20  16:05:28  v1.0
 **************************************************************************************
*/
Int32 RootMeanSquare(const Int32 input[], Int32 DataLen, Int32 NoiseLen)
{
	Int32 i, sigma;
	const Int32 *p;
	double v, sum = 0;
	
	p = &input[DataLen-1];
	for(i = 0; i < NoiseLen; i++)
	{
		v = (double)(*p--);
		sum += v * v;
	}
	sigma = (Int32)sqrt(sum / NoiseLen);
	sigma = MAX(sigma, 1);
	return sigma;
}

/*
 **************************************************************************************
 *  ???????ƣ? RootMeanSquareNormal
 *  ?????????? ??????????input?????????ݵľ?????ֵ??ʹ????̬?ֲ???????��?��?
 *  ???ڲ????? input     : ?????ź?
 *             TargetLen : Ҫ?????Ķ????ĳ???
 *  ???ز????? 
 *  ????ʱ?䣺 2013-12-20 8:41:19
 **************************************************************************************
*/
#if 0
Int32 RootMeanSquareNormal(const Int32 input[], Int32 TargetLen)
{
    Int32   i, sigma, k;
	Int32   *BufTemp;
    
/*************************************** ???봦???ռ? ******************************************/
    BufTemp = MEM_ALLOC(TargetLen, Int32, 4);
    if(MEM_ILLEGAL == BufTemp)  return 0;
	
/************************************** ԭʼ????ȡ????ֵ ***************************************/
    for(i = 0; i < TargetLen; i++)  BufTemp[i] = (input[i] >= 0) ? input[i] : -input[i];
    


    k = (Int32)(0.6833 * TargetLen);
    sigma = kth_smallest(BufTemp, TargetLen, k);
    
    MEM_FREE(BufTemp, TargetLen, Int32);
    return MAX(sigma, 1);
}
#endif

/*
 **************************************************************************************
 *  ???????ƣ? AbsTooLowData
 *  ?????????? ???????????ݵ???-30????sigma????ȡ??????ֵ
 *  ???ڲ????? input   : ?????ź?
 *             DataLen : ?źų???
 *             sigma   : ?????V????
 *  ???ز????? 
 *  ????ʱ?䣺 2010-10-20  16:05:28  v1.0
 **************************************************************************************
*/
void AbsTooLowData(Int32 input[], Int32 DataLen, Int32 sigma)
{
	Int32 i, temp = -15*sigma;
	
	if((OtdrState.MeasureParam.MeasureLength_m > 10000) ||
	   (OtdrState.MeasureParam.PulseWidth_ns >= 80))      return;
#if 1
	for(i = 0; i < DataLen; i++)
	{
		if(input[i] < temp)     input[i] = -input[i];
	}
#else
    Int32 j, s, e, next;
    i = 1;
    while(i < DataLen)
    {
        if(input[i] < temp)
        {
            s = i-1;
            for(j = i-1; j >= 0; j--)
            {
                if(input[j] >= 0)
                {
                    s = j;
                    break;
                }
            }
            
            e = s+1;
            next = i+1;
            for(j = next; j < DataLen; j++)
            {
                if(input[j] >= 0)
                {
                    e = j;
                    next = j+1;
                    break;
                }
            }
            

            {
                Int32 d, ts = input[s], te = input[e];
                
                d = (te - ts) / (e - s);
                for(j = s+1; j <= e; j++)
                {
                    input[j] = input[j-1] + d;
                }
            }
            
            i = next;
        }
        else    i++;
    }
#endif
}

/*
 **************************************************************************************
 *  ???????ƣ? PulseWidthInSampleNum
 *  ?????????? ????һ???????????ȵĲ???????
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2010-08-13  10:34:02
 **************************************************************************************
*/
Int32 PulseWidthInSampleNum(void)
{
    Int32 m;
    m = (Int32)(OtdrState.MeasureParam.PulseWidth_ns * (OtdrState.RealSampleRate_Hz / 1000) / 1e6);

    m = MAX(m, 2);
    return m;
}

/*
 **************************************************************************************************
 *  ????  ???? GetSaturateThreshold
 *  ?????????? ??ȡƽ????ֵ??ֻ?????߹??????ߣ??͹??????߲???
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-12-27  11:41:54  v1.0
 **************************************************************************************************
*/
Int32 GetSaturateThreshold(const Int32 input[])
{
    Int32 i, count, maxcount, SatThreshold;
    Int32 acc, mv, yes = 0;
    
    MaxValue(input, 0, DATA_LEN-NOISE_LEN-OtdrState.M, &SatThreshold, &i, DATA_TYPE_INT);

    

    printf("GetSaturateThreshold : MaxValue at %d is %d", i, SatThreshold);
    SatThreshold *= Tenth_div5(-0.05);
	printf(", low down to %d\n", SatThreshold);
    
    count = 0;
    maxcount = MIN(OtdrState.M, 100);
    maxcount = MAX(maxcount, 5);
    for(i = 0; i < DATA_LEN-NOISE_LEN-OtdrState.M; i++)
    {
        if(input[i] >= SatThreshold)
        {
            count++;
            if(count >= maxcount)     break;
        }
    }
    
    if(count < maxcount)
    {

        acc = OtdrCtrl.AccCountOnce-2;
        if(OtdrState.MeasureParam.MeasureLength_m == 5000)  acc = OtdrCtrl.AccCountOnce-4;
        acc = acc * (OtdrState.TotalMeasureTime / TIMES_COUNT_ONCE);
        mv  = acc * 1020;
        
        printf("There're NO saturate points(%d < %d)", count, maxcount);
        if(OtdrState.MaxRawData >= mv)
        {
            yes = 1;
            printf(", but I find it in anther way\n");
        }
        else    printf("\n");
    }
    else
    {
        printf("There's saturate point from %d\n", i);
        yes = 1;
    }
    
    if(!yes)     SatThreshold *= 1.5;
    OtdrState.SatThreshold = SatThreshold;
    return yes;
}

/*
 **************************************************************************************************
 *  ????  ???? GetMeasureLengthIndex    GetPulseWidthIndex  GetMeasureTimeIndex GetLambdaIndex
 *  ?????????? ??��?̺?????ת?????±?????
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2011-09-20  15:00:46  v1.0
 **************************************************************************************************
*/
Uint32 GetMeasureLengthIndex(Uint32 MeasureLength_m)
{
    Uint32 index;
    

    if(MeasureLength_m <= 4000)             index = 0;
    else if(MeasureLength_m <= 8000)        index = 1;
    else if(MeasureLength_m <= 16000)       index = 2;
    else if(MeasureLength_m <= 32000)       index = 3;
    else if(MeasureLength_m <= 64000)       index = 4;
    else if(MeasureLength_m <= 128000)      index = 5;
    else /*if(MeasureLength_m <= 256000)*/  index = 6;
    
    return index;
}

Uint32 GetPulseWidthIndex(Uint32 PulseWidth_ns)
{
    Uint32 index;
    
    if(PulseWidth_ns <= 5)                  index = 0;
    else if(PulseWidth_ns <= 10)            index = 1;
    else if(PulseWidth_ns <= 20)            index = 2;
    else if(PulseWidth_ns <= 40)            index = 3;
    else if(PulseWidth_ns <= 80)            index = 4;
    else if(PulseWidth_ns <= 160)           index = 5;
    else if(PulseWidth_ns <= 320)           index = 6;
    else if(PulseWidth_ns <= 640)           index = 7;
    else if(PulseWidth_ns <= 1280)          index = 8;
    else if(PulseWidth_ns <= 2560)          index = 9;
    else if(PulseWidth_ns <= 5120)          index = 10;
    else if(PulseWidth_ns <= 10240)         index = 11;
    else /*if(PulseWidth_ns <= 20480)*/     index = 12;
    
    return index;
}

Uint32 GetMeasureTimeIndex(Uint32 MeasureTime_ms)
{
    Uint32 index;
    
    if(MeasureTime_ms <= 5000)              index = 0;
    else if(MeasureTime_ms <= 10000)        index = 1;
    else if(MeasureTime_ms <= 15000)        index = 2;
    else if(MeasureTime_ms <= 30000)        index = 3;
    else if(MeasureTime_ms <= 60000)        index = 4;
    else if(MeasureTime_ms <= 90000)        index = 5;
    else if(MeasureTime_ms <= 120000)       index = 6;
    else if(MeasureTime_ms <= 150000)       index = 7;
    else /* if(MeasureTime_ms <= 180000) */ index = 8;
    
    return index;
}

Uint32 GetLambdaIndex(Uint32 Lambda)
{
    Uint32 i, j = 1;
    
    for(i = 0; i < LAMBDA_NUM; i++)
    {
        if(OtdrLambdaIndex[i] == Lambda)
        {
            j = i;
            break;
        }
    }
    
    return j;
}

OtdrPowerLevel_t GetPowerLevelIndex(Uint32 Lambda)
{
    OtdrPowerLevel_t pl;
    
    if(Lambda == 1310)          pl = OtdrPowerLevel1310;
    else if(Lambda == 1625)     pl = OtdrPowerLevel1625;
    else if(Lambda == 1650)     pl = OtdrPowerLevel1625;
    else                        pl = OtdrPowerLevel1550;
    
    return pl;
}

/*
 **************************************************************************************************
 *  ????  ???? GetPulseWidthSaturateBlindZone                   GetPulseWidthLowPowerCount
 *  ?????????? ??ȡ??????ä??(EstimateCurveConnect)     ??ȡ?????ĵ͹??ʷ???????(GetPowerLevel)
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-12-13  08:54:02  v1.0
 **************************************************************************************************
*/
Uint32 GetPulseWidthSaturateBlindZone(Uint32 Lambda_nm, Uint32 PulseWidth_ns)
{
    Int32 a, b, i, Cn, st;


    i = GetLambdaIndex(Lambda_nm);
    i = MIN(i, 1);
    
    PulseWidth_ns = MIN(PulseWidth_ns, 20480);
    if((PulseWidth_ns >= 640) && (PulseWidth_ns < 1280))
        {a = OtdrBlindZone[i][0], b = OtdrBlindZone[i][1], st = 640;}
    else if((PulseWidth_ns >= 1280) && (PulseWidth_ns < 2560))
        {a = OtdrBlindZone[i][1], b = OtdrBlindZone[i][2], st = 1280;}
    else if((PulseWidth_ns >= 2560) && (PulseWidth_ns < 5120))
        {a = OtdrBlindZone[i][2], b = OtdrBlindZone[i][3], st = 2560;}
    else if((PulseWidth_ns >= 5120) && (PulseWidth_ns < 10240))
        {a = OtdrBlindZone[i][3], b = OtdrBlindZone[i][4], st = 5120;}
    else if((PulseWidth_ns >= 10240) && (PulseWidth_ns < 20480))
        {a = OtdrBlindZone[i][4], b = OtdrBlindZone[i][5], st = 10240;}
    else /* PulseWidth_ns == 20480 */
        {a = OtdrBlindZone[i][5], b = OtdrBlindZone[i][5], st = 20480;}
    
    a &= 0xffff;
    b &= 0xffff;
    Cn = a + (float)(b - a) * (PulseWidth_ns - st) / st;
    return Cn;
}

Uint32 GetPulseWidthLowPowerCount(Uint32 Lambda_nm, Uint32 PulseWidth_ns)
{
    Int32 c, i;
    

    i = GetLambdaIndex(Lambda_nm);
    i = MIN(i, 1);
    
    PulseWidth_ns = MIN(PulseWidth_ns, 20480);
    if((PulseWidth_ns >= 640) && (PulseWidth_ns < 1280))            c = OtdrBlindZone[i][0];
    else if((PulseWidth_ns >= 1280) && (PulseWidth_ns < 2560))      c = OtdrBlindZone[i][1];
    else if((PulseWidth_ns >= 2560) && (PulseWidth_ns < 5120))      c = OtdrBlindZone[i][2];
    else if((PulseWidth_ns >= 5120) && (PulseWidth_ns < 10240))     c = OtdrBlindZone[i][3];
    else if((PulseWidth_ns >= 10240) && (PulseWidth_ns < 20480))    c = OtdrBlindZone[i][4];
    else    c = OtdrBlindZone[i][5];
    
    c >>= 16;
    

    return 5000*c / TIMES_COUNT_ONCE;
}

Uint32 GetPulseWidthEventBlindZone(Uint32 PulseWidth_ns)
{
    Uint32 i, d;
    

    i = GetPulseWidthIndex(OtdrState.MeasureParam.PulseWidth_ns);
    d = (Uint32)(OtdrState.Points_1m * OtdrEventBlindZone[i]);
    return d;
}

/*
 **************************************************************************************************
 *  ????  ???? CapacityLinearCompensate
 *  ?????????? ???ǵ???????????ʱ?Ĳ???
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-11-15  08:25:58  v1.0
 **************************************************************************************************
*/
Int32 CapacityLinearCompensate(Int32 input[], Int32 DataLen, Int32 FilterLen, Int32 sigma)
{
    Int32   i, temp, BadPoint, freeRT = 1, freeTC = 1;
    Int32   MinTrend, Avg, NoiseStart;
	Int32   *TrendCurve = NULL, *RawTemp = NULL;
    
    if(!OtdrCtrl.EnableCapacityCompensate)      return 0;
    
/*************************************** ???????????˲? ******************************************/
    RawTemp = (Int32*)malloc(DATA_LEN * sizeof(Int32));
    if(NULL == RawTemp)
    {
        RawTemp = RT;
        freeRT = 0;
    }
    
    TrendCurve = (Int32*)malloc(DATA_LEN * sizeof(Int32));
    if(NULL == TrendCurve)
    {
        TrendCurve = TC;
        freeTC = 0;
    }

	nfir_center(input, TrendCurve, DataLen, FilterLen);
/************************************ Ѱ???˲??????ݵĹ????? *************************************/

    if(OtdrState.MeasureParam.PulseWidth_ns < MIN_PW_DATA_COMBINE)
    {
        BadPoint = DataLen - NOISE_LEN;
        MinTrend = 0;
        for(i = FilterLen; i < DataLen-1-NOISE_LEN; i++)
        {
            if(TrendCurve[i] <= MinTrend)   break;
        }
        BadPoint = i;
    }
    else
    {
        MinValue(TrendCurve, FilterLen, DataLen-1 - NOISE_LEN, &MinTrend, &BadPoint, DATA_TYPE_INT);
        if(MinTrend > -sigma/2)         return 0;
    }
    if(BadPoint > DataLen-NOISE_LEN)    return 0;


	Avg = MinTrend;
	
    OtdrState.SignalBadPoint = BadPoint;
    printf("OtdrState.SignalBadPoint = %d\n", BadPoint);

/**************************************** ??ʼ???ݲ??? *******************************************/
	memcpy(RawTemp, input, DataLen*sizeof(Int32));
	{
/************************************** ?????????߽??в??? ***************************************/
        for(i = BadPoint; i < DataLen - NOISE_LEN; i++)
        {
            input[i] = input[i] - TrendCurve[i];
    	}
    	for(i = 0; i < BadPoint; i++)
    	{
    	    input[i] -= Avg;
    	}
    	
    	OtdrState.SatThreshold -= Avg;
    	

    	temp     = 3.5*sigma;
    	if(RawTemp != NULL)
    	{
    	    for(i = BadPoint; i < DataLen - NOISE_LEN; i++)
            {
        	    if(input[i] > temp)
        	    {
        	        input[i] = RawTemp[i];
        	    }
        	}
    	}

    	NoiseStart = DataLen-1;
    	temp     = -3*sigma;
    	for(i = BadPoint; i < DataLen - NOISE_LEN; i++)
        {
    	    if(input[i] < temp)
    	    {
    	        input[i] = input[NoiseStart--];
    	        if(NoiseStart <= DataLen-NOISE_LEN+FilterLen)   NoiseStart = DataLen-1;
    	    }
    	}
    	

    	for(i = DataLen-NOISE_LEN-FilterLen; i < DataLen - NOISE_LEN; i++)
        {
    	    if(input[i] < temp)
    	    {
    	        input[i] = input[NoiseStart--];
    	        if(NoiseStart <= DataLen-NOISE_LEN+FilterLen)   NoiseStart = DataLen-1;
    	    }
    	}
    }
    
    if(freeRT)  free(RawTemp);
    if(freeTC)  free(TrendCurve);
    
    return 1;
}

/*
 **************************************************************************************************
 *  ????  ???? OtdrConcatCurve
 *  ?????????? ?????????ߺ?С????????ƴ????��
 *  ???ڲ????? Hsigma : ????????????????????ֵ
 *  ???ز????? 
 *  ???ڰ汾?? 2013-8-6 10:50:01
 **************************************************************************************************
*/
void OtdrConcatCurve(Int32 Hsigma)
{

    

    Int32   m, i, Lavg, Havg, ratio;
	Int32   *An, *Bn;
	float   v;
    
    An = OtdrData.LowPowerData;
    Bn = OtdrData.ChanData;
/********************************** ????????Ԥ???? *************************************/
    if(OtdrCtrl.LowPowerDataProcessed)
    {
        ratio = ENLARGE_FACTOR(OtdrState.LowPowerTime);
        ratio = MAX(ratio, 1);
    
        RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
        EnlargeData(An, DATA_LEN, ratio);
        AdjustCurve(An, DATA_LEN);
/******************************** ?????˲??????ݲ??? ***********************************/
    	m = GetNfirWidth(POWER_MODE_LOW);
    	nfir(An, An, DATA_LEN, m);
    	OtdrState.sigmaLp = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    	CapacityLinearCompensate(An, DATA_LEN, MAX(OtdrState.M, 128), OtdrState.sigmaLp);
    	OtdrCtrl.LowPowerDataProcessed = 0;
    }
/***************************************** ????ƴ??***********************************************/

    m = OtdrState.CurveConcatPoint;
    MeanValue(An, m-20, m+20, &Lavg, DATA_TYPE_INT);
    MeanValue(Bn, m-20, m+20, &Havg, DATA_TYPE_INT);
    

    v = (float)Havg/Lavg;
    OtdrState.HighMinusLow = 5*FastLog10(v);
}

/*
 **************************************************************************************************
 *  ????  ???? FpgaEchoTest
 *  ?????????? FPGA??Ӧ????
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2011-10-31  15:08:46  v1.0
 **************************************************************************************************
*/
#if 1
#define TEST_TIME   64
Uint32 FpgaEchoTest(void)
{
    Uint32 i, Wtemp, Rtemp, Equal = 1;
    volatile Uint32 *addr = (volatile Uint32 *)0x90000000;
#if 0    

    for(i = 0; i < TEST_TIME; i++)
    {

        if(i == 0)                  Wtemp = 0;
        else if(i == TEST_TIME-1)   Wtemp = 0xFFFFFFFF & CHAN_MASK;
        else                        Wtemp = ((*addr) * (i+1)) & CHAN_MASK;
        FPGA_ECHO_WRITE(Wtemp);
        

        addr++;
        addr--;
        addr++;
        

        Rtemp  = FPGA_ECHO_READ();
        Rtemp &= CHAN_MASK;
        
        if(Rtemp != Wtemp)
        {
            Equal = 0;
            break;
        }
    }
#endif
	return Equal;
}
#else
Uint32 FpgaEchoTest(void)
{
    Uint32 i, j, Wtemp, Rtemp, Equal = 1;
    volatile Uint32 *addr = (volatile Uint32 *)0x90000000;
#if 0 

    for(j = 0; j < 100; j++)
    for(i = 0; i < 0xffff; i++)
    {

        Wtemp = i;
        Wtemp &= CHAN_MASK;
        FPGA_ECHO_WRITE(Wtemp);
        

        addr++;
        addr--;
        addr++;
        

        Rtemp  = FPGA_ECHO_READ();
        Rtemp &= CHAN_MASK;
        
        if(Rtemp != Wtemp)
        {
            Equal = 0;
            break;
        }
    }
#endif
	return Equal;
}
#endif

/*
 **************************************************************************************
 *  ????  ???? FpgaStart
 *  ?????????? ????FPGA???в??ԡ???Ӧ????200ms????һ??
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2010-09-09  17:52:06  v1.0
 **************************************************************************************
*/
static Uint32 getLaserMap(Uint32 lambda)
{
    int i, t = 1310;
    for(i = 0; i < 6; i++)
    {
        if(lambda == OtdrLaserMap1550[i])
        {
            t = 1550;
            break;
        }
    }
    return t;
}

void FpgaStart(Uint32 powerlevel, Uint32 rcv, Uint32 post)
{
	extern OTDR_TouchMeasureParam_t tmp;
	extern sem_t sem_touch_cmd;
    static Int32 lastR = 0x7fffffff, lastP = 0x7fffffff, lastV = 0x7fffffff;
	Uint32 lambda, clk, pulse_width, acc_time, sample_num, sleeptime = 0;
    
    lambda = getLaserMap(OtdrState.MeasureParam.Lambda_nm);

    clk         = OtdrState.RealSampleRate_MHz;
    acc_time    = (Uint32)(1000*TIMES_COUNT_ONCE/OtdrCtrl.PulsePeriod_us);
    sample_num  = DATA_LEN;
    
    if(OtdrState.MeasureParam.PulseWidth_ns <= 5)   pulse_width = PULSE_WIDTH_5NS;
    else                    pulse_width = OtdrState.MeasureParam.PulseWidth_ns / 20;
    

    if(clk == CLK_25MHz)    pulse_width /= 2;
    if(clk == CLK_12MHz)    pulse_width /= 4;

#if EDMA_GET_DATA_TOUCH
	tmp.MeasureParam.Lambda_nm = OtdrState.MeasureParam.Lambda_nm;
	tmp.MeasureParam.MeasureLength_m = OtdrState.MeasureParam.MeasureLength_m;
	tmp.MeasureParam.PulseWidth_ns = OtdrState.MeasureParam.PulseWidth_ns;
	tmp.MeasureParam.MeasureTime_ms = OtdrState.MeasureParam.MeasureTime_ms;
	tmp.MeasureParam.R = rcv;
	tmp.MeasureParam.P = powerlevel;
	tmp.MeasureParam.ApdV = OtdrState.ApdV;
	if(post) sem_post(&sem_touch_cmd);

#else
	//HWI_disable();
	//OTDR_SET_RECEIVER(rcv);
	//OTDR_SET_APDV(OtdrState.ApdV);
	//OTDR_SET_LAMBDA(lambda);
	//OTDR_SET_PULSE_WIDTH(pulse_width);
	//OTDR_SET_SAMPLE_RATE(clk);
	//OTDR_SET_SAMPLE_NUM(sample_num);
	//OTDR_SET_ACC_COUNT(acc_time);
	//OTDR_SET_POWER(powerlevel);
	//HWI_enable();
	//
	//if(powerlevel != lastP)
	//{
	//    lastP = powerlevel;
	//    sleeptime = 50;
	//}
	//if(rcv != lastR)
	//{
	//    lastR = rcv;
	//    sleeptime = 80;
	//}
	//if(OtdrState.ApdV != lastV)
	//{
	//    lastV = OtdrState.ApdV;
	//    sleeptime = 350;
	//}
	//if(sleeptime)   usleep(1000*sleeptime);
	//
	//HWI_disable();
	//FPGA_START_ADC();
	//HWI_enable();
#endif
	printf("clk = %d,  pw = %d, P = %d,  R = %d\n", clk, pulse_width, powerlevel, rcv);
}

/*
 **************************************************************************************************
 *    End    of    File
 **************************************************************************************************
*/
