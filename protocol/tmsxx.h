/**
 ******************************************************************************
 * @file	tmsxx.h
 * @brief	MenglongWu\n
 	TMSxx协议内容

	依据《网管与MCU通信协议20150325.doc》
 *


- 2015-3-25,Menglong Wu,DreagonWoo@163.com
 	- 编写预留接口
*/

#ifndef _TMSXX_H_
#define _TMSXX_H_
#include "autoconfig.h"
#include "glink.h"
#include "osnet/epollserver.h"


#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib.h"

#ifdef HEBEI2_DBG
#define hb2_dbg(format, args...) printf("\e[36m" format "\e[0m", ##args)
#else
#define hb2_dbg(format, args...)
#endif

#ifdef TRACE_DBG
#define trace_dbg(format, args...) printf("\e[33m" format "\e[0m", ##args)
#else
#define trace_dbg(format, args...)
#endif

#if defined(TMS_DEBUG)
#define dbg_tms( format, args...) printf(format, ##args)
#else
#define dbg_tms( format, args...)
#endif

#if defined(TMS_DEBUG1)
#define dbg_tms1( format, args...) printf(format, ##args)
#else
#define dbg_tms1( format, args...)
#endif

#if defined(TMS_DEBUG2)
#define dbg_tms2( format, args...) printf(format, ##args)
#else
#define dbg_tms2( format, args...)
#endif

/*
-------------------------------------------------------------------------------------------------
|   struct unknow1   |<- struct_A -> | <- struct_B->|<- struct_B->|<- struct_B->|   struct unknow2|
-------------------------------------------------------------------------------------------------
                     ^ptrA           |              B_count                                       ^ptrEnd

*/

#define CHECK_PTR(ptrA, struct_A, struct_B, B_Count, PtrEnd) 	\
	( ((uint8_t*)(ptrA)) + sizeof(struct_A) + \
	  sizeof(struct_B) * (int32_t)(B_Count-1) <= ((uint8_t*)(PtrEnd)-sizeof(struct_B)) )

////////////////////////////////////////////////////////////////////////////////
// 机框，槽位数
#define MAX_FRAME 16
#define MAX_SLOT 16

////////////////////////////////////////////////////////////////////////////////
// Section 1 返回值
#define RET_ERROR 				(-1)///< 错误
#define RET_SUCCESS 			0///< --- 命令成功
#define RET_CMD_INVALID 			1///<  --- 无效命令
#define RET_CMD_DOING 			3///<  --- 无效命令


////////////////////////////////////////////////////////////////////////////////
// Section  Proccess 数据包处理方式
#define	PROCCESS_COPY2USE	0	///<00--MCU本地文处理	
#define	PROCCESS_CALLBACK	1	///<01--MCU本地吴处理	
#define	PROCCESS_2DEV	2	///<02-->下发到板卡	
#define	PROCCESS_2MANAGE	3	///<03<--上传到网管	
#define	PROCCESS_2DEV_AND_COPY2USE	4	///<04-->下发到板卡、自身也处理	
#define	PROCCESS_2MANAGE_AND_COPY2USE	5	///<05--上传网管、自身也处理	
#define	PROCCESS_2DEV_OR_COPY2USE	6	///<06-->下发或自身处理	
#define	PROCCESS_2MANAGE_OR_COPY2USE	7	///<07-->上传或自身处理	
#define	PROCCESS_DONOT	8	///<08--不做任何处理	
#define	PROCCESS_SPECAIAL	9	///<09--特殊处理	


////////////////////////////////////////////////////////////////////////////////
// Section 2 设备类型
#define DEV_PWU					1	///<电源板
#define DEV_MCU					2	///<主控板
#define DEV_OPM					3	///<光功率模块板
#define DEV_OSW					4	///<光开关板
#define DEV_OTDR				5	///<OTDR板
#define DEV_OLS					6	///<光源板
#define DEV_OLP					7	///<光功率板
#define DEV_SMS					8	///<短信模块板
#define DEV_TU					9	///<子机框TU模块板

