/**
 * @file hb_spi.h
 * @synopsis  spi操作的头文件
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-24
 */
#ifndef _HB_APP_H
#define _HB_APP_H_

#include <pthread.h>
#include "global.h"
#include "../otdr_ch/otdr_ch.h"
#ifdef __cplusplus
extern "C" {
#endif
	//记录光纤段可变信息，此数据保存在文件头部
struct _tagFiberSecHead
{
	int32_t data_num;
	int32_t sec_num;
	int32_t event_num;
};
//初始化光纤段，节点名称等相关参数
int32_t initialize_sys_para();
//创建文件夹
//协议80000003,对应内容的保存，节点名称和地址
int32_t save_node_name_address(const struct _tagDevMisc *pdev_misc);
int32_t read_node_name_address(struct _tagDevMisc *pdev_misc);
//协议0x80000004内容，光纤段内容对应和保存
int32_t save_fiber_sec_para(int ch, struct tms_fibersectioncfg *pfiber_sec);
int32_t read_fiber_sec_para(int ch, struct _tagCHFiberSec *pch_fiber_sec);
//保存或者读取otdr fpga相关信息
int32_t read_ch_fpga_info(const struct _tagCHInfo *pch_fpga_info,int32_t ch_num);
int32_t read_ch_fpga_info(const struct _tagCHInfo *pch_fpga_info,int32_t ch_num);
//创建文件夹
int32_t creat_folder(const char folder_path[]);
//为光纤段分配缓冲
int32_t alloc_fiber_sec_buf(struct _tagFiberSecHead secHead,
		struct _tagFiberSecCfg *pFiberSecCfg);
//释放光纤段存储区
int32_t free_fiber_sec_buf(struct _tagFiberSecCfg *pFiberSecCfg);
//初始化otdr模块
int32_t initialize_otdr_dev(struct _tagOtdrDev *pOtdrDev,
		int32_t ch_num);
//初始化光纤段参数
int32_t initialize_fiber_sec_cfg();
//更新通道参数
int32_t get_ch_para_from_fiber_sec(struct _tagCHPara *pCHPara, 
		const struct _tagCHFiberSec *FiberSec);
//自定义快速锁操作函数
int32_t quick_lock( QUICK_LOCK *plock);
int32_t quick_unlock( QUICK_LOCK *plock);
//检查用户点名测量参数
int32_t check_usr_otdr_test_para(struct tms_get_otdrdata *pget_otdrdata);
//检查用户配置光纤段测量参数
int32_t check_fiber_sec_para(const struct tms_fibersectioncfg *pfiber_sec_cfg);
//创建调度，算法任务
int32_t create_usr_tsk();
//通过目的地址获取context
int32_t get_context_by_dst(int32_t dst, struct tms_context *pcontext0);
//回应
int32_t hb_Ret_Ack(int32_t dst, struct tms_ack ack);
int32_t read_slot();
int32_t read_net_flag();





#ifdef __cplusplus
	}
#endif

#endif

