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
#define RET_SUCCESS 			0///< 0 --- 命令成功（成功执行无返回数据命令时的返回值）
#define RET_UNEXIST 			1///< 1 --- 业务板卡不存在
#define RET_COMMU_ABORT 						2///< 2 --- MCU与业务板卡通信异常
#define RET_UNMATCH_FRAME 			3///< 3 --- 命令中机框编号与业务板卡本身所处的机框编号不一致
#define RET_UNMATCH_SLOT 			4///< 4 --- 命令中槽位编号与业务板卡本身所处的槽位编号不一致
#define RET_UNMATCH_TYPE 			5///< 5 ---  命令中设备类型与业务板卡本身的设备类型不一致
#define RET_PARAM_INVALID 			6///< 6 --- 命令参数出现非法值
#define RET_IGNORE_SAVE 			7///< 7 --- 命令参数无法保存
#define RET_WAIT 					8///< 8 --- 已经存在一条命令，请稍后重试
#define RET_OTDR_ILLEGAL 			9///< 9 ---  OTDR测量异常
#define RET_OTDR_TIMEOUT 			10///< 10 --- OTDR测量超时
#define RET_UPDATE_ILLEGAL 			11///< 11 ---  RTU升级过程出现异常。
#define RET_CMD_INVALID 			12///< 12 --- 无效命令
#define RET_OLP_CANT_SWITCH     	13///<olp不能切换到指定线路
#define RET_OSW_SWITCH_ABORT    	14///<光开关切换异常
#define RET_SEND_CMMD_TIMEOUT       15///<发送命令超时
#define RET_UNEXIST_ROW				16///<不存在行
#define RET_OLP_REFUSE				17///<不能对OLP模块走业务的光路端口进行OTDR测试
#define RET_RESOURCE_LOW			18///<系统资源分配不成功，或者其他原因导致设备重启
#define RET_SAVE_SOURCE             19///<存在两个相同的源地址连接，两个网管
#define RET_CONNECT_PORT_NOT_TEST   20///<本端口属于级联端口，不能进行测试
#define RET_MODULE_ROUTE_NO_EXIST   21///<模块路由不存在
#define RET_MODULE_ROUTE_ILLEGALT   22///<模块路由非法
#define RET_SMS_PHONE_ILLEGALT		23 /// 短信电话非法
#define RET_SMS_TEXT_VOERFLOW		24 /// 短信内容长度越界
#define RET_SMS_EQ_ERROR			25 //短信猫设备故障
#define RET_SYS_NEW_MEM_ERR         26 //系统分配内存失败
#define RET_GET_LOCAL_WLAN_IP_FAIL	27 //获取本地IP失败
#define RET_SMS_EQ_NO_EXIST			28 //短信猫不存在
#define RET_SMS_EQ_NITIALING			29 //短信猫正在初始化

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
#define 	ID_TRACE0			0x10000002	///< MCU打印调试信息
#define 	ID_TRACE1			0x10000003	///< MCU打印调试信息
#define 	ID_TRACE2			0x10000004	///< MCU打印调试信息
#define 	ID_R_COMMAMD			0x10000005	///< MCU上传字符串命令
#define 	ID_COMMAMD			0x10000006	///< 网管下发字符串命令

////////////////////////////////////////////////////////////////////////////////
// 命令类型:网管与MCU之间的通信
#define ID_UNUSE (0)

#define ID_GET_DEVTYPE			ID_UNUSE///< 0x60000000	///<工控板查询设备板卡类型信息
#define ID_RET_DEVTYPE			ID_UNUSE///< 0x60000001	///<返回设备类型信息
#define ID_CU_NOTE_NET			ID_UNUSE///< 0x60000002///<工控板与CU通信的网段选择切换通知
#define ID_CU_NOTE_MANAGE_CONNECT		ID_UNUSE///< 0x60000003///<CU通知工控板网管的连接状态

#define ID_GET_OPM_OLP_RAYPOWER		ID_UNUSE///< 0x60000004///<工控板查询某槽位上OPM或OLP模块总的光功率告警
#define ID_CMD_OSW_SWITCH			ID_UNUSE///< 0x60000005///<工控板通知OSW模块切换到某路
#define ID_CMD_OLP_REQ_OTDR			ID_UNUSE///< 0x60000006///<OLP模块向工控机请求OTDR测试
#define ID_CMD_OLP_START_OTDR		ID_UNUSE///< 0x60000007///<工控板通知OLP模块OTDR测试开始
#define ID_CMD_OLP_FINISH_OTDR		ID_UNUSE///< 0x60000008///<工控板通知OLP模块OTDR测试完成
#define ID_GET_ALARM_TEST			ID_UNUSE///< 0x60000009 ///<MCU对OTDR告警测试
#define ID_RET_ALARM_TEST			ID_UNUSE///< 0x60000010 ///<OTDR返回告警测试曲线
#define ID_GET_DEV_STATE_FROM_TU 	ID_UNUSE///< 0x60000011 ///<TU工控板向TU板卡查询其所在机框的板卡插拔状态
#define ID_RET_DEV_STATE_FROM_TU	ID_UNUSE///< 0x60000012 ///<TU板卡返回其所在机框的板卡插拔状态
#define ID_GET_POWER_STATE_FROM_TU 	ID_UNUSE///< 0x60000013 ///<工控板向TU板卡查询其所在机框的电源组成信息
#define ID_RET_POWER_STATE_FROM_TU 	ID_UNUSE///< 0x60000014 ///<TU板卡返回其所在机框的电源组成信息
#define ID_MCU_GET_DEV_ALARM	 	ID_UNUSE///< 0x60000015 ///<工控板查询某槽位上总的硬件告警
#define ID_DEV_RET_MCU_ALARM	 	ID_UNUSE///< 0x60000016 ///<各业务单板向MCU返回总的硬件告警
#define ID_OLP_REQUEST_OTDR		ID_UNUSE///< 0x60000017 ///<OLP板卡向MCU请求OTDR测试


////////////////////////////////////////////////////////////////////////////////
// 命令类型:网管与MCU之间的通信
///<网管查询MCU的设备序列号

#define ID_CHANGE_ADDR			ID_UNUSE///< 0x80000000	///<网管修改RTU的IP地址(RTU做TCP服务器，端口号6000)
#define ID_GET_SN				ID_UNUSE///< 0x80000001	///<网管激活RTU
#define ID_RET_SN				ID_UNUSE///< 0x80000002	///<MCU返回设备序列号
#define ID_CFG_SMS				ID_UNUSE///< 0x80000003	///<网管发送告警短信发送权限到RTU
#define ID_CFG_SMS_CLEAR		ID_UNUSE///< 0x80000004	///<网管发送清除告警短信发送权限到MCU

#define ID_GET_COMPOSITION		ID_UNUSE///< 0x80000005	///<网管查询板卡组成信息
#define ID_RET_COMPOSITION		ID_UNUSE///< 0x80000006	///<MCU返回板卡组成信息s

#define ID_CFG_MCU_OSW_PORT			ID_UNUSE///< 0x80000007	///<网管发送OSW模块各光端口关联光缆信息到MCU
#define ID_CFG_MCU_OSW_PORT_CLEAR		ID_UNUSE///< 0x80000008	///<网管发送清除OSW模块各光端口关联光缆信息到MCU
#define ID_CFG_MCU_OLP_PORT	ID_UNUSE///< 0x80000009	///<网管发送OLP模块各光端口关联光缆信息到MCU
#define ID_CFG_MCU_OLP_PORT_CLEAR	ID_UNUSE///< 0x80000010	///<网管发送清除OLP模块各光端口关联光缆信息到MCU


#define ID_CFG_MCU_U_OPM_OSW	ID_UNUSE///< 0x80000011	///<网管发送OPM光端口与OSW光端口联动关系到MCU
#define ID_CFG_MCU_U_OPM_OSW_CLEAR	ID_UNUSE///< 0x80000012	///<网管发送清除OPM光端口与OSW光端口联动关系到MCU
#define ID_CFG_MCU_U_OLP_OSW	ID_UNUSE///< 0x80000013	///<网管发送OLP模块光端口与OSW模块光端口联动关系到MCU
#define ID_CFG_MCU_U_OLP_OSW_CLEAR	ID_UNUSE///< 0x80000014	///<网管发送清除OLP模块光端口与OSW模块光端口联动关系到MCU



//////////////////////

#define ID_CFG_OPM_REF_LEVEL	ID_UNUSE///< 0x80000015	///<网管发送OPM模块各光端口关联光缆的参考光功率及告警门限
#define ID_GET_OPM_OP			ID_UNUSE///< 0x80000016	///<网管查询OPM模块各光端口当前功率
#define ID_RET_OPM_OP			ID_UNUSE///< 0x80000017	///<MCU返回OPM模块各光端口当前功率

#define ID_CFG_OLP_REF_LEVEL	ID_UNUSE///< 0x80000018	///<网管发送OLP模块各光端口关联光缆的光功率及告警门限到MCU
#define ID_GET_OLP_OP			ID_UNUSE///< 0x80000019	///<网管查询OLP模块各光端口当前功率
#define ID_RET_OLP_OP			ID_UNUSE///< 0x80000020	///<MCU返回OLP模块各光端口当前功率

#define ID_CFG_OTDR_REF				ID_UNUSE///< 0x80000021	///<网管发送各监测光路的OTDR参考曲线数据到MCU

#define ID_CFG_MCU_OSW_CYCLE	ID_UNUSE///< 0x80000022	///<网管发送OSW各光端口需要周期测试的光缆以及周期间隔到MCU
#define ID_CFG_OSW_CYCLE ID_UNUSE///< ID_CFG_MCU_OSW_CYCLE 

