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
	int32_t save_node_name_address();
	int32_t read_node_name_address();
	//创建文件夹
	int32_t creat_folder(const char folder_path[]);
	//协议0x80000004内容，光纤段内容对应和保存
	int32_t save_fiber_sec_para(int ch, struct tms_fibersectioncfg *pfiber_sec);
	int32_t read_fiber_sec_para(int ch, struct _tagFiberSecCfg *pfiber_sec);
	//为光纤段分配缓冲
	int32_t alloc_fiber_sec_buf(struct _tagFiberSecHead secHead,
			struct _tagFiberSecCfg *pFiberSecCfg);
	//释放光纤段存储区
	int32_t free_fiber_sec_buf(struct _tagFiberSecCfg *pFiberSecCfg);
	//初始化otdr模块
	int32_t initialize_otdr_dev(struct _tagOtdrDev *pOtdrDev,
		struct _tagFiberSecCfg *pFiberSec,
		int32_t ch_num);
	//初始化光纤段参数
	int32_t initialize_fiber_sec_cfg(struct _tagFiberSecCfg *pFiberSec, int32_t num);
	//更新通道参数
	int32_t get_ch_para_from_fiber_sec(struct _tagCHPara *pCHPara, 
			const struct _tagFiberSecCfg *FiberSec);




#ifdef __cplusplus
}
#endif

#endif

