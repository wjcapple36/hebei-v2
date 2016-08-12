/*
 **************************************************************************************************
 *                        桂林光通电子工程公司
 *
 *  文件描述： 工具函数
 *
 *
 *  文件名  ： Utility.c
 *  创建者  ： 彭怀敏
 *  创建日期： 2010-09-28  17:24:24
 *  当前版本： v1.0
 *
 ***** 修改记录 *****
 *  修改者  ：
 *  修改日期：
 *  备    注：
 **************************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Otdr.h"
#include "DspFpgaReg.h"

/*
 **************************************************************************************
 *  函数名称： LeastSquareErrorMethod
 *  函数描述： 最小二乘法对输入数据进行线性拟合，输出常数项b和一次项系数k。其原理如下：
 *             拟合值 fi = k*i+b, 误差 e = fi - di = (b+k*i) - di, 总误差 E = sum(e^2)
 *             欲使E最小，则其对自变量b和k的导数为0，即dE/db = 0, dE/dk = 0.
 *             dE/db = 2*sum(e * de/db) = 2*sum(e) = 0
 *             dE/dk = 2*sum(e * de/dk) = 2*sum(e*i) = 0, 所以有
 *             sum(fi    ) = sum(di    )  ==>  b*n      + k*sum(i  ) = sum(di  )
 *             sum(fi * i) = sum(di * i)  ==>  b*sum(i) + k*sum(i^2) = sum(di*i)
 *             然而i∈[0, n-1],所以有
 *             sum(i) = n*(n-1)/2; sum(i^2) = n*(n-1)*(2*n-1)/6，上面的方程等价于
 *             b*n  + k*si  = sd   ==>  k = (sd*si - sdi*n)   / (si*si - si2*n)
 *             b*si + k*si2 = sdi  ==>  b = (sdi*si - sd*si2) / (si*si - si2*n)
 *  入口参数： di ： 输入待拟合的浮点数据
 *             n  ： 数据长度
 *  返回参数： 这是对应于拟合区间为输入数据的[0, n-1]的结果，如果拟合区间为[n1, n2]，
 *             则结果为 k1 = k; b1 = b - k*n1
 *  日期版本： 2010-11-04  08:17:59  v1.0
 **************************************************************************************
*/
void LeastSquareErrorMethod(const float di[], int n, float *k, float *b)
{
	int    i;
	double v, sd, sdi/*, si, si2, determinant*/;

	if(NULL == k)   return;

	if(n < 2)
	{
		*k = 0;
		*b = 0;
	}
	else
	{
//		si   = n*(n-1)/2;
//		si2  = n*(n-1)*(2*n-1)/6;
//		determinant = si*si - si2*n;	// determinant = (n^2-n^4)/12

		sd  = 0;
		sdi = 0;
		for(i = 0; i < n; i++)
		{
			v    = (double)di[i];
			sd  += v;
			sdi += v * i;
		}

		*k = (float)((12*sdi/n - 6*(n-1)*sd/n) / (n*n - 1));
//		if(NULL != b)   *b = (float)((2*(n-1)*(2*n-1)*sd/n - 6*(n-1)*sdi/n) / (n*n - 1));
		if(NULL != b)   *b = (float)(((4*n-2)*sd - 6*sdi) / (n*n + n));
	}
}

void LeastSquareMethod(const float di[], int from, int to, float *k, float *b)
{
    float   k1, b1;
    int     n;

    if(NULL == k)   return;

    n = to - from + 1;
    LeastSquareErrorMethod(&di[from], n, &k1, &b1);
    *k = k1;
    if(NULL != b)   *b = b1 - k1*from;
}


