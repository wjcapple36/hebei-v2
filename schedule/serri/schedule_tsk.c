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
#include "../common/hb_app.h"
#include "../common/global.h"
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
//spi 读取数据的缓冲区 spi 数据格式： fe 4B 2B fe 4B 2B
#define SPI_OP_SPACE_MS		3000 //spi设备的操作间隔  

/*
 *本任务负责根据通道参数更新OtdrCtrl,OtdrState，
 *并将获取到的高低功率数据复制到OtdrData，并启动算法运算
 */
extern OTDR_ChannelData_t OtdrData;
extern OtdrCtrlVariable_t OtdrCtrl; 
extern OtdrStateVariable_t OtdrState; 
extern pthread_mutex_t mutex_otdr;


static int32_t get_test_ch(int32_t ch, int32_t step);
/* --------------------------------------------------------------------------*/
/**
 i* @synopsis  thread_schedule 
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
#if 0
struct _tagThreadInfo tsk_schedule_info;
int32_t tsk_schedule(void *arg)
{
	int32_t index, ret, buf_index, last_index;
	uint32_t cmd;
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHState *pCHState;
	struct _tagCHPara *pCHPara;
	struct _tagCHPara appoint_test_para;
	struct _tagLaserCtrPara laser_para;
	struct _tagThreadInfo *ptsk_info;
	const int32_t invalid_index = -1;
	const int32_t step = 1;
	
	ptsk_info = (struct _tagThreadInfo *)arg;
	//htop 命令显示的就是下面这个东西
	ptsk_info->htop_id = (long int)syscall(224);
	
	
	

	index = invalid_index;
	last_index = invalid_index;

	while(1)
	{
		index += step;
		index %= CH_NUM;
		index = get_test_ch(index,step);
		//如果是点名测量，则保存上次的轮询的通道号
		if(usrOtdrTest.state == USR_OTDR_TEST_WAIT){
			usrOtdrTest.state = USR_OTDR_TEST_ACCUM;
			//如果last_index不等-1，说明是连续点名测量
			last_index = last_index == invalid_index ? index : last_index;
			index = usrOtdrTest.ch;
			cmd = usrOtdrTest.cmd;
			
		}
		else{
			if(last_index != invalid_index)
				index = last_index;
			last_index = invalid_index;
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
			get_usr_otdr_test_para(&appoint_test_para,\
					(const struct _tagUsrOtdrTest*)&usrOtdrTest);
		}
		else
			pCHCtrl->mod = OTDR_TEST_MOD_MONITOR;
		
		//更新本通道参数
		ret = pre_measure(index, &otdrDev[index],&appoint_test_para);

		if(ret != OP_OK)
			continue;
		//新一轮的测试
		get_laser_ctrl_para(pCHPara->MeasureLength_m,
			pCHPara->PulseWidth_ns,
			pCHPara->Lambda_nm,
			0,
			&laser_para);

		ret = start_otdr_test(index, &spiDev, pCHPara,&laser_para);
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
		ret = read_otdr_data(index, &spiDev, pCHPara, &laser_para,NULL,0);
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
#endif
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_test_ch  从输入的通道号中获取可测量通道
 *
 * @param ch	 通道
 * @param step	步长，下一步设想，分组测试，组内并行，组与组之间串行
 *
 * @returns   返回通道
 */
/* ----------------------------------------------------------------------------*/
static int32_t get_test_ch(int32_t ch, int32_t step)
{
	int32_t index, is_return;
	is_return = 0;
	index = ch;
	while(1)
	{
		for(;index < CH_NUM;index +=step)
		{
			if(otdrDev[index].ch_ctrl.enable ||\
					usrOtdrTest.state == USR_OTDR_TEST_WAIT){
				is_return = 1;
				break;
			}
		}
		if(is_return)
			break;
		else
			index = 0;
		usleep(500000);
	}
	index %= CH_NUM;
	return index;
}

