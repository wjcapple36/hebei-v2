/*
 **************************************************************************************
 *                        ���ֹ�ͨ���ӹ��̹�˾
 *
 *  �ļ������� Edma�����ļ�
 *
 *
 *  �ļ���  �� OTDR4_Edma.c
 *  ������  �� ����
 *  �������ڣ� 2010-09-08  17:00:33
 *  ��ǰ�汾�� v1.0
 * 
 ***** �޸ļ�¼ *****
 *  �޸���  �� 
 *  �޸����ڣ� 
 *  ��    ע�� 
 **************************************************************************************
*/
#include "Otdr.h"
#include "OtdrEdma.h"
#include "prototypes.h"
#include "DspFpgaReg.h"

#if !EDMA_GET_DATA_TOUCH

// �����Ȳ�����ping-pong���������Ժ�����ȶ������ٿ�����
static Int32 chan_src_data_hp[DATA_LEN];        // ��������
static Int32 chan_src_data_lp[DATA_LEN];        // С��������
static Int32 lpd_back[DATA_LEN];                // ���ݵ�С��������

static int file_index = 0;

/*
 **************************************************************************************
 *  �������ƣ� EDMA_Chan_ClrData
 *  ���������� ��������������
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2011-5-7 12:19:04  v1.0
 **************************************************************************************
*/
void EDMA_Chan_ClrData(void)
{
	file_index = 0;
    memset(chan_src_data_hp, 0, sizeof(chan_src_data_hp));
    memset(chan_src_data_lp, 0, sizeof(chan_src_data_lp));
    memset(lpd_back        , 0, sizeof(lpd_back        ));
}

/*
 **************************************************************************************
 *  �������ƣ� CheckDataAvailable
 *  ���������� ���FPGA�Ƿ��Ѿ��ɼ�������
 *  ��ڲ����� chan_num : ͨ���ţ���1��ʼ
 *  ���ز����� 0 : ����δ�ɼ���
 *             1 : �����Ѳɼ���
 *  ���ڰ汾�� 2011-02-16  11:35:52  v1.0
 **************************************************************************************
*/
Uint32 CheckDataAvailable(void)
{
    return 1;
}

/*
 **************************************************************************************************
 *  ����  ���� EDMA_Chan_CopyData
 *  ���������� �����ݸ��Ƶ�ͨ�����ݱ�����
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2011-05-20  08:19:48  v1.0
 **************************************************************************************************
*/
void EDMA_Chan_CopyData(void)
{
	file_index++;
#if 0    // �������������ݵ�ǰNOISE_LEN��λ��
    // ��������
    memcpy((void*)OtdrData.ChanData, chan_src_data_hp + NOISE_LEN, (DATA_LEN-NOISE_LEN)*sizeof(Int32));
    memcpy((void*)(OtdrData.ChanData + DATA_LEN-NOISE_LEN), chan_src_data_hp, NOISE_LEN*sizeof(Int32));
    
    if(OtdrCtrl.LowPowerDataChanged)    // С��������
    {
        memcpy((void*)OtdrData.LowPowerData, chan_src_data_lp + NOISE_LEN, (DATA_LEN-NOISE_LEN)*sizeof(Int32));
        memcpy((void*)(OtdrData.LowPowerData + DATA_LEN-NOISE_LEN), chan_src_data_lp, NOISE_LEN*sizeof(Int32));
        OtdrCtrl.LowPowerDataProcessed = 1;
    }
#else   // �������������ݵĺ�NOISE_LEN��λ��
    // ��������
    memcpy((void*)(OtdrData.ChanData), chan_src_data_hp, DATA_LEN*sizeof(Int32));
    
    if(OtdrCtrl.LowPowerDataChanged)    // С��������
    {
        memcpy((void*)(OtdrData.LowPowerData) , chan_src_data_lp, DATA_LEN*sizeof(Int32));
        OtdrCtrl.LowPowerDataProcessed = 1;
    }
#endif
}

/*
 **************************************************************************************
 *  �������ƣ� EDMA_Chan_GetData
 *  ���������� ��FPGA��ȡ����
 *  ��ڲ����� 
 *  ���ز����� �ɹ�Ϊ1��ʧ��Ϊ0
 *  ���ڰ汾�� 2013-8-6 9:57:30     2014-7-9 11:59:44
 **************************************************************************************
*/
#if 0
Int32 EDMA_Chan_GetData(void)
{
	volatile Int32 i, *src, *dst;

    src = (Int32*)FPGA_OTDR_BASE_ADDR;
    if(OtdrCtrl.NullReadCount > 16)     OtdrCtrl.NullReadCount = 16;
    for(i = 0; i < OtdrCtrl.NullReadCount; i++)     *src++;
    
    // ����ǵ͹������ݣ������ chan_src_data_lp, ������� chan_src_data_hp
    if(OtdrState.TreatAsHighPowerData)  dst = (Int32*)chan_src_data_hp;
    else                                dst = (Int32*)chan_src_data_lp;
    
    for(i = 0; i < DATA_LEN; i++)
   	{
   		*dst++ += (*src++ & CHAN_MASK);
   	}
   	return 1;
}

#else // Get data from files
Int32 EDMA_Chan_GetData(void)
{
	char datafilename[32];
	FILE *fp;
	int i, *dst;

    // ����ǵ͹������ݣ������ chan_src_data_lp, ������� chan_src_data_hp
    if(OtdrState.TreatAsHighPowerData)  dst = (Int32*)chan_src_data_hp;
    else                                dst = (Int32*)chan_src_data_lp;
    
	memset(datafilename, 0, sizeof(datafilename));
	//sprintf(datafilename, "otdr_data_static/OtdrDataFile_%02d", 0);
	sprintf(datafilename, "otdr_data_static/OtdrDataFile_%02d", file_index);
	fp = fopen(datafilename, "r");
	if(fp == NULL){
		printf("data file open failed\n");
		return -1;
	}

    for(i = 0; i < DATA_LEN; i++)
   	{
   		fscanf(fp, "%d", &dst[i]);
   	}
	fclose(fp);
   	return 1;
}

#endif

void Thread_TcpTouch(void){}
#endif // EDMA_GET_DATA_TOUCH

/*
 **************************************************************************************
 *    End    of    File
 **************************************************************************************
*/
