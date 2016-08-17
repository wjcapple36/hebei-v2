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
#include "../../protocol/tmsxx.h"
#ifdef __cplusplus
extern "C" {
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#define STR_LEN_IP 			16
//定义返回给网管的错误码
#define CMD_RET_OK			0 // 成功	
#define CMD_RET_PARA_INVLADE		1 //参数非法
#define CMD_RET_CANT_SAVE		2 //不能保存
#define CMD_RET_EXIST_CMD		3 //已存在一条命令
#define CMD_RET_CH_UNUSE		4 //通道未启用
#define CMD_RET_CH_UNCFG		5 //通道未配置
#define CMD_RET_MANAGER_EXIST		6 //节点管理器已存在
#define CMD_RET_CH_UNEXIST		7 //通道不存在
#define CMD_RET_TEST_ERROR		8 //测试异常
#define CMD_RET_FPGA_COMMU_ERROR	9 //FPGA通信异常
//节点管理器地址，网管客户端地址，网管服务器	
#define ADDR_HOST_NODE			0x0000001e
#define ADDR_HOST_CLIENT		0x0000002e
#define ADDR_HOST_SERVER		0x0000003e
//定义一些其他关键变量
#define MAX_RANG_M	180000
#define OUTPUT_USR_MSG   1
//光纤告警级别	
#define FIBER_ALARM_LEV0	0
#define FIBER_ALARM_LEV1	1
#define FIBER_ALARM_LEV2	2
#define FIBER_ALARM_LEV3	3
#define SEC_NUM_IN_CH		10
//通道的基数，从fpga中获取到，通道号+该基数为对外使用的通道号
extern volatile int32_t ch_offset; 
//通道的光纤段参数
extern struct _tagCHFiberSec chFiberSec[CH_NUM];
//定义otdr通道资源
extern struct _tagOtdrDev otdrDev[CH_NUM];
//spi设备
extern struct _tagSpiDev spiDev;
//用户点名测量
extern struct _tagUsrOtdrTest usrOtdrTest;
//通道状态
extern struct tms_cfgpip_status ch_state;
extern struct _tagDevMisc devMisc;
//调度任务信息
extern struct _tagThreadInfo tsk_schedule_info;
//otdr 算法线程
extern struct _tagThreadInfo tsk_otdr_info;
//otdr线程锁
extern pthread_mutex_t mutex_otdr;
//fpga信息
extern struct _tagCHInfo chFpgaInfo;
//算法线程计算时的通道信息
extern struct _tagAlgroCHInfo algroCHInfo;
//硬件软件版本号
struct _tagVersion DevVersion;

#ifdef __cplusplus
}
#endif

#endif