/* --------------------------------------------------------------------------*/
/**
 i* @synopsis  thread_schedule 
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
//保存调度线程参数
struct _tagThreadInfo tsk_schedule_info;
//系统管理otdr线程的参数
struct _tagOtdrAlgroPara OtdrAlgroPara;
#define RET_SWITCH_CH   0x80000036
#define OTDR_MOD_SERRI
int32_t process_every_ch(
		int32_t ch,
	       	struct _tagOtdrDev * pOtdrDev,
		struct _tagUsrOtdrTest *pUsrTest,
		struct _tagSpiDev *pspi_dev,
		struct _tagOtdrAlgroPara *pAlgroPara);
int32_t first_process(
		int32_t ch,
	       	struct _tagOtdrDev * pOtdrDev,
		struct _tagUsrOtdrTest *pUsrTest,
		struct _tagSpiDev *pspi_dev);
int32_t agin_process(
		int32_t ch,
		struct _tagOtdrDev * pOtdrDev,
		struct _tagUsrOtdrTest *pUsrTest,
		struct _tagSpiDev *pspi_dev,
		struct _tagOtdrAlgroPara *pAlgroPara			
		);

int32_t initial_otdr_dev(struct _tagOtdrDev *dev);
int32_t start_otdr_algro(
		int32_t ch,
	       	struct _tagOtdrDev * pOtdrDev,
		struct _tagUsrOtdrTest *pUsrTest,
		struct _tagOtdrAlgroPara *pAlgroPara);			

int32_t tsk_schedule(void *arg)
{
	int32_t ch, ret, buf_index, next_ch;
	int32_t working_ch;
	const int32_t sleep_time = 3;
	struct _tagThreadInfo *ptsk_info;
	ptsk_info = (struct _tagThreadInfo *)arg;
	//htop 命令显示的就是下面这个东西
	ptsk_info->htop_id = (long int)syscall(224);
	
	printf("%s %s  pid %d  self_id 0x%x \n",__FILENAME__, __FUNCTION__, ptsk_info->htop_id,(int)pthread_self());

	OtdrAlgroPara.pCHInfo = &algroCHInfo;
	OtdrAlgroPara.pCtrl = &OtdrCtrl;
	OtdrAlgroPara.pState = &OtdrState;
	while(1)
	{
		for(ch = 0; ch < CH_NUM; ch++)
		{
#ifdef OTDR_MOD_SERRI
			//处理点名测量
			if(usrOtdrTest.state == USR_OTDR_TEST_WAIT){
				usrOtdrTest.reserv_ch = ch;
				ch = usrOtdrTest.ch % CH_NUM;
				otdrDev[working_ch].ch_ctrl.on_ff = OTDR_CH_OFF;
				initial_otdr_dev(&otdrDev[working_ch]);
				otdrDev[ch].ch_ctrl.on_ff = OTDR_CH_ON;
				working_ch = ch;
			}
			//顺序执行,如果当前通道没有处理完，不处理别的通道
			if(otdrDev[ch].ch_ctrl.on_ff == OTDR_CH_ON){
				ret = process_every_ch(ch,&otdrDev[ch],&usrOtdrTest, &spiDev, &OtdrAlgroPara);
				if(ret == RET_SWITCH_CH){
					next_ch = (ch + 1)%CH_NUM;
					otdrDev[next_ch].ch_ctrl.on_ff = OTDR_CH_ON;
					working_ch = ch;
				}
				else
					break;
			}
#else
			//如果是并行otdr，管他娘的，直接处理了
			ret = process_every_ch(ch,&otdrDev[ch],&usrOtdrTest, &spiDev);
#endif
		}
		//该函数每1s休眠一次，如果有otdr点名测试，则及时返回
		usr_delay(0, SPI_OP_SPACE_MS/1000);
	}

}
int32_t process_every_ch(
		int32_t ch,
		struct _tagOtdrDev * pOtdrDev,
		struct _tagUsrOtdrTest *pUsrTest,
		struct _tagSpiDev *spi_dev,
		struct _tagOtdrAlgroPara *pAlgroPara)
{
	uint32_t cmd;
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHPara *pCHPara;
	int32_t ret;
	ret = OP_OK;
	/*
	 * 如果有点名测量任务，如果是本通道，那么终止当前任务
	 * 如果不是当前通道，直接返回
	 */
	if(pUsrTest->state == USR_OTDR_TEST_WAIT){
		pUsrTest->state = USR_OTDR_TEST_ACCUM;
		if(pUsrTest->ch == ch) {
			pOtdrDev->ch_ctrl.accum_num = 0;
			pOtdrDev->ch_ctrl.mod = OTDR_TEST_MOD_USR;
			pUsrTest->reserv_ch = ch;
		}
		else if(pOtdrDev->ch_ctrl.on_ff == OTDR_CH_ON ) {
			ret = RET_SWITCH_CH;
			goto usr_exit;
		}
	}
	/*
	 *如果是监测模式，并且没有配置，直接返回
	 */
	if((pOtdrDev->ch_ctrl.mod == OTDR_TEST_MOD_MONITOR)&&
			!pOtdrDev->ch_ctrl.is_cfged){
		ret = RET_SWITCH_CH;
		goto usr_exit;
	}

	if(!pOtdrDev->ch_ctrl.send_num)
		ret = first_process( ch, pOtdrDev, pUsrTest,spi_dev);
	else
		ret = agin_process( ch, pOtdrDev, pUsrTest,spi_dev, pAlgroPara);


	pOtdrDev->ch_ctrl.send_num++;

