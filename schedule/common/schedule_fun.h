/**
 * @file schedule_fun.h
 * @synopsis  通道调度函数接口的头文件
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-28
 */
#ifndef _SCHEEDULE_FUN_H
#define  _SCHEEDULE_FUN_H

#include "../otdr_ch/otdr_ch.h"

#ifdef __cplusplus
extern "C" {
#endif
	//获取累加次数
	int32_t get_accum_counts(int32_t measurtime, int32_t *lp_counts, int32_t *hp_counts);
	//更新通道高低功率累计次数
	int32_t refresh_ch_accum_counts(
			int ch,
			const struct _tagCHPara * pCHPara, 
			struct _tagCHCtrl *pCtrl,
			struct _tagCHState *pState);
	//otdr算法中的函数可重入版本
	int32_t OtdrUpdateParam_r(
			int ch,
			const struct _tagCHPara * pCHPara, 
			OtdrCtrlVariable_t  *pOtdrCtrl,
			OtdrStateVariable_t  *pOtdrState);
	//校正用户输入的量程脉宽
	void ModifyMeasureLength_PulseWidth_r(
			OtdrStateVariable_t  *pOtdrState
			);
	//初始化功率模式
	void PowerModeInit_r(
			OtdrCtrlVariable_t  *pOtdrCtrl,
			OtdrStateVariable_t *pOtdrState);
	//otdr算法可重入函数
	void OtdrStateInit_r(OtdrStateVariable_t  *pOtdrState);
	//计算输入脉宽对应的采样点数目
	int32_t PulseWidthInSampleNum_r(OtdrStateVariable_t *pOtdrState);
	//根据测量数据估算是否需要拼接，如果需要拼接，计算拼接点等相关参数，很重要
	void EstimateCurveConnect_r(
			int32_t chan_data[],
			OtdrCtrlVariable_t  *pOtdrCtrl,
			OtdrStateVariable_t  *pOtdrState);
	//设定低功率模式，根据EstimateCurveConnect估算结果调用该函数
	void UseLowPowerMode_r(
			OtdrCtrlVariable_t  *pOtdrCtrl,
			OtdrStateVariable_t  *pOtdrState);
	//矫正采样率和脉宽周期
	void AdaptSampleFreq_PulsePeriod_r(
			int ch,
			OtdrCtrlVariable_t  *pOtdrCtrl,
			OtdrStateVariable_t  *pOtdrState
			);
	//开始新的测量前，更新参数等准备活动
	int32_t pre_measure(int32_t ch, struct _tagOtdrDev *potdrDev,
			struct _tagCHPara *pUsrPara);
	//延时
	int32_t usr_delay(int32_t ch, int32_t time_ms);
	//将点名测量的参数赋值到本地
	int32_t get_usr_otdr_test_para(struct _tagCHPara *pusr_para, 
		const struct _tagUsrOtdrTest *pnet_para);
	//获取激光器控制参数，启动测量时使用
	int32_t get_laser_ctrl_para(
			int32_t range_m,
			int32_t pl_ns,
			int32_t lamda,
			int32_t is_low_power,
			struct _tagLaserCtrPara *plaser_ctr_para);



#ifdef __cplusplus
}
#endif

#endif