#define ID_GET_OTDR_TEST		ID_UNUSE///< 0x80000023	///<网管对OTDR点名测试
#define ID_RET_OTDR_TEST		ID_UNUSE///< 0x80000024	///<MCU返回OTDR测量曲线

#define ID_CFG_OLP_MODE			ID_UNUSE///< 0x80000025	///<网管对OLP模块的工作模式和返回时间设定
#define ID_CMD_OLP_SWITCH		ID_UNUSE///< 0x80000026	///<网管对OLP模块的指令倒换命令
#define ID_REPORT_OLP_ACTION 	ID_UNUSE///< 0x80000027 ///<MCU上报OLP人工切换或自动切换的动作信息

#define ID_ALARM_OPM			ID_UNUSE///< 0x80000028	///<MCU上报某槽位总的光功率告警
#define ID_ALARM_OPM_CHANGE		ID_UNUSE///< 0x80000029	///<MCU上报某槽位变化的光功率告警
#define ID_ALARM_LINE			ID_UNUSE///< 0x80000030	///<MCU返回线路告警
#define ID_ALARM_HW				ID_UNUSE///< 0x80000031	///<MCU返回硬件告警
#define ID_RET_OTDR_CYC			ID_UNUSE///< 0x80000032	///<MCU返回OTDR周期性测试曲线

#define ID_CMD_SMS_TEXT			ID_UNUSE///< 0x80000033	///<网管发送短信内容到MCU
#define ID_CMD_SMS_ERROR		ID_UNUSE///< 0x80000034	///<MCU返回短信操作返回码
#define ID_GET_VERSION			ID_UNUSE///< 0x80000035	///<网管查询板卡版本号
#define ID_RET_VERSION			ID_UNUSE///< 0x80000036	///<MCU返回板卡版本号
#define ID_ADJUST_TIME				ID_UNUSE///< 0x80000037	///<网管给MCU校时
#define ID_CMD_ACK				ID_UNUSE///< 0x80000038	///<MCU返回应答错误码
#define	ID_GET_OTDR_TEST_CYC    0x80000039 ///<网管对OTDR周期性测试
#define ID_RET_OTDR_TEST_CYC    ID_UNUSE///< 0x80000040 ///<MCU返回OTDR周期性测量曲线

#define ID_GET_OTDR_PARAM		ID_UNUSE///< 0x80000050 ///<网管查询OTDR板卡参数信息
#define ID_RET_OTDR_PARAM		ID_UNUSE///< 0x80000051 ///<MCU返回OTDR板卡参数信息
#define ID_GET_DEV_PRODUCE		ID_UNUSE///< 0x80000052 ///<网管查询板卡硬件生产信息
#define ID_RET_DEV_PRODUCE		ID_UNUSE///< 0x80000053 ///<MCU返回板卡硬件生产信息
#define ID_INSERT_TBROUTE		ID_UNUSE///< 0x80000054 ///<网管发送添加“模块级联表”内容到MCU
#define ID_DELALL_TBROUTE		ID_UNUSE///< 0x80000055 ///<网管发送清除“全部”“模块级联表”到MCU
#define ID_INSERT_TBUNIT		ID_UNUSE///< 0x80000056 ///<网管发送添加“光端口联动触发表”到MCU 
#define ID_DEL_TBUNIT			ID_UNUSE///< 0x80000057 ///<网管“逐条”删除“光端口联动触发表”到MCU 
#define ID_DELALL_TBUNIT		ID_UNUSE///< 0x80000058 ///<网管清除“全部”“光端口联动触发表”到MCU 
#define ID_INSERT_TBCYCTEST		ID_UNUSE///< 0x80000059 ///<网管发送添加“周期性测试表”到MCU 
#define ID_DEL_TBCYCTEST		ID_UNUSE///< 0x80000060 ///<网管发送“逐条”删除“周期性测试表”关系到MCU
#define ID_DELALL_TBCYCTEST		ID_UNUSE///< 0x80000061 ///<网管发送清除“全部”“周期性测试表”关系到MCU
#define ID_DEL_TBOTDRREFDATA	ID_UNUSE///< 0x80000062 ///<网管发送“逐条”清除各监测光路的OTDR参考曲线数据到MCU
#define ID_DELALL_TBOTDRREFDATA	ID_UNUSE///< 0x80000063 ///<网管发送清除“全部”各监测光路的OTDR参考曲线数据到MCU
#define ID_0x80000064	ID_UNUSE///< 0x80000064 ///<没有这个命令
#define ID_GET_COMPOSITION_RT	ID_UNUSE///< 0x80000065 ///<网管查询实时板卡组成信息
#define ID_RET_COMPOSITION_RT	ID_UNUSE///< 0x80000066 ///<MCU返回实时板卡组成信息
#define ID_ACK_COMPOSITION		ID_UNUSE///< 0x80000067 ///<网管确认板卡组成信息
#define ID_RET_ALARM_HW_CHANGE		ID_UNUSE///< 0x80000068 ///< MCU返回变化的硬件告警
#define ID_GET_OP_GATE		ID_UNUSE///< 0x80000069 ///< 网管查询OPM或OLP模块各光端口的参考光功率及告警门限
#define ID_RET_OP_GATE		ID_UNUSE///< 0x80000070 ///< MCU返回OPM或OLP模块各光端口的参考光功率及告警门限
#define ID_GET_MCU_TIME		ID_UNUSE///< 0x80000071 ///< 网管查询MCU当前运行时间
#define ID_RET_MCU_TIME		ID_UNUSE///< 0x80000072 ///< MCU返回当前运行时间
#define ID_ALARM_SOUND_ON_OFF		ID_UNUSE///< 0x80000073 ///< 网管开启或关闭MCU告警声音 
#define ID_GET_ALARM_SOUND_STATE		ID_UNUSE///< 0x80000074 ///< 网管查询MCU告警声音关闭状态 
#define ID_RET_ALARM_SOUND_STATE		ID_UNUSE///< 0x80000075 ///< 返回MCU告警声音关闭状态

#define ID_GET_TOTAL_OP_ALARM	ID_UNUSE///< 0x80000076 ///< 查询总的光功告警
#define ID_RET_TOTAL_OP_ALARM	ID_UNUSE///< 0x80000077 ///< 主控返回总的光功告警
#define ID_GET_TOTAL_HW_ALARM	ID_UNUSE///< 0x80000078 ///< 查询总的硬件告警
#define ID_GET_OLP_ACTION_LOG	ID_UNUSE///< 0x80000079 ///< 网管查询OLP切换记录
#define ID_RET_OLP_ACTION_LOG	ID_UNUSE///< 0x80000080 ///< 返回OLP切换记录

#define ID_GET_ALARM_POWER	ID_UNUSE///< 0x80000081 ///< 网管查询告警短信权限
#define ID_RET_ALARM_POWER	ID_UNUSE///< 0x80000082 ///< MCU返回告警短信权限
#define ID_GET_MCU_OSW_PORT	ID_UNUSE///< 0x80000083 ///< 网管查询OSW、OTDR或OLP模块各光端口关联光路信息
#define ID_RET_MCU_OSW_PORT	ID_UNUSE///< 0x80000084 ///< MCU 返回OSW、OTDR或OLP模块各光端口关联光路信息
#define ID_GET_OTDR_REF	ID_UNUSE///< 0x80000085 ///< 网管查询参考曲线
#define ID_RET_OTDR_REF	ID_UNUSE///< 0x80000086 ///< MCU返回参考曲线
#define ID_GET_TBROUTE	ID_UNUSE///< 0x80000087 ///< 网管查询模块级联表
#define ID_RET_TBROUTE	ID_UNUSE///< 0x80000088 ///< MCU返回模块级联表
#define ID_GET_TBUNIT	ID_UNUSE///< 0x80000089 ///< 网管查询“光端口联动触发表”
#define ID_RET_TBUNIT	ID_UNUSE///< 0x80000090 ///< MCU返回“光端口联动触发表”
#define ID_GET_TBCYCTEST	ID_UNUSE///< 0x80000091 ///< 网管查询周期性测量
#define ID_RET_TBCYCTEST	ID_UNUSE///< 0x80000092 ///< MCU返回周期性测量
#define ID_GET_OLP_LINE ID_UNUSE///< 0x80000093 ///< 网管查询OLP当前所在线路
#define ID_RET_OLP_LINE ID_UNUSE///< 0x80000094 ///< MCU返回OLP当前所在线路
#define ID_GET_OLP_INFO ID_UNUSE///< 0x80000095 ///< 网管查询OLP模块的工作模式、返回时间和切换门限
#define ID_RET_OLP_INFO ID_UNUSE///< 0x80000096 ///< MCU返回模块的工作模式、返回时间和切换门限


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


// end hebei 2

// hebei2
// 0x80000004

