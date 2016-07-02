/*
 **************************************************************************************
 *                        ���ֹ�ͨ���ӹ��̹�˾
 *
 *  �ļ������� �����˲������������˲����йصĴ��붼��������ļ���
 *             ����FIR�����˲��������Զ�����������˲�����IIR�˲���
 *             �Լ���ֵ�˲����ȡ�
 *
 *  �ļ���  �� Filter.c
 *  ������  �� ����
 *  �������ڣ� 2010-12-22  14:40:45
 *  ��ǰ�汾�� v1.0
 * 
 ***** �޸ļ�¼ *****
 *  �޸���  �� 
 *  �޸����ڣ� 
 *  ��    ע�� 
 **************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>	// for qsort
#include <math.h>

#include "Otdr.h"
#include "prototypes.h"

/*
 **************************************************************************************
 *  �������ƣ� nfir
 *  ���������� �Գ���Ϊ DataLen ����������input[]���� FilterLen ��ƽ���˲���
 *			   ��������output[]�С�֧��ԭ��ַ���㡣m*y(n-1) = m*y(n) + x(n-m) - x(n)
 *  ��ڲ����� input     : ������������
 *			   output    : �����������
 *			   DataLen   : �������ݳ���
 *			   FilterLen : �˲�������
 *  ���ز�����      
 *  ���ڰ汾�� 2010-10-20  10:33:21  v1.0		2010-10-21 21:17:32 v2.0 �����˲�
 *             2010-12-22 14:46:25  ���ļ�OTDR.c���Ƶ����ļ�����
 **************************************************************************************
*/
void nfir(Int32 *const input, Int32 *const output, Int32 DataLen, Int32 FilterLen)
{
	Int32 i, temp;
	Int32 *pInput_head, *pInput_tail, *pOutput;
	float coef;
	double sum;
	
	// ���ֻ��һ���˲���������ˣ��������ԭ��ַ������򵥸�ֵ����
	if(FilterLen <= 1)
	{
	    if(output != input)
	    {
    	    pOutput = output;
    	    pInput_head = input;
    	    for(i = 0; i < DataLen; i++)		// �����n_filter_level�����ݵĺ�
        	{
        		*pOutput++ = *pInput_head++;
        	}
        }
	    return;
	}
	
	coef = 1.0 / (float)FilterLen;
	
#if 1		// �Ӻ�����ǰ���˲�
	// �ȼ���� FilterLen �����ݵĺ�
	sum = 0;
	pInput_head = &input[DataLen - 1];
	for(i = 0; i < FilterLen; i++)		// ����� FilterLen �����ݵĺ�
	{
		sum += *pInput_head--;
	}
	
	// �����˲��� DataLen - FilterLen �����ݣ�ÿ����������ǰ FilterLen �����ݵļ�Ȩƽ��	
	pOutput = &output[DataLen - 1];
	pInput_tail = &input[DataLen - 1];
	for(i = DataLen - FilterLen; i > 0; i--)	// �˲�
	{
		temp = *pInput_tail--;			// ���ʹ��ԭ��ַ���㣬�����ڱ������һ����Ҫ�����ǵ�����
		*pOutput-- = (Int32)(sum * coef);
		
		sum += *pInput_head--;
		sum -= temp;
	}
	
	// ǰ FilterLen �����ݣ����㴦��0����
	for(i = FilterLen; i > 0; i--)
	{
		temp = *pInput_tail--;			// ���ʹ��ԭ��ַ���㣬�����ڱ������һ����Ҫ�����ǵ�����
		*pOutput-- = (Int32)(sum / FilterLen);
		
		sum -= temp;
	}

#else		// ��ǰ���������˲�
	// �ȼ���ǰ FilterLen �����ݵĺ�
	sum = 0;
	pInput_head = input;
	pInput_tail = input;
	for(i = 0; i < FilterLen; i++)		// ����ǰ FilterLen �����ݵĺ�
	{
		sum += *pInput_head++;
	}
	
	// �����˲�ǰ DataLen - FilterLen �����ݣ�ÿ����������� FilterLen �����ݵļ�Ȩƽ��	
	pOutput = output;
	for(i = 0; i < DataLen - FilterLen; i++)	// �˲�
	{
		*pOutput++ = (Int32)(sum * coef);
		
		sum += *pInput_head++;
		sum -= *pInput_tail++;
	}
	
	// �� FilterLen ������
	for(i = 0; i < FilterLen; i++)	// �˲�
	{
		*pOutput++ = (Int32)(sum / FilterLen);
		sum -= *pInput_tail++;
	}
#endif
}

