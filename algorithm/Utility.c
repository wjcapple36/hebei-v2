/*
 **************************************************************************************************
 *                        ���ֹ�ͨ���ӹ��̹�˾
 *
 *  �ļ������� ���ߺ���
 *
 *
 *  �ļ���  �� Utility.c
 *  ������  �� ����
 *  �������ڣ� 2010-09-28  17:24:24
 *  ��ǰ�汾�� v1.0
 *
 ***** �޸ļ�¼ *****
 *  �޸���  ��
 *  �޸����ڣ�
 *  ��    ע��
 **************************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Otdr.h"
#include "DspFpgaReg.h"

/*
 **************************************************************************************
 *  �������ƣ� LeastSquareErrorMethod
 *  ���������� ��С���˷����������ݽ���������ϣ����������b��һ����ϵ��k����ԭ�����£�
 *             ���ֵ fi = k*i+b, ��� e = fi - di = (b+k*i) - di, ����� E = sum(e^2)
 *             ��ʹE��С��������Ա���b��k�ĵ���Ϊ0����dE/db = 0, dE/dk = 0.
 *             dE/db = 2*sum(e * de/db) = 2*sum(e) = 0
 *             dE/dk = 2*sum(e * de/dk) = 2*sum(e*i) = 0, ������
 *             sum(fi    ) = sum(di    )  ==>  b*n      + k*sum(i  ) = sum(di  )
 *             sum(fi * i) = sum(di * i)  ==>  b*sum(i) + k*sum(i^2) = sum(di*i)
 *             Ȼ��i��[0, n-1],������
 *             sum(i) = n*(n-1)/2; sum(i^2) = n*(n-1)*(2*n-1)/6������ķ��̵ȼ���
 *             b*n  + k*si  = sd   ==>  k = (sd*si - sdi*n)   / (si*si - si2*n)
 *             b*si + k*si2 = sdi  ==>  b = (sdi*si - sd*si2) / (si*si - si2*n)
 *  ��ڲ����� di �� �������ϵĸ�������
 *             n  �� ���ݳ���
 *  ���ز����� ���Ƕ�Ӧ���������Ϊ�������ݵ�[0, n-1]�Ľ��������������Ϊ[n1, n2]��
 *             ����Ϊ k1 = k; b1 = b - k*n1
 *  ���ڰ汾�� 2010-11-04  08:17:59  v1.0
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
 *  �������ƣ� FastLog10
 *  ���������� ��Ϊ10�ĵ����ȸ����������㷨
 *  ��ڲ����� ���뵥���ȸ�����x
 *  ���ز����� ����ֵ
 *  ���ڰ汾�� 2010-11-03  11:31:43  v1.0
 **************************************************************************************
*/
float FastLog10(float x)
{
#if 1	// 3�αƽ�
	float t1 = 0.42858, t2 = -0.17718, t3 = 0.05006;	// Taylor�ƽ�ϵ��
	int xi = *(int *)&x;			// ��ø�����x��16���Ʊ�ʾ
	int x_int = (xi >> 23) - 127;		// ��λ���ָ��E
	xi &= 0x007FFFFF;					// �Ƚ�ָ����������
	xi |= ((int)127 << 23);			// �ٽ�����Ϊ127���� E = 0
	x = *(float *)&xi - 1;				// ��ԭ�ɸ�������ͬʱҪ��ȥ1���β��
	x = x * (t1 + x * (t2 + t3 * x));	// Taylor�ƽ���ʽ

#else	// 2�αƽ�

	float t1 = 0.40875, t2 = -0.11077;	// Taylor�ƽ�ϵ��
	int xi = *(int *)&x;			// ��ø�����x��16���Ʊ�ʾ
	int x_int = (xi >> 23) - 127;		// ��λ���ָ��E
	xi &= 0x007FFFFF;					// �Ƚ�ָ����������
	xi |= ((int)127 << 23);			// �ٽ�����Ϊ127���� E = 0
	x = *(float *)&xi - 1;				// ��ԭ�ɸ�������ͬʱҪ��ȥ1���β��
	x = x * (t1 + x * t2);				// Taylor�ƽ���ʽ
#endif

	return (x_int * 0.3010 + x);
}

