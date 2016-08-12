/*
 **************************************************************************************
 *                        桂林光通电子工程公司
 *
 *  文件描述： 数字滤波器。所有与滤波器有关的代码都放在这个文件里
 *             它有FIR线性滤波器，线性多迭代非线性滤波器，IIR滤波器
 *             以及中值滤波器等。
 *
 *  文件名  ： Filter.c
 *  创建者  ： 彭怀敏
 *  创建日期： 2010-12-22  14:40:45
 *  当前版本： v1.0
 * 
 ***** 修改记录 *****
 *  修改者  ： 
 *  修改日期： 
 *  备    注： 
 **************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>	// for qsort
#include <math.h>

#include "Otdr.h"
#include "prototypes.h"

/*
 **************************************************************************************
 *  函数名称： nfir
 *  函数描述： 对长度为 DataLen 的输入数据input[]进行 FilterLen 点平均滤波，
 *			   结果存放在output[]中。支持原地址计算。m*y(n-1) = m*y(n) + x(n-m) - x(n)
 *  入口参数： input     : 整型输入数据
 *			   output    : 整型输出数据
 *			   DataLen   : 输入数据长度
 *			   FilterLen : 滤波器长度
 *  返回参数：      
 *  日期版本： 2010-10-20  10:33:21  v1.0		2010-10-21 21:17:32 v2.0 后向滤波
 *             2010-12-22 14:46:25  从文件OTDR.c中移到本文件中来
 **************************************************************************************
*/
void nfir(Int32 *const input, Int32 *const output, Int32 DataLen, Int32 FilterLen)
{
	Int32 i, temp;
	Int32 *pInput_head, *pInput_tail, *pOutput;
	float coef;
	double sum;
	
	// 如果只有一个滤波宽度则不用滤，如果不是原地址计算则简单赋值返回
	if(FilterLen <= 1)
	{
	    if(output != input)
	    {
    	    pOutput = output;
    	    pInput_head = input;
    	    for(i = 0; i < DataLen; i++)		// 计算后n_filter_level个数据的和
        	{
        		*pOutput++ = *pInput_head++;
        	}
        }
	    return;
	}
	
	coef = 1.0 / (float)FilterLen;
	
#if 1		// 从后面往前面滤波
	// 先计算后 FilterLen 个数据的和
	sum = 0;
	pInput_head = &input[DataLen - 1];
	for(i = 0; i < FilterLen; i++)		// 计算后 FilterLen 个数据的和
	{
		sum += *pInput_head--;
	}
	
	// 依次滤波后 DataLen - FilterLen 个数据，每个数据是其前 FilterLen 个数据的加权平均	
	pOutput = &output[DataLen - 1];
	pInput_tail = &input[DataLen - 1];
	for(i = DataLen - FilterLen; i > 0; i--)	// 滤波
	{
		temp = *pInput_tail--;			// 如果使用原地址计算，它用于保存最后一个将要被覆盖的数据
		*pOutput-- = (Int32)(sum * coef);
		
		sum += *pInput_head--;
		sum -= temp;
	}
	
	// 前 FilterLen 个数据，不足处用0代替
	for(i = FilterLen; i > 0; i--)
	{
		temp = *pInput_tail--;			// 如果使用原地址计算，它用于保存最后一个将要被覆盖的数据
		*pOutput-- = (Int32)(sum / FilterLen);
		
		sum -= temp;
	}

#else		// 从前面往后面滤波
	// 先计算前 FilterLen 个数据的和
	sum = 0;
	pInput_head = input;
	pInput_tail = input;
	for(i = 0; i < FilterLen; i++)		// 计算前 FilterLen 个数据的和
	{
		sum += *pInput_head++;
	}
	
	// 依次滤波前 DataLen - FilterLen 个数据，每个数据是其后 FilterLen 个数据的加权平均	
	pOutput = output;
	for(i = 0; i < DataLen - FilterLen; i++)	// 滤波
	{
		*pOutput++ = (Int32)(sum * coef);
		
		sum += *pInput_head++;
		sum -= *pInput_tail++;
	}
	
	// 后 FilterLen 个数据
	for(i = 0; i < FilterLen; i++)	// 滤波
	{
		*pOutput++ = (Int32)(sum / FilterLen);
		sum -= *pInput_tail++;
	}
#endif
}

