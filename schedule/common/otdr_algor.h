/**
 * @file schedule_fun.h
 * @synopsis  通道调度函数接口的头文件
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-28
 */
#ifndef SCHEEDULE_FUN_H
#define  SCHEEDULE_FUN_H

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
			OtdrCtrlVariable_t  *pOtdrCtrl);
	//otdr算法可重入函数
	void OtdrStateInit_t(OtdrStateVariable_t  *pOtdrState);
	//计算输入脉宽对应的采样点数目
	int32_t PulseWidthInSampleNum_r (uint32_t pl_ns, uint32_t sample_hz);
	//根据测量数据估算是否需要拼接，如果需要拼接，计算拼接点等相关参数，很重要
	void EstimateCurveConnect(
			const int32_t chan_data[],
			OtdrCtrlVariable_t  *pOtdrCtrl,
			OtdrStateVariable_t  *pOtdrState);
	//设定低功率模式，根据EstimateCurveConnect估算结果调用该函数
	void UseLowPowerMode_r(
		OtdrCtrlVariable_t  *pOtdrCtrl,
		OtdrStateVariable_t  *pOtdrState);








#ifdef __cplusplus
}
#endif

#endif
