/**
 * @file schedule_fun.c
 * @synopsis  调度时使用的公共函数
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-28
 */

#include "schedule_fun.h"
#include "program_run_log.h"
#include "../otdr_ch/hb_spi.h"
#include "global.h"
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_accum_counts  用测量时间按照3:1的比例换算出高低功率累加时间	
 * @	
 * @param measurtime	测试时间
 * @param lp_counts	高功率累加次数
 * @param hp_counts	低功率累加次数
 *
 * @returns   0 成功
 */
/* ----------------------------------------------------------------------------*/
int32_t get_accum_counts(int32_t measurtime, int32_t *lp_counts, int32_t *hp_counts)
{
	int32_t ret, accum_num;

	ret = RET_SUCCESS;

	accum_num = measurtime / MEASURE_TIME_MIN_MS;

	if(accum_num < 0)
		return RET_ERROR;
	//要确保高功率累加次数
	switch (accum_num)
	{
		case 0:
		case 1:
			{
				*lp_counts = 0;
				*hp_counts = 1;
				break;
			}
		case 2:
			{
				*lp_counts = 1;
				*hp_counts = 1;
				break;
			}
		case 3:
			{
				*lp_counts = 1;
				*hp_counts = 2;
				break;
			}
		default:
			{
				*lp_counts = accum_num / 4;
				*hp_counts = accum_num - (*lp_counts);
			}
	}
	return ret;

}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  refresh_ch_ctrl_state 更新通道的高低功率累计次数
 *
 * @param ch		通道
 * @param pCHPara	测量参数
 * @param pCtrl		控制参数
 * @param pState	状态
 *
 * @returns   0 OK ,others, error
 */