usr_exit:
	//切换通道前，将本通道初始化
	if(ret == RET_SWITCH_CH){
		if(pOtdrDev->ch_ctrl.mod == OTDR_TEST_MOD_USR)
			memset(pUsrTest, 0, sizeof(struct _tagUsrOtdrTest));
		pOtdrDev->ch_ctrl.on_ff = OTDR_CH_OFF;
		initial_otdr_dev(pOtdrDev);//控制变量恢复默认值
	}
	return ret;
}
int32_t first_process(
		int32_t ch,
		struct _tagOtdrDev * pOtdrDev,
		struct _tagUsrOtdrTest *pUsrTest,
		struct _tagSpiDev *spi_dev)
{
	int32_t ret;
	struct _tagCHPara appoint_para;
	struct _tagCHPara *pCHPara;
	struct _tagCHCtrl *pCHCtrl;
	struct tms_ack ack;
	pCHPara = &pOtdrDev->ch_para;
	pCHCtrl = &pOtdrDev->ch_ctrl;

	ret = OP_OK;
	if( pCHCtrl->mod == OTDR_TEST_MOD_USR){
		get_usr_otdr_test_para(&appoint_para,pUsrTest);
		ret = pre_measure(ch, pOtdrDev,&appoint_para);
	}
	else
		ret = pre_measure(ch, pOtdrDev, NULL);

	if(ret != OP_OK){
		goto usr_exit;

	}
	//新一轮的测试
	get_laser_ctrl_para(pCHPara->MeasureLength_m,
			pCHPara->PulseWidth_ns,
			pCHPara->Lambda_nm,
			1,
			&pOtdrDev->laser_para);
	ret = start_otdr_test(ch, spi_dev, pCHPara,&pOtdrDev->laser_para);
usr_exit:
	/*
	 * 如果操作失败，那么指定下一个可测量的通道;
	 * 如果是点名测量，还要通知host
	 */
	if(ret != OP_OK){
		ack.cmdid = pUsrTest->cmd;
		ack.errcode =  CMD_RET_TEST_ERROR;
		hb_Ret_Ack(pUsrTest->src_addr, ack);
		ret = RET_SWITCH_CH;
	}

	return ret;
}
int32_t agin_process(
		int32_t ch,
		struct _tagOtdrDev * pOtdrDev,
		struct _tagUsrOtdrTest *pUsrTest,
		struct _tagSpiDev *pspi_dev,
		struct _tagOtdrAlgroPara *pAlgroPara)
{
	int32_t ret, is_ack;
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHPara *pCHPara;
	struct _tagCHState *pCHState;
	struct _tagCHBuf *pCHBuf;
	struct tms_ack ack;
	OtdrCtrlVariable_t *pOtdrCtl;	
	OtdrStateVariable_t *pOtdrState;

	pCHCtrl = &pOtdrDev->ch_ctrl;
	pCHPara = &pOtdrDev->ch_para;
	pCHState = &pOtdrDev->ch_state;
	pCHBuf = &pOtdrDev->ch_buf;
	pOtdrState = &pOtdrDev->otdr_state;
	pOtdrCtl = &pOtdrDev->otdr_state;
	ret = OP_OK;
	is_ack = 1;

	ret = read_otdr_data(ch, pspi_dev,pCHPara, &pOtdrDev->laser_para,pspi_dev->buf, pspi_dev->len);
	if(ret == SPI_RET_RCV_ERROR){
		ret = RET_SWITCH_CH;
		goto usr_exit;
	}
	//接收数据失败，如果累积时间达到测量时间之后，判定为失败
	if(ret){
		pCHCtrl->accum_ms += SPI_OP_SPACE_MS;
		if(pCHCtrl->accum_ms > ( pCHPara->MeasureTime_ms + 7000)){
			is_ack = 0;
			ret = RET_SWITCH_CH;
			goto usr_exit;
		}
		else{
			ret = OP_OK;
			goto usr_exit;
		}
	}
	ret =  get_data_from_spi_buf(pCHBuf->hp_buf, DATA_LEN, pspi_dev->buf,pspi_dev->len,1);
	if(ret){
		ret = RET_SWITCH_CH;
		goto usr_exit;
	}
	//第一次收到数据，要判断是否拼接
	if(pCHCtrl->send_num == 1){
		EstimateCurveConnect_r(pCHBuf->hp_buf,pOtdrCtl, pOtdrState);
		pCHCtrl->curv_cat = pOtdrCtl->CurveConcat;
		if(!pOtdrCtl->CurveConcat){
			pCHCtrl->hp_num = pCHCtrl->accum_num;
			pCHCtrl->lp_num = 0;
		}

	}
	//测量完成，启动算法进程
	if(pCHCtrl->accum_num <= 0){
		is_ack = 0; 
		ret = RET_SWITCH_CH;
		goto usr_exit;
	}