/*
 **************************************************************************************
 *  �������ƣ� FastLog10_Addr
 *  ���������� ��Ϊ10�ĵ����ȸ����������㷨
 *  ��ڲ����� ���뵥���ȸ�����x�ĵ�ַxa
 *  ���ز����� ����ֵ
 *  ���ڰ汾�� 2010-11-03  11:31:43  v1.0
 **************************************************************************************
*/
#if 0
float FastLog10_Addr(float *xa)
{
    float x = *xa;

#if 1	// 3�αƽ�
	float t1 = 0.42858, t2 = -0.17718, t3 = 0.05006;	// Taylor�ƽ�ϵ��
	int xi = *(int *)&x;			// ��ø�����x��16���Ʊ�ʾ
	int x_int = (xi >> 23) - 127;		// ��λ���ָ��E
	xi &= 0x007FFFFF;					// �Ƚ�ָ����������
	xi |= ((int)127 << 23);			// �ٽ�����Ϊ127���� E = 0
	x = *(float *)&xi - 1;				// ��ԭ�ɸ�������ͬʱҪ��ȥ1���β��
	x = x * (t1 + x * (t2 + t3 * x));	// Taylor�ƽ���ʽ

#else	// 2�αƽ�
	float t1 = 0.40875, t2 = -0.11077;	// Taylor�ƽ�ϵ��
	int xi = *(int *)&x;			// ��ø�����x��16���Ʊ�ʾ
	int x_int = (xi >> 23) - 127;		// ��λ���ָ��E
	xi &= 0x007FFFFF;					// �Ƚ�ָ����������
	xi |= ((int)127 << 23);			// �ٽ�����Ϊ127���� E = 0
	x = *(float *)&xi - 1;				// ��ԭ�ɸ�������ͬʱҪ��ȥ1���β��
	x = x * (t1 + x * t2);				// Taylor�ƽ���ʽ
#endif

	return (x_int * 0.3010 + x);
}
#endif

/*
 **************************************************************************************
 *  �������ƣ� FastLog10Vector      FastLog10Vector2
 *  ���������� ��Ϊ10�ĵ����ȸ����������㷨
 *  ��ڲ����� ���뵥���ȸ�������input
 *  ���ز����� ����ֵ
 *  ���ڰ汾�� 2010-11-03  11:31:43  v1.0
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
            int xi = *(int *)&x;			// ��ø�����x��16���Ʊ�ʾ
            int x_int = (xi >> 23) - 127;		// ��λ���ָ��E
            xi &= 0x007FFFFF;					// �Ƚ�ָ����������
            xi |= ((int)127 << 23);			// �ٽ�����Ϊ127���� E = 0
            x = *(float *)&xi - 1;				// ��ԭ�ɸ�������ͬʱҪ��ȥ1���β��
            x = x * (0.42858 + x * (0.05006 * x - 0.17718));	// Taylor�ƽ���ʽ
            x = x + x_int * 0.3010;
        }
        x = 2.5*x;
    #else
        // log10(x)
        {
            int xi = (*(int *)&x) & 0x7FFFFFFF; // ��ø�����x��16���Ʊ�ʾ��ȡ����ֵ
            int xe = (xi >> 23) - 127;		// ��λ���ָ��E
            xi &= 0x007FFFFF;					// �Ƚ�ָ����������
            xi |= 0x3F800000;			// �ٽ�����Ϊ127���� E = 0
            x = *(float *)&xi - 1;				// ��ԭ�ɸ�������ͬʱҪ��ȥ1���β��
            x = x * (0.42858 + x * (0.05006 * x - 0.17718));	// Taylor�ƽ���ʽ
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
    
    // ��ȡ�͹������ߵ�ǰ��
    input = data->LowPowerData;
    for(i = 0; i < cp; i++)
    {
        x = input[i] * tmp;

        // log10(x)
        {
            int xi = (*(int *)&x) & 0x7FFFFFFF; // ��ø�����x��16���Ʊ�ʾ��ȡ����ֵ
            int xe = (xi >> 23) - 127;		// ��λ���ָ��E
            xi &= 0x007FFFFF;					// �Ƚ�ָ����������
            xi |= 0x3F800000;			// �ٽ�����Ϊ127���� E = 0
            x = *(float *)&xi - 1;				// ��ԭ�ɸ�������ͬʱҪ��ȥ1���β��
            x = x * (0.42858 + x * (0.05006 * x - 0.17718));	// Taylor�ƽ���ʽ
            x = x + xe * 0.3010;
        }
        x = 5*x + diff;
        output[i] = MAX(x, minLevel);
    }
    
    // ��ȡ�߹������ߵĺ��
    input = data->ChanData;
    for(i = cp; i < DataLen; i++)
    {
        x = input[i] * tmp;

        // log10(x)
        {
            int xi = (*(int *)&x) & 0x7FFFFFFF; // ��ø�����x��16���Ʊ�ʾ��ȡ����ֵ
            int xe = (xi >> 23) - 127;		// ��λ���ָ��E
            xi &= 0x007FFFFF;					// �Ƚ�ָ����������
            xi |= 0x3F800000;			// �ٽ�����Ϊ127���� E = 0
            x = *(float *)&xi - 1;				// ��ԭ�ɸ�������ͬʱҪ��ȥ1���β��
            x = x * (0.42858 + x * (0.05006 * x - 0.17718));	// Taylor�ƽ���ʽ
            x = x + xe * 0.3010;
        }

        x = 5*x;
        output[i] = MAX(x, minLevel);
    }
}

/*
 **************************************************************************************
 *  �������ƣ� MaxValue     MinValue    MeanValue   TurningPoint    GetCount
 *  ���������� ��һ�����ݵ����ֵ����Сֵ��ƽ��ֵ��ת�۵� ��ȡ��ֵ���ֵļ���
 *  ��ڲ�����
 *  ���ز�����
 *  ���ڰ汾�� 2011-03-09  17:54:42  v1.0
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

    if(!strcmp(direction, "neg"))    // ����ת�۵�
    {
        for(i = from+1; i < to; i++)
        {
            if(input[i] < input[i-1])   break;
        }
        return i;
    }
    else /* if(!strcmp(direction, "pos"))    // ����ת�۵� */
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
 *  �������ƣ� qsort_cmp
 *  ���������� ��������ȽϺ���(��С����Ƚ�)�������¼���Ŀ���������
 *  ��ڲ�����
 *  ���ز�����
 *  ���ڰ汾�� 2011-01-05  16:45:37  v1.0
 **************************************************************************************