/* ----------------------------------------------------------------------------*/
int32_t refresh_ch_accum_counts(
		int ch,
		const struct _tagCHPara * pCHPara, 
		struct _tagCHCtrl *pCtrl,
		struct _tagCHState *pState)
{
	int32_t ret;
	char log_msg[NUM_CHAR_LOG_MSG] = {0};
	ret = OP_OK;
	//获取累加次数
	ret = get_accum_counts(pCHPara->MeasureTime_ms, &(pCtrl->lp_num), &(pCtrl->hp_num));
	if(ret != OP_OK)
	{
		printf("%s(): Line : %d get accum counts error ret %d teset time_ms %d\n", \
				__FUNCTION__, __LINE__, ret, pCHPara->MeasureTime_ms);
		//记录到日志里面
		snprintf(log_msg,NUM_CHAR_LOG_MSG,\
				"get accum counts error ret %d time_ms %d ch %d",\
				ret,pCHPara->MeasureTime_ms,ch);
		LOGW(__FUNCTION__,__LINE__, LOG_LEV_USUAL_MSG,log_msg);
		goto usr_exit;
	}
	pCtrl->accum_num =  pCtrl->lp_num + pCtrl->hp_num;
usr_exit:
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  OtdrUpdateParam_r 根据输入的otdr参数，更新对应的通道的OtdrCtrl
 *		和OtdrState 此函数是彭怀敏对应函数的克重入版本
 * @param ch	通道
 * @param pCHPara
 * @param pOtdrCtrl
 * @param pOtdrState
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t OtdrUpdateParam_r(
		int ch,
		const struct _tagCHPara * pCHPara, 
		OtdrCtrlVariable_t  *pOtdrCtrl,
		OtdrStateVariable_t  *pOtdrState)
{
	int32_t ret;
	char log_msg[NUM_CHAR_LOG_MSG] = {0};
	ret = OP_OK;
	/////////////////////////////////////////////////////////////
	//
	//修改彭怀敏的代码
	//
	//////////////////////////////////////////////////////////////
	uint32_t temp;
	pOtdrCtrl->LowPowerDataChanged     = 0;
	pOtdrCtrl->LowPowerDataProcessed   = 0;
	pOtdrCtrl->RefreshData        = 0;
	pOtdrCtrl->UserAction         = USER_ACTION_NO_ACTION;
	pOtdrCtrl->FindEvent          = 0;

	pOtdrCtrl->OtdrDataBusyNow    = 0;
	pOtdrCtrl->OtdrDataReadyFlag  = DATA_READY_FLAG_I_AM_DEAD;
	pOtdrCtrl->FpgaTimeOut        = 0;

	pOtdrCtrl->PulsePeriod_us     = 1000;             
	pOtdrCtrl->AccCountOnce       = TIMES_COUNT_ONCE; 
	pOtdrCtrl->RawDataLevel = 0xffffffff;

	// 根据测量参数设定下面的模式
	if(0 == pCHPara->MeasureLength_m)      
		pOtdrCtrl->MeasureMode = MEASURE_MODE_AUTO;
	else                                                    
		pOtdrCtrl->MeasureMode = MEASURE_MODE_MANUAL;

	if(0 == pCHPara->NonRelectThreshold)
		pOtdrCtrl->NonReflectThreMode = NR_MODE_AUTO;
	else                                                    
		pOtdrCtrl->NonReflectThreMode = NR_MODE_MANUAL;

	//实时模式，不刷新数据
	if(OTDR_MODE_REALTIME == pOtdrCtrl->OtdrMode)
	{
		pOtdrCtrl->RefreshData = 0;
	}

	//GroupEvent 在算法启动前初始化
	//memset(&GroupEvent, 0, sizeof(GroupEvent));
	memset(pOtdrState, 0, sizeof(OtdrStateVariable_t) );
	memcpy(&pOtdrState->MeasureParam, pCHPara, sizeof(pOtdrState->MeasureParam));

	// 接收波长
	pOtdrState->RcvLambda = pOtdrState->MeasureParam.Lambda_nm;
	pOtdrState->RcvPw     = pOtdrState->MeasureParam.PulseWidth_ns;

	//设定自动测量的脉宽，波长
	if(MEASURE_MODE_AUTO == pOtdrCtrl->MeasureMode)       // ×Ô¶¯²âÊÔ
	{
		if((850 == pOtdrState->MeasureParam.Lambda_nm) ||\
				(1300 == pOtdrState->MeasureParam.Lambda_nm))
		{
			pOtdrState->MeasureParam.MeasureLength_m = 30000; // 30km
			pOtdrState->MeasureParam.PulseWidth_ns   = 640;   // 640ns
		}
		else
		{
			pOtdrState->MeasureParam.MeasureLength_m = 180000; // 180km
			pOtdrState->MeasureParam.PulseWidth_ns   = 6000;    // 6us
		}
	}
	else
	{
		ModifyMeasureLength_PulseWidth_r(pOtdrState);
	}

	if(NR_MODE_AUTO == pOtdrCtrl->NonReflectThreMode)    
	{
		pOtdrState->MeasureParam.EndThreshold = \
							MAX(pOtdrState->MeasureParam.EndThreshold, 3);
	}
	else    
	{
		pOtdrState->MeasureParam.EndThreshold =\ 
			MAX(pOtdrState->MeasureParam.EndThreshold, 3);
		pOtdrState->MeasureParam.NonRelectThreshold =\ 
			MAX(pOtdrState->MeasureParam.NonRelectThreshold, 0.01);
	}

	//调用修改过的可重入的函数
	AdaptSampleFreq_PulsePeriod_r(ch,pOtdrCtrl, pOtdrState);

	// 设定刷新周期，河北项目中没有用到次项为了兼容，进行初始化
	temp = 3000;
	temp = MAX(temp, 1000);
	temp = MIN(temp, 5000);
	temp = (temp + 999) / 1000 * 1000;
	pOtdrState->RefreshPeriod_ms = temp;


	//按照最小测试时间进行修正 
	if(OTDR_MODE_AVG == pOtdrCtrl->OtdrMode)
	{
		temp = pOtdrState->MeasureParam.MeasureTime_ms;
		temp = MIN(temp, 180000);
		temp = MAX(temp, MEASURE_TIME_MIN_MS);
		temp = temp / MEASURE_TIME_MIN_MS * MEASURE_TIME_MIN_MS;
		pOtdrState->MeasureParam.MeasureTime_ms = temp;
	}
	// 实时模式
	else if(OTDR_MODE_REALTIME == pOtdrCtrl->OtdrMode)
	{
		pOtdrState->MeasureParam.MeasureTime_ms = pOtdrState->RefreshPeriod_ms;
	}
	else if(OTDR_MODE_CYCLE_TEST != pOtdrCtrl->OtdrMode)
	{
		pOtdrState->MeasureParam.MeasureTime_ms = 15000;
	}

	//功率模式
	PowerModeInit_r(pOtdrCtrl, pOtdrState);
	OtdrStateInit_r(pOtdrState);

	return ret;
}
void ModifyMeasureLength_PulseWidth_r(
		OtdrStateVariable_t  *pOtdrState
		)
{
	int32_t ret;
	uint32_t i, MeasureLength_m, PulseWidth_ns;

	ret = OP_OK;
	MeasureLength_m = pOtdrState->MeasureParam.MeasureLength_m;
	PulseWidth_ns   = pOtdrState->MeasureParam.PulseWidth_ns;

	//调整脉宽 
	if(PulseWidth_ns < 10)
	{
		PulseWidth_ns = 5;
	}
	else if(PulseWidth_ns < 20)
	{
		PulseWidth_ns = 10;
	}
	else
	{
		PulseWidth_ns = (PulseWidth_ns / 20) * 20;
	}

	// 调整量程
	MeasureLength_m = MAX(MeasureLength_m, OtdrMeasureLength[0]);
	MeasureLength_m = MIN(MeasureLength_m, OtdrMeasureLength[MEASURE_LENGTH_NUM-1]);
	for(i = 0; i < MEASURE_LENGTH_NUM; i++)
	{
		if(MeasureLength_m <= OtdrMeasureLength[i])
		{
			MeasureLength_m = OtdrMeasureLength[i];
			break;
		}
	}

	pOtdrState->MeasureParam.MeasureLength_m = MeasureLength_m;
	pOtdrState->MeasureParam.PulseWidth_ns   = PulseWidth_ns;

	return;
}
void AdaptSampleFreq_PulsePeriod_r(
		int ch,
		OtdrCtrlVariable_t  *pOtdrCtrl,
		OtdrStateVariable_t  *pOtdrState
		)
{
	uint32_t MeasureLength_m = pOtdrState->MeasureParam.MeasureLength_m;
	uint32_t index, clk_num;

	index   = GetMeasureLengthIndex(MeasureLength_m);
	clk_num = OtdrSampleRate[index] / CLK_50MHz;
	clk_num = MAX(clk_num, 1);

	pOtdrState->RealSampleRate_MHz = OtdrSampleRate[index];
	pOtdrState->RealSampleRate_Hz  = OtdrSampleRateHz[index];

	pOtdrCtrl->PulsePeriod_us = OtdrPulsePeriod[index];
	pOtdrCtrl->AccCountOnce =\
				 (uint32_t)(1000*TIMES_COUNT_ONCE/pOtdrCtrl->PulsePeriod_us / clk_num);
	pOtdrCtrl->NullReadCount       = 2*clk_num;
}
int32_t IsCatCurve(uint32_t pl_ns, uint32_t measure_m)
{
	int32_t ret;
	ret = 0;
	if(pl_ns >= MIN_PW_DATA_CONCAT && measure_m >= 100000)
		ret = 1;
	return ret;
}
void PowerModeInit_r(
		OtdrCtrlVariable_t  *pOtdrCtrl,
		OtdrStateVariable_t *pOtdrState)
{
	int is_cat;
	is_cat = IsCatCurve(pOtdrState->MeasureParam.PulseWidth_ns, \
			pOtdrState->MeasureParam.MeasureLength_m);
	if(is_cat)
	{
		if(OTDR_MODE_REALTIME == pOtdrCtrl->OtdrMode)
		{
			pOtdrCtrl->PowerMode = POWER_MODE_LOW; 
		}
		else    
		{
			pOtdrCtrl->PowerMode = POWER_MODE_UNDEF;
		}
	}
	else 
	{	
		pOtdrCtrl->PowerMode = POWER_MODE_HIGH;
	}
}

void OtdrStateInit_r(OtdrStateVariable_t  *pOtdrState)
{
	uint32_t i, MeasureLength_m, PulseWidth_ns;
	
	MeasureLength_m = pOtdrState->MeasureParam.MeasureLength_m;
	PulseWidth_ns = pOtdrState->MeasureParam.PulseWidth_ns;
	i = GetMeasureLengthIndex(MeasureLength_m);

	pOtdrState->TreatAsHighPowerData = 1;
	pOtdrState->PowerLevel = MAX_POWER_LEVEL;

	pOtdrState->M = PulseWidthInSampleNum_r(PulseWidth_ns, pOtdrState->RealSampleRate_Hz);
	pOtdrState->Points_1m = 2*pOtdrState->MeasureParam.n * pOtdrState->RealSampleRate_Hz /C;
	if(MeasureLength_m > 60000)
	{
		if(PulseWidth_ns > 10240)           pOtdrState->MinNoiseAmp = -8;
		else if(PulseWidth_ns > 5120)       pOtdrState->MinNoiseAmp = -7;
		else if(PulseWidth_ns > 2560)       pOtdrState->MinNoiseAmp = -6;
		else                                pOtdrState->MinNoiseAmp = -5;
	}
	else    pOtdrState->MinNoiseAmp = -5;

	if(pOtdrState->MeasureParam.MeasureLength_m == 100000)   
		pOtdrState->MeasureLengthPoint = DATA_LEN - NOISE_LEN;
	else
	{
		pOtdrState->MeasureLengthPoint = (int32_t)(pOtdrState->MeasureParam.MeasureLength_m * pOtdrState->Points_1m);
		pOtdrState->MeasureLengthPoint = MIN(pOtdrState->MeasureLengthPoint, DATA_LEN-NOISE_LEN);
	}
	pOtdrState->CurveStartPoint = OtdrParam.OtdrStartPoint[i];
}
int32_t PulseWidthInSampleNum_r (uint32_t pl_ns, uint32_t sample_hz)
{
    Int32 m;
    m = (int32_t)(pl_ns * (sample_hz / 1000) / 1e6);
    m = MAX(m, 2);
    return m;
}
void EstimateCurveConnect_r(
		int32_t chan_data[],
		OtdrCtrlVariable_t  *pOtdrCtrl,
		OtdrStateVariable_t  *pOtdrState)
{
	int32_t   m, i, avg, sigma, Cn, accept, FrontFlat = 0, fiberlessthan30km = 0;
	int32_t   *An = NULL;
	uint32_t  PulseWidth_ns;
	float   v, k;

	An = chan_data;

	PulseWidth_ns       = pOtdrState->MeasureParam.PulseWidth_ns;
	RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
	EnlargeData(An, DATA_LEN, 128);
	AdjustCurve(An, DATA_LEN);
	i = GetSaturateThreshold(An);
	//debug
	if(i == 0)
	{
		printf( "%s():line : %d NOT found saturation",__FUNCTION__, __LINE__);
	}
	m = GetNfirWidth(pOtdrCtrl->PowerMode);   // 2013-1-11 9:17:01
	nfir(An, An, DATA_LEN, m);  // 2011-7-4 11:37:27
	sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-pOtdrState->M);
	pOtdrState->sigma = sigma;