struct tms_fibersection_hdr
{
	char fiber_id[20];
	uint32_t count;
};
struct tms_fibersection_val
{
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

struct tms_otdr_param
{
	char otdr_id[20];
	uint32_t range;
	uint32_t wl;
	uint32_t pw;
	uint32_t time;
	float	gi;
	float	end_threshold;
	float	none_reflect_threshold;
};
struct tms_test_result
{
	char result[20];
	float range;
	float loss;
	float atten;
	char time[20];
};
struct tms_hebei2_data_hdr
{
	int8_t  dpid[12];					///< 测试数据单元名称
	int32_t count;					///< 数据点个数
};

// OTDR返回信息B部分
struct tms_hebei2_data_val
{
	uint16_t data;
};

struct tms_hebei2_event_hdr
{
	int8_t eventid[12];
	int32_t count;
};

struct tms_hebei2_event_val
{
	int32_t distance;		///< 该事件点距离
	int32_t event_type;		///< 该事件点类型
	float   att;			///< 该事件点两事件点与前事件点之间光纤衰减系数
	float   loss;			///< 该事件点连接损耗
	float   reflect;		///< 该事件点反射损耗
	float   link_loss;		///< 该事件点累计损耗
};

struct tms_fibersectioncfg
{
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
struct tms_cfgpip_status
{
	uint32_t status;
};

struct tms_getcyctestcuv
{
	uint32_t pipe;
};

struct tms_getstatus_data
{
	uint32_t pipe;
};

struct tms_getstatus_data_hdr
{
	uint32_t count;
};
struct tms_getstatus_data_val
{
	uint32_t pipe;
	uint32_t section_num;
	char time[20];
	float section_atten;
};
// 0x80000011
struct tms_otdr_crc_hdr
{
	uint32_t crc;
	char id[12];
	char name[64];
	char addr[64];
	char hw_ver[12];
	char sf_ver[12];
	uint32_t count;
};
struct tms_otdr_crc_val
{
	uint32_t pipe;
	uint32_t wl;
	uint32_t dr;
	char wdm[16];
	uint32_t reserved0;
};

struct tms_otdr_ch_status
{
	char id[12];
	uint32_t ch_status;
};

struct tms_otdr_param_hdr
{
	char id[20];
	uint32_t count;
};
struct tms_otdr_param_val
{
	uint32_t pipe;
	uint32_t range;
	uint32_t wl;
	uint32_t pw;
	uint32_t time;
	float	gi;
	float	end_threshold;
	float	none_reflect_threshold;
};

extern struct tms_fibersection_hdr;
extern struct tms_fibersection_val;

struct tms_otdrbaseinfo
{
	struct tms_otdr_crc_hdr     *otdr_crc_hdr;
	struct tms_otdr_crc_val     *otdr_crc_val;
	struct tms_otdr_ch_status   *otdr_ch_status;
	struct tms_otdr_param_hdr   *otdr_param_hdr;
	struct tms_otdr_param_val   *otdr_param_val;
	struct tms_fibersection_hdr *fiber_hdr;
	struct tms_fibersection_val *fiber_val;
};
// end 0x80000011
struct tms_get_otdrdata
{
	uint32_t pipe;
	uint32_t range;
	uint32_t wl;
	uint32_t pw;
	uint32_t time;
	float	gi;
	float	end_threshold;
	float	none_reflect_threshold;
};


struct tms_getstandardcurv
{
	uint32_t pipe;
};
// 20000000
struct tms_setotdrfpgainfo
{
	uint32_t pipe;
	uint32_t wl;
	uint32_t dr;
	char     wdm[16];
	uint32_t reserved0;
};
// 70000000
struct tms_setocvmpara
{
	float 		cable_len;
	uint32_t 	host_thr;
	uint32_t 	slave_thr;
	float 		amend;
};

struct tms_setocvmfpgainfo
{
	float 		max_cable_len;
	char		wl[64];
};
// end hebei2

struct pro_list
{
	char name[52];
	// int len;
};

#ifdef CONFIG_TEST_NET_STRONG
struct test_netpacket
{
	uint8_t fname[16];
	int32_t save;
	uint32_t id;
	uint32_t thread_id;
};
#endif
////////////////////////////////////////////////////////////////////////////////
// Section 4 TMSxx通信各种包结构
struct tms_dev_slot
{
	int32_t frame;
	int32_t slot;
};
struct tms_dev_type
{
	int32_t frame;
	int32_t slot;
	int32_t type;
};

struct tms_dev_version
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	uint8_t vsersion[48];
};

struct tms_dev_update_hdr
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	uint8_t target[16];
	int32_t flen;
};
struct tms_dev_md5
{
	uint8_t md5[32];
};



// ID_CHANGE_ADDR			2	
// #define TMS_NAME_LEN 64
struct tms_change_addr
{
	uint8_t ip[16];
	uint8_t mask[16];
	uint8_t gw[16];
};

// // ID_CFG_SMS				3
struct tms_sms_alarm
{
	int32_t type;
	int32_t level;
};
#define TLE_LEN 16
struct tms_sms_clerk
{
	int32_t alarm_count;
	int8_t  phone[TLE_LEN];
	struct tms_sms_alarm *alarm;
};

struct tms_sms_duty
{
	int32_t time;
	int32_t clerk_count;
	struct tms_sms_clerk *clerk;
};

// struct tms_cfg_sms
// {
// 	int32_t time;
// 	int8_t  phone[TLE_LEN];
// 	int32_t type;
// 	int32_t level;
// };

struct tms_sn
{
	int8_t sn[128];
};
//ID_CFG_SMS
struct tms_cfg_sms
{
	int32_t count;
};

struct tms_cfg_sms_val
{
	int32_t time;
	int8_t  phone[TLE_LEN];
	int32_t type;	///< 告警类型
#define SMSAT_LINE 		2 ///< 线路告警
#define SMSAT_POWERDOWN 1 ///< 光功率告警
#define SMSAT_HW 		3 ///< 硬件告警
#define SMSAT_COMMM 	4 ///< 通信告警
#define SMSAT_OLP 		5 ///< OLP告警
	int32_t level;	///< 告警级别
// SMSAT_LINE 和 SMSAT_POWERDOWN
#define SMSAL_LV0		1		///< 紧急告警
#define SMSAL_LV1		2		///< 主要告警
#define SMSAL_LV2		3		///< 次要告警

// SMSAT_HW 和 SMSAT_COMMM
#define SMSAL_OCCUR		1		///< 发生告警

// SMSAT_OLP
#define SMSAL_OLP_SWITCH	1	///< 指令切换
#define SMSAL_OLP_MANUAL	2	///< 人工切换
#define SMSAL_OLP_AUTO		3		///< 自动保护切换
#define SMSAL_OLP_BACK		4		///< 保护返回
};


// // ID_GET_DEVICE		4
// // struct tms_
// // {

// // };

// // ID_SET_DEVICE		5
// struct tms_SET_device
// {
// 	char 	device_name[TMS_NAME_LEN];
// 	int 	frame_count;
// 	int 	frame;
// 	int 	slot;
// 	int 	slot_type;
// 	int 	port_num;
// };
//ID_CFG_MCU_OSW_PORT 7
struct tms_cfg_mcu_osw_port
{
	int32_t frame;			///< 机框号
	int32_t slot;			///< 槽位号
	int32_t type;			///< 设备类型
	int32_t port;			///< 端口号
	uint8_t dev_name[64];	///< 设备名
	uint8_t cable_name[64];	///< 光纤名
	uint8_t start_name[64];	///< 本局端名
	uint8_t end_name[64];	///< 远局端名
};

// ID_CFG_OSW_PORT			6
struct tms_dev_port
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t port;
};

struct tms_ret_dev_type
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t port;
	int32_t reserved0;
	int32_t reserved1;
	int32_t reserved2;
	int32_t reserved3;
};
// #define tms_dev_base tms_dev_port 
	
struct tms_dev_composition
{
	int32_t count;//frame
	// struct tms_dev_port *dev;
};

struct tms_dev_composition_val
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t port;
	int32_t reserved0;
	int32_t reserved1;
	int32_t reserved2;
	int32_t reserved3;
};

// 0x60000012
struct tms_dev_status_from_tu
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t dev_status;
};

//###################################################################
// 0x60000014 ID_RET_POWER_STATE_FROM_TU
struct tms_ret_power_state_from_tu
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t count;
};

struct tms_ret_power_state_from_tu_val
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t port;
	int32_t reserve0;
	int32_t reserve1;
	int32_t reserve2;
	int32_t reserve3;

};

//#end
// 0x60000015 ID_MCU_GET_DEV_ALARM
struct tms_mcu_get_dev_alarm
{
	int32_t frame;
	int32_t slot;
	int32_t type;
};

//###################################################################
// 0x60000016 ID_DEV_RET_MCU_ALARM
struct tms_dev_ret_mcu_alarm
{
	int32_t	alarm_type;
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t count;
};

struct tms_dev_ret_mcu_alarm_val
{
	int32_t level;
	int32_t frame;
	int32_t slot;
	uint32_t reason;
	int8_t time[20];
};
//#end

// 0x60000017 ID_OLP_REQUEST_OTDR
struct tms_olp_request_otdr
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t port;
};




struct tms_alarm_hw_change
{
	int32_t type;
	int32_t total;
	int32_t count;
};

struct tms_alarm_hw_change_val
{
	int32_t level;
	int32_t frame;
	int32_t slot;
	int32_t type;
	// uint16_t reason[64];
	uint32_t reason;
	int8_t time[20];
};
// // ID_CFG_OLP_PORT			7
// struct tms_
// {
// int32_t frame		;
// int32_t slot		;
// int32_t type;
// int32_t port_num		;
// int8_t cable_name	[TMS_NAME_LEN]	;
// int32_t fiber_num		;
// uint8_t a_station_name	[TMS_NAME_LEN]	;
// int8_t landmark_name	[TMS_NAME_LEN]	;
// char	Z Station Name	[TMS_NAME_LEN]	;
// char	  ZLandmark ame	[TMS_NAME_LEN]	;
// int	A Station Position		;
// int	Cable  Length		;