/*
 **************************************************************************************************
 *  函数  名： nfir_center
 *  函数描述： 对长度为 DataLen 的输入数据input[]进行 FilterLen 点平均滤波，
 *			   结果存放在output[]中。不支持原地址计算
 *  入口参数： input     : 整型输入数据
 *			   output    : 整型输出数据
 *			   DataLen   : 输入数据长度
 *			   FilterLen : 滤波器长度
 *  返回参数： 
 *  日期版本： 2011-06-24  11:09:39  v1.0
 **************************************************************************************************
*/
void nfir_center(Int32 *const input, Int32 *const output, Int32 DataLen, Int32 FilterLen)
{
	Int32 i, n;
	float coef;
	double sum;
	
	n = 2*FilterLen+1;
	
	coef = 1.0 / (float)n;
	
	// 先计算前 FilterLen 个数据的和
	sum = 0;
	for(i = 0; i < n; i++)
	{
		sum += input[i];
	}
	
	// 依次滤波后 DataLen - FilterLen 个数据，每个数据是其前 FilterLen 个数据的平均	
	for(i = FilterLen; i < DataLen-FilterLen; i++)
	{
		output[i]= (Int32)(sum * coef);
		
		sum += input[i+FilterLen+1];
		sum -= input[i-FilterLen];
	}
	
	// 前 FilterLen 个数据不变
	for(i = 0; i < FilterLen; i++)
	{
		output[i] = input[i];
	}
	// 后 FilterLen 个数据用输出的前 FilterLen 个数据代替
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
 *  函数  名： GetIIRFilterThreshold
 *  函数描述： 获取 IIR_AdaptiveFilter 的阈值参数
 *  入口参数： 
 *  返回参数： K          : 误差的均方根倍数
 *             NoiseWidth : 噪声宽度
 *  日期版本： 2011-07-14  10:03:12  v1.0
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
 *  函数名称： IIR_AdaptiveFilter
 *  函数描述： 使用1阶IIR滤波器来自适应滤波。分别进行3次滤波，如果最强滤波误差超过一定阈值，
 *             则判断次强滤波是否也超过，如果是则判断最弱滤波是否也超过，如果不超过，则
 *             使用滤波结果，否则使用原数据。   支持原地址计算。
 *  入口参数： 
 *  返回参数：      
 *  日期版本： 2011-7-11 15:18:33  v1.0
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
/************************************ 计算输入信号噪声平均值 *************************************/
/*************************************************************************************************/
    for(i = DataLen-1; i >= DataLen-50; i--)      sum += input[i];
    output[DataLen-1] = (Int32)(sum / 50);
    Zin = output[DataLen-1];
    
/*************************************************************************************************/
/************************************* 开始自适应滤波 ********************************************/
/*************************************************************************************************/
    for(i = DataLen-2; i >= 0; i--)
    {
        Xin  = input[i];
        Fout = IIR_a * Zin + (1-IIR_a) * Xin;           // 进行最强滤波
        
        // 判断其误差是否超过阈值
        if(abs(Fout - Xin) >= Threshold)                // 超过阈值
        {
            Fout = IIR_b * Zin + (1-IIR_b) * Xin;       // 进行次强滤波
            
            // 判断其误差是否超过阈值
            if(abs(Fout - Xin) >= Threshold)            // 超过阈值
            {
                Fout = IIR_c * Zin + (1-IIR_c) * Xin;   // 进行最弱滤波
                
                // 判断其误差是否超过阈值
                if(abs(Fout - Xin) >= Threshold)        // 超过阈值，输出不变，使用原数据
                {
                    output[i] = input[i];
                    c0++;
                }
                else                                    // 未超过阈值，使用滤波输出
                {
                    output[i] = Fout;
                    cc++;
                }
            }
            else                                        // 未超过阈值，使用滤波输出
            {
                output[i] = Fout;
                cb++;
            }
        }
        else                                            // 未超过阈值，使用滤波输出
        {
            output[i] = Fout;
            ca++;
        }
        
        // 更新滤波器状态
        Zin = output[i];
    }
    
/*************************************************************************************************/
/************************************* 补偿二次反射峰的下降 **************************************/
/*************************************************************************************************/
    // 计算滤波后数据的噪声均方根
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
/************************************ 计算输入信号噪声平均值 *************************************/
/*************************************************************************************************/
    for(i = DataLen-1; i >= DataLen-50; i--)      sum += input[i];
    output[DataLen-1] = (Int32)(sum / 50);
    Z = output[DataLen-1];
    
/*************************************************************************************************/
/************************************* 开始自适应滤波 ********************************************/
/*************************************************************************************************/
    for(i = DataLen-2; i >= NoiseWidth; i--)
    {
        X = input[i];
        Y = (Int32)(IIR_a * Z + (1-IIR_a) * X);     // 进行最强滤波
        
        // 判断其误差是否超过阈值
        if(abs(Y - X) > Threshold)                 // 超过阈值
        {
            // 判断是否属于零星大噪声，即查看其前 NoiseWidth 个输出误差是否都超过阈值
            isNoise = 0;
            for(j = i-1; j >= i-NoiseWidth; j--)
            {
                TempX = input[j];
                TempY = (Int32)(IIR_a * Z + (1-IIR_a) * TempX); // 计算其前 NoiseWidth 个输出
                if(abs(TempY - TempX) < Threshold)              // 存在小于阈值的误差输出，说明该点属于零星大噪声
                {
                    isNoise = 1;
                    break;
                }
            }
            
            // 如果是噪声，则照样滤除它
            if(isNoise)
            {
#if 1
                output[i] = Y;      // 使用强滤波结果
#else
                output[i] = (Int32)(IIR_b * Z + (1-IIR_b) * X);     // 使用稍弱滤波结果
#endif
                cb++;
            }
            else    // 不是噪声，使用原数据
            {
                output[i] = X;
                c0++;
            }
        }
        else        // 未超过阈值，使用滤波输出
        {
            output[i] = Y;
            ca++;
        }
        
        // 更新滤波器状态
        Z = output[i];
    }
    
    // 前 NoiseWidth 个数据不变
    for(i = 0; i < NoiseWidth; i++)
    {
        output[i] = input[i];
    }
    
/*************************************************************************************************/
/************************************* 补偿二次反射峰的下降 **************************************/
/*************************************************************************************************/
    // 计算滤波后数据的噪声均方根
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
/******************************** 补偿由于滤波导致的噪声稀疏 *************************************/
/*************************************************************************************************/
    // 从噪声开始的地方开始补偿，可以从头到尾遍历，直到发现数据小于sigmaIn的地方开始
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
 *  函数  名： IIR_AdaptiveFilter_Pretty
 *  函数描述： 从前面向后面滤波，并对反射峰作特殊处理，效果给力
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-08-23  10:23:44  v1.0
 **************************************************************************************************
*/
void IIR_AdaptiveFilter_Pretty(Int32 input[], Int32 output[], Int32 DataLen, Int32 sigmaIn, Int32 Threshold, Int32 NoiseWidth)
{
    extern Int32 RootMeanSquare(const Int32 input[], Int32 DataLen, Int32 Noise);
    
	Int32 i, j, k, X, Y, Z, TempX, TempY, isNoise, isTail;

/*************************************************************************************************/
/************************************ 计算输入信号噪声平均值 *************************************/
/*************************************************************************************************/
    output[0] = input[0];
    Z = output[0];
    
/*************************************************************************************************/
/************************************* 开始自适应滤波 ********************************************/
/*************************************************************************************************/
    i = 1;
    while(i < DataLen-NoiseWidth)
    {
        X = input[i];
        Y = (Int32)(IIR_a * Z + (1-IIR_a) * X);     // 进行IIR滤波
        
        // 判断其误差是否超过阈值
        if(abs(Y - X) > Threshold)                 // 超过阈值
        {
            // 判断是否属于零星大噪声，即查看其前 NoiseWidth 个输出误差是否都超过阈值
            isNoise = 0;
            for(j = i+1; j <= i+NoiseWidth; j++)
            {
                TempX = input[j];
                TempY = (Int32)(IIR_a * Z + (1-IIR_a) * TempX); // 计算其前 NoiseWidth 个输出
                if(abs(TempY - TempX) < Threshold)              // 存在小于阈值的误差输出，说明该点属于零星大噪声
                {
                    isNoise = 1;
                    break;
                }
            }
            
            // 如果是噪声，则照样滤除它
            if(isNoise)
            {
#if 1
                output[i] = Y;      // 使用强滤波结果
#else
                output[i] = (Int32)(IIR_b * Z + (1-IIR_b) * X);     // 使用稍弱滤波结果
#endif
            }
            else    // 不是噪声，确认是有事件，寻找转折点
            {
                if(X > Y)       // 上升沿
                {
                    for(j = i; j < DataLen-1; j++)
                    {
                        if(input[j+1] < input[j])   break;
                    }
                    
                    // 复制上升沿
                    for(k = i; k <= j; k++)
                    {
                        output[k] = input[k];
                    }
                }
                else            // 下降沿
                {
                    // 首先判断该点是否处于平顶后的拖尾上
                    isTail = 0;
                    for(j = 0; j < OtdrState.OtdrCurveSaturate.SatNum; j++)
                    {
                        if((i >= OtdrState.OtdrCurveSaturate.SatStart[j]) && (i <= OtdrState.OtdrCurveSaturate.TailEnd[j]))
                        {
                            isTail = 1;
                            break;
                        }
                    }
                    
                    // 如果是处于拖尾上，则直接滤波
                    if(isTail)
                    {
                #if 0       // 从前面向后面滤波，这样会导致拖尾变长
                        for(k = i; k <= OtdrState.OtdrCurveSaturate.TailEnd[j]; k++)
                        {
                            output[k] = (Int32)(IIR_a * output[k-1] + (1-IIR_a) * input[k]);
                        }
                        j = k;
                #else       // 从后面向前面滤波
                        for(k = OtdrState.OtdrCurveSaturate.TailEnd[j]; k >= i; k--)
                        {
                            output[k] = (Int32)(IIR_a * input[k+1] + (1-IIR_a) * input[k]);
                        }
                        j = OtdrState.OtdrCurveSaturate.TailEnd[j];
                #endif
                    }
                    else    // 否则直接使用原数据
                    {
                        // 寻找转折点
                        for(j = i; j < DataLen-1; j++)
                        {
                            if(input[j+1] > input[j])   break;
                        }
                        
                        // 复制下降沿
                        for(k = i; k <= j; k++)
                        {
                            output[k] = input[k];
                        }
                    }
                }
                
                // 更新下一位置
                i = j;
            }
        }
        else        // 未超过阈值，使用滤波输出
        {
            output[i] = Y;
        }
        
        // 更新滤波器状态
        Z = output[i];
        i++;
    }
    
    // 后 NoiseWidth 个数据用其前 NoiseWidth 个数据代替
    for(i = DataLen-NoiseWidth; i < DataLen; i++)
    {
        output[i] = output[i-NoiseWidth];
    }
}

/*
 **************************************************************************************************
 *  函数  名： GetIIRFilterThreshold
 *  函数描述： 获取 IIR_AdaptiveFilter 的阈值参数
 *  入口参数： 
 *  返回参数： K          : 误差的均方根倍数
 *             NoiseWidth : 噪声宽度
 *  日期版本： 2013-1-15 17:27:08  v1.0
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
 *  函数  名： FilterPulse
 *  函数描述： 滤除脉冲
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2013-01-15  15:31:55  v1.0
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
    if(input != output)     dst = output;   // 非原地址计算
    else                    // 原地址计算，必须申请额外空间
    {
        dst = (Int32*)malloc(DataLen*sizeof(Int32));
        if(NULL == dst)
    	{
    		TCPDEBUG_PRINT("dst alloc fail in FilterPulse\n");
    		return;
    	}
    }
    
    // 先计算前FilterLen个数据的和
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
    
    // 开始滤波
    while(i < DataLen - FilterLen)
    {
        suml -= src[i-FilterLen];
        suml += src[i];
        avgl = suml * coef;
        // 判断误差是否太大
        if(abs(avgl - src[i]) > Threshold)    // 很可能是事件点，寻找事件的结束点
        {
            fit = 0;
            sumr = suml;
            for(j = i+1; j < DataLen-FilterLen; j++)
            {
                sumr -= src[j-FilterLen];
                sumr += src[j];
                avgr = sumr * coef;
                
                if(abs(avgr - src[j]) < Threshold)    // 恢复回来
                {
                    fit = 1;
                    break;
                }
            }
            
            if(fit)     // 成功找到恢复点
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
    
    // 复制回去并释放空间
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