	i = MAX(m, 128);
	CapacityLinearCompensate(An, DATA_LEN, i, sigma);


	int32_t Pos5dB, tmp;

	An = chan_data;
	Cn = (Int32)(pOtdrState->Points_1m * 30000);
	MeanValue(An, Cn-20, Cn+20, &avg, DATA_TYPE_INT);

	Pos5dB = DATA_LEN;
	tmp = Tenth_div5(6.0) * sigma;
	for(i = pOtdrState->M; i < DATA_LEN; i++)
	{
		if(An[i] <= tmp)
		{
			Pos5dB = i;
			break;
		}
	}

	if((avg < sigma*Tenth_div5(5)) || (Cn > Pos5dB))
	{
		fiberlessthan30km = 1;
	}

	int32_t temp, cc = 0;

	Cn = GetPulseWidthSaturateBlindZone(pOtdrState->MeasureParam.Lambda_nm, PulseWidth_ns);
	Cn = (Int32)(pOtdrState->Points_1m * Cn);

	An = chan_data;
	temp = pOtdrState->SatThreshold;
	cc = 0;
	//10平顶的，跳出
	for(i = Cn+5; i < Cn+200; i++)
	{
		if(An[i] >= temp)
		{
			if(++cc > 10)
			{
				FrontFlat = 1;
				break;
			}
		}
	}
	//前端不平顶
	if(!FrontFlat)
	{
		pOtdrCtrl->CurveConcat = 0;
		pOtdrCtrl->PowerMode = POWER_MODE_HIGH;
		printf("%s():line:%d front not saturate, use high power mode\n",\
				__FUNCTION__, __LINE__);
		return;
	}
	//下面的处理代码是总前端平顶的情况下进行的
	if(fiberlessthan30km)
	{
		UseLowPowerMode_r(pOtdrCtrl, pOtdrState);
		return;
	}
	

