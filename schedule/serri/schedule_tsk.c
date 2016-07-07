/**
 * @file schedule.c
 * @synopsis  调度otdr通道使用spi进行测量
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-24
 */
#include "../otdr_ch/otdr_ch.h"
#include "../otdr_ch/hb_spi.h"
#include "../common/schedule_fun.h"
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
extern pthread_mutex_t mutex_otdr;



void thread_schedule(void *arg)
{
	int index, ret, buf_index;
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHState *pCHState;
	struct _tagCHPara *pCHPara;

	index = 0;

	while(1)
	{
		index %= CH_NUM;
		buf_index = index % CH_BUF_NUM;
		pCHState = &(otdrDev[index].ch_state);
		pCHCtrl = &(otdrDev[index].ch_ctrl);
		pCHPara = &(otdrDev[index].ch_para);
		
		//更新本通道参数
		ret = pre_measure(index, &otdrDev[index]);

		if(ret != OP_OK)
			continue;
		//新一轮的测试
		ret = start_otdr_test(&spiDev, NULL);
		if(ret != OP_OK)
			continue;
		//延时7s之后，读取数据
		usr_delay(MEASURE_TIME_MIN_S);
		//此时需要使用缓冲区了
		if(chBuf[buf_index].is_uesd)
			pCHState->ch_buf_collid_num++;

		pthread_mutex_lock(&chBuf[buf_index].lock);
		ret = read_otdr_data(&spiDev, NULL);
		if(ret != OP_OK)
		{
			pthread_mutex_unlock(&chBuf[buf_index].lock);
			continue;
		}
		pCHCtrl->accum_num--;
		pCHCtrl->hp_num--;

		//判断是否拼接,如果不拼接，那么将低功率累加次数设置成0
		EstimateCurveConnect_r(chBuf[buf_index].hp_buf,NULL, NULL);
		if(!pCHCtrl->curv_cat)
		{
			pCHCtrl->hp_num = pCHCtrl->accum_num;
			pCHCtrl->lp_num = 0;
		}
		ret = otdr_test(index, &(otdrDev[index]),&spiDev, &chBuf[buf_index]);
		//测试完毕，正确获取数据启动otdr算法
		if(ret == 0)
		{
			//说明算法没有处理完成，7s的时间没有处理完，说明有问题
			if(ALGO_READY_FLAG_START_NEW == OtdrCtrl.OtdrAlgoReadyFlag)
				pCHState->algro_collid_num++;
			//只要能获取到信号量，说明那边的算法已经处理完毕
			pthread_mutex_lock(&mutex_otdr);
			memcpy(&OtdrCtrl, &otdrDev[index].otdr_ctrl,sizeof(OtdrCtrl));
			memcpy(&OtdrState,&otdrDev[index].otdr_state, sizeof(OtdrState));
			pthread_mutex_unlock(&mutex_otdr);
			//启动算法处理
			OtdrCtrl.OtdrDataReadyFlag = ALGO_READY_FLAG_START_NEW ;


		}
	}
}
