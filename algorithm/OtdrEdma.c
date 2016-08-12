/*
 **************************************************************************************
 *                        桂林光通电子工程公司
 *
 *  文件描述： Edma设置文件
 *
 *
 *  文件名  ： OTDR4_Edma.c
 *  创建者  ： 彭怀敏
 *  创建日期： 2010-09-08  17:00:33
 *  当前版本： v1.0
 * 
 ***** 修改记录 *****
 *  修改者  ： 
 *  修改日期： 
 *  备    注： 
 **************************************************************************************
*/
#include "Otdr.h"
#include "OtdrEdma.h"
#include "prototypes.h"
#include "DspFpgaReg.h"

#if !EDMA_GET_DATA_TOUCH

// 现在先不考虑ping-pong操作，到以后程序稳定下来再考虑了
static Int32 chan_src_data_hp[DATA_LEN];        // 大功率曲线
static Int32 chan_src_data_lp[DATA_LEN];        // 小功率曲线
static Int32 lpd_back[DATA_LEN];                // 备份的小功率曲线

static int file_index = 0;

/*
 **************************************************************************************
 *  函数名称： EDMA_Chan_ClrData
 *  函数描述： 将数组数据清零
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-5-7 12:19:04  v1.0
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
 *  函数名称： CheckDataAvailable
 *  函数描述： 检查FPGA是否已经采集好数据
 *  入口参数： chan_num : 通道号，从1开始
 *  返回参数： 0 : 数据未采集好
 *             1 : 数据已采集好
 *  日期版本： 2011-02-16  11:35:52  v1.0
 **************************************************************************************
*/
Uint32 CheckDataAvailable(void)
{
    return 1;
}

/*
 **************************************************************************************************
 *  函数  名： EDMA_Chan_CopyData
 *  函数描述： 将数据复制到通道数据变量里
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-05-20  08:19:48  v1.0
 **************************************************************************************************
*/
void EDMA_Chan_CopyData(void)
{
	file_index++;
#if 0    // 噪声被放在数据的前NOISE_LEN个位置
    // 大功率曲线
    memcpy((void*)OtdrData.ChanData, chan_src_data_hp + NOISE_LEN, (DATA_LEN-NOISE_LEN)*sizeof(Int32));
    memcpy((void*)(OtdrData.ChanData + DATA_LEN-NOISE_LEN), chan_src_data_hp, NOISE_LEN*sizeof(Int32));
    
    if(OtdrCtrl.LowPowerDataChanged)    // 小功率曲线
    {
        memcpy((void*)OtdrData.LowPowerData, chan_src_data_lp + NOISE_LEN, (DATA_LEN-NOISE_LEN)*sizeof(Int32));
        memcpy((void*)(OtdrData.LowPowerData + DATA_LEN-NOISE_LEN), chan_src_data_lp, NOISE_LEN*sizeof(Int32));
        OtdrCtrl.LowPowerDataProcessed = 1;
    }
#else   // 噪声被放在数据的后NOISE_LEN个位置
    // 大功率曲线
    memcpy((void*)(OtdrData.ChanData), chan_src_data_hp, DATA_LEN*sizeof(Int32));
    
    if(OtdrCtrl.LowPowerDataChanged)    // 小功率曲线
    {
        memcpy((void*)(OtdrData.LowPowerData) , chan_src_data_lp, DATA_LEN*sizeof(Int32));
        OtdrCtrl.LowPowerDataProcessed = 1;
    }
#endif
}

/*
 **************************************************************************************
 *  函数名称： EDMA_Chan_GetData
 *  函数描述： 从FPGA读取数据
 *  入口参数： 
 *  返回参数： 成功为1，失败为0
 *  日期版本： 2013-8-6 9:57:30     2014-7-9 11:59:44
 **************************************************************************************
*/
#if 0
Int32 EDMA_Chan_GetData(void)
{
	volatile Int32 i, *src, *dst;

    src = (Int32*)FPGA_OTDR_BASE_ADDR;
    if(OtdrCtrl.NullReadCount > 16)     OtdrCtrl.NullReadCount = 16;
    for(i = 0; i < OtdrCtrl.NullReadCount; i++)     *src++;
    
    // 如果是低功率数据，则放入 chan_src_data_lp, 否则放入 chan_src_data_hp
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

    // 如果是低功率数据，则放入 chan_src_data_lp, 否则放入 chan_src_data_hp
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