/*
 **************************************************************************************
 *  函数名称： FastLog10
 *  函数描述： 底为10的单精度浮点数快速算法
 *  入口参数： 输入单精度浮点数x
 *  返回参数： 对数值
 *  日期版本： 2010-11-03  11:31:43  v1.0
 **************************************************************************************
*/
float FastLog10(float x)
{
#if 1	// 3次逼近
	float t1 = 0.42858, t2 = -0.17718, t3 = 0.05006;	// Taylor逼近系数
	int xi = *(int *)&x;			// 获得浮点数x的16进制表示
	int x_int = (xi >> 23) - 127;		// 移位获得指数E
	xi &= 0x007FFFFF;					// 先将指数部分清零
	xi |= ((int)127 << 23);			// 再将其设为127，即 E = 0
	x = *(float *)&xi - 1;				// 还原成浮点数，同时要减去1获得尾数
	x = x * (t1 + x * (t2 + t3 * x));	// Taylor逼近公式

#else	// 2次逼近

	float t1 = 0.40875, t2 = -0.11077;	// Taylor逼近系数
	int xi = *(int *)&x;			// 获得浮点数x的16进制表示
	int x_int = (xi >> 23) - 127;		// 移位获得指数E
	xi &= 0x007FFFFF;					// 先将指数部分清零
	xi |= ((int)127 << 23);			// 再将其设为127，即 E = 0
	x = *(float *)&xi - 1;				// 还原成浮点数，同时要减去1获得尾数
	x = x * (t1 + x * t2);				// Taylor逼近公式
#endif

	return (x_int * 0.3010 + x);
}

/*
 **************************************************************************************
 *  函数名称： FastLog10_Addr
 *  函数描述： 底为10的单精度浮点数快速算法
 *  入口参数： 输入单精度浮点数x的地址xa
 *  返回参数： 对数值
 *  日期版本： 2010-11-03  11:31:43  v1.0
 **************************************************************************************
*/
#if 0
float FastLog10_Addr(float *xa)
{
    float x = *xa;

#if 1	// 3次逼近
	float t1 = 0.42858, t2 = -0.17718, t3 = 0.05006;	// Taylor逼近系数
	int xi = *(int *)&x;			// 获得浮点数x的16进制表示
	int x_int = (xi >> 23) - 127;		// 移位获得指数E
	xi &= 0x007FFFFF;					// 先将指数部分清零
	xi |= ((int)127 << 23);			// 再将其设为127，即 E = 0
	x = *(float *)&xi - 1;				// 还原成浮点数，同时要减去1获得尾数
	x = x * (t1 + x * (t2 + t3 * x));	// Taylor逼近公式

#else	// 2次逼近
	float t1 = 0.40875, t2 = -0.11077;	// Taylor逼近系数
	int xi = *(int *)&x;			// 获得浮点数x的16进制表示
	int x_int = (xi >> 23) - 127;		// 移位获得指数E
	xi &= 0x007FFFFF;					// 先将指数部分清零
	xi |= ((int)127 << 23);			// 再将其设为127，即 E = 0
	x = *(float *)&xi - 1;				// 还原成浮点数，同时要减去1获得尾数
	x = x * (t1 + x * t2);				// Taylor逼近公式
#endif

	return (x_int * 0.3010 + x);
}
#endif

/*
 **************************************************************************************
 *  函数名称： FastLog10Vector      FastLog10Vector2
 *  函数描述： 底为10的单精度浮点数快速算法
 *  入口参数： 输入单精度浮点数组input
 *  返回参数： 对数值
 *  日期版本： 2010-11-03  11:31:43  v1.0
 **************************************************************************************
*/
void FastLog10Vector(const int input[], float output[], int DataLen, int sigma, float minLevel)
{
    float tmp, x;
    int i;

    tmp = 1.0 / sigma;
    for(i = 0; i < DataLen; i++)
    {
        x = input[i] * tmp;
    #if 0
        x = x * x;

        // log10(x)
        {
            int xi = *(int *)&x;			// 获得浮点数x的16进制表示
            int x_int = (xi >> 23) - 127;		// 移位获得指数E
            xi &= 0x007FFFFF;					// 先将指数部分清零
            xi |= ((int)127 << 23);			// 再将其设为127，即 E = 0
            x = *(float *)&xi - 1;				// 还原成浮点数，同时要减去1获得尾数
            x = x * (0.42858 + x * (0.05006 * x - 0.17718));	// Taylor逼近公式
            x = x + x_int * 0.3010;
        }
        x = 2.5*x;
    #else
        // log10(x)
        {
            int xi = (*(int *)&x) & 0x7FFFFFFF; // 获得浮点数x的16进制表示并取绝对值
            int xe = (xi >> 23) - 127;		// 移位获得指数E
            xi &= 0x007FFFFF;					// 先将指数部分清零
            xi |= 0x3F800000;			// 再将其设为127，即 E = 0
            x = *(float *)&xi - 1;				// 还原成浮点数，同时要减去1获得尾数
            x = x * (0.42858 + x * (0.05006 * x - 0.17718));	// Taylor逼近公式
            x = x + xe * 0.3010;
        }
        x = 5*x;
    #endif
        output[i] = MAX(x, minLevel);
    }
}