// 在线升级设备名
#define TARGET_PWU				"pwu.bin" ///<电源板
#define TARGET_MCU				"mcu.bin" ///<主控板
#define TARGET_OPM				"opm.bin" ///<光功率模块板
#define TARGET_OSW_GX_8                                "osw-gx-8.bin" ///<光开关板
#define TARGET_OSW				"osw.bin" ///<光开关板
#define TARGET_OTDR				"otdr.exe" ///<OTDR板
#define TARGET_OLS				"ols.bin" ///<光源板
#define TARGET_OLP				"olp.bin" ///<光功率板
#define TARGET_SMS				"sms.bin" ///<短信模块板


#define LEVEL_UI    1
#define LEVEL_TC    0
#define LEVEL_R_CMD 3
#define LEVEL_CMD   4

#define OLP_SWITCH_MODE_UNBACK       0	///<保护不返回，ID_CFG_OLP_MODE
#define OLP_SWITCH_MODE_BACK        1	///<表示保护返回，ID_CFG_OLP_MODE

#define OLP_SWITCH_ACTION_PERSION  1	///<人工切换，ID_REPORT_OLP_ACTION
#define OLP_SWITCH_ACTION_AUTO     2	///<自动保护倒换，ID_REPORT_OLP_ACTION
#define OLP_SWITCH_ACTION_BACK     3	///<保护返回，ID_REPORT_OLP_ACTION

#define OLP_SWITCH_ACTION_MASTER       0	///<切换到主光路，ID_REPORT_OLP_ACTION
#define OLP_SWITCH_ACTION_SLAVE        1	///<切换到备光路，ID_REPORT_OLP_ACTION

#define OLP_SWITCH_A        1	///<切换到 A 路
#define OLP_SWITCH_B        2	///<切换到 B 路

#define OTDR_SAMPLE_16000 16000			///< OTDR数据点个数
#define OTDR_SAMPLE_32000 32000			///< OTDR数据点个数
#define OTDR_EVENT_MAX 1024

#define MANAGE_COUNT 6

#define MT_MANAGE 0 	///< 普通网管\n
#define MT_TRACE 1 		///< 支持命令行的网管


///< 缺省填充源地址，目的地址
#define NODE_MANAGE 0x1e
#define CLIENT_MANAGE 0x2e
#define SERVER_MANAGE 0x3e
#define DEV_OTDR 0x0e
#define DEV_OCVM 0x100000E
#define DEV_SMS 0x2000000e

#ifdef _MANAGE
#define TMS_DEFAULT_LOCAL_ADDR GLINK_MANAGE_ADDR
#define TMS_DEFAULT_RMOTE_ADDR GLINK_CU_ADDR
#else
#if 1
#define TMS_DEFAULT_LOCAL_ADDR DEV_OTDR
#define TMS_DEFAULT_RMOTE_ADDR NODE_MANAGE
#endif
#if 0
// TMSxxV1.2 废除 GLINK_4412_ADDR
#define TMS_DEFAULT_LOCAL_ADDR  GLINK_CU_ADDR
#define TMS_DEFAULT_RMOTE_ADDR  GLINK_MANAGE_ADDR
#endif
#endif


////////////////////////////////////////////////////////////////////////////////
// Section 3 命令ID
#ifdef CONFIG_TEST_NET_STRONG
#define 	ID_TEST_NETPACK_SAVE			0x20000000	///< 测试网络强壮性
#define 	ID_TEST_NETPACK_ECHO			0x20000001	///< 测试网络强壮性
#define 	ID_TEST_NETPACK_ACK				0x20000002	///< 测试网络强壮性

#endif

#define 	ID_TICK			0x10000000	///< 心跳
#define 	ID_UPDATE			0x10000001	///< 在线升级


////////////////////////////////////////////////////////////////////////////////
// 命令类型:网管与MCU之间的通信
///<网管查询MCU的设备序列号



// hebei 2
#define		ID_SETOTDRFPGAINFO	0x20000000	///	返回参数校验结果



#define		ID_SETOTDRFPGAINFO	0x70000000	///	设置OTDR FPGA信息
#define		ID_SETOCVMPARA	0x70000001	///	设置OCVM测试参数



#define		ID_SETSMSINFO	0x60000000	///	设置短信模块信息
#define		ID_SMSINFO	0x60000001	///	返回短信模块信息
#define		ID_SENDSMSINFO	0x60000002	///	发送短信
#define		ID_SENDSMSINFORETCODE	0x60000003	///	返回短信操作返回码