/*
 **************************************************************************************************
 *  ����  ���� nfir_center
 *  ���������� �Գ���Ϊ DataLen ����������input[]���� FilterLen ��ƽ���˲���
 *			   ��������output[]�С���֧��ԭ��ַ����
 *  ��ڲ����� input     : ������������
 *			   output    : �����������
 *			   DataLen   : �������ݳ���
 *			   FilterLen : �˲�������
 *  ���ز����� 
 *  ���ڰ汾�� 2011-06-24  11:09:39  v1.0
 **************************************************************************************************
*/
void nfir_center(Int32 *const input, Int32 *const output, Int32 DataLen, Int32 FilterLen)
{
	Int32 i, n;
	float coef;
	double sum;
	
	n = 2*FilterLen+1;
	
	coef = 1.0 / (float)n;
	
	// �ȼ���ǰ FilterLen �����ݵĺ�
	sum = 0;
	for(i = 0; i < n; i++)
	{
		sum += input[i];
	}
	
	// �����˲��� DataLen - FilterLen �����ݣ�ÿ����������ǰ FilterLen �����ݵ�ƽ��	
	for(i = FilterLen; i < DataLen-FilterLen; i++)
	{
		output[i]= (Int32)(sum * coef);
		
		sum += input[i+FilterLen+1];
		sum -= input[i-FilterLen];
	}
	
	// ǰ FilterLen �����ݲ���
	for(i = 0; i < FilterLen; i++)
	{
		output[i] = input[i];
	}
	// �� FilterLen �������������ǰ FilterLen �����ݴ���
	for(i = DataLen-FilterLen; i < DataLen; i++)
	{
		output[i] = output[i-FilterLen];
	}
}

#define	IIR_a	0.92
#define	IIR_b	(IIR_a * 0.9)
#define	IIR_c	(IIR_b * 0.9)
#if 0
/*
 **************************************************************************************************
 *  ����  ���� GetIIRFilterThreshold
 *  ���������� ��ȡ IIR_AdaptiveFilter ����ֵ����
 *  ��ڲ����� 
 *  ���ز����� K          : ���ľ���������
 *             NoiseWidth : �������
 *  ���ڰ汾�� 2011-07-14  10:03:12  v1.0
 **************************************************************************************************
*/
void GetIIRFilterThreshold(Int32 *K, Int32 *NoiseWidth)
{
    Int32 MeasureLength_m, PulseWidth_ns, m, k = 3;
    
    MeasureLength_m = OtdrState.MeasureParam.MeasureLength_m;
    PulseWidth_ns   = OtdrState.MeasureParam.PulseWidth_ns;
    m               = PulseWidthInSampleNum();
    
    if(MeasureLength_m <= 8000)
    {
        if(PulseWidth_ns <= 10)         k = 5;
        else if(PulseWidth_ns <= 20)    k = 4;
        else if(PulseWidth_ns <= 40)    k = 4;
    }
    else if(MeasureLength_m <= 16000)
    {
        if(PulseWidth_ns <= 10)         k = 5;
        else if(PulseWidth_ns <= 20)    k = 4;
        else if(PulseWidth_ns <= 40)    k = 4;
    }
    else if(MeasureLength_m <= 32000)
    {
        if(PulseWidth_ns <= 10)         k = 5;
        else if(PulseWidth_ns <= 20)    k = 4;
        else if(PulseWidth_ns <= 40)    k = 4;
    }
    else if(MeasureLength_m <= 64000)
    {
        if(PulseWidth_ns <= 20)         k = 5;
        else if(PulseWidth_ns <= 40)    k = 5;
        else if(PulseWidth_ns <= 80)    k = 5;
        else if(PulseWidth_ns <= 160)   k = 4;
    }
    else if(MeasureLength_m <= 128000)
    {
        if(PulseWidth_ns <= 40)         k = 6;
        else if(PulseWidth_ns <= 80)    k = 6;
        else if(PulseWidth_ns <= 160)   k = 6;
        else if(PulseWidth_ns <= 320)   k = 5;
        else if(PulseWidth_ns <= 640)   k = 4;
    }
    else/* if(MeasureLength_m < 256000) */
    {
        if(PulseWidth_ns <= 80)         k = 7;
        else if(PulseWidth_ns <= 160)   k = 6;
        else if(PulseWidth_ns <= 320)   k = 6;
        else if(PulseWidth_ns <= 640)   k = 5;
    }
    
    *K          = k;
    *NoiseWidth = MIN(MAX(m/2, 4), 8);
}