// };
/////////////////////////////////////
// OPM/OLP 与 OSW关联数据结构大体相同
// ID_CFG_MCU_U_OPM_OSW  ID_CFG_MCU_U_OLP_OSW
struct tms_cfg_mcu_u_any_osw
{
	int32_t any_frame;	///< OPM/OLP机框编号
	int32_t any_slot;	///< OPM/OLP槽位编号
	int32_t any_type;	///< OPM/OLP设备类型
	int32_t	count;		

};
struct tms_cfg_mcu_u_any_osw_val
{
	int32_t any_port;	///< OPM/OLP端口编号
	int32_t osw_frame;	///< OSW机框编号
	int32_t osw_slot;	///< OSW槽位编号
	int32_t osw_type;	///<OSW设备类型，无用函数里自动设置成DEV_OSW
	int32_t osw_port;	///< OSW端口编号
};
// ID_CFG_MCU_U_OPM_OSW_CLEAR  ID_CFG_MCU_U_OLP_OSW_CLEAR
struct tms_cfg_mcu_any_port_clear_val
{
	int32_t any_port;
};




// ID_CFG_MCU_U_OLP_OSW_CLEAR






// struct tms_8_2
// {
// 	int32_t port_num		;
// 	int	Relation Num		;

// };
// struct tms_8_3
// {
// 	int32_t frame		;
// 	int32_t slot		;
// 	int32_t type;
// 	int32_t port_num		;
// };

// // ID_CFG_OLP_UNITE_OSW	9	
// struct tms_
// {
// 	int32_t frame		;
// 	int32_t slot		;
// 	int32_t type;
// 	int	Relation Num		;
// };

// struct tms_9_2
// {
// 	int32_t port_num		;
// 	int32_t frame		;
// 	int32_t slot		;
// 	int32_t type;
// 	int32_t port_num		;

// };
// // ID_CFG_OPM_REF_LEVEL	10	
struct tms_cfg_any_power_ref
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t count;
};

struct tms_cfg_opm_ref_val
{
	int32_t port;
	int32_t isminitor;
	int32_t wave;
	int32_t ref_power;
	int32_t leve0; // 紧急告警门限
	int32_t leve1; // 主要告警门限
	int32_t leve2; // 次要告警门限
};

struct tms_cfg_olp_ref_val
{
	int32_t port;
	int32_t isminitor;
	int32_t wave;
	int32_t ref_power;
	int32_t leve0; // 紧急告警门限
	int32_t leve1; // 主要告警门限
	int32_t leve2; // 次要告警门限
};


struct tms_any_op
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t count;
};
struct tms_any_op_val
{
	int32_t port;
	int32_t power;
};
// struct tms_10_2
// {
// 	int32_t port_num		;
// 	int	Is Monitor		;
// 	int	   Wave Length		;
// 	int	Ref Power		;
// 	int	Serious Alarm Th		;
// 	int	Primary Alarm Th		;
// 	int	Slight Alarm Th		;
// };
// // ID_GET_OPM_OP		11	
// struct tms_
// {
// 	int32_t frame		;
// 	int32_t slot		;
// 	int32_t type;
// };
// // ID_SET_OPM_OP		12	
// struct tms_
// {
// 	int32_t frame		;
// 	int32_t slot		;
// 	int32_t type;
// };
// struct tms_
// {
// 	int32_t port_num		;
// 	int	Current Power		;

// };

// // ID_CFG_OLP_REF_LEVEL	13	
// struct tms_
// {
// 	int32_t frame		;
// 	int32_t slot		;
// 	int32_t type;
// };
// struct tms_
// {
// 	int32_t port_num		;
// 	int	 Power		;
// 	int	Serious Alarm Th		;
// 	int	Primary Alarm Th		;
// 	int	Slight Alarm Th		;


// };
// // ID_GET_OLP_OP		14	
// struct tms_
// {
// 	int32_t frame		;
// 	int32_t slot		;
// 	int32_t type;
// };
// // ID_SET_OLP_OP		15	
// struct tms_
// {
// 	int32_t frame		;
// int32_t slot		;
// int32_t type;
// char	Monitor Port Num		;

// };
// struct tms_
// {

// int32_t port_num		;
// int	Transmit Power		;

// };



// //ID_CFG_OTDR				16	///<
// struct tms_
// {
// 	int32_t frame		;
// int32_t slot		;
// int32_t type;
// int32_t port_num		;
// char	Pipe Serial umber		;
// char	OTDRTest Parameter onfig[20];
// char	RANG		;
// char	WL		;
// char	PW		;
// char	TIME		;
// float	GI		;
// float	END HRESHOLD		;
// float	NONE EFLECTTHRESHOLD		;
// int	 Reserved		;
// int	 Reserved		;
// int	 Reserved		;


// };
// struct tms_
// {
// 	char	DPID(Data Points Block ID)		[12];
// 	char	TNDP(Number of Data Points)		;
// 	short data[20000];
// };


// // TODO (c)事件点信息单元
// struct tms_
// {
// };

// struct tms_
// {
// 	char	OTDR Test Result Information[20] 		;
// 	float	 Chain Range		;
// 	float	Chain Loss		;
// 	float	Chain Attenuation		;

// };

// struct tms_
// {
// 	int	Serious Alarm Th		;
// 	int	Primary Alarm Th		;
// 	int	Slight Alarm Th		;

// };










// //ID_CFG_MCU_OSW_CYCLE		22	
struct tms_cfg_mcu_osw_cycle
{
	int32_t  frame;		///< OSW机框编号
	int32_t  slot;		///< OSW槽位编号
	int32_t  type;		///< OSW设备类型
	uint32_t count;		///< 有多少个 struct tms_cfg_mcu_osw_cycle_val 配置项
};
struct tms_cfg_mcu_osw_cycle_val
{
	int32_t port;		///< OSW端口编号
	int32_t iscyc;		///< 是否周期测试
	int32_t interval;	///< 周期测试间隔
};


// ID_CFG_OLP_MODE
struct tms_cfg_olp_mode
{
	// int32_t  frame;		///< 
	// int32_t  slot;		///< 
	// int32_t  type;		///< 
	// int32_t  flag;
	// int32_t  mode;		///< 0表示保护不返回，1表示保护返回
	// int32_t  backtime;	///< 返回时间，单位分钟。
	// int32_t  protect;	///< 保护线路，0表示保护线路为主路，1表示保护线路为备路
	// int32_t  sw_gate;	///< 切换门限


	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t flag;
	int32_t mode;
	int32_t back_time;
	int32_t protect_port;
	int32_t work_line;
	int32_t sw_level1;
	int32_t sw_level2;
	int32_t tx_ref;
};
// struct tms_
// {
// 	char	Port Number		;
// 	char	Is CycTest		;
// 	char	CycTest Interval		;
// 	char	delay		;
// };
//ID_GET_OTDR_TEST	18	

struct tms_alarm_line_hdr
{
	int32_t	alarm_type;
	int32_t	alarm_level;
	int32_t frame;
	int32_t slot;
	int32_t port;
	int32_t alarm_position;
	int8_t time[20];
	int32_t reserve0;
};
// 不同平台hdr不同
struct tms_getotdr_test_hdr
{
	int32_t frame;				///< OSW机框号
	int32_t slot;				///< OSW槽位号
	int32_t type;				///< OSW设备类型
	int32_t port;				///< OSW端口号
	int32_t	reserve0;			///< OSW保留


	// int32_t otdr_frame;				///< OTDR机框号
	// int32_t otdr_slot;				///< OTDR槽位号
	// int32_t otdr_type;				///< OTDR设备类型
	// int32_t otdr_port;				///< OTDR端口号
};

// 不同型号获取OTDR参数保持一致
struct tms_getotdr_test_param
{
	int32_t	range;					///< 测试量程
	int32_t	wl;						///< 测试波长
	int32_t	pw;						///< 测试脉宽
	int32_t	time;					///< 测试时间
	float	gi;						///< 群折射率
	float	end_threshold;			///< 结束门限
	float	none_reflect_threshold; ///< 非反射门限
	int32_t reserve0;				///< 保留 
	int32_t reserve1;				///< 保留
	int32_t reserve2;				///< 保留
};


// OTDR参考曲线头
struct tms_otdr_ref_hdr
{
	int32_t osw_frame;		///< osw机框号
	int32_t osw_slot;		///< osw槽位号
	int32_t osw_type;		///< osw设备类型
	int32_t osw_port;		///< osw端口
	int32_t otdr_port;		///< otdr端口
	int8_t  strid[20];		///< OTDR测量参数的识别ID，


	// int32_t otdr_frame;				///< OTDR机框号
	// int32_t otdr_slot;				///< OTDR槽位号
	// int32_t otdr_type;				///< OTDR设备类型
	// int32_t otdr_port;				///< OTDR端口号
	
};

struct tms_cfg_otdr_ref_val
{
	int32_t leve0; // 紧急告警门限
	int32_t leve1; // 主要告警门限
	int32_t leve2; // 次要告警门限
};
// OTDR返回信息A部分，不同TMS系列可能不同
struct tms_retotdr_test_hdr
{
	int32_t osw_frame;	///< osw机框号
	int32_t osw_slot;	///< osw槽位号
	int32_t osw_type;	///< osw设备类型
	int32_t osw_port;	///< osw端口
	int8_t  time[20];	///< 测量时间，格式如下2015-03-19 14:02:15。

	int32_t otdr_frame;	///< otdr机框号
	int32_t otdr_slot;	///< otdr槽位号
	int32_t otdr_type;	///< otdr设备类型
	int32_t otdr_port;	///< otdr端口
};
struct tms_retotdr_test_param
{
	int32_t	range;					///< 测试量程
	int32_t	wl;						///< 测试波长
	int32_t	pw;						///< 测试脉宽
	int32_t	time;					///< 测试时长
	float	gi;						///< 群折射率
	float	end_threshold;			///< 结束门限
	float	none_reflect_threshold; ///< 非反射门限
	int32_t sample;					///< 采样速率
	int32_t reserve0;				///< 保留 
	int32_t reserve1;				///< 保留
	//int32_t reserve2;				///< 保留
};