#define		ID_GETBASICINFO 	0x80000000	///	查询节点基本信息格式
#define		ID_GETNODETIME	0x80000001	///	查询节点时间
#define		ID_RETNODETIME	0x80000002	///	返回节点时间
#define		ID_NAMEANDADDRESS	0x80000003	///	配置节点名称和地址
#define		ID_FIBERSECTIONCFG	0x80000004	///	发送光纤段参数配置格式
#define		ID_CONFIGPIPESTATE	0x80000005	///	配置OTDR通道使用状态
#define		ID_GETCYCLETESTCUV	0x80000006	///	查询周期测量结果
#define		ID_GETSTATUSDATA	0x80000007	///	查询统计数据格式
#define		ID_STATUSDATA	0x80000008	///	返回统计数据格式
#define		ID_CRCCHECKOUT	0x80000009	///	参数校验
#define		ID_CHECKOUTRESULT	0x80000010	///	返回参数校验结果
#define		ID_OTDRBASICINFO	0x80000011	///	返回OTDR的节点信息
#define		ID_CONFIGNODETIME	0x80000012	///	配置节点时间
#define		ID_CURALARM	0x80000013	///	返回当前告警信息格式
#define		ID_GETOTDRDATA_14	0x80000014	///	给定参数（点名测量）
#define		ID_GETOTDRDATA_15	0x80000015	///	0
#define		ID_RETOTDRDATA_16	0x80000016	///	返回测量曲线
#define		ID_RETOTDRDATA_17	0x80000017	///	0
#define		ID_RETOTDRDATA_18	0x80000018	///	0
#define		ID_RETOTDRDATA_19	0x80000019	///	0
#define		ID_GETSTANDARDCURV	0x80000020	///	获取标准曲线格式
#define		ID_ERROR	0x80000021	///	监测模块传回的错误代码的信息格式

#define ADDR_MANGER (0x3e)		// 网管地址
#define ADDR_NODE_MANGER (0x1e) 	//节点管理器地址
// end hebei 2

// hebei2

struct tms_nameandaddr 
{
	char name[64];
	char addr[16];
	char mask[16];
	char gw[16];
};
// 0x80000004

struct tms_fibersection_hdr {
	char id[20];
	uint32_t count;
};
struct tms_fibersection_val {
	uint32_t pipe_num;
	uint32_t fiber_num;
	char     fiber_route[64];
	char     fiber_name[64];

	uint32_t start_coor;
	char     start_inf[64];
	uint32_t end_coor;
	char     end_inf[64];

	float    fibe_atten_init;
	float    level1;
	float    level2;
	float    listen_level;
};

struct tms_otdr_param {
	char otdr_id[20];
	uint32_t range;
	uint32_t wl;
	uint32_t pw;
	uint32_t time;
	float	gi;
	float	end_threshold;
	float	none_reflect_threshold;
};
struct tms_test_result {
	char result[20];
	float range;
	float loss;
	float atten;
	char time[20];
};
struct tms_hebei2_data_hdr {
	int8_t  dpid[12];					///< 测试数据单元名称
	int32_t count;					///< 数据点个数
};

// OTDR返回信息B部分
struct tms_hebei2_data_val {
	uint16_t data;
};

struct tms_hebei2_event_hdr {
	int8_t eventid[12];
	int32_t count;
};

struct tms_hebei2_event_val {
	int32_t distance;		///< 该事件点距离
	int32_t event_type;		///< 该事件点类型
	float   att;			///< 该事件点两事件点与前事件点之间光纤衰减系数
	float   loss;			///< 该事件点连接损耗
	float   reflect;		///< 该事件点反射损耗
	float   link_loss;		///< 该事件点累计损耗
};

struct tms_fibersectioncfg {
	struct tms_fibersection_hdr *fiber_hdr;
	struct tms_fibersection_val *fiber_val;
	struct tms_otdr_param       *otdr_param;
	struct tms_test_result      *test_result;
	struct tms_hebei2_data_hdr  *otdr_hdr;
	struct tms_hebei2_data_val  *otdr_val;
	struct tms_hebei2_event_hdr *event_hdr;
	struct tms_hebei2_event_val *event_val;
};
// end 0x80000004

// 0x80000005
struct tms_cfgpip_status {
	uint32_t status;
};

struct tms_getcyctestcuv {
	uint32_t pipe;
};

struct tms_getstatus_data {
	uint32_t pipe;
};

