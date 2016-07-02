// Access OTDR using TCP
#ifndef _PROTOCOL_H__
#define _PROTOCOL_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "Otdr.h"

/*************************************************************************************************/
/**************************************** 命令宏定义 *********************************************/
/*************************************************************************************************/
#define CMD_HOST_START_MEASURE      0x10000000      // 配置测试参数并启动测试
#define CMD_HOST_STOP_MEASURE       0x10000001      // 取消或终止测试
#define CMD_HOST_SET_IP             0x10000002      // 设置IP
#define CMD_HOST_GET_BATTERY        0x10000003      // 查询电池电量
#define CMD_HOST_NETWORK_IDLE       0x10000004      // 网络空闲

#define CMD_HOST_SET_RESET_PERIOD	0x1000000A		// 设置空闲状态下的定时复位周期，单位为min

#define CMD_HOST_MEASURE_FIBER      0x1000000F      // 配置测试参数并启动测试，只获取长度和事件点

#define CMD_HOST_GETDATA_UNFILTER   0x10000010      // 读取未经滤波的数据

#define CMD_HOST_GET_VERSION        0x11000000      // 读取版本和编译时间日期

#define CMD_HOST_GET_DELTA          0x1FFFFFFE      // 上位机获取增量函数和阈值函数
#define CMD_HOST_GET_RAWDATA        0x1FFFFFFF      // 上位机获取曲线原始数据测试

#define CMD_OTDR_UPDATE             0x20000000      // OTDR在线升级
#define CMD_GET_IP                  0x30000000      // 上位机UDP搜索IP
#define CMD_DSP_UPLOAD_ALL_DATA     0x90000000      // 上传全部测试数据
#define CMD_DSP_UPLOAD_REF_DATA     0x90000001      // 上传刷新测试数据
#define CMD_DSP_UPLOAD_BATTERY      0x90000002      // 上传电池电量
#define CMD_OTDR_RESET              0x9000000A      // 通知上位机，OTDR准备复位
#define CMD_DSP_UPLOAD_PARAM_DATA   0x9000000F      // 上传测试参数数据，对应 0x1000000F

#define CMD_DSP_UPLOAD_DATA_UNFILTER   0x90000010   // 上传未经滤波的数据 0x10000010

#define CMD_DSP_UPLOAD_VERSION      0x91000000      // 上传版本信息
#define CMD_DSP_HEART_BEAT          0x92000000      // 上传心跳包

#define CMD_DSP_UPLOAD_DELTA        0x9FFFFFFE      // 上传增量函数和阈值函数
#define CMD_DSP_UPLOAD_RAW          0x9FFFFFFF      // 上传原始数据

#define CMD_RESPONSE_STATE          0xA0000000      // 返回状态码
#define CMD_SEND_IP                 0xB0000000      // 上传IP地址

/******************************* 帧格式定义 **************************************/
// 版本号
#define REV_ID      0
#define RSVD_VALUE  0xffffeeee

// 帧类型
#define FRAMETYPE_HOST2TARGET  0   // 主控命令类型
#define FRAMETYPE_TARGET2HOST  1   // 响应命令类型

#define	FRAME_SYNC_STRING	"GLinkOtdr-3800M"

// 帧头标志
typedef struct
{
    char    FrameSync[16];  // GLinkOtdr-3800M
    uint32_t  TotalLength;    // 总帧长
    uint32_t  Rev;            // 版本号
    uint32_t  FrameType;      // 帧类型
    uint32_t  Src;            // 源地址
    uint32_t  Dst;            // 目的地址
    uint32_t  PacketID;       // 流水号
    uint32_t  RSVD;           // 保留
}FrameHeader_t;

// 配置测试参数并启动测试
typedef struct
{
    uint32_t  Cmd;            // 命令码 0x10000000    0x1FFFFFFF
    uint32_t  PacketLength;   // 数据长度
    
    // 控制部分
    struct
    {
        uint32_t  OtdrMode;           // OTDR模式
        uint32_t  OtdrOptMode;        // OTDR优化模式
        uint32_t  RSVD;               // 当获取原始数据时，它标记数据的级别，见 DATA_LEVEL
        uint32_t  EnableRefresh;      // 使能或禁止刷新
        uint32_t  RefreshPeriod_ms;   // 刷新周期，单位ms，不能小于800
    }Ctrl;
    
    // 状态部分
    struct
    {
        uint32_t	Lambda_nm;			// 工作光波长，单位nm
    	uint32_t	MeasureLength_m;	// 测量长度，单位m
    	uint32_t	PulseWidth_ns;		// 光脉冲宽度，单位ns
    	uint32_t	MeasureTime_ms;		// 测量时间，单位ms
    	float   n;                  // 折射率
    	
    	float   EndThreshold;       // 结束门限
    	float   NonRelectThreshold; // 非反射门限
    }State;
    
    uint32_t  RSVD;
}OTDR_MeasureParam_t;

// 取消测试
typedef struct
{
    uint32_t  Cmd;            // 命令码 0x10000001
    uint32_t  PacketLength;   // 数据长度
    
    // 控制方式
    uint32_t  Cancel_Or_Abort;    // 取消或者终止测试
    
    uint32_t  RSVD;
}OTDR_Cancel_t;