	m = PulseWidthInSampleNum();
	for(i = Cn+10; i < DATA_LEN; i++)
	{
		if(An[i] < pOtdrState->SatThreshold)
		{
			pOtdrState->CurveConcatPoint = \
				i + (Int32)(pOtdrState->Points_1m * 4000);  
			pOtdrState->CurveConcatPoint = \
				MAX(pOtdrState->CurveConcatPoint, i+3*m);

			if((pOtdrState->CurveConcatPoint >= \
						pOtdrState->MeasureLengthPoint*8/10) ||\
					(pOtdrState->CurveConcatPoint >= DATA_LEN - 6*m))
			{
				UseLowPowerMode_r(pOtdrCtrl, pOtdrState);
				printf("%s(): Line:%d connection point is too far away, use low power mode\n",__FUNCTION__, __LINE__);
				return;
			}
			break;
		}
	}



	if(pOtdrCtrl->CurveConcat)
	{
		An = chan_data;
		Cn = pOtdrState->CurveConcatPoint;
		GetEventFindingParamBeforeLog(NULL, NULL, &k, &m, "Large", 0);
		k = k*m;
		while((Cn < pOtdrState->MeasureLengthPoint*8/10) && (Cn < DATA_LEN - 6*m))
		{
			accept = 1;
			for(i = Cn-m/2; i < Cn+m/2; i++)
			{
				if((An[i] < sigma) || (An[i+m] < sigma))
				{
					pOtdrCtrl->CurveConcat = 0;
					accept                = -1;
					break;
				}
				v = (float)An[i] / An[i+m];
				v = 5* FastLog10(v) - k;

				if(fabs(v) > 0.5) 
				{
					accept = 0;
					break;
				}
			}
			if(1 == accept)
			{
				pOtdrState->CurveConcatPoint = Cn;
				break;
			}
			else if(-1 == accept)
			{
				UseLowPowerMode_r(pOtdrCtrl, pOtdrState);
				printf("%s(): line:%d can't find a proper cat point, use low power mode\n",__FUNCTION__, __LINE__);
				return;
			}
			else    Cn += m;
		}
	}

