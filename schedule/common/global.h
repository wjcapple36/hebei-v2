/**
 * @file schedule_fun.h
 * @synopsis  通道调度函数接口的头文件
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-28
 */
#ifndef _GLOBAL_H
#define  _GLOBALE_H

#include "../otdr_ch/otdr_ch.h"

#ifdef __cplusplus
extern "C" {
#endif
	//通道的基数，从fpga中获取到，通道号+该基数为对外使用的通道号
	extern volatile int32_t ch_offset; 
	//通道的光纤段参数
	extern struct _tagFiberSecCfg FiberSecCfg[CH_NUM];
	//定义otdr通道资源
	extern struct _tagOtdrDev otdrDev[CH_NUM];
	//通道缓冲区，存放高低功率累加数据
	extern struct _tagCHBuf chBuf[CH_BUF_NUM];
	//spi设备
	extern struct _tagSpiDev spiDev;




#ifdef __cplusplus
}
#endif

#endif