/*
 **************************************************************************************
 *  �������ƣ� IIR_AdaptiveFilter
 *  ���������� ʹ��1��IIR�˲���������Ӧ�˲����ֱ����3���˲��������ǿ�˲�����һ����ֵ��
 *             ���жϴ�ǿ�˲��Ƿ�Ҳ��������������ж������˲��Ƿ�Ҳ�������������������
 *             ʹ���˲����������ʹ��ԭ���ݡ�   ֧��ԭ��ַ���㡣
 *  ��ڲ����� 
 *  ���ز�����      
 *  ���ڰ汾�� 2011-7-11 15:18:33  v1.0
 **************************************************************************************
*/

void IIR_AdaptiveFilter(Int32 input[], Int32 output[], Int32 DataLen, Int32 sigmaIn, Int32 Threshold, Int32 NoiseWidth)
{
    extern Int32 RootMeanSquare(const Int32 input[], Int32 DataLen, Int32 Noise);
    
	Int32 i, j, Xin, Zin, Fout, sigma;
	float sum = 0;
	Int32 c0, ca, cb, cc;
    
    c0 = ca = cb = cc = 0;
/*************************************************************************************************/
/************************************ ���������ź�����ƽ��ֵ *************************************/
/*************************************************************************************************/
    for(i = DataLen-1; i >= DataLen-50; i--)      sum += input[i];
    output[DataLen-1] = (Int32)(sum / 50);
    Zin = output[DataLen-1];
    
/*************************************************************************************************/
/************************************* ��ʼ����Ӧ�˲� ********************************************/
/*************************************************************************************************/
    for(i = DataLen-2; i >= 0; i--)
    {
        Xin  = input[i];
        Fout = IIR_a * Zin + (1-IIR_a) * Xin;           // ������ǿ�˲�
        
        // �ж�������Ƿ񳬹���ֵ
        if(abs(Fout - Xin) >= Threshold)                // ������ֵ
        {
            Fout = IIR_b * Zin + (1-IIR_b) * Xin;       // ���д�ǿ�˲�
            
            // �ж�������Ƿ񳬹���ֵ
            if(abs(Fout - Xin) >= Threshold)            // ������ֵ
            {
                Fout = IIR_c * Zin + (1-IIR_c) * Xin;   // ���������˲�
                
                // �ж�������Ƿ񳬹���ֵ
                if(abs(Fout - Xin) >= Threshold)        // ������ֵ��������䣬ʹ��ԭ����
                {
                    output[i] = input[i];
                    c0++;
                }
                else                                    // δ������ֵ��ʹ���˲����
                {
                    output[i] = Fout;
                    cc++;
                }
            }
            else                                        // δ������ֵ��ʹ���˲����
            {
                output[i] = Fout;
                cb++;
            }
        }
        else                                            // δ������ֵ��ʹ���˲����
        {
            output[i] = Fout;
            ca++;
        }
        
        // �����˲���״̬
        Zin = output[i];
    }
    
/*************************************************************************************************/
/************************************* �������η������½� **************************************/
/*************************************************************************************************/
    // �����˲������ݵ�����������
    sigma = RootMeanSquare(output, DataLen, NOISE_LEN);
    j = DataLen-1;
    
    sigma = -3*sigma;
    for(i = 1; i < DataLen; i++)
    {
        if(output[i] < sigma)
        {
            output[i] = output[j--];
            if(j <= DataLen-NOISE_LEN)  j = DataLen-1;
        }
    }
    
	LOG_printf(&trace, "ca = %d", ca);
	LOG_printf(&trace, "cb = %d", cb);
	LOG_printf(&trace, "cc = %d", cc);
	LOG_printf(&trace, "c0 = %d", c0);
}
#endif

