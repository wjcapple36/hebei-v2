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
//用户点名测量，配置测量
struct _tagUsrOtdrTest usrOtdrTest;
//周期性测量存储区,每个通道一个存储区，网管查询时立马返回
struct _tagCycCurv cycCurvBuf[CH_NUM];
//光纤段统计数据
struct _tagFiberStatisData statisDataBuf[CH_NUM];
//算法运行时当前的通道信息,开始测量的时候赋值，算法运行完毕清空
struct _tagAlgroCHInfo algroCHInfo;


/*
 *本任务负责根据通道参数更新OtdrCtrl,OtdrState，
 *并将获取到的高低功率数据复制到OtdrData，并启动算法运算
 */
extern OTDR_ChannelData_t OtdrData;
extern OtdrCtrlVariable_t OtdrCtrl; 
extern OtdrStateVariable_t OtdrState; 
extern pthread_mutex_t mutex_otdr;



/* --------------------------------------------------------------------------*/
/**
 * @synopsis  thread_schedule 
 * 流程：1、首先检查是否有用户指定的测量，如果有，将命令码，保存，并设置对应的
 * 	    对应的测量模式，并用用户指定的参数初始化控制测量参数。
 * 	 2、测试时间不应小于7s，小于7s以7s计。高功率测试第一阶段，收集曲线，如果
 *	    需要拼接，则需要用低功率测量，否则，全部使用高功率测量.
 *	 3、如果第一个7秒测完之后还需要测量，那么根据第二步判断标准进行测量。
 *	 4、全部测试完成，启动otdr算法线程。最小测试周期7s，算法运行线程应该不会
 *	 阻塞的吧。在发送完测试命令之后，等待过程中，如果有用户测量，则马上中断
 *	 返回，重头开始。
 *	 5、需要增加一些查询otdr运行状态的命令，向外界展示otdr工作状态
 *	       
 * @param arg
 */
/* ----------------------------------------------------------------------------*/
int32_t thread_schedule(void *arg)
{
	int32_t index, ret, buf_index, last_index;
	uint32_t cmd;
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHState *pCHState;
	struct _tagCHPara *pCHPara;
	struct _tagCHPara usr_para;


	index = -1;
	last_index = -1;

	while(1)
	{
		index++;
		//如果是点名测量，则保存上次的轮询的通道号
		if(usrOtdrTest.state == USR_OTDR_TEST_WAIT){
			usrOtdrTest.state = USR_OTDR_TEST_ACCUM;
			//如果last_index不等-1，说明是连续点名测量
			last_index = last_index == -1 ? index : last_index;
			index = usrOtdrTest.ch;
			cmd = usrOtdrTest.cmd;
			
		}
		else{
			if(last_index != -1)
				index = last_index;
			last_index = -1;
			index %= CH_NUM;
			cmd = 0x80000013;
		}
		buf_index = index % CH_BUF_NUM;
		pCHState = &(otdrDev[index].ch_state);
		pCHCtrl = &(otdrDev[index].ch_ctrl);
		pCHPara = &(otdrDev[index].ch_para);
		//获取otdr测量参数	
		if(usrOtdrTest.state != USR_OTDR_TEST_IDLE){
			pCHCtrl->mod = OTDR_TEST_MOD_USR;
			get_usr_otdr_test_para(&usr_para,\
					(const struct _tagUsrOtdrTest*)&usrOtdrTest);
		}
		else
			pCHCtrl->mod = OTDR_TEST_MOD_MONITOR;
		//更新本通道参数
		ret = pre_measure(index, &otdrDev[index],&usr_para);

		if(ret != OP_OK)
			continue;
		//新一轮的测试
		ret = start_otdr_test(&spiDev, NULL);
		if(ret != OP_OK)
			continue;
		//延时7s之后，读取数据
		ret = usr_delay(index,MEASURE_TIME_MIN_S);
		if(ret != OP_OK)
			continue;

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
		if(pCHCtrl->accum_num > 0){

			EstimateCurveConnect_r(chBuf[buf_index].hp_buf,NULL, NULL);
			if(!pCHCtrl->curv_cat)
			{
				pCHCtrl->hp_num = pCHCtrl->accum_num;
				pCHCtrl->lp_num = 0;
			}
			ret = otdr_test(index, &(otdrDev[index]),&spiDev, &chBuf[buf_index]);
		}
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
			//保存算法运行期间的通道信息
			algroCHInfo.ch = index;
			algroCHInfo.cmd = cmd;
			algroCHInfo.mod = pCHCtrl->mod;
			algroCHInfo.state = 1;
			algroCHInfo.resource_id = pCHState->resource_id;			
			pthread_mutex_unlock(&mutex_otdr);
			//启动算法处理
			OtdrCtrl.OtdrDataReadyFlag = ALGO_READY_FLAG_START_NEW ;


		}
		if(usrOtdrTest.state != USR_OTDR_TEST_IDLE)
			usrOtdrTest.state = USR_OTDR_TEST_IDLE;
	}
	return 0;
}