void FastLog10Vector2(const OTDR_ChannelData_t *data, float output[], int DataLen, OtdrStateVariable_t *state)
{
    float tmp, x, diff, minLevel;
    const int *input;
    int i, cp;

    tmp = 1.0 / state->sigma;
    cp  = state->CurveConcatPoint;
    minLevel = state->MinNoiseAmp;
    diff     = state->HighMinusLow;
    
    // 先取低功率曲线的前端
    input = data->LowPowerData;
    for(i = 0; i < cp; i++)
    {
        x = input[i] * tmp;

        // log10(x)
        {
            int xi = (*(int *)&x) & 0x7FFFFFFF; // 获得浮点数x的16进制表示并取绝对值
            int xe = (xi >> 23) - 127;		// 移位获得指数E
            xi &= 0x007FFFFF;					// 先将指数部分清零
            xi |= 0x3F800000;			// 再将其设为127，即 E = 0
            x = *(float *)&xi - 1;				// 还原成浮点数，同时要减去1获得尾数
            x = x * (0.42858 + x * (0.05006 * x - 0.17718));	// Taylor逼近公式
            x = x + xe * 0.3010;
        }
        x = 5*x + diff;
        output[i] = MAX(x, minLevel);
    }
    
    // 再取高功率曲线的后端
    input = data->ChanData;
    for(i = cp; i < DataLen; i++)
    {
        x = input[i] * tmp;

        // log10(x)
        {
            int xi = (*(int *)&x) & 0x7FFFFFFF; // 获得浮点数x的16进制表示并取绝对值
            int xe = (xi >> 23) - 127;		// 移位获得指数E
            xi &= 0x007FFFFF;					// 先将指数部分清零
            xi |= 0x3F800000;			// 再将其设为127，即 E = 0
            x = *(float *)&xi - 1;				// 还原成浮点数，同时要减去1获得尾数
            x = x * (0.42858 + x * (0.05006 * x - 0.17718));	// Taylor逼近公式
            x = x + xe * 0.3010;
        }

        x = 5*x;
        output[i] = MAX(x, minLevel);
    }
}

/*
 **************************************************************************************
 *  函数名称： MaxValue     MinValue    MeanValue   TurningPoint    GetCount
 *  函数描述： 求一段数据的最大值和最小值和平均值及转折点 获取数值出现的计数
 *  入口参数：
 *  返回参数：
 *  日期版本： 2011-03-09  17:54:42  v1.0
 **************************************************************************************
*/
void MaxValue(const void *input, int from, int to, void *MV, int *pos, int type)
{
    int i, index, im, *int_data = (int *)input;
    float fm, *float_data = (float *)input;

    if(DATA_TYPE_INT == type)
    {
        im = int_data[from];
        index = from;
        for(i = from+1; i <= to; i++)
        {
            if(int_data[i] > im)
            {
                im = int_data[i];
                index = i;
            }
        }

        if(NULL != MV)      *(int*)MV  = im;
        if(NULL != pos)     *(int*)pos = index;
    }
    else if(DATA_TYPE_FLOAT == type)
    {
        fm = float_data[from];
        index = from;
        for(i = from+1; i <= to; i++)
        {
            if(float_data[i] > fm)
            {
                fm = float_data[i];
                index = i;
            }
        }

        if(NULL != MV)      *(float*)MV = fm;
        if(NULL != pos)     *(int*)pos  = index;
    }
}

void MinValue(const void *input, int from, int to, void *MV, int *pos, int type)
{
    int i, index, im, *int_data = (int *)input;
    float fm, *float_data = (float *)input;

    if(DATA_TYPE_INT == type)
    {
        im = int_data[from];
        index = from;
        for(i = from+1; i <= to; i++)
        {
            if(int_data[i] < im)
            {
                im = int_data[i];
                index = i;
            }
        }

        if(NULL != MV)      *(int*)MV  = im;
        if(NULL != pos)     *(int*)pos = index;
    }
    else if(DATA_TYPE_FLOAT == type)
    {
        fm = float_data[from];
        index = from;
        for(i = from+1; i <= to; i++)
        {
            if(float_data[i] < fm)
            {
                fm = float_data[i];
                index = i;
            }
        }

        if(NULL != MV)      *(float*)MV = fm;
        if(NULL != pos)     *(int*)pos  = index;
    }
}