#if 0

void IIR_AdaptiveFilter(Int32 input[], Int32 output[], Int32 DataLen, Int32 sigmaIn, Int32 Threshold, Int32 NoiseWidth)
{
    extern Int32 RootMeanSquare(const Int32 input[], Int32 DataLen, Int32 Noise);
    
	Int32 i, j, X, Y, Z, TempX, TempY, sigmaOut, isNoise;
	float sum = 0;
	Int32 c0, ca, cb;
    
    c0 = ca = cb = 0;
/*************************************************************************************************/
/************************************ ���������ź�����ƽ��ֵ *************************************/
/*************************************************************************************************/
    for(i = DataLen-1; i >= DataLen-50; i--)      sum += input[i];
    output[DataLen-1] = (Int32)(sum / 50);
    Z = output[DataLen-1];
    
/*************************************************************************************************/
/************************************* ��ʼ����Ӧ�˲� ********************************************/
/*************************************************************************************************/
    for(i = DataLen-2; i >= NoiseWidth; i--)
    {
        X = input[i];
        Y = (Int32)(IIR_a * Z + (1-IIR_a) * X);     // ������ǿ�˲�
        
        // �ж�������Ƿ񳬹���ֵ
        if(abs(Y - X) > Threshold)                 // ������ֵ
        {
            // �ж��Ƿ��������Ǵ����������鿴��ǰ NoiseWidth ���������Ƿ񶼳�����ֵ
            isNoise = 0;
            for(j = i-1; j >= i-NoiseWidth; j--)
            {
                TempX = input[j];
                TempY = (Int32)(IIR_a * Z + (1-IIR_a) * TempX); // ������ǰ NoiseWidth �����
                if(abs(TempY - TempX) < Threshold)              // ����С����ֵ����������˵���õ��������Ǵ�����
                {
                    isNoise = 1;
                    break;
                }
            }
            
            // ������������������˳���
            if(isNoise)
            {
#if 1
                output[i] = Y;      // ʹ��ǿ�˲����
#else
                output[i] = (Int32)(IIR_b * Z + (1-IIR_b) * X);     // ʹ�������˲����
#endif
                cb++;
            }
            else    // ����������ʹ��ԭ����
            {
                output[i] = X;
                c0++;
            }
        }
        else        // δ������ֵ��ʹ���˲����
        {
            output[i] = Y;
            ca++;
        }
        
        // �����˲���״̬
        Z = output[i];
    }
    
    // ǰ NoiseWidth �����ݲ���
    for(i = 0; i < NoiseWidth; i++)
    {
        output[i] = input[i];
    }
    
/*************************************************************************************************/
/************************************* �������η������½� **************************************/
/*************************************************************************************************/
    // �����˲������ݵ�����������
    sigmaOut = RootMeanSquare(output, DataLen, NOISE_LEN);
    j = DataLen-1;
    
    TempX = -3*sigmaOut;
    for(i = 1; i < DataLen; i++)
    {
        if(output[i] < TempX)
        {
            output[i] = output[j--];
            if(j <= DataLen-NOISE_LEN)  j = DataLen-1;
        }
    }

#if 0
/*************************************************************************************************/
/******************************** ���������˲����µ�����ϡ�� *************************************/
/*************************************************************************************************/
    // ��������ʼ�ĵط���ʼ���������Դ�ͷ��β������ֱ����������С��sigmaIn�ĵط���ʼ
    TempX = 3*sigmaIn;
    TempY = 3*sigmaOut;
    sum   = (float)sigmaOut/sigmaIn;
    for(i = OtdrState.SignalBadPoint; i < DataLen; i++)
    {
//        if((abs(input[i]) <= TempX) && (abs(output[i]) <= TempY))
        {
            output[i] = (Int32)(input[i] * sum);
        }
    }
#endif

	LOG_printf(&trace, "ca = %d", ca);
	LOG_printf(&trace, "cb = %d", cb);
	LOG_printf(&trace, "c0 = %d", c0);
}

