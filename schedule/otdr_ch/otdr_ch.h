/**
 * @file otdr_ch.h
 * @synopsis  定义描述otdr通道资源的头文件
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-23
 */
#ifndef _OTDR_CH_H_
#define _OTDR_CH_H_

#include <stdint.h>
#include "../../algorithm/Otdr.h"
#include "../../algorithm/OtdrEdma.h"
#include "../../algorithm/prototypes.h"
#include "../../protocol/tmsxx.h"
//声明otdr算法中定义的全局变量
extern const Uint32 OtdrMeasureLength[MEASURE_LENGTH_NUM];


#ifdef __cplusplus
extern "C" {
#endif

#define CH_NUM		4 /* 通道数目*/
#define CH_BUF_NUM	(CH_NUM / 2)/*测试的最小粒度为7秒，定义为通道数目的一半*/
#define MEASURE_TIME_MIN_MS	7000 /*最小测试时间*/	
#define OTDR_TEST_MOD_MONITOR		0	//监测模式
#define OTDR_TEST_MOD_APPOINT		1	//点名测量
	//#pragma pack (1) /*按照1B对齐*/

	//描述通道控制参数
	struct _tagCHCtrl
	{
		int32_t mod;		//测量模式
		int32_t accum_num;	//累加次数0，表示本通道累加结束
		int32_t hp_num;		//高功率次数
		int32_t lp_num;		//低功率次数
		int32_t cur_pw_mod;	//当前测量的是高功率还是低功率
		int32_t curv_cat;	//曲线拼接标志
		int32_t refresh_para;	//更新光纤段参数
	};
	//描述otdr通道状态
	struct _tagCHState
	{
		int32_t algro_run;	//算法运行状态，0表示算法运行结束
		int32_t resource_id;	//资源id，tsk_otdr保留一个副本
		int32_t success_num;	//测试成功的计数
		int32_t fail_num;	//测试失败的计数
		int32_t spi_error_num;	//spi通信失败的次数


	};
	//通道的测量参数
	struct _tagCHPara
	{
		uint32_t Lambda_nm;		// 波长，单位nm
		uint32_t MeasureLength_m;	// 量程，单位m
		uint32_t PulseWidth_ns;		// 光脉冲宽度，单位ns
		uint32_t MeasureTime_ms;	// 测量时间，单位ms
		float n;                  	// 折射率

		float   EndThreshold;       // 结束门限
		float   NonRelectThreshold; // 非反射门限;
	};
	//otdr设备，包含描述该设备的其他结构体变量
	struct _tagOtdrDev
	{
		struct _tagCHCtrl ch_ctrl;	//通道控制状态
		struct _tagCHState ch_state;	//通道状态
		struct _tagCHPara ch_para;	//通道的参数
		OtdrCtrlVariable_t otdr_ctrl;	//与otdr算法同类型全局变量匹配
		OtdrStateVariable_t otdr_state;	//与otdr算法同类型全局变量匹配


	};
	//通道缓存区，存放高低功率的累加数据
	struct _tagCHBuf
	{
		int32_t hp_buf_1310[DATA_LEN];	//高功率曲线buf
		int32_t lp_buf_1310[DATA_LEN];	//低功率曲线buf
		int32_t hp_buf_1550[DATA_LEN];	//高功率曲线buf
		int32_t lp_buf_1550[DATA_LEN];	//低功率曲线buf
	};
	//描述启动测量发送到fpga的参数
	struct _tagFpgaPara
	{
		uint8_t no;
		uint8_t pulse;
		uint8_t range;
		uint8_t power;

	};





	//#pragma pack () /*恢复默认的对其方式*/
#ifdef __cplusplus
}
#endif

#endif