struct tms_retotdr_data_hdr
{
	int8_t  dpid[12];					///< 测试数据单元名称
	int32_t count;					///< 数据点个数
};

// OTDR返回信息B部分
struct tms_retotdr_data_val
{
	uint16_t data;
};

struct tms_retotdr_event_hdr
{
	int8_t eventid[12];
	int32_t count;
};
// // OTDR返回信息C部分
struct tms_retotdr_event_val
{
	int32_t distance;		///< 该事件点距离
	int32_t event_type;		///< 该事件点类型
	float   att;			///< 该事件点两事件点与前事件点之间光纤衰减系数
	float   loss;			///< 该事件点连接损耗
	float   reflect;		///< 该事件点反射损耗
	float   link_loss;		///< 该事件点累计损耗
};

// OTDR返回信息D部分
struct tms_retotdr_chain
{
	int8_t inf[20];			///< 测试结果
	float range;		///< 链长
	float loss;		///< 链损耗
	float att;		///< 链衰减系数
};


// //ID_SET_OTDR_TEST	19	
// struct tms_
// {
// int32_t frame		;
// int32_t slot		;
// int32_t type;
// int32_t port_num		;
// char	Test Time		;
// int32_t frame		;
// int32_t slot		;
// int32_t type;
// char	Pipe Number		;
// char	RANG		;
// char	WL		;
// char	PW		;
// char	TIME		;
// float	GI		;
// float	END THRESHOLD		;
// float	NONE REFLECT THRESHOLD		;
// int	 Sampleing Rate		;
// int	 Reserved		;
// int	 Reserved		;

// };

// struct tms_
// {
// 	char	DPID(Data Points Block ID)		[12];
// 	char	TNDP(Number of Data Points)		;
// 	short data[20000];

// };

// struct tms_
// {
// 	char	 Key Events Block ID		;
// 	char	 Number of Key Events		;
// };

// struct tms_event
// {
// 	int	Event Distance		;
// 	int	 Event Number		;
// 	float	 Attenuation Coefficient		;
// 	float	 Event Loss		;
// 	float	 Event Reflectance		;
// 	float	 Event Total Link Loss		;

// };

// struct tms_
// {
// char	OTDR Test Result Information 		;
// float	 Chain Range		;
// float	Chain Loss		;
// float	Chain Attenuation		;

// };
// //ID_CMD_OSW_SWITCH111111111111111111111111111		20	
// struct tms_
// {
// int32_t frame		;
// int32_t slot		;
// int	Device Type		;
// int	Switch Type		;
// int	Switch		;

// };
// //ID_CMD_OSW_SWITCH2222222222222222222222222222		21	
// struct tms_
// {
// int32_t frame		;
// int32_t slot		;
// int	Device Type		;
// int	Switch Type		;
// int	Switch		;

// };


//ID_ALARM_OPM			28
struct tms_alarm_opm
{
	int	alarm_type;
	int32_t frame;
	int32_t slot;
	// int32_t type;
	int32_t	count;
};
struct tms_alarm_opm_val
{
	int32_t port;
	int32_t	levelx;
	int32_t	power;
	uint8_t	time[20];
};

struct tms_alarm_opm_change
{
	int	alarm_type;
	int32_t frame;
	int32_t slot;
    //int32_t type;
	int32_t alarm_cnt;
	int32_t	count;
};

// ID_REPORT_OLP_ACTION
struct tms_report_olp_action
{
	int32_t  frame;
	int32_t  slot;
	int32_t  type;
	int32_t  sw_type;///< 0表示人工切换，1表示自动保护倒换，2表示保护返回。
	int32_t  sw_port;     ///< 0表示切换到主路，1表示切换到备路
	uint8_t	time[20];
};
// // 与上面的合并
// struct tms_
// {
// int	Alarm Type		;
// int32_t frame		;
// int32_t slot		;
// int	Num		;
// };

// struct tms_
// {
// int32_t port_num		;
// int	Alarm Level		;
// int	Current Power		;
// char	Time[20]		;
// };

// //ID_ALARM_LINE			23	
// struct tms_
// {
// int	Alarm Type		;
// int	Alarm Level		;
// int32_t frame		;
// int32_t slot		;
// int32_t port_num		;
// int	Alarm Position		;
// char	Time[20]		;
// int	Reserved		;

// };
//ID_ALARM_HW				24	
struct tms_alarm_hw
{
	int	alarm_type;
	int	count;
};

struct tms_alarm_hw_val
{
	int32_t level;
	int32_t frame;
	int32_t slot;
	int32_t type;
	// uint8_t	reason[128];
	uint32_t	reason;
	uint8_t	time[20];
};
// //ID_CMD_OTDR_CYC			25	
// struct tms_
// {
// 	int CycTest Curve Num[2000];
// 	// TODO
// };

// //ID_CMD_SMS_TEXT			26	
// struct tms_sms_text
// {
// 	char       phone_num[16];
// 	uint32_t   len;
// 	short      msg[70];
// };
// //ID_CMD_SMS_ERROR		27	
struct tms_sms_state
{
	char     phone[16];
	uint32_t state;
};
struct tms_adjust_tm
{
	char     tm[20];
};
// //ID_GET_VERSION		28	
// struct tms_
// {
// int32_t frame		;
// 	int32_t slot		;
// 	int32_t type;
// };
// //ID_SET_VERSION		29	
// struct tms_
// {
// int32_t frame		;
// 	int32_t slot		;
// 	int32_t type;
// 	char version;

// };
// //ID_CMD_TIME				30	
// struct tms_
// {
// 	char Current Time[20];
// };
//ID_CMD_ACK				31	
struct tms_ack
{
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
struct tms_devbase
{
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
struct tms_manage
{
	int fd[MANAGE_COUNT];       ///< 第三方开发的网管
	int fd_addr[MANAGE_COUNT];
	int fdtc[MANAGE_COUNT];		///< 支持控制台的网管数
	int fdtc_addr[MANAGE_COUNT];
};

struct tms_man_base
{
	int fd;
	uint32_t addr;
};
///< 设备上下文描述
// 主要处理心跳计数，用与当设备长时间断开连接后能快速响应
// 应用层“刷新”命令读取tick，如果tick一直不变表示死链接
struct tms_context
{
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

struct tms_dev_produce
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int8_t hw_ver[20];
	int8_t date[20];
	int8_t sn[64];
	int32_t reserved0;
	int32_t reserved1;
};


#pragma pack(1)
struct tms_otdr_produce_hdr
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int16_t count;
};
struct tms_otdr_produce_val
{
	int16_t index;
	int16_t dB;
};
#pragma pack(4)

struct tms_route
{
	int32_t ip_src;
	int32_t ip_dst;

	int32_t  frame_a;		///< 机框编号a
	int32_t  slot_a;		///< 槽位编号a
	int32_t  type_a;		///< 设备类型a
	int32_t  port_a;		///< 端口编号a

	int32_t  frame_b;		///< 机框编号b
	int32_t  slot_b;		///< 槽位编号b
	int32_t  type_b;		///< 设备类型b
	int32_t  port_b;		///< 端口编号b
};

struct tms_cyctest
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t port;
	int32_t reserved0;
	int32_t iscyc;

};
struct tms_unit
{
	int32_t  frame_a;		///< 机框编号a
	int32_t  slot_a;		///< 槽位编号a
	int32_t  type_a;		///< 设备类型a
	int32_t  port_a;		///< 端口编号a

	int32_t  frame_b;		///< 机框编号b
	int32_t  slot_b;		///< 槽位编号b
	int32_t  type_b;		///< 设备类型b
	int32_t  port_b;		///< 端口编号b
};

// ID_GET_TOTAL_OP_ALARM
struct tms_total_op_alarm_hdr
{
	int32_t type;
	int32_t count;
};

struct tms_total_op_alarm_val
{
	int32_t frame;
	int32_t slot;
	int32_t port;
	int32_t alarm_level;
	int32_t cur_power;
	int8_t time[20];
};

// ID_GET_OLP_ACTION_LOG
struct tms_olp_action_log_hdr
{
	int32_t count;
};

struct tms_olp_action_log_val
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t sw_type;
	int32_t sw_port;
	int8_t  time[20];
};
struct tms_get_olp_line
{
	int32_t frame;
	int32_t slot;
	int32_t type;
};

struct tms_ret_olp_line
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t current_line;
};
struct tms_get_olp_info
{
	int32_t frame;
	int32_t slot;
	int32_t type;
};

struct tms_ret_olp_info
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t flag;
	int32_t mode;
	int32_t back_time;
	int32_t protect_port;
	int32_t work_line;
	int32_t sw_level1;
	int32_t sw_level2;
	int32_t tx_ref;
	int32_t tx_jump;

};

struct tms_get_mcuosw_port
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t port;
};

struct tms_get_otdrref
{
	int32_t frame;
	int32_t slot;
	int32_t type;
	int32_t port;
};
#include "bipbuffer.h"
///< 应用程序
struct tmsxx_app
{
	// int    fd;                   ///<socket fd
	int    morebyte;             ///<为防止bipbuffer环形缓存浪费
	struct bipbuffer bb;         ///<glink接收环形缓存，长度初始化后永远不变

	int enable_lbb;
	int lbyte;					 ///<
	struct bipbuffer lbb;		 ///<临时环形缓存，仅用于接收大于bb长度的帧
	struct tms_context context;  ///<tms包描述符


};