#endif

/*
 **************************************************************************************************
 *  ����  ���� IIR_AdaptiveFilter_Pretty
 *  ���������� ��ǰ��������˲������Է���������⴦��Ч������
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2011-08-23  10:23:44  v1.0
 **************************************************************************************************
*/
void IIR_AdaptiveFilter_Pretty(Int32 input[], Int32 output[], Int32 DataLen, Int32 sigmaIn, Int32 Threshold, Int32 NoiseWidth)
{
    extern Int32 RootMeanSquare(const Int32 input[], Int32 DataLen, Int32 Noise);
    
	Int32 i, j, k, X, Y, Z, TempX, TempY, isNoise, isTail;

/*************************************************************************************************/
/************************************ ���������ź�����ƽ��ֵ *************************************/
/*************************************************************************************************/
    output[0] = input[0];
    Z = output[0];
    
/*************************************************************************************************/
/************************************* ��ʼ����Ӧ�˲� ********************************************/
/*************************************************************************************************/
    i = 1;
    while(i < DataLen-NoiseWidth)
    {
        X = input[i];
        Y = (Int32)(IIR_a * Z + (1-IIR_a) * X);     // ����IIR�˲�
        
        // �ж�������Ƿ񳬹���ֵ
        if(abs(Y - X) > Threshold)                 // ������ֵ
        {
            // �ж��Ƿ��������Ǵ����������鿴��ǰ NoiseWidth ���������Ƿ񶼳�����ֵ
            isNoise = 0;
            for(j = i+1; j <= i+NoiseWidth; j++)
            {
                TempX = input[j];
                TempY = (Int32)(IIR_a * Z + (1-IIR_a) * TempX); // ������ǰ NoiseWidth �����
                if(abs(TempY - TempX) < Threshold)              // ����С����ֵ����������˵���õ��������Ǵ�����
                {
                    isNoise = 1;
                    break;
                }
            }
            
            // ������������������˳���
            if(isNoise)
            {
#if 1
                output[i] = Y;      // ʹ��ǿ�˲����
#else
                output[i] = (Int32)(IIR_b * Z + (1-IIR_b) * X);     // ʹ�������˲����
#endif
            }
            else    // ����������ȷ�������¼���Ѱ��ת�۵�
            {
                if(X > Y)       // ������
                {
                    for(j = i; j < DataLen-1; j++)
                    {
                        if(input[j+1] < input[j])   break;
                    }
                    
                    // ����������
                    for(k = i; k <= j; k++)
                    {
                        output[k] = input[k];
                    }
                }
                else            // �½���
                {
                    // �����жϸõ��Ƿ���ƽ�������β��
                    isTail = 0;
                    for(j = 0; j < OtdrState.OtdrCurveSaturate.SatNum; j++)
                    {
                        if((i >= OtdrState.OtdrCurveSaturate.SatStart[j]) && (i <= OtdrState.OtdrCurveSaturate.TailEnd[j]))
                        {
                            isTail = 1;
                            break;
                        }
                    }
                    
                    // ����Ǵ�����β�ϣ���ֱ���˲�
                    if(isTail)
                    {
                #if 0       // ��ǰ��������˲��������ᵼ����β�䳤
                        for(k = i; k <= OtdrState.OtdrCurveSaturate.TailEnd[j]; k++)
                        {
                            output[k] = (Int32)(IIR_a * output[k-1] + (1-IIR_a) * input[k]);
                        }
                        j = k;
                #else       // �Ӻ�����ǰ���˲�
                        for(k = OtdrState.OtdrCurveSaturate.TailEnd[j]; k >= i; k--)
                        {
                            output[k] = (Int32)(IIR_a * input[k+1] + (1-IIR_a) * input[k]);
                        }
                        j = OtdrState.OtdrCurveSaturate.TailEnd[j];
                #endif
                    }
                    else    // ����ֱ��ʹ��ԭ����
                    {
                        // Ѱ��ת�۵�
                        for(j = i; j < DataLen-1; j++)
                        {
                            if(input[j+1] > input[j])   break;
                        }
                        
                        // �����½���
                        for(k = i; k <= j; k++)
                        {
                            output[k] = input[k];
                        }
                    }
                }
                
                // ������һλ��
                i = j;
            }
        }
        else        // δ������ֵ��ʹ���˲����
        {
            output[i] = Y;
        }
        
        // �����˲���״̬
        Z = output[i];
        i++;
    }
    
    // �� NoiseWidth ����������ǰ NoiseWidth �����ݴ���
    for(i = DataLen-NoiseWidth; i < DataLen; i++)
    {
        output[i] = output[i-NoiseWidth];
    }
}