	if(pOtdrCtrl->CurveConcat)
	{
		MeanValue(An, pOtdrState->CurveConcatPoint-m, \
				pOtdrState->CurveConcatPoint+m, &avg, DATA_TYPE_INT);
		if(avg < sigma*Tenth_div5(15))
		{
			UseLowPowerMode_r(pOtdrCtrl, pOtdrState);     // 2012-12-12 10:19:42
			printf("%s():%d connection point lower than 15dB, use low power mode\n"\
					,__FUNCTION__, __LINE__);
			return;
		}
	}

	if(pOtdrCtrl->CurveConcat)
	{
		//        pOtdrState->CurveConcatPoint = 6200; // debug
		pOtdrCtrl->PowerMode = POWER_MODE_COMBINE;
		TCPDEBUG_PRINT("connection point = %d(%.3fkm)\n", pOtdrState->CurveConcatPoint,
				(float)pOtdrState->CurveConcatPoint/pOtdrState->Points_1m/1000);
	}
	return ;
}

 void UseLowPowerMode_r(
		OtdrCtrlVariable_t  *pOtdrCtrl,
		OtdrStateVariable_t  *pOtdrState)
{
    pOtdrCtrl->CurveConcat = 0;
    pOtdrCtrl->PowerMode    = POWER_MODE_LOW;
    pOtdrState->CurveConcatPoint = 0;
    return;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  pre_measure 测量前更新相关参数
 *
 * @param ch 通道
 * @param potdrDev otdr资源
 *
 * @returns   0 ok others error
 */
/* ----------------------------------------------------------------------------*/
int32_t pre_measure(int32_t ch, struct _tagOtdrDev *potdrDev,struct _tagCHPara *pUsrPara)
{
	int ret;
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHState *pCHState;
	struct _tagCHPara *pCHPara;

	ret = OP_OK;

	pCHState = &(potdrDev->ch_state);
	pCHCtrl = &(potdrDev->ch_ctrl);
	//如果是用户测量，则使用用户指定参数
	if(pCHCtrl->mod == OTDR_TEST_MOD_USR)
		pCHPara = pUsrPara;
	else
		pCHPara = &(potdrDev->ch_para);
	//运行算法的时候根据下面的标志选取对应的通道进行操作
	//更新本通道参数
	ret = OtdrUpdateParam_r(ch,pCHPara, &potdrDev->otdr_ctrl,&potdrDev->otdr_state);
	if(ret != OP_OK)
	{
		printf("%s():%d: ch %d update para error,ret %d.\n",\
				__FUNCTION__,__LINE__, ch, ret);
		return ret;
	}
	//获取累加次数
	ret = get_accum_counts(pCHPara->MeasureTime_ms,&(pCHCtrl->hp_num),\
			&(pCHCtrl->lp_num));	
	if(ret != OP_OK)
	{
		printf("%s():%d: ch %d get accum counts error,ret %d.\n",\
				__FUNCTION__,__LINE__,ret);
		return ret;
	}
	return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  usr_delay 延时函数，使用sleep延时，每次1s
 *
 * @param time_s
 *
 * @returns   0, 超时退出，-1中断测量退出 
 */
/* ----------------------------------------------------------------------------*/
int32_t usr_delay(int32_t ch, int32_t time_s)
{
	int32_t ret,count, sleep_time;
	ret = OP_OK;
	count = time_s;

	while(count > 1)
	{
		if(usrOtdrTest.state == USR_OTDR_TEST_WAIT){
			ret = -1;
			break;
		}
		ret = sleep(1);
		//返回值为剩余的时间，如果其他原因返回了继续睡觉
		if(ret == 0)
			count--;
	}

	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  otdr_test 根据高低功率累加次数进行测量，并读取数据，完成累加
 *	该函数回阻塞等待叠加时间
 * @param ch	通道编号
 * @param potdrDev	otdr设备
 * @param pspiDev	spi设备
 * @param pchBuf	存储空间
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t otdr_test(
		int32_t ch, 
		struct _tagOtdrDev *potdrDev,
	   	struct _tagSpiDev *pspiDev,
		struct _tagCHBuf *pchBuf)
{
	int32_t ret;
	struct _tagCHCtrl *pCHCtrl;
	struct _tagCHState *pCHState;
	struct _tagLaserCtrPara *plaser_para;
	struct _tagCHPara *pCHPara;
	ret = OP_OK;

	pCHState = &(potdrDev->ch_state);
	pCHCtrl = &(potdrDev->ch_ctrl);
	pCHPara = &(potdrDev->ch_para);
	while(pCHCtrl->accum_num > 0)
	{
		//如果需要拼接，高低功率均需要曲线均需要采集，否则，只需要高功率曲线
		if(pCHCtrl->hp_num > 0)
			ret = start_otdr_test(ch,pspiDev,pCHPara, plaser_para);
		else if(pCHCtrl->lp_num > 0)
			ret = start_otdr_test(ch, pspiDev, pCHPara, plaser_para);
		else
			break;

		if(ret != OP_OK)
			break;
		usr_delay(ch,MEASURE_TIME_MIN_S);

		pCHCtrl->accum_num--;
		pCHCtrl->hp_num--;
		if(ret != OP_OK)
			break;
	}
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_usr_otdr_test_para 将点名测量的参数赋值给本地
 *
 * @param pusr_para
 * @param pnet_para
 *
 * @returns   0
 */
/* ----------------------------------------------------------------------------*/
int32_t get_usr_otdr_test_para(
		struct _tagCHPara *pusr_para, 
		const struct _tagUsrOtdrTest *pnet_para)
{
	int32_t ret;
	ret = OP_OK;
	pusr_para->Lambda_nm = pnet_para->wl;
	pusr_para->MeasureLength_m = pnet_para->range;
	pusr_para->MeasureTime_ms = pnet_para->time*1000;
	pusr_para->NonRelectThreshold = pnet_para->none_ref_th;
	pusr_para->PulseWidth_ns = pnet_para->pw;
	pusr_para->n = pnet_para->gi;
	pusr_para->EndThreshold = pnet_para->end_th;
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_laser_ctrl_para 根据量程，脉宽，波长获取激光器功率，接收机
 *			夸阻.一次测量使用高功率
 * @param range_m	量程
 * @param pl_ns		脉宽
 * @param lamda		波长
 * @param is_low_power	是否使用低功率，仅在100km以上适用
 * @param plaser_ctr_para	传递出去的参数
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t get_laser_ctrl_para(
		int32_t range_m,
		int32_t pl_ns,
		int32_t lamda,
		int32_t is_low_power,
		struct _tagLaserCtrPara *plaser_ctr_para)
{
	uint32_t i, j, power, hp, lp, rcv, rh, rl;
	uint32_t apdv;

	OtdrPowerLevel_t OtdrPowerLevel;

	i  = GetMeasureLengthIndex(range_m);
	j  = GetPulseWidthIndex(pl_ns);
	OtdrPowerLevel = GetPowerLevelIndex(lamda);

	power = OtdrPowerLevel[i][j];
	rcv = OtdrReceiver[i][j];


	rh = (rcv >> 8) & 0xff;
	rl = (rcv & 0xff);
	if(rh == 0)
		rh = rl;

	hp = (power >> 8) & 0xff;
	lp = (power & 0xff);
	if(hp == 0)
		hp = lp;


	if(!is_low_power)
	{
		power = hp;
		rcv = rh;
	}
	else 
	{
		power = lp;
		rcv = rl;
	}

	apdv = APDV_LOW;

	if(pl_ns <= 40)
	{
		apdv = APDV_HIGH;
	}

	plaser_ctr_para->rcv = rcv;
	plaser_ctr_para->power = power;
	plaser_ctr_para->apd = apdv;
	if(plaser_ctr_para->rcv == R_6)           plaser_ctr_para->trail_length = 1000;
	else if(plaser_ctr_para->rcv == R_7)      plaser_ctr_para->trail_length = 1500;
	else if(plaser_ctr_para->rcv == R_A)      plaser_ctr_para->trail_length = 2000;
	else if(plaser_ctr_para->rcv == R_C)      plaser_ctr_para->trail_length = 3000;
	else                                    plaser_ctr_para->trail_length = 5000;
	
}