struct tms_callback
{
	int32_t (*ptrfun)(int fd, int cmdid, void *ptr);
	int32_t (*ptrfun1)(int fd, int cmdid, void *ptr);
	// int32_t (*pf_OnCopy2Use)(char *data, int32_t datalen, int msec, int fd);
	int32_t (*pf_OnCopy2Use)(char *data, int32_t datalen, int msec, void* fd);
	int32_t (*pf_OnGetDeviceComposition)(struct tms_context *pcontext, int8_t *pdata, int32_t len);
	int32_t (*pf_OnRetDeviceComposition)(struct tms_context *pcontext, int8_t *pdata, int32_t len,
			struct tms_dev_composition *pcfg_hr,
			struct tms_dev_composition_val *plist);
	int32_t (*pf_OnCommand)(struct tms_context *pcontext, char *strcmd, int32_t slen);
	int32_t (*pf_OnCUNoteNet)(void);
	int32_t (*pf_OnCUNoteManageConnect)(int32_t state);
	int32_t (*pf_OnRetSerialNumber)(uint8_t (*psn)[128]);
	int32_t (*pf_OnSetIPAddress)(uint8_t (*ip)[16], uint8_t (*mask)[16], uint8_t (*gw)[16]);
	int32_t (*pf_OnRetOTDRTest)(struct tms_context *pcontext,
			struct tms_retotdr_test_hdr   *ptest_hdr,
			struct tms_retotdr_test_param *ptest_param,
			struct tms_retotdr_data_hdr   *pdata_hdr,
			struct tms_retotdr_data_val   *pdata_val,
			struct tms_retotdr_event_hdr  *pevent_hdr,
			struct tms_retotdr_event_val  *pevent_val,
			struct tms_retotdr_chain      *pchain);

	int32_t (*pf_OnAlarmLine)(struct tms_context *pcontext,
			struct tms_alarm_line_hdr     *palarm,
			struct tms_retotdr_test_hdr   *ptest_hdr,
			struct tms_retotdr_test_param *ptest_param,
			struct tms_retotdr_data_hdr   *pdata_hdr,
			struct tms_retotdr_data_val   *pdata_val,
			struct tms_retotdr_event_hdr  *pevent_hdr,
			struct tms_retotdr_event_val  *pevent_val,
			struct tms_retotdr_chain      *pchain);

	int32_t (*pf_OnCfgOTDRRef)(struct tms_context *pcontext,
			struct tms_retotdr_test_param *ptest_param,
			struct tms_retotdr_data_hdr   *pdata_hdr,
			struct tms_retotdr_data_val   *pdata_val,
			struct tms_retotdr_event_hdr  *pevent_hdr,
			struct tms_retotdr_event_val  *pevent_val,
			struct tms_retotdr_chain      *pchain,
			struct tms_cfg_otdr_ref_val   *pref_data);
	int32_t (*pf_OnSpAnyGetOTDRTest)(struct tms_context *pcontext, int8_t *pdata, int32_t len);
	int32_t (*pf_OnSpSendSMS)(struct tms_context *pcontext, int8_t *pdata, int32_t len);
	int32_t (*pf_OnSpAck)(struct tms_context *pcontext, int8_t *pdata, int32_t len);
	int32_t (*pf_OnRetDevType)(struct tms_context *pcontext, struct tms_ret_dev_type *pval);
	int32_t (*pf_OnRetVersion)(struct tms_context *pcontext, struct tms_dev_version *pval);
	int32_t (*pf_OnRetAnyOP)(struct tms_context *pcontext, struct tms_any_op *phdr, struct tms_any_op_val  *plist);
	int32_t (*pf_OnGetSerialNumber)(struct tms_context *pcontext);
	int32_t (*pf_OnInsertTbRoute)(struct tms_context *pcontext,int count ,struct tms_route *plist);


	// hebei2
	// 80000000
	int32_t (*pf_OnGetBasicInfo)(struct tms_context *pcontext);
	int32_t (*pf_OnGetNodeTime)(struct tms_context *pcontext);
	int32_t (*pf_OnRetNodeTime)(struct tms_context *pcontext);
	int32_t (*pf_OnNameAndAddress)(struct tms_context *pcontext);
	int32_t (*pf_OnFiberSectionCfg)(struct tms_context *pcontext, struct tms_fibersectioncfg *pval);
	int32_t (*pf_OnConfigPipeState)(struct tms_context *pcontext, struct tms_cfgpip_status *pval);
	int32_t (*pf_OnGetCycleTestCuv)(struct tms_context *pcontext,struct tms_getcyctestcuv *pval);
	int32_t (*pf_OnGetStatusData)(struct tms_context *pcontext,struct tms_getstatus_data *pval);
	int32_t (*pf_OnStatusData)(struct tms_context *pcontext);
	int32_t (*pf_OnCRCCheckout)(struct tms_context *pcontext);

	int32_t (*pf_OnCheckoutResult)(struct tms_context *pcontext);
	int32_t (*pf_OnOTDRBasicInfo)(struct tms_context *pcontext);
	int32_t (*pf_OnConfigNodeTime)(struct tms_context *pcontext);
	int32_t (*pf_OnCurAlarm)(struct tms_context *pcontext);
	int32_t (*pf_OnGetOTDRData)(struct tms_context *pcontext);
	int32_t (*pf_OnGetStandardCurv)(struct tms_context *pcontext, struct tms_getstandardcurv *pval);

	// 20000000
	int32_t (*pf_OnSetOTDRFPGAInfo)(struct tms_context *pcontext, struct tms_setotdrfpgainfo *pval);

	// 70000000
	int32_t (*pf_OnSetOCVMPara)(struct tms_context *pcontext, struct tms_setocvmpara *pval);
	int32_t (*pf_OnSetOCVMFPGAInfo)(struct tms_context *pcontext, struct tms_setocvmpara *pval);
	

};


///< 回调函数列表结构，采用函数指针方法，避免多个if、switch的低效率
struct tms_analyse_array
{
	int (*ptrfun)(struct tms_context *pcontext, int8_t *pdata, int32_t len);
	int dowhat;
};



////////////////////////////////////////////////////////////////////////
// Section 6 MCU与业务板通信接口
int32_t tms_Tick(int fd,struct glink_addr *paddr);
int32_t tms_Update(
		int fd,	
		struct glink_addr *paddr,
		int32_t frame, 
		int32_t slot,
		int32_t type,
		uint8_t (*target)[16],
		int32_t flen,
		uint8_t *pdata);
void tms_Trace(struct glink_addr *paddr,char *strout, int32_t len, int level);
int32_t tms_Command(
	int fd, 
	struct glink_addr *paddr,
	char *strcmd, 
	int32_t len);

// 应用层不需要调用该函数
int32_t tms_MCUtoDevice(
	int     fd, 
	struct glink_addr *paddr,
	int32_t frame, 
	int32_t slot,
	int32_t type,
	int32_t port,
	int32_t cmdID,
	int32_t len);
int32_t tms_TestPacketID(
    int fd,
    struct glink_addr *paddr,
    uint8_t *pdata,
    uint32_t len,
    uint32_t cmdid);
// 一下函数间接调用tms_MCUtoDevice
int32_t tms_MCU_GetDeviceType(
	int fd,
	struct glink_addr *paddr);//, int32_t frame, int32_t slot);

int32_t tms_MCU_RetDeviceType(
	int     fd,
	struct glink_addr *paddr,
	int32_t frame, 
	int32_t slot, 
	int32_t type, 
	int32_t port);
int32_t tms_MCU_RetDeviceType_V2(
	int     fd, 
	struct glink_addr *paddr, 
	struct tms_ret_dev_type *pval);
// int32_t tms_MCU_GetOPMRayPower(
int32_t tms_MCU_GetOPMAlarm(
	int     fd, 
	struct glink_addr *paddr,
	int32_t frame, 
	int32_t slot);
// int32_t tms_MCU_GetOLPRayPower(
int32_t tms_MCU_GetOLPAlarm(
	int     fd, 
	struct glink_addr *paddr,
	int32_t frame, 
	int32_t slot);
int32_t tms_MCU_GetOLPAlarm(
	int     fd, 
	struct glink_addr *paddr, 
	int32_t frame, 
	int32_t slot);
int32_t tms_MCU_OSWSwitch(
	int     fd, 
	struct glink_addr *paddr,
	int32_t frame, 
	int32_t slot,
	int32_t port);
int32_t tms_MCU_OLPReqOTDRTest(
	int     fd, 
	struct glink_addr *paddr,
	int32_t frame, 
	int32_t slot,
	int32_t port);
int32_t tms_MCU_OLPStartOTDRTest(
	int     fd, 
	struct glink_addr *paddr,
	int32_t frame, 
	int32_t slot,
	int32_t port);
int32_t tms_MCU_OLPFinishOTDRTest(
	int     fd, 
	struct glink_addr *paddr,
	int32_t frame, 
	int32_t slot,
	int32_t port);
int32_t tms_Tick(int fd,struct glink_addr *paddr);




////////////////////////////////////////////////////////////////////////
// 网管与MCU之间的通信
int32_t tms_SetIPAddress(
	int     fd, 
	struct glink_addr *paddr,
	uint8_t *ip, 
	uint8_t *mask, 
	uint8_t *gw);
int32_t tms_GetSerialNumber(int fd, struct glink_addr *paddr);
int32_t tms_RetSerialNumber(int fd, struct glink_addr *paddr, uint8_t (*psn)[128]);
int32_t tms_CfgSMSAuthorization(
		int fd,
		struct glink_addr *paddr,
		int32_t count,
		struct tms_cfg_sms_val *smslist);
uint32_t tms_RetDeviceComposition(
		int      fd, 
		struct glink_addr *paddr,
		uint32_t count, 
		struct tms_dev_composition_val *list);
uint32_t tms_RetDeviceCompositionRT(
        int      fd,
        struct glink_addr *paddr,
        uint32_t count,
        struct tms_dev_composition_val *list);

uint32_t tms_GetDeviceComposition(
		int     fd,
		struct glink_addr *paddr);
