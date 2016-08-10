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

/*
 *下面定义的结构体在读取保存光纤段配置的时候使用，作为文件头信息
*/

//记录光纤段可变信息，此数据保存在文件头部
struct _tagFiberSecHead
{
	int32_t data_num;
	int32_t sec_num;
	int32_t event_num;
};

/*
 *下面定义的结构体是在找告警位置的时候使用
*/

//描述光纤段起始位置
struct _tagFiberSecCoord
{
	int32_t start;
	int32_t end;
};
//通过事件找到的告警点
struct _tagEventAlarmData
{
	int32_t index;
	int32_t pos;
	int32_t lev;
	float	diff;
};
struct _tagEventAlarm
{
	struct _tagEventAlarmData first;
	struct _tagEventAlarmData highest;
};

/*********************************************************/

//初始化光纤段，节点名称等相关参数
int32_t initialize_sys_para();
//创建文件夹
//协议80000003,对应内容的保存，节点名称和地址
int32_t save_node_name_address(const struct _tagDevMisc *pdev_misc);
int32_t read_node_name_address(struct _tagDevMisc *pdev_misc);
//协议0x80000004内容，光纤段内容对应和保存
int32_t save_fiber_sec_para(int ch,
	       	struct tms_fibersectioncfg *pfiber_sec,
	       	struct _tagCHFiberSec *pch_fiber_sec,
		struct _tagOtdrDev *potdr_dev
	       	);
int32_t read_fiber_sec_para(int ch, struct _tagCHFiberSec *pch_fiber_sec);
//保存或者读取otdr fpga相关信息
int32_t save_ch_fpga_info(const struct _tagCHInfo *pch_fpga_info,int32_t ch_num);
int32_t read_ch_fpga_info(const struct _tagCHInfo *pch_fpga_info,int32_t ch_num);
//创建文件夹
int32_t creat_folder(const char folder_path[]);
//为光纤段分配缓冲
int32_t alloc_fiber_sec_buf(struct _tagFiberSecHead secHead,
		struct _tagCHFiberSec *pch_fiber_sec);
//释放光纤段存储区
int32_t free_fiber_sec_buf(struct _tagCHFiberSec *pch_fiber_sec);
//初始化otdr模块
int32_t initialize_otdr_dev(struct _tagOtdrDev *pOtdrDev,
		int32_t ch_num);
//初始化光纤段参数
int32_t initialize_fiber_sec_cfg();
//更新通道参数
int32_t get_ch_para_from_fiber_sec(struct _tagCHPara *pCHPara, 
		const struct _tagCHFiberSec *FiberSec);
//自定义快速锁操作函数
int32_t quick_lock_init(QUICK_LOCK *plock);
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
//读取槽位号
int32_t read_slot();
//读取网络标志号
int32_t read_net_flag();
//向主机发送曲线数据
int32_t send_otdr_data_host(OTDR_UploadAllData_t *pResult, struct _tagAlgroCHInfo *pCHInfo );
//算法测量参数转换到hebei2协议中来
int32_t get_test_para_from_algro(struct tms_ret_otdrparam *pHost,const OTDR_UploadAllData_t *pAlgro);
int32_t get_test_result_from_algro(struct tms_test_result *pHost,const OTDR_UploadAllData_t *pAlgro);
int32_t get_test_event_from_algro(struct tms_hebei2_event_val *pHost,const OTDR_UploadAllData_t *pAlgro, int32_t count);
//自我了断
int32_t exit_self(int32_t err_code, char function[], int32_t line, char msg[]);
//根据输的命令码返回对应曲线的CMD
int32_t get_ret_curv_cmd(int32_t in_cmd, int32_t *ret_cmd);
//获取曲线起始点
int32_t get_curv_start_point(int32_t Sigma,
		OTDR_ChannelData_t *pOtdrData,
	       	OtdrCtrlVariable_t *pOtdrCtrl,
		OtdrStateVariable_t *pOtdrState);
//通过事件点比较获取光纤段之间的告警
int32_t get_fiber_alarm_from_event(
		struct _tagFiberSecCoord *pSecCoord,
		struct tms_hebei2_event_hdr *pstd_event_hdr,
		struct tms_hebei2_event_val *pstd_event_val,
		OTDR_UploadAllData_t  *AllEvent,
		OtdrStateVariable_t *pOtdrState,
		struct tms_fibersection_val* pstd_fiber_val,
		struct _tagEventAlarm *pEventAlarm
		);
//获取告警级别
int32_t get_alarm_lev(
		float loss,
		struct tms_fibersection_val *pfiber_sec
		);
//获取插损的差值
int32_t get_inser_loss_diff(
		float cur_loss,
		float std_loss,
		float *diff_loss
		);
//测量结束，将测量结果放入填入周期性测量曲线
int32_t refresh_cyc_curv_after_test(
		int32_t ch,
		OTDR_UploadAllData_t *pResult, 
		struct _tagCycCurv *pCycCurv);












#ifdef __cplusplus
	}
#endif

#endif