void MinValueEQ(const void *input, int from, int to, void *MV, int *pos, int type)
{
    int i, index, im, *int_data = (int *)input;
    float fm, *float_data = (float *)input;

    if(DATA_TYPE_INT == type)
    {
        im = int_data[from];
        index = from;
        for(i = from+1; i <= to; i++)
        {
            if(int_data[i] <= im)
            {
                im = int_data[i];
                index = i;
            }
        }

        if(NULL != MV)      *(int*)MV  = im;
        if(NULL != pos)     *(int*)pos = index;
    }
    else if(DATA_TYPE_FLOAT == type)
    {
        fm = float_data[from];
        index = from;
        for(i = from+1; i <= to; i++)
        {
            if(float_data[i] <= fm)
            {
                fm = float_data[i];
                index = i;
            }
        }

        if(NULL != MV)      *(float*)MV = fm;
        if(NULL != pos)     *(int*)pos  = index;
    }
}

void MeanValue(const void *input, int from, int to, void *MV, int type)
{
    int i, *int_data = (int *)input;
    float sum = 0, *float_data = (float *)input;

    if(DATA_TYPE_INT == type)
    {
        for(i = from; i <= to; i++)
        {
            sum += int_data[i];
        }
        if(NULL != MV)      *(int*)MV  = (int)(sum / (to-from+1));
    }
    else if(DATA_TYPE_FLOAT == type)
    {
        for(i = from; i <= to; i++)
        {
            sum += float_data[i];
        }
        if(NULL != MV)      *(float*)MV  = sum / (to-from+1);
    }
}

int TurningPoint(const int input[], int from, int to, char *direction)
{
    int i;

    if(!strcmp(direction, "neg"))    // 负向转折点
    {
        for(i = from+1; i < to; i++)
        {
            if(input[i] < input[i-1])   break;
        }
        return i;
    }
    else /* if(!strcmp(direction, "pos"))    // 正向转折点 */
    {
        for(i = from+1; i < to; i++)
        {
            if(input[i] > input[i-1])   break;
        }
        return i;
    }
}

int GetCount(const int input[], int from, int to, int dst, int tol)
{
    int i, t1, t2, count = 0;

    t1 = dst - tol;
    t2 = dst + tol;
    for(i = from; i < to; i++)
    {
        if((input[i] >= t1) && (input[i] <= t2))    count++;
    }
    return count;
}

/*
 **************************************************************************************
 *  函数名称： qsort_cmp
 *  函数描述： 快速排序比较函数(从小到大比较)，用在事件点的快速排序中
 *  入口参数：
 *  返回参数：
 *  日期版本： 2011-01-05  16:45:37  v1.0
 **************************************************************************************
*/
int qsort_cmp(const void *a , const void *b)
{
	return *(int *)a - *(int *)b;
}

#if 0
/*
 **************************************************************************************
 *  函数名称： HostFloat2NetworkFloat   NetworkFloat2HostFloat
 *  函数描述： 主机浮点数与网络浮点数互相转换
 *  入口参数：
 *  返回参数：
 *  日期版本： 2011-03-23  17:09:23  v1.0
 **************************************************************************************
*/
float HostFloat2NetworkFloat(float f)
{
    Uint32 Un;

    Un = *(Uint32*)&f;
    Un = htonl(Un);
	return *(float *)&Un;
}

float NetworkFloat2HostFloat(float f)
{
    Uint32 Un;

    Un = *(Uint32*)&f;
    Un = ntohl(Un);
	return *(float *)&Un;
}

#endif

/*
 **************************************************************************************************
 *  函数  名： KickOutOfArray
 *  函数描述： 从数组中剔除某一元素，并连带剔除其他数组相同位置的元素
 *  入口参数： BaseArray : 基准数组，以它为基础搜索坏元素
 *             BadEgg    : 坏元素，要被剔除的
 *             Array1    : 连带数组1
 *             Array2    : 连带数组2
 *             ArrayLen  : 三个数组的共同长度
 *  返回参数： i         : 最终数组元素个数
 *  日期版本： 2011-04-28  15:20:47  v1.0
 **************************************************************************************************
*/
int KickOutOfArray(int *BaseArray, int BadEgg, int *Array1, int *Array2, int ArrayLen)
{
    int i, j, temp, GoodAfterBad;

    for(i = 0; i < ArrayLen; i++)
    {
        if(BaseArray[i] == BadEgg)      // 发现坏元素，搜索下一个好元素
        {
            GoodAfterBad = 0;           // 记录在坏元素之后是否存在好元素
            for(j = i+1; j < ArrayLen; j++)
            {
                if(BaseArray[j] != BadEgg)      // 发现好元素，替换
                {
                    temp         = BaseArray[i];
                    BaseArray[i] = BaseArray[j];
                    BaseArray[j] = temp;

                    temp         = Array1[i];
                    Array1[i]    = Array1[j];
                    Array1[j]    = temp;

                    temp         = Array2[i];
                    Array2[i]    = Array2[j];
                    Array2[j]    = temp;

                    GoodAfterBad = 1;
                    break;
                }
            }

            // 如果不存在好元素，则表明当前坏元素之后都是坏元素，直接退出
            if(GoodAfterBad == 0)   break;
        }
    }

    return i;   // 返回最终数组元素个数
}

