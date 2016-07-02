/**
 * @file schedule.c
 * @synopsis  调度otdr通道使用spi进行测量
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-24
 */
#include "../otdr_ch/otdr_ch.h"
#include "../otdr_ch/hb_spi.h"
//定义otdr通道资源
struct _tagOtdrDev otdrDev[CH_NUM];
//通道缓冲区，存放高低功率累加数据
struct _tagCHBuf chBuf[CH_BUF_NUM];
//spi设备
struct _tagSpiDev spiDev;
/*
 *本任务负责根据通道参数更新OtdrCtrl,OtdrState，
 *并将获取到的高低功率数据复制到OtdrData，并启动算法运算
 */
extern OTDR_ChannelData_t OtdrData;
extern OtdrCtrlVariable_t OtdrCtrl; 
extern OtdrStateVariable_t OtdrState; 



void thread_schedule(void *arg)
{
	int index;
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHState *pCHState;

	index = 0;

	while(1)
	{
		index %= CH_NUM;
		pCHState = &(otdrDev[index].ch_state);
		pCHCtrl = &(otdrDev[index].ch_ctrl);
		//新一轮的测试
		if(!pCHCtrl->accum_num)
		{
			;

		}
	}
}
