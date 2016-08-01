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
#include <pthread.h>
//声明otdr算法中定义的全局变量


#ifdef __cplusplus
extern "C" {
#endif


#define CH_NUM		8 /* 通道数目*/
//通道缓冲区交叉使用，0，2通道对应对应0缓冲区，1，3通道使用1缓冲
#define CH_BUF_NUM	2 //串行测试，2个缓冲区即可
#define MEASURE_TIME_MIN_MS	7000 /*最小测试时间*/	
#define MEASURE_TIME_MIN_S	(MEASURE_TIME_MIN_MS / 1000)
#define OTDR_TEST_MOD_MONITOR		0	//监测模式
#define OTDR_TEST_MOD_USR		1	//点名测量
//定义操作码
#define OP_OK	0
//文件操作返回码
#define OPEN_ERROR	1  //打开文件失败
#define SAVE_DATA_LEN_NO_EQ	2	//写入的数据长度和要写入的不相等
#define READ_DATA_LEN_ERROR	3	//读取文件长度错误
#define FILE_RUIN		4	//文件损坏	
#define NEW_BUF_FAIL		5	//分配内存失败:w
	//#pragma pack (1) /*按照1B对齐*/
#define LOCK_TYPE_SPIN
//定义快速锁 万一有什么问题，直接在这里修改定义
#ifdef LOCK_TYEP_SPIN
	//自旋转锁
	typedef pthread_spinlock_t QUICK_LOCK; 
#else
	//带任务切换的互斥量
	typedef pthread_mutex_t QUICK_LOCK; 
#endif
#define OTDR_TEST_MONITOR	0	//监控测量
#define OTDR_TEST_USR		1	//用户指定测量
#define OTDR_CH_ON		1	//通道可操作
#define OTDR_CH_OFF		0	//通道不可操作
//描述通道控制参数
struct _tagCHCtrl
{
	int32_t enable;		//是否启用
	int32_t is_cfged;	//是否配置
	int32_t refresh_para;	//更新光纤段参数
	/*下面的代码部分初始化部分要赋值成0*/
	int32_t mod;		//0,轮询，1点名测量
	int32_t on_ff;		//1, on, 0 ff, 串行工作模式，如果处于off状态则意味这不可操作
	int32_t send_num;	//发送测量参数次数
	int32_t accum_ms;	//实际累加的时间，如果超过测量时间，那么意味着失败
	int32_t accum_num;	//累加次数0，表示本通道累加结束
	int32_t hp_num;		//高功率次数
	int32_t lp_num;		//低功率次数
	int32_t cur_pw_mod;	//当前测量的是高功率还是低功率
	int32_t curv_cat;	//曲线拼接标志
};
//描述otdr通道状态
struct _tagCHState
{
	int32_t algro_run;	//算法运行状态，0表示算法运行结束
	int32_t resource_id;	//资源id，tsk_otdr保留一个副本
	int32_t success_num;	//测试成功的计数
	int32_t fail_num;	//测试失败的计数
	int32_t spi_error_num;	//spi通信失败的次数
	int32_t ch_buf_collid_num;	//ch buf 访问冲突的次数
	int32_t algro_collid_num;	//算法线程访问冲突的次数


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
//激光器控制参数
struct _tagLaserCtrPara
{
	int32_t rcv;
	int32_t power;
	int32_t apd;
	int32_t trail_length;
};
//通道缓存区，存放高低功率的累加数据
struct _tagCHBuf
{
	QUICK_LOCK lock;		//资源锁
	int32_t is_uesd;		//是否使用的标志
	int32_t hp_buf[DATA_LEN];	//高功率曲线buf
	int32_t lp_buf[DATA_LEN];	//低功率曲线buf
};
struct _tagCycCurvBuf
{
	QUICK_LOCK lock;		//资源锁
	int32_t is_empty;		//是否使用的标志
	int32_t buf[DATA_LEN];	//高功率曲线buf
};

//otdr设备，包含描述该设备的其他结构体变量
struct _tagOtdrDev
{
	struct _tagCHCtrl ch_ctrl;		//通道控制状态
	struct _tagCHState ch_state;		//通道状态
	struct _tagCHPara ch_para;		//通道的参数
	struct _tagLaserCtrPara laser_para;	//激光器参数
	OtdrCtrlVariable_t otdr_ctrl;		//与otdr算法同类型全局变量匹配
	OtdrStateVariable_t otdr_state;		//与otdr算法同类型全局变量匹配
	struct _tagCHBuf ch_buf;		//存放高低功率数据
	struct _tagCycCurvBuf curv_buf;		//周期性测量曲线buf	


};
//算法运行的时候一些指示信息
struct _tagAlgroCHInfo
{
	int32_t cmd;
	int32_t state;	//0 空闲，1正忙
	int32_t ch;	//通道号
	int32_t resource_id;	//资源id
	int32_t mod;	//测试方式 点名测量，轮询
	int32_t src_addr; //如果是点名测量，用来指示数据发向哪里
};


//与tms_fibersectioncfg的差别是将固定长度的变量设置成了非指针
struct _tagFiberSecCfg
{
	struct tms_fibersection_hdr fiber_hdr; //光纤段头
	struct tms_fibersection_val *fiber_val;//光纤段信息
	struct tms_otdr_param       otdr_param;//otdr参数
	struct tms_test_result      test_result;//测量结果
	struct tms_hebei2_data_hdr  otdr_hdr;//采样点数据头
	struct tms_hebei2_data_val  *otdr_val;//otdr数据部分
	struct tms_hebei2_event_hdr event_hdr;//时间点头
	struct tms_hebei2_event_val *event_val;//时间点缓冲区
	int32_t is_initialize;
	int32_t error_num;
};
//需要加锁
struct _tagCHFiberSec
{
	QUICK_LOCK lock;
	struct _tagFiberSecCfg para;
};
//上报时使用的otdr参数,下面配置的时候会多20个字节的标志，sb
struct _tagUpOtdrPara
{
	int32_t rang_m;	//量程
	int32_t lamda_nm;	//波长
	int32_t pl;	//脉宽
	int32_t test_time_s;	//测试时间
	float gi;	//折射率
	float end_th;	//结束门限门限
	float no_ref_th;//非反射门限

};
//测量结果
struct _tagUpOtdrTestResult
{
	char id[20];	//标志
	float chain;	//链长
	float loss;	//链损耗
	float attu;	//衰减
	char date[20];	//日期
};
struct _tagOtdrEvent
{
	int32_t distance;
	int32_t type;
	float insert_loss;
	float attu;
	float total_loss;
};
struct _tagUpOtdrEvent
{
	char id[12];
	struct _tagOtdrEvent buf[MAX_EVENT_NUM];
};
struct _tagUpOtdrData
{
	char id[12];
	int32_t num;
	int16_t buf[DATA_LEN];
};
//上传曲线结构
struct _tagUpOtdrCurv
{
	struct _tagUpOtdrPara para;
	struct _tagUpOtdrTestResult result;
	struct _tagUpOtdrData data;
	struct _tagUpOtdrEvent event;
};

//周期性测量曲线,每个测试完毕，就更新一次
struct _tagCycCurv
{
	QUICK_LOCK lock;
	struct _tagUpOtdrCurv curv;
};
//设备状态
struct _tagDevState
{
	int32_t on_use;//处于启用的状态
	int32_t error_state;
};
#define USR_OTDR_TEST_IDLE	0	//点名测量空闲，可以点名测量
#define USR_OTDR_TEST_WAIT	1	//正在等待测量
#define USR_OTDR_TEST_ACCUM	2	//正处于累加中，不能响应点名测量
#define USR_OTDR_TEST_ALGRO	3	//正处于找时间点的中，
//点名测量结构体
struct _tagUsrOtdrTest
{
	uint32_t state;		//空闲？累加？找事件点？
	uint32_t cmd;		//0x80*014/0x80*15 点名测量，配置测量
	uint32_t src_addr;	//操作设备的地址
	uint32_t reserv_ch;	//点名测量前抢占的的通道

	uint32_t ch;
	uint32_t range;
	uint32_t wl;
	uint32_t pw;
	uint32_t time;
	float	gi;
	float	end_th;
	float	none_ref_th;

};
struct _tagSecStatisData
{
	int32_t ch_no;
	int32_t sec_no;
	char date[20];
	float attu_vale;
};
//光纤段统计数据,在读取文件的时候分配
struct _tagFiberStatisData
{
	int32_t state;		/*状态，0，ok，其他出现错误*/
	int32_t counts;		/*计数，当前数据是第几次更新*/
	int32_t ch;		/*通道号*/
	int32_t sec_num;	/*段的数目*/
	struct _tagSecStatisData *buf;
};
//通道相关的信息，包括激光器波长，动态范围等相关硬件信息
struct _tagFpgaPara
{
	int32_t ch;
	int32_t lamda;
	int32_t scope_dB;
	char wdm[16];
	int32_t option;

};
//通道信息
struct _tagCHInfo
{
	int32_t initial; //是否初始化标志 0，未初始化，1初始化
	struct _tagFpgaPara para;
};	
//节点名称和地址
struct _tagDevNameAddr
{
	char name[64];
	char ip[16];
	char mask[16];
	char gate[16];
};
struct _tagDevCHState
{
	int32_t state;
};
//杂项，乱七八糟
struct _tagDevMisc
{ 
	struct _tagDevNameAddr name;
	struct _tagDevCHState ch_state;
};
//保存线程sys_id ,htop id htop id 主要用来查看线程占用率
struct _tagThreadInfo
{
	pthread_t tidp;	//create_thread 赋值
	int32_t htop_id;	//线程自身赋值
	int32_t ch;		//
};
struct _tagOtdrAlgroPara
{
	OtdrCtrlVariable_t *pCtrl;	
	OtdrStateVariable_t *pState;
	struct _tagAlgroCHInfo *pCHInfo;
	OTDR_ChannelData_t *pOtdrData;

};

//#pragma pack () /*恢复默认的对其方式*/
#ifdef __cplusplus
}
#endif

#endif