// IP地址及子网掩码、默认网关
typedef struct
{
    uint32_t  Cmd;            // 命令码 0x10000002
	uint32_t  PacketLength;   // 数据长度
	
	char    LocalIPAddr[16];
	char    LocalIPMask[16];
	char    GatewayIP[16];

	uint32_t  RSVD;
}OTDR_IP_t;

// OTDR上传全部测试数据
#define	MAX_EVENT_NUM		500
#define RSVD_FLOAT          8192.0      // 浮点数保留值，表明该值无法确定或无需确定
typedef struct
{
	uint32_t  Cmd;            // 命令码 0x90000000
	uint32_t  PacketLength;   // 数据长度
	
	// 测试条件信息
	struct
	{
	    uint32_t	SampleRate_Hz;
		uint32_t	MeasureLength_m;
		uint32_t	PulseWidth_ns;
		uint32_t	Lambda_nm;
		uint32_t	MeasureTime_ms;
		float	n;
		float	FiberLength;	    // 光纤链长
		float	FiberLoss;		    // 链损耗
		float	FiberAttenCoef;	    // 链衰减系数
		
    	float   NonRelectThreshold; // 非反射门限
    	float   EndThreshold;       // 结束门限
    	
    	uint32_t  OtdrMode;           // OTDR模式： 实时、单次
        uint32_t  MeasureMode;        // 测试模式： 自动、手动
	}MeasureParam;
	
	// 测试数据信息
	struct
	{
		uint32_t	DataNum;
		uint16_t	dB_x1000[DATA_LEN];
	}OtdrData;
	
	// 事件点信息单元
	struct
	{
		uint32_t	EventNum;			// 事件点数目
		// 事件点
		struct
		{
			uint32_t	EventXlabel;		// 事件点在上传数据数组中的序号
			uint32_t	EventType;			// 事件点类型
			float	EventReflectLoss;	// 反射损耗 / 回波损耗
			float	EventInsertLoss;    // 插入损耗
			float	AttenCoef;	        // 与下一事件点之间衰减系数
			float	EventTotalLoss;	    // 事件点累计损耗
		}EventPoint[MAX_EVENT_NUM];
	}Event;
	
	uint32_t  RSVD;
}OTDR_UploadAllData_t;

// OTDR上传刷新测试数据
typedef struct
{
	uint32_t  Cmd;            // 命令码 0x90000001
	uint32_t  PacketLength;   // 数据长度
	
	// 测试数据信息
	struct
	{
		uint32_t	DataNum;
		uint16_t	dB_x1000[DATA_LEN];
	}OtdrData;

	uint32_t  RSVD;
}OTDR_UploadRefData_t;

// OTDR上传原始数据
typedef struct
{
	uint32_t  Cmd;            // 命令码 0x9FFFFFFF
	uint32_t  PacketLength;   // 数据长度
	
	// 测试条件信息
	struct
	{
	    uint32_t	SampleRate_Hz;
		uint32_t	MeasureLength_m;
		uint32_t	PulseWidth_ns;
		uint32_t	Lambda_nm;
		uint32_t	MeasureTime_ms;
		float	n;
	}MeasureParam;
	
	// 测试数据信息
	uint32_t	DataNum;
	int32_t	RawData[DATA_LEN];

	uint32_t  RSVD;
}OTDR_UploadRawData_t;

// OTDR上传未经滤波的对数曲线数据
typedef struct
{
	uint32_t  Cmd;            // 命令码 0x90000010
	uint32_t  PacketLength;   // 数据长度
	
	// 测试条件信息
	struct
	{
	    uint32_t	SampleRate_Hz;
		uint32_t	MeasureLength_m;
		uint32_t	PulseWidth_ns;
		uint32_t	Lambda_nm;
		uint32_t	MeasureTime_ms;
		float	n;
	}MeasureParam;
	
	// 测试数据信息
	uint32_t	DataNum;
	uint16_t	dB_x1000[DATA_LEN];

	uint32_t  RSVD;
}OTDR_UploadUnFilterData_t;

// OTDR上传增量函数和阈值函数
typedef struct
{
	uint32_t  Cmd;            // 命令码 0x9FFFFFFE
	uint32_t  PacketLength;   // 数据长度

	// 测试数据信息
	float	t1;
	float   t2;
	int32_t   sigma;
	int32_t	Delta[DATA_LEN];
	int32_t	Threshold[DATA_LEN];

	uint32_t  RSVD;
}OTDR_UploadDelta_t;

// OTDR上传电池电量
typedef struct
{
	uint32_t  Cmd;            // 命令码 0x90000002
	uint32_t  PacketLength;   // 数据长度
	
	uint32_t  Data;

	uint32_t  RSVD;
}OTDR_UploadBattery_t, HostSetIdleResetPeriod_t;