struct tms_getstatus_data_hdr {
	uint32_t count;
};
struct tms_getstatus_data_val {
	uint32_t pipe;
	uint32_t section_num;
	char time[20];
	float section_atten;
};
// 0x80000011
struct tms_otdr_crc_hdr {
	uint32_t crc;
	char id[12];
	char name[64];
	char addr[16];
	char hw_ver[12];
	char sf_ver[12];
	uint32_t count;
};
struct tms_otdr_crc_val {
	uint32_t pipe;
	uint32_t wl;
	uint32_t dr;
	char wdm[16];
	uint32_t reserved0;
};

struct tms_otdr_ch_status {
	char id[12];
	uint32_t ch_status;
};

struct tms_otdr_param_hdr {
	char id[20];
	uint32_t count;
};
struct tms_otdr_param_val {
	uint32_t pipe;
	uint32_t range;
	uint32_t wl;
	uint32_t pw;
	uint32_t time;
	float	gi;
	float	end_threshold;
	float	none_reflect_threshold;
};

// extern struct tms_fibersection_hdr;
// extern struct tms_fibersection_val;

struct tms_otdrbaseinfo {
	struct tms_otdr_crc_hdr     *otdr_crc_hdr;
	struct tms_otdr_crc_val     *otdr_crc_val;
	struct tms_otdr_ch_status   *otdr_ch_status;
	struct tms_otdr_param_hdr   *otdr_param_hdr;
	struct tms_otdr_param_val   *otdr_param_val;
	struct tms_fibersection_hdr *fiber_hdr;
	struct tms_fibersection_val *fiber_val;
};
// end 0x80000011

// 0x80000012
struct tms_confignodetime
{
	char time[20];
};
struct tms_get_otdrdata {
	uint32_t pipe;
	uint32_t range;
	uint32_t wl;
	uint32_t pw;
	uint32_t time;
	float	gi;
	float	end_threshold;
	float	none_reflect_threshold;
};


struct tms_getstandardcurv {
	uint32_t pipe;
};

// 0x80000013
struct tms_alarmlist_hdr {
	uint32_t count;
};
struct tms_alarmlist_val {
	uint32_t pipe;
	uint32_t fiber;
	uint32_t level;
	uint32_t type;
	char     time[20];
	char     reserved0[20];
	uint32_t location[3];
	uint32_t reserved1;
};

struct tms_alarmline_hdr {
	uint32_t count;
};

struct tms_alarmline_val {
	uint32_t pipe;
	uint32_t datalen; ///< 特别注意：曲线长度，Sizeof(Int32) + 曲线数据的长度

	//  OTDR 曲线内容
	struct tms_ret_otdrparam    *ret_otdrparam;
	struct tms_test_result      *test_result;
	struct tms_hebei2_data_hdr  *hebei2_data_hdr;
	struct tms_hebei2_data_val  *hebei2_data_val;
	struct tms_hebei2_event_hdr *hebei2_event_hdr;
	struct tms_hebei2_event_val *hebei2_event_val;
};

struct tms_curalarm {
	struct tms_alarmlist_hdr *alarmlist_hdr;
	struct tms_alarmlist_val *alarmlist_val;
	struct tms_alarmline_hdr *alarmline_hdr;
	struct tms_alarmline_val *alarmline_val;
};

// 0x80000016 ~ 0x80000019
struct tms_ret_otdrparam {
	uint32_t pipe;
	uint32_t range;
	uint32_t wl;
	uint32_t pw;
	uint32_t time;
	float	gi;
	float	end_threshold;
	float	none_reflect_threshold;
};

struct tms_ret_otdrparam_p1 {
	uint32_t pipe;
};

struct tms_ret_otdrparam_p2 {
	uint32_t range;
	uint32_t wl;
	uint32_t pw;
	uint32_t time;
	float	gi;
	float	end_threshold;
	float	none_reflect_threshold;
};

struct tms_ret_otdrdata {
	struct tms_ret_otdrparam    *ret_otdrparam;
	struct tms_test_result      *test_result;
	struct tms_hebei2_data_hdr  *hebei2_data_hdr;
	struct tms_hebei2_data_val  *hebei2_data_val;
	struct tms_hebei2_event_hdr *hebei2_event_hdr;
	struct tms_hebei2_event_val *hebei2_event_val;
};
// 20000000
struct tms_setotdrfpgainfo {
	uint32_t pipe;
	uint32_t wl;
	uint32_t dr;
	char     wdm[16];
	uint32_t reserved0;
};
// 70000000
struct tms_setocvmpara {
	float 		cable_len;
	uint32_t 	host_thr;
	uint32_t 	slave_thr;
	float 		amend;
};