int32_t tms_CfgMCUAnyPort(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t port, 
		int32_t type, 
		int32_t cmdID, 
		uint8_t (*dev_name)[64], 
		uint8_t (*cable_name)[64], 
		uint8_t (*start_name)[64], 
		uint8_t (*end_name)[64]);
int32_t tms_RetVersion(
		int fd,	
		struct glink_addr *paddr,
		int32_t frame, 
		int32_t slot,
		int32_t type,
		uint8_t  *version);
int32_t tms_Ack(
	int fd,
	struct glink_addr *addr,
	int errcode,
	int cmdid);
int32_t tms_AckEx(
	int fd, 
	struct glink_addr *paddr, 
	struct tms_ack *pACK);

int32_t tms_CfgOPMRefLevel(
		int fd,
		struct glink_addr *paddr,
		int32_t frame,
		int32_t slot,
		int32_t count,
		struct tms_cfg_opm_ref_val *list);
int32_t tms_CfgOLPRefLevel(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t count, 
		struct tms_cfg_olp_ref_val *list);

int32_t tms_GetOPMOP(
		int fd, 
		struct glink_addr *paddr,
		int32_t frame,  
		int32_t slot);
// int32_t tms_RetOPMOP(
// 		int fd,
// 		int32_t frame,
// 		int32_t slot,
// 		int32_t count,
// 		struct tms_any_op_val *opmList);
int32_t tms_CfgOLPMode(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t mode, 
		int32_t backtime, 
		int32_t protect,
		int32_t sw_gate);
int32_t tms_RetOPMOP(
		int fd,
		struct glink_addr *paddr,
		int32_t frame,
		int32_t slot,
		int32_t count,
		struct tms_any_op_val *list);
int32_t tms_MCU_OLPSwitch(
	int     fd, 
	struct glink_addr *paddr, 
	int32_t frame, 
	int32_t slot, 
	int32_t sw_port);
int32_t tms_ReportOLPAction(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t sw_type, 
		int32_t sw_port);
int32_t tms_AlarmOPM(
		int fd,
		struct glink_addr *paddr,
		int32_t frame,
		int32_t slot,
		// int     alarm_type,
		int32_t count,
		struct tms_alarm_opm_val *list);

int32_t tms_AlarmOPMChange(
		int     fd,
		struct glink_addr *paddr,
		int32_t frame,
		int32_t slot,
		// int     alarm_type,
		int 	alarm_cnt,
		int32_t count,
		struct  tms_alarm_opm_val *list);
int32_t tms_GetVersion(
		int fd,	
		struct glink_addr *paddr,
		int32_t frame, 
		int32_t slot,
		int32_t type);

void tms_Trace(struct glink_addr *paddr,char *strout, int32_t len, int level);
#define tms_Trace3(a,b) tms_Trace(a,b,LEVEL_UI)
// int32_t tms_Analyse(int fd, int8_t *pdata, int32_t len);
int32_t tms_Analyse(struct tms_context *pcontext, int8_t *pdata, int32_t len);


int32_t tms_GetSockFD(int32_t frame, int32_t slot);
int32_t tms_GetDevBaseByFd(int fd, struct tms_devbase *pout);
int32_t tms_GetDevBaseByLocation(int32_t frame, int32_t slot,struct tms_devbase *pout);
int32_t tms_GetTCManageFd(
		int (*fd)[MANAGE_COUNT]);
int32_t tms_GetManageFd();
int32_t tms_RemoveDev(int fd);
int32_t tms_ManageCount();
// int32_t tms_AddManage(int fd, int32_t type);
int32_t tms_AddManage(struct tms_context *pcontext, int fd, int32_t type);
// int32_t tms_DelManage(int fd);
int32_t tms_DelManage(struct tms_context *pcontext, int fd);
int32_t tms_GetFrame(int32_t frame, struct tms_devbase (*pdev)[MAX_SLOT]);

void tms_Init();
int32_t tms_CfgMCUOSWCycle(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t count, 
		struct tms_cfg_mcu_osw_cycle_val *oswCycleList);
int32_t tms_CfgMCUOSWPort(
		int fd,
		struct glink_addr *paddr,		
		int32_t frame,
		int32_t slot,
		int32_t port,
		uint8_t (*dev_name)[64],
		uint8_t (*cable_name)[64],
		uint8_t (*start_name)[64],
		uint8_t (*end_name)[64]);
int32_t tms_CfgMCUOLPPort(
		int fd,
		struct glink_addr *paddr,		
		int32_t frame,
		int32_t slot,
		int32_t port,
		uint8_t (*dev_name)[64],
		uint8_t (*cable_name)[64],
		uint8_t (*start_name)[64],
		uint8_t (*end_name)[64]);
int32_t tms_CfgMCUUniteAnyOSW(
		int fd,
		struct glink_addr *paddr,
		int32_t any_frame,
		int32_t any_slot,
		int32_t any_type,
		int32_t cmdID,
		int32_t	count,
		struct tms_cfg_mcu_u_any_osw_val *list);

int32_t tms_CfgMCUOSWPortClear(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t any_slot, 
		int32_t	count, 
		struct tms_cfg_mcu_any_port_clear_val *list);
int32_t tms_CfgMCUOPMPortClear(
		int fd,
		struct glink_addr *paddr,
		int32_t frame,
		int32_t any_slot,
		int32_t	count,
		struct tms_cfg_mcu_any_port_clear_val *list);
int32_t tms_CfgMCUOLPPortClear(
		int fd,
		struct glink_addr *paddr,
		int32_t frame,
		int32_t any_slot,
		int32_t	count,
		struct tms_cfg_mcu_any_port_clear_val *list);
int32_t tms_CfgMCUUniteOPMOSW(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t	count, 
		struct tms_cfg_mcu_u_any_osw_val *list);
int32_t tms_CfgMCUUniteOPMOSWClear(
		int fd,
		struct glink_addr *paddr,
		int32_t any_frame,
		int32_t any_slot,
		int32_t	count,
		struct tms_cfg_mcu_any_port_clear_val *list);
int32_t tms_CfgMCUUniteOLPOSWClear(
		int fd,
		struct glink_addr *paddr,
		int32_t frame,
		int32_t slot,
		int32_t	count,
		struct tms_cfg_mcu_any_port_clear_val *list);
int32_t tms_CfgMCUUniteOLPOSW(
		int fd,
		struct glink_addr *paddr,
		int32_t frame,
		int32_t slot,
		int32_t	count,
		struct tms_cfg_mcu_u_any_osw_val *list);
int32_t tms_CfgMCUAnyPortClear(
		int fd, 
		struct glink_addr *paddr, 
		int32_t any_frame, 
		int32_t any_slot, 
		int32_t any_type, 
		int32_t cmdID, 
		int32_t	count, 
		struct  tms_cfg_mcu_any_port_clear_val *list);
int32_t tms_CfgAnyRefLevel(
		int fd, 
		struct glink_addr *paddr, 
		int32_t any_frame, 
		int32_t any_slot, 
		int32_t any_type, 
		int32_t cmdID, 
		int32_t count, 
		void    *list);
void tms_SetDoWhat(int cmdh, int count, int *arr);
int32_t tms_SendSMS(
		int fd,
		struct glink_addr *paddr,
		char (*phone)[16],
		int32_t count,
		wchar_t *text);
int32_t tms_RetSMSError(
		int fd, 
		struct glink_addr *paddr, 
		char phone[16], 
		uint32_t state);
int32_t tms_GetOTDRTest(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t type, 
		int32_t port, 
		int32_t otdr_port,
		struct tms_getotdr_test_param *val);
int32_t tms_GetOTDRTestCycle(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t type, 
		int32_t port,
		int32_t otdr_port, 
		struct tms_getotdr_test_param *val);
int32_t tms_AnyRetOTDRTest(
		int fd,
		struct glink_addr *paddr,
		struct tms_retotdr_test_hdr   *ptest_hdr,		
		struct tms_retotdr_test_param *ptest_param,

		struct tms_retotdr_data_hdr   *pdata_hdr,
		struct tms_retotdr_data_val   *pdata_val,

		struct tms_retotdr_event_hdr  *pevent_hdr,
		struct tms_retotdr_event_val  *pevent_val,

		struct tms_retotdr_chain      *pchain,
		int32_t cmdID);
int32_t tms_RetOTDRTest(
		int fd,
		struct glink_addr *paddr,
		struct tms_retotdr_test_hdr *ptest_hdr,		
		struct tms_retotdr_test_param *ptest_param,

		struct tms_retotdr_data_hdr *pdata_hdr,
		struct tms_retotdr_data_val *pdata_val,

		struct tms_retotdr_event_hdr *pevent_hdr,
		struct tms_retotdr_event_val *pevent_val,

		struct tms_retotdr_chain    *pchain);
int32_t tms_RetOTDRTestCycle(
		int fd,
		struct glink_addr *paddr,
		struct tms_retotdr_test_hdr   *ptest_hdr,		
		struct tms_retotdr_test_param *ptest_param,

		struct tms_retotdr_data_hdr   *pdata_hdr,
		struct tms_retotdr_data_val   *pdata_val,

		struct tms_retotdr_event_hdr  *pevent_hdr,
		struct tms_retotdr_event_val  *pevent_val,

		struct tms_retotdr_chain      *pchain);

int32_t tms_CfgOTDRRef(
		int fd,
		struct glink_addr *paddr,
		struct tms_otdr_ref_hdr       *pref_hdr,		
		struct tms_retotdr_test_param *ptest_param,

		struct tms_retotdr_data_hdr   *pdata_hdr,
		struct tms_retotdr_data_val   *pdata_val,

		struct tms_retotdr_event_hdr  *pevent_hdr,
		struct tms_retotdr_event_val  *pevent_val,

		struct tms_retotdr_chain      *pchain,
		struct tms_cfg_otdr_ref_val   *pref_date);