/*
 **************************************************************************************************
 *  ����  ���� GetIIRFilterThreshold
 *  ���������� ��ȡ IIR_AdaptiveFilter ����ֵ����
 *  ��ڲ����� 
 *  ���ز����� K          : ���ľ���������
 *             NoiseWidth : �������
 *  ���ڰ汾�� 2013-1-15 17:27:08  v1.0
 **************************************************************************************************
*/
void GetPulseFilterLen(Int32 *FilterLen, Int32 *Threshold)
{
    Int32 w;
    w = 4*OtdrState.M;
    w = MAX(64, w);
    
    *FilterLen = w+1;
    *Threshold = 5*OtdrState.sigma;
}

#if 0
/*
 **************************************************************************************************
 *  ����  ���� FilterPulse
 *  ���������� �˳�����
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2013-01-15  15:31:55  v1.0
 **************************************************************************************************
*/
void FilterPulse(Int32 input[], Int32 output[], Int32 DataLen)
{
    Int32 i, j, avgl, avgr, fit = 0;
    Int32 d, FilterLen, Threshold;
    Int32 *src, *dst;
    float coef, suml, sumr;
    
    GetPulseFilterLen(&FilterLen, &Threshold);
    coef = 1.0/FilterLen;
    
    src = input;
    if(input != output)     dst = output;   // ��ԭ��ַ����
    else                    // ԭ��ַ���㣬�����������ռ�
    {
        dst = (Int32*)malloc(DataLen*sizeof(Int32));
        if(NULL == dst)
    	{
    		TCPDEBUG_PRINT("dst alloc fail in FilterPulse\n");
    		return;
    	}
    }
    
    // �ȼ���ǰFilterLen�����ݵĺ�
    suml = 0;
    for(i = 0; i < FilterLen; i++)
    {
        suml += src[i];
    }
    avgl = suml * coef;
    for(i = 0; i < FilterLen; i++)
    {
        dst[i] = avgl;
    }
    
    // ��ʼ�˲�
    while(i < DataLen - FilterLen)
    {
        suml -= src[i-FilterLen];
        suml += src[i];
        avgl = suml * coef;
        // �ж�����Ƿ�̫��
        if(abs(avgl - src[i]) > Threshold)    // �ܿ������¼��㣬Ѱ���¼��Ľ�����
        {
            fit = 0;
            sumr = suml;
            for(j = i+1; j < DataLen-FilterLen; j++)
            {
                sumr -= src[j-FilterLen];
                sumr += src[j];
                avgr = sumr * coef;
                
                if(abs(avgr - src[j]) < Threshold)    // �ָ�����
                {
                    fit = 1;
                    break;
                }
            }
            
            if(fit)     // �ɹ��ҵ��ָ���
            {
                d = (dst[i-1] - avgr) / (j-i+2);
                dst[i++] = dst[i-1];
                for(; i <= j; i++)
                {
                    dst[i] = dst[i-1] - d;
                }
                i = j;
                suml = sumr;
            }
            else
            {
                dst[i] = dst[i-1];
            }
        }
        else        dst[i] = avgl;
        
        i++;
    }
    for(i = DataLen-FilterLen; i < DataLen; i++)
    {
        dst[i] = dst[i-FilterLen];
    }
    
    // ���ƻ�ȥ���ͷſռ�
    if(input == output)
    {
        memcpy(output, dst, DataLen*sizeof(Int32));
        free(dst);
    }
}
#endif

/*
 ********************************************************************
 *    End    of    File
 ********************************************************************
*/