/*
 **************************************************************************************************
 *  函数  名： InsertionSortAssociatedArray
 *  函数描述： 使用插入排序来为关联的数组排序，这些关联的数组被一起处理
 *  入口参数： BaseArray : 基准数组，以它为基础进行排序
 *             AssArray1 : 连带数组1，随BaseArray一起排序
 *             AssArray2 : 连带数组2，随BaseArray一起排序
 *             ArrayLen  : 三个数组的共同长度
 *  返回参数： 
 *  日期版本： 2012-10-17  09:56:54  v1.0
 **************************************************************************************************
*/
int InsertionSortAssociatedArray(int BaseArray[], int AssArray1[], int AssArray2[], int ArrayLen)
{
    int i,j;
    int temp1, temp2, temp3;
    for (i = 1; i < ArrayLen; i++)
    {
        temp1 = BaseArray[i];
        temp2 = AssArray1[i];
        temp3 = AssArray2[i];
        j     = i-1;
        
        //与已排序的数逐一比较，大于temp时，该数后移
        while((j >= 0) && (BaseArray[j] > temp1))
        {
            BaseArray[j+1] = BaseArray[j];
            AssArray1[j+1] = AssArray1[j];
            AssArray2[j+1] = AssArray2[j];
            
            j--;
        }
        BaseArray[j+1] = temp1;      //被排序数放到正确的位置
        AssArray1[j+1] = temp2;      //被排序数放到正确的位置
        AssArray2[j+1] = temp3;      //被排序数放到正确的位置
    }
	return 0;
}

/*
 **************************************************************************************
 *  函数名称： qsort_partitions
 *  函数描述： 快速排序一次分组，定位一个元素的最终位置，将原数组分成两个分组
 *  入口参数： Buf  : 数据缓冲区
 *             low  : 数组左边下标
 *             high : 数组右边下标
 *  返回参数： 
 *  日期版本： 2011-04-11  10:05:23  v1.0
 **************************************************************************************
*/
int qsort_partitions(int buf[], int low, int high)
{
    int PivotKey = buf[low];
    
    while(low < high)
    {
        // 从后向前遍历
        while((buf[high] >= PivotKey) && (low < high))   --high;
        buf[low] = buf[high];
        
        // 从前向后遍历
        while((buf[low] <= PivotKey) && (low < high))    ++low;
        buf[high] = buf[low];
    }
    buf[low] = PivotKey;
    return low;
}

/*
 **************************************************************************************************
 *  函数  名： kth_smallest
 *  函数描述： 寻找数组里第k小值，即从小到大排序后，处于数组下标索引为k处的值
 *  入口参数： buf : 输入数组，待处理数组
 *             n   : 数组长度
 *             k   : 希望找出的位置
 *  返回参数： 
 *  日期版本： 2011-05-12  14:39:04  v1.0
 **************************************************************************************************
*/
#define ELEM_SWAP(a,b) {register int temp = (a); (a) = (b); (b) = temp;}

int kth_smallest(int buf[], int n, int k)
{
    register int i,j,low,high;
    register int x;

    {
        low = 0, high = n-1;
        while(low < high)
        {
            x = buf[k];
            i = low, j = high;
            do
            {
                while(buf[i] < x)   i++;
                while(x < buf[j])   j--;
                if(i <= j)
                {
                    ELEM_SWAP(buf[i], buf[j]);
                    i++, j--;
                }
            }while(i <= j);
            if(j < k) low  = i;
            if(k < i) high = j;
        }
        return buf[k];
    }
}


/*
 **************************************************************************************************
 *    End    of    File
 **************************************************************************************************
*/