/************************************** OTDR上传版本信息 *****************************************/
typedef struct
{
	uint32_t  Cmd;            // 命令码 0x91000000
	uint32_t  PacketLength;   // 数据长度
	
	// 高8位为主版本号，低8位为次版本号
	uint16_t  Major_Minor;
	
	// 代码编译日期
	uint16_t  BuildYear;
    uint16_t  BuildMon;
    uint16_t  BuildDay;
    
    // 代码编译时间     高16位为小时(0~23)，低16位里的高8位为分钟(0~59)，低8位为秒(0~59)
    uint32_t  BuildTime;
    char    Hardware[16];   // TR600-0126-A     或者    TR600-0126-C
    
	uint32_t  RSVD;
}OTDR_UploadVersion_t;

/************************************** OTDR上传版本信息 *****************************************/
typedef struct
{
	uint32_t  Cmd;            // 命令码 0x92000000
	uint32_t  PacketLength;   // 数据长度
	
	uint32_t  MeasureState;   // 0 : OTDR_MODE_IDLE   1 : OTDR_MODE_AVG   2 : OTDR_MODE_REALTIME
	uint32_t  CpuTime;        // 当前CPU运行时间，ms
	uint32_t  RSVD;
}OTDR_HeartBeat_t;

// OTDR回帧命令数据结构
#define	STATE_CODE_CMD_OK                   0       // 成功处理命令
#define	STATE_CODE_FRAME_SYNC_ERROR         1       // 非法帧起始字符串
#define	STATE_CODE_REV_ERROR                2       // 非法版本号
#define	STATE_CODE_FRAME_TYPE_ERROR         3       // 非法帧类型
#define	STATE_CODE_CMD_ID_ERROR             4       // 非法命令标识
#define	STATE_CODE_PACKET_LENTH_ERROR       5       // 非法数据长度

#define	STATE_CODE_ML_OR_PW_ERROR           16      // 量程或脉宽非法
#define	STATE_CODE_N_ERROR                  17      // 群折射率非法
#define	STATE_CODE_NR_ERROR                 18      // 非反射门限非法
#define STATE_CODE_OTDR_BUSY_MEASURE        19      // 非法请求测试
#define STATE_CODE_IP_ERROR                 20      // 非法IP地址
#define STATE_CODE_NO_UNFILTERDATA          21      // 不存在未滤波的数据

// 在线升级状态码
#define STATE_CODE_FILE_CONTENT_ERROR       100     // 升级文件内容出错
#define STATE_CODE_OTDR_UPDATE_START        101     // 开始升级
#define STATE_CODE_OTDR_UPDATE_FAIL         102     // 升级失败
#define STATE_CODE_OTDR_UPDATE_DONE         103     // 升级完成

typedef struct
{
	uint32_t	Cmd;		    // 命令码 0xA0000000
	uint32_t  PacketLength;   // 数据长度
	uint32_t	StateCode;		// 响应码
	uint32_t  RSVD;
}OTDR_State_t;

// 在线升级文件数据结构开销
// 升级文件的结构为 EncData(n) + DSP_CODE_ID(16) + CodeLen(4) + Checksum(4)
typedef struct
{
    uint32_t  Cmd;            // 命令码 0x20000000
    uint32_t  PacketLength;   // 数据长度
//	uint8_t   content[0];
	uint32_t  CodeLen;
	uint32_t  Checksum;
}OTDR_DspCodeFile_t;

/*************************************************************************************************/
/****************************************** 默认的IP地址 *****************************************/
/*************************************************************************************************/
#define	DEFAULT_IP_ADDR		"192.168.1.249"
#define	DEFAULT_IP_MASK		"255.255.255.0"
#define	DEFAULT_IP_GATEWAY	"192.168.1.1"
#define	DEFAULT_PORT_ADDR	5000

/*************************************************************************************************/
/******************************************** 事件数组 *******************************************/
/*************************************************************************************************/
#define MAX_GROUP_NUM   5
typedef struct
{
    int32_t   ValidGroupNum;      // 有几组事件数组是有效的
    struct
    {
        int32_t   FinalEvents[MAX_EVENT_NUM];
        int32_t   FinalEventsEnd[MAX_EVENT_NUM];
        int32_t   EventType[MAX_EVENT_NUM];
        int32_t   EventNum;
    }GroupEventArray[MAX_GROUP_NUM];
}GroupEvent_t;

// touch measure struct
typedef struct
{
	Uint32  Cmd;            // 命令码 0x1000FFFF
	Uint32  PacketLength;   // 数据长度

	Uint32  RefreshPeriod_ms;   // 刷新周期，单位ms，不能小于800
	struct
	{
		Uint32	Lambda_nm;			// 工作光波长，单位nm
		Uint32	MeasureLength_m;	// 测量长度，单位m
		Uint32	PulseWidth_ns;		// 光脉冲宽度，单位ns
		Uint32	MeasureTime_ms;		// 测量时间，单位ms
		Uint32  R;					// 接收机跨阻
		Uint32  P;					// 激光器功率
		Uint32  ApdV;				// APD电压
	}MeasureParam;
	
	Uint32  RSVD;
}OTDR_TouchMeasureParam_t;

#endif	// _TCP_OTDR_H