struct tms_setocvmfpgainfo {
	float 		max_cable_len;
	char		wl[64];
};
// end hebei2

struct pro_list {
	char name[52];
	// int len;
};

struct tms_dev_update_hdr {
	int32_t frame;
	int32_t slot;
	int32_t type;
	uint8_t target[16];
	int32_t flen;
};
struct tms_dev_md5 {
	uint8_t md5[32];
};



// // OTDR返回信息C部分
struct tms_retotdr_event_val {
	int32_t distance;		///< 该事件点距离
	int32_t event_type;		///< 该事件点类型
	float   att;			///< 该事件点两事件点与前事件点之间光纤衰减系数
	float   loss;			///< 该事件点连接损耗
	float   reflect;		///< 该事件点反射损耗
	float   link_loss;		///< 该事件点累计损耗
};


//ID_CMD_ACK				31
struct tms_ack {
	int32_t errcode;
	int32_t cmdid;
	// int32_t reserve1;
	// int32_t reserve2;
	// int32_t reserve3;
	// int32_t reserve4;
};
// //ID_CMD_TICK				32
// struct tms_
// {

// };




///< 设备基本类型描述
// 用于刷新设备连接状态图形界面，
struct tms_devbase {
	int fd;
	int32_t  frame;
	int32_t  slot;
	int32_t type;
	int32_t port;
	int32_t wave;
	int32_t reserved0;
	int32_t reserved1;
	int32_t reserved2;
};


////////////////////////////////////////////////////////////////////////
// Section 5 TMSxx管理算法相关数据结构

///< 网关表
// struct tms_manage
// {
// 	int fd[MANAGE_COUNT];       ///< 第三方开发的网管
// 	int fd_addr[MANAGE_COUNT];
// 	int fdtc[MANAGE_COUNT];		///< 支持控制台的网管数
// 	int fdtc_addr[MANAGE_COUNT];
// };


///< 设备上下文描述
// 主要处理心跳计数，用与当设备长时间断开连接后能快速响应
// 应用层“刷新”命令读取tick，如果tick一直不变表示死链接
struct tms_context {
	int fd;          ///<socket描述符
	struct glink_base *pgb;
	uint32_t frame;  ///<机框号
	uint32_t slot;   ///<槽位号
	uint32_t tick;   ///<心跳计数
	struct tms_callback *ptcb;
	// void     *ptr_analyse_arr;  ///<应用层不要读取任何值，tmsxx协议内部使用
	struct tms_analyse_array *ptr_analyse_arr;
#ifdef CONFIG_TEST_NET_STRONG
	uint32_t net_pack_id;
#endif
	pthread_mutex_t mutex;
};


#include "bipbuffer.h"
///< 应用程序
struct tmsxx_app {
	// int    fd;                   ///<socket fd
	int    morebyte;             ///<为防止bipbuffer环形缓存浪费
	struct bipbuffer bb;         ///<glink接收环形缓存，长度初始化后永远不变

	int enable_lbb;
	int lbyte;					 ///<
	struct bipbuffer lbb;		 ///<临时环形缓存，仅用于接收大于bb长度的帧
	struct tms_context context;  ///<tms包描述符


};
extern int g_201fd;
extern struct ep_t ep;
struct tms_callback {
	int32_t (*pf_OnCopy2Use)(char *data, int32_t datalen, int msec, void *fd);

	// hebei2
	// 80000000
	int32_t (*pf_OnGetBasicInfo)(struct tms_context *pcontext);
	int32_t (*pf_OnGetNodeTime)(struct tms_context *pcontext);
	int32_t (*pf_OnRetNodeTime)(struct tms_context *pcontext);
	int32_t (*pf_OnNameAndAddress)(struct tms_context *pcontext, struct tms_nameandaddr *pval);
	int32_t (*pf_OnFiberSectionCfg)(struct tms_context *pcontext, struct tms_fibersectioncfg *pval);
	int32_t (*pf_OnConfigPipeState)(struct tms_context *pcontext, struct tms_cfgpip_status *pval);
	int32_t (*pf_OnGetCycleTestCuv)(struct tms_context *pcontext, struct tms_getcyctestcuv *pval);
	int32_t (*pf_OnGetStatusData)(struct tms_context *pcontext, struct tms_getstatus_data *pval);
	int32_t (*pf_OnStatusData)(struct tms_context *pcontext);
	int32_t (*pf_OnCRCCheckout)(struct tms_context *pcontext);