int32_t tms_DelAll_TbUnit(
	int     fd, 
	struct glink_addr *paddr);
uint32_t tms_GetDeviceComposition(
		int     fd,
		struct glink_addr *paddr);
int32_t tms_Command(int fd, struct glink_addr *paddr, char *strcmd, int32_t len);
int32_t tms_AdjustTime(
	int fd,
	struct glink_addr *paddr,
	int8_t (*time)[20]);

int32_t tms_GetOPMOP(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame,  
		int32_t slot);
int32_t tms_GetOLPOP(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame,  
		int32_t slot);
int32_t tms_RetOLPOP(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t count, 
		struct tms_any_op_val *list);
int32_t tms_RetOTDRCycle(
		int    fd,
		struct glink_addr *paddr,
		struct tms_retotdr_test_hdr   *ptest_hdr,		
		struct tms_retotdr_test_param *ptest_param,

		struct tms_retotdr_data_hdr   *pdata_hdr,
		struct tms_retotdr_data_val   *pdata_val,

		struct tms_retotdr_event_hdr  *pevent_hdr,
		struct tms_retotdr_event_val  *pevent_val,

		struct tms_retotdr_chain      *pchain);
int32_t tms_RetOTDRCycle_V2(
		int    fd, 
		struct glink_addr *paddr, 
		struct tms_alarm_line_hdr     *palarm,
		struct tms_retotdr_test_hdr   *ptest_hdr, 		
		struct tms_retotdr_test_param *ptest_param, 

		struct tms_retotdr_data_hdr   *pdata_hdr, 
		struct tms_retotdr_data_val   *pdata_val, 

		struct tms_retotdr_event_hdr  *pevent_hdr, 
		struct tms_retotdr_event_val  *pevent_val, 

		struct tms_retotdr_chain      *pchain);
int32_t tms_RetDevProduce(
	int     fd, 
	struct glink_addr *paddr, 
	struct tms_dev_produce *pval);

int32_t tms_TbRoute_Insert(
	int fd,
	struct glink_addr *paddr,
	int count,
	struct tms_route *plist);
int32_t tms_MCU_GetOTDRParam(
	int     fd, 
	struct glink_addr *paddr, 
	int32_t frame, 
	int32_t slot);
int32_t tms_GetDevProduce(
	int     fd, 
	struct glink_addr *paddr, 
	int32_t frame, 
	int32_t slot,
	int32_t type);
uint32_t tms_GetDeviceCompositionRT(
		int     fd, 
		struct glink_addr *paddr);
int32_t tms_DelAll_TbRoute(
	int     fd, 
	struct glink_addr *paddr);
int32_t tms_Insert_TbUnit(
	int fd,
	struct glink_addr *paddr,
	int count,
	struct tms_unit *plist);
int32_t tms_Del_TbUnit(
	int fd,
	struct glink_addr *paddr,
	int count,
	struct tms_unit *plist);

void tms_Print_tms_alarm_line_hdr(struct tms_alarm_line_hdr *pval);
void tms_Print_tms_cfg_sms_val(struct tms_cfg_sms_val *pval, int count);
void tms_Print_tms_cfg_opm_ref_val(struct tms_cfg_opm_ref_val *pref_hdr,int count);
void tms_Print_tms_cfg_olp_ref_val(struct tms_cfg_olp_ref_val *pref_hdr,int count);
void tms_Print_tms_otdr_ref_hdr(struct tms_otdr_ref_hdr *pref_hdr);
void tms_Print_tms_getotdr_test_hdr(struct tms_getotdr_test_hdr *ptest_hdr);
void tms_Print_tms_retotdr_test_hdr(struct tms_retotdr_test_hdr *ptest_hdr);
void tms_Print_tms_retotdr_event(struct tms_retotdr_event_hdr *pevent_hdr, struct tms_retotdr_event_val *pevent_val);
void tms_Print_tms_retotdr_test_param(struct tms_retotdr_test_param *ptest_param);
void tms_Print_tms_retotdr_chain(struct tms_retotdr_chain *pchain);
void tms_Print_tms_cfg_otdr_ref_val(struct tms_cfg_otdr_ref_val *pref_data);
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


int32_t tms_Transmit2Dev(struct tms_context *pcontext, int8_t *pdata, int32_t len);
int32_t tms_Transmit2Manager(struct tms_context *pcontext, int8_t *pdata, int32_t len);

int32_t tms_GetTempFd();
int32_t tms_ClearSMSAuthorization(
	int     fd, 
	struct glink_addr *paddr);
int32_t tms_RemoveByLocation(int32_t frame, int32_t slot);
void tms_AddDev(int32_t frame, int32_t slot, struct tms_devbase *pdev);
int32_t tms_AlarmLine(
		int     fd, 
		struct glink_addr *paddr,
		struct tms_alarm_line_hdr     *palarm,
		struct tms_retotdr_test_hdr   *ptest_hdr, 		
		struct tms_retotdr_test_param *ptest_param, 

		struct tms_retotdr_data_hdr   *pdata_hdr, 
		struct tms_retotdr_data_val   *pdata_val, 

		struct tms_retotdr_event_hdr  *pevent_hdr, 
		struct tms_retotdr_event_val  *pevent_val, 

		struct tms_retotdr_chain      *pchain,
		int32_t cmdID);
int32_t tms_GetCUFd();
int32_t tms_SetCUFd(int fd);
int32_t tms_AnyGetOTDRTest(
		int fd, 
		struct glink_addr *paddr, 
		int32_t frame, 
		int32_t slot, 
		int32_t type, 
		int32_t port, 
		int32_t otdr_port,
		struct tms_getotdr_test_param *val, 
		int32_t cmdID);
void tms_FillGlinkFrame(
		struct glink_base *pbase_hdr, 
		struct glink_addr *paddr);
int32_t tms_SaveOTDRBin(
			struct tms_retotdr_test_hdr   *ptest_hdr, 
			struct tms_retotdr_test_param *ptest_param, 
			struct tms_retotdr_data_hdr   *pdata_hdr, 
			struct tms_retotdr_data_val   *pdata_val, 
			struct tms_retotdr_event_hdr  *pevent_hdr, 
			struct tms_retotdr_event_val  *pevent_val, 
			struct tms_retotdr_chain      *pchain, 
			char *path);
int32_t tms_OpenOTDRBin(
			char **pbuf,
			// struct tms_retotdr_test_hdr   **ptest_hdr, 
			// struct tms_retotdr_test_param **ptest_param, 
			// struct tms_retotdr_data_hdr   **pdata_hdr, 
			// struct tms_retotdr_data_val   **pdata_val, 
			// struct tms_retotdr_event_hdr  **pevent_hdr, 
			// struct tms_retotdr_event_val  **pevent_val, 
			// struct tms_retotdr_chain      **pchain, 
			char *path);
int32_t tms_AlarmHW(
		int     fd, 
		struct glink_addr *paddr, 
		int32_t count, 
		struct  tms_alarm_hw_val *list);
void PrintfMemory(uint8_t *buf, uint32_t len);
void tms_UseEpollServer(struct ep_t *p);
int32_t tms_SelectFdByAddr(uint32_t *paddr);
inline void tms_FillGlinkFrame(
		struct glink_base *pbase_hdr, 
		struct glink_addr *paddr);

uint32_t tms_RetAlarmHWChange(
		int      fd, 
		struct glink_addr *paddr, 
		int32_t total,
		int32_t count,
		struct tms_alarm_hw_change_val *list);
int32_t tms_RetMCUTime(int fd, struct glink_addr *paddr, uint8_t *time);
int32_t tms_RetAlarmSoundState(int fd, struct glink_addr *paddr, uint32_t state);
void tms_Echo(int en);
int32_t tms_SendAllManager(struct glink_base  *pbase_hdr, uint8_t *pdata, int32_t len);
int32_t tms_SendAllManagerDot(struct glink_base  *pbase_hdr, int group, uint8_t *fmt,...);
int32_t tms_CountList(struct tms_man_base *list, int32_t count);
int32_t tms_RetOLPActionLog(int fd, struct glink_addr *paddr, int32_t count, struct tms_olp_action_log_val *pval);
int32_t tms_ManageCount();


// 2016-1-18
int32_t tms_ReportOLPActionEx(
    int fd,
    struct glink_addr *paddr,
    struct tms_report_olp_action *val);

int32_t tms_AlarmOPMEx(
    int fd,
    struct glink_addr *paddr,
    struct tms_alarm_opm      *hdr,
    struct tms_alarm_opm_val  *list);
int32_t tms_RetTotalOPAlarmEx(int fd, struct glink_addr *paddr,
                            struct tms_total_op_alarm_hdr *phdr,
                            struct tms_total_op_alarm_val *pval);
int32_t tms_CfgOLPModeEx(
    int fd,
    struct glink_addr *paddr,
    struct tms_cfg_olp_mode *pval);
// 新增协议
int32_t tms_RetOLPLine(int fd, struct glink_addr *paddr,
                            struct tms_ret_olp_line *pval);
int32_t tms_RetOLPInfo(int fd, struct glink_addr *paddr,
                            struct tms_ret_olp_info *pval);
// end 2016-1-18
int32_t tms_RetTotalOPAlarm(int fd, struct glink_addr *paddr,
                            int32_t type,
                            int32_t count,
                            struct tms_total_op_alarm_val *pval);
int32_t tms_Insert_TbCycTest(
    int fd,
    struct glink_addr *paddr,
    int32_t count,
    struct tms_cyctest *plist);
#ifdef __cplusplus
}
#endif

#endif