*/
int qsort_cmp(const void *a , const void *b)
{
	return *(int *)a - *(int *)b;
}

#if 0
/*
 **************************************************************************************
 *  �������ƣ� HostFloat2NetworkFloat   NetworkFloat2HostFloat
 *  ���������� ���������������縡��������ת��
 *  ��ڲ�����
 *  ���ز�����
 *  ���ڰ汾�� 2011-03-23  17:09:23  v1.0
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
 *  ����  ���� KickOutOfArray
 *  ���������� ���������޳�ĳһԪ�أ��������޳�����������ͬλ�õ�Ԫ��
 *  ��ڲ����� BaseArray : ��׼���飬����Ϊ����������Ԫ��
 *             BadEgg    : ��Ԫ�أ�Ҫ���޳���
 *             Array1    : ��������1
 *             Array2    : ��������2
 *             ArrayLen  : ��������Ĺ�ͬ����
 *  ���ز����� i         : ��������Ԫ�ظ���
 *  ���ڰ汾�� 2011-04-28  15:20:47  v1.0
 **************************************************************************************************
*/
int KickOutOfArray(int *BaseArray, int BadEgg, int *Array1, int *Array2, int ArrayLen)
{
    int i, j, temp, GoodAfterBad;

    for(i = 0; i < ArrayLen; i++)
    {
        if(BaseArray[i] == BadEgg)      // ���ֻ�Ԫ�أ�������һ����Ԫ��
        {
            GoodAfterBad = 0;           // ��¼�ڻ�Ԫ��֮���Ƿ���ں�Ԫ��
            for(j = i+1; j < ArrayLen; j++)
            {
                if(BaseArray[j] != BadEgg)      // ���ֺ�Ԫ�أ��滻
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

            // ��������ں�Ԫ�أ��������ǰ��Ԫ��֮���ǻ�Ԫ�أ�ֱ���˳�
            if(GoodAfterBad == 0)   break;
        }
    }

    return i;   // ������������Ԫ�ظ���
}

/*
 **************************************************************************************************
 *  ����  ���� InsertionSortAssociatedArray
 *  ���������� ʹ�ò���������Ϊ����������������Щ���������鱻һ����
 *  ��ڲ����� BaseArray : ��׼���飬����Ϊ������������
 *             AssArray1 : ��������1����BaseArrayһ������
 *             AssArray2 : ��������2����BaseArrayһ������
 *             ArrayLen  : ��������Ĺ�ͬ����
 *  ���ز����� 
 *  ���ڰ汾�� 2012-10-17  09:56:54  v1.0
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
        
        //�������������һ�Ƚϣ�����tempʱ����������
        while((j >= 0) && (BaseArray[j] > temp1))
        {
            BaseArray[j+1] = BaseArray[j];
            AssArray1[j+1] = AssArray1[j];
            AssArray2[j+1] = AssArray2[j];
            
            j--;
        }
        BaseArray[j+1] = temp1;      //���������ŵ���ȷ��λ��
        AssArray1[j+1] = temp2;      //���������ŵ���ȷ��λ��
        AssArray2[j+1] = temp3;      //���������ŵ���ȷ��λ��
    }
	return 0;
}

/*
 **************************************************************************************
 *  �������ƣ� qsort_partitions
 *  ���������� ��������һ�η��飬��λһ��Ԫ�ص�����λ�ã���ԭ����ֳ���������
 *  ��ڲ����� Buf  : ���ݻ�����
 *             low  : ��������±�
 *             high : �����ұ��±�
 *  ���ز����� 
 *  ���ڰ汾�� 2011-04-11  10:05:23  v1.0
 **************************************************************************************
*/
int qsort_partitions(int buf[], int low, int high)
{
    int PivotKey = buf[low];
    
    while(low < high)
    {
        // �Ӻ���ǰ����
        while((buf[high] >= PivotKey) && (low < high))   --high;
        buf[low] = buf[high];
        
        // ��ǰ������
        while((buf[low] <= PivotKey) && (low < high))    ++low;
        buf[high] = buf[low];
    }
    buf[low] = PivotKey;
    return low;
}

/*
 **************************************************************************************************
 *  ����  ���� kth_smallest
 *  ���������� Ѱ���������kСֵ������С��������󣬴��������±�����Ϊk����ֵ
 *  ��ڲ����� buf : �������飬����������
 *             n   : ���鳤��
 *             k   : ϣ���ҳ���λ��
 *  ���ز����� 
 *  ���ڰ汾�� 2011-05-12  14:39:04  v1.0
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