	int32_t (*pf_OnCheckoutResult)(struct tms_context *pcontext);
	int32_t (*pf_OnOTDRBasicInfo)(struct tms_context *pcontext);
	int32_t (*pf_OnConfigNodeTime)(struct tms_context *pcontext, struct tms_confignodetime *pval);
	int32_t (*pf_OnCurAlarm)(struct tms_context *pcontext);
	int32_t (*pf_OnGetOTDRData)(struct tms_context *pcontext, struct tms_get_otdrdata *pval);
	int32_t (*pf_OnGetStandardCurv)(struct tms_context *pcontext, struct tms_getstandardcurv *pval);

	// 20000000
	int32_t (*pf_OnSetOTDRFPGAInfo)(struct tms_context *pcontext, struct tms_setotdrfpgainfo *pval);

	// 70000000
	int32_t (*pf_OnSetOCVMPara)(struct tms_context *pcontext, struct tms_setocvmpara *pval);
	int32_t (*pf_OnSetOCVMFPGAInfo)(struct tms_context *pcontext, struct tms_setocvmfpgainfo *pval);

};


///< 回调函数列表结构，采用函数指针方法，避免多个if、switch的低效率
struct tms_analyse_array {
	int (*ptrfun)(struct tms_context *pcontext, int8_t *pdata, int32_t len);
	int dowhat;
};



////////////////////////////////////////////////////////////////////////
// Section 6 MCU与业务板通信接口
void tms_Init();
int32_t tms_Analyse(struct tms_context *pcontext, int8_t *pdata, int32_t len);
int32_t tms_Tick(int fd, struct glink_addr *paddr);
int32_t tms_Update(
    int fd,
    struct glink_addr *paddr,
    int32_t frame,
    int32_t slot,
    int32_t type,
    uint8_t (*target)[16],
    int32_t flen,
    uint8_t *pdata);
void tms_Trace(struct glink_addr *paddr, char *strout, int32_t len, int level);

int32_t tms_AckEx(
    int fd,
    struct glink_addr *paddr,
    struct tms_ack *pACK);

void tms_SaveOTDRData(
    struct tms_retotdr_test_hdr   *ptest_hdr,
    struct tms_retotdr_test_param *ptest_param,
    struct tms_retotdr_data_hdr   *pdata_hdr,
    struct tms_retotdr_data_val   *pdata_val,
    struct tms_retotdr_event_hdr  *pevent_hdr,
    struct tms_retotdr_event_val  *pevent_val,
    struct tms_retotdr_chain      *pchain,
    char *path,
    int32_t flag);
// hebei2
int32_t tms_RetNodeTime(
    struct tms_context *pcontext,
    struct glink_addr *paddr,
    char *tm);
// end hebei2
/**
 * @brief	得到OTDR返回数据的指针，pdata是连续的缓存区，如果采用环形缓存必须保证
 数据起始部分不在环形缓存末尾，结束部分在环形缓存开头。pdata的数据内容是glink协议中
 命令ID、数据长度、数据包内容三部分
 * @remarks	<h2><center>TMS_PTR_OTDRTest的使用方法建议查看下面的例程链接</center></h2>\n
 */

#define TMS_PTR_OTDRTest(pdata, ptest_hdr, ptest_param, pdata_hdr, pdata_val, pevent_hdr, pevent_val, pchain) \
	ptest_hdr   = (struct tms_retotdr_test_hdr   *)(pdata + 8 ); \
	ptest_param = (struct tms_retotdr_test_param *)(((char*)ptest_hdr)   + sizeof(struct tms_retotdr_test_hdr)); \
	pdata_hdr   = (struct tms_retotdr_data_hdr   *)(((char*)ptest_param) + sizeof(struct tms_retotdr_test_param)); \
	pdata_val   = (struct tms_retotdr_data_val   *)(((char*)pdata_hdr) + sizeof(struct tms_retotdr_data_hdr)); \
	pevent_hdr  = (struct tms_retotdr_event_hdr  *)(((char*)pdata_val) + sizeof(struct tms_retotdr_data_val) * (pdata_hdr->count)); \
	pevent_val  = (struct tms_retotdr_event_val  *)(((char*)pevent_hdr) + sizeof(struct tms_retotdr_event_hdr)); \
	pchain      = (struct tms_retotdr_chain      *)(((char*)pevent_val) + sizeof(struct tms_retotdr_event_val) * (pevent_hdr->count));