	if(pCHCtrl->hp_num > 0)
		ret = start_otdr_test(ch, pspi_dev, pCHPara,&pOtdrDev->laser_para);
	else{
		if(pCHCtrl->hp_num == 0){
			pCHCtrl->hp_num--;
			get_laser_ctrl_para(pCHPara->MeasureLength_m,
					pCHPara->PulseWidth_ns,
					pCHPara->Lambda_nm,
					1,
					&pOtdrDev->laser_para);
		}
		ret = start_otdr_test(ch, pspi_dev, pCHPara,&pOtdrDev->laser_para);
	}

usr_exit:
	if(pCHCtrl->mod == OTDR_TEST_MOD_USR && is_ack && ret != OP_OK){
		ack.cmdid = pUsrTest->cmd;
		ack.errcode = CMD_RET_TEST_ERROR;
		hb_Ret_Ack(pUsrTest->src_addr, ack);
	}

	return ret;
}
int32_t initial_otdr_dev(struct _tagOtdrDev *dev)
{
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHState *pCHState;

	pCHCtrl = &dev->ch_ctrl;
	pCHState = &dev->ch_state;

	pCHCtrl->mod = 0;		//0,轮询，1点名测量
	pCHCtrl->on_ff = OTDR_CH_OFF;		//1, on, 0 ff, 串行工作模式，如果处于off状态则意味这不可操作
	pCHCtrl->send_num = 0;	//发送测量参数次数
	pCHCtrl->accum_num = 0;	//累加次数0，表示本通道累加结束
	pCHCtrl->hp_num = 0;		//高功率次数
	pCHCtrl->lp_num = 0;		//低功率次数
	pCHCtrl->cur_pw_mod = 0;	//当前测量的是高功率还是低功率
	pCHCtrl->curv_cat = 0;	//曲线拼接标志
	pCHCtrl->accum_ms = 0;
	memset(&dev->ch_buf.hp_buf, 0, sizeof(dev->ch_buf.hp_buf));
	memset(&dev->ch_buf.lp_buf, 0, sizeof(dev->ch_buf.lp_buf));

	return OP_OK;
}
int32_t start_otdr_algro(
		int32_t ch,
	       	struct _tagOtdrDev * pOtdrDev,
		struct _tagUsrOtdrTest *pUsrTest,
		struct _tagOtdrAlgroPara *pAlgroPara)
{
	int32_t ret;
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHPara *pCHPara;
	struct _tagCHState *pCHState;
	struct _tagCHBuf *pCHBuf;
	struct tms_ack ack;
	struct _tagAlgroCHInfo *pAlgroCHInfo;

	OtdrCtrlVariable_t *pOtdrCtl;	
	OtdrStateVariable_t *pOtdrState;


	ret = 0;
	pCHCtrl = &pOtdrDev->ch_ctrl;
	pCHPara = &pOtdrDev->ch_para;
	pCHState = &pOtdrDev->ch_state;
	pCHBuf = &pOtdrDev->ch_buf;
	pOtdrState = &pOtdrDev->otdr_state;
	pOtdrCtl = &pOtdrDev->otdr_state;
	pAlgroCHInfo = &pAlgroPara->pCHInfo;

	//说明算法没有处理完成，7s的时间没有处理完，说明有问题
	if(ALGO_READY_FLAG_START_NEW == pAlgroPara->pCtrl->OtdrAlgoReadyFlag)
		pCHState->algro_collid_num++;
	//只要能获取到信号量，说明那边的算法已经处理完毕
	pthread_mutex_lock(&mutex_otdr);
	memset(&pAlgroPara->pCHInfo, 0, sizeof(struct _tagAlgroCHInfo));
	memcpy(&pAlgroPara->pCtrl, pOtdrCtl,sizeof(OtdrCtrl));
	memcpy(&pAlgroPara->pState,pOtdrState, sizeof(OtdrState));
	//保存算法运行期间的通道信息
	pAlgroCHInfo->ch = ch;
	pAlgroCHInfo->mod = pCHCtrl->mod;
	if(pAlgroCHInfo->mod == OTDR_TEST_MOD_USR){
		pAlgroCHInfo->cmd = pUsrTest->cmd;
		pAlgroCHInfo->src_addr = pUsrTest->src_addr;
		get_usr_otdr_test_para((struct _tagCHPara *)(&OtdrState.MeasureParam), pUsrTest);

	}

	pAlgroCHInfo->state = 1;
	pAlgroCHInfo->resource_id = pCHState->resource_id;			
	pthread_mutex_unlock(&mutex_otdr);
	//启动算法处理
	pAlgroPara->pCtrl->OtdrDataReadyFlag = ALGO_READY_FLAG_START_NEW ;
	return ret;
}