#define TMS_PTR_OTDRBin(pdata, ptest_hdr, ptest_param, pdata_hdr, pdata_val, pevent_hdr, pevent_val, pchain) \
	ptest_hdr   = (struct tms_retotdr_test_hdr   *)(pdata); \
	ptest_param = (struct tms_retotdr_test_param *)(((char*)ptest_hdr)   + sizeof(struct tms_retotdr_test_hdr)); \
	pdata_hdr   = (struct tms_retotdr_data_hdr   *)(((char*)ptest_param) + sizeof(struct tms_retotdr_test_param)); \
	pdata_val   = (struct tms_retotdr_data_val   *)(((char*)pdata_hdr) + sizeof(struct tms_retotdr_data_hdr)); \
	pevent_hdr  = (struct tms_retotdr_event_hdr  *)(((char*)pdata_val) + sizeof(struct tms_retotdr_data_val) * (pdata_hdr->count)); \
	pevent_val  = (struct tms_retotdr_event_val  *)(((char*)pevent_hdr) + sizeof(struct tms_retotdr_event_hdr)); \
	pchain      = (struct tms_retotdr_chain      *)(((char*)pevent_val) + sizeof(struct tms_retotdr_event_val) * (pevent_hdr->count));

int connect_first_card(char *str_addr, char *str_port);
int32_t tms_RetOTDRData(
    int fd,
    struct glink_addr *paddr,
    struct tms_ret_otdrdata *val,
    unsigned long cmdid);
int32_t tms_RetStatusData(struct tms_context *pcontext,
                          struct glink_addr *paddr,
                          struct tms_getstatus_data_hdr *hdr,
                          struct tms_getstatus_data_val *val,
                          int32_t ilen);

void tms_Print_tms_fibersection_hdr(struct tms_fibersection_hdr *pval);
void tms_Print_tms_fibersection_val(struct tms_fibersection_val *pval);
void tms_Print_tms_otdr_param(struct tms_otdr_param *pval);
void tms_Print_tms_test_result(struct tms_test_result *pval);
void tms_Print_tms_hebei2_event(struct tms_hebei2_event_hdr *pevent_hdr, struct tms_hebei2_event_val *pevent_val);


int32_t tms_Transmit2Dev(struct tms_context *pcontext, int8_t *pdata, int32_t len);
int32_t tms_Transmit2Manager(struct tms_context *pcontext, int8_t *pdata, int32_t len);
int32_t  tms_SelectContextByFD(int fd, struct tms_context *context);
int32_t tms_SelectMangerContext(struct tms_context *context);
int32_t tms_SelectNodeMangerContext(struct tms_context *context);


int32_t tms_OTDRBasicInfo(
    struct tms_context *pcontext,
    struct glink_addr *paddr,
    struct tms_otdrbaseinfo *pval);
void tms_RemoveAnyMangerContext(int fd);
int32_t tms_CurAlarm(
    int fd,
    struct glink_addr *paddr,
    struct tms_curalarm *val);
int32_t tms_CurAlarm_V2(
    int fd,
    struct glink_addr *paddr,
    struct tms_curalarm *val);

struct tms_attr
{
	char cu_ip[16]; // CU 的IP地址
	char local_ip[16]; // 本地IP地址(暂时无用)

	/*
	只能取值192.168.1.201
	192.168.0.201，与自身IP处于同一网段
	*/
	char _201_ip[16]; // 本地第一块板卡的IP地址，
};
int32_t tms_MergeCurAlarm(int fd);
int tms_connect();
void tms_SetAttribute(struct tms_attr *attr);
void tms_GetAttribute(struct tms_attr *attr);
#ifdef __cplusplus
}
#endif

#endif
