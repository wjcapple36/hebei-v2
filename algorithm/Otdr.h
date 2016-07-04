/*
 **************************************************************************************
 *                        桂林光通电子工程公司
 *
 *  文件描述： OTDR计算头文件。以下是采样点数N和采样频率Fs与量程L和脉冲周期pp的对应关系
 *               N    Fs(MHz)  L(km)   pp(ms)     200ms里单个时钟累加次数
 *             32000    400     5       0.2             200/0.1 / 8
 *             32000    200     10      0.4             200/0.2 / 4
 *             32000    100     30      0.5             200/0.5 / 2
 *             32000    50      60       1              200/1
 *             32000    25      100    200/150          200 / (200/150)
 *             32000    12.5    180    200/75           200 / (200/75)
 *             32000    6.25    300    200/30           200 / (200/30)
 *  文件名  ： Otdr.h
 *  创建者  ： 彭怀敏
 *  创建日期： 2011-3-28 17:20:02
 *  当前版本： v2.0
 *
 **************************************************************************************
*/
#ifndef _OTDR_H__
#define _OTDR_H__

#include <math.h>
#include <time.h>
#include "DspFpgaReg.h"

// define how to get data
#define	EDMA_GET_DATA_TOUCH	1
//2016-07-02 old version by peng, is defined in Otdr.c
#if TR600_A_CLASS
    #undef  MIN_PW_DATA_CONCAT
    #define MIN_PW_DATA_CONCAT     2560
#else
    #undef  MIN_PW_DATA_CONCAT
    #define MIN_PW_DATA_CONCAT     640
#endif
//*************************** data type definition *********************************//
typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int  Uint32;
typedef char Int8;
typedef short Int16;
typedef int Int32;
//**********************************************************************************//
// 定义是否普通OTDR模块，还是定制模块
#define GL3800M_GENERIC     1

// 当前软件版本
#define VERSION_MAJOR       5
#if GL3800M_JIAHUI
    #define PROJECT_NAME    "GL3800M-V5-JH"
    #define VERSION_MINOR   2
#else
    #define PROJECT_NAME        "GL3800M-V5"
    #define VERSION_MINOR   1
#endif

#define TR600_A_CLASS   0
#define TR600_C_CLASS   1

#if ((TR600_A_CLASS + TR600_C_CLASS) != 1)
//#if (TR600_A_CLASS && TR600_C_CLASS) || (!TR600_A_CLASS && !TR600_C_CLASS)
#error "A类仪表和C类仪表有且只能有一类被定义"
#endif


/*
 **************************************************************************************
 *   数据结构定义
 **************************************************************************************
*/
#define	DATA_LEN		32000       // 数据长度
#define	NOISE_LEN		1000        // 噪声长度
#define	CHAN_MASK		0x000FFFFF	// 20 bits data

#define	C				  299792458		// 光速 3e8
#define	TIMES_COUNT_ONCE		200		// 1次命令的累加时间200ms

// 最大脉宽 ns
#define PULSE_WIDTH_5NS     0x5555
#define MAX_PULSE_WIDTH     20480   // 20us
// 最大量程 m
#define MAX_MEASURE_LENTH   180000  // 180km

// 量程数量以及全部脉宽数量
#define MEASURE_LENGTH_NUM  7
#define PULSE_WIDTH_NUM     13
#define LAMBDA_NUM          10

// 数据拼接的脉宽下限
#define MIN_PW_DATA_COMBINE     640    // 5120ns
#define MAX_POWER_LEVEL         4

// OTDR通道数据结构
typedef struct
{
	Int32 ChanData[DATA_LEN];
	float Ai[DATA_LEN];
	Int32 Delta[DATA_LEN];
	Int32 LowPowerData[DATA_LEN];
}OTDR_ChannelData_t;

// 事件点处理数据结构，用于搜索事件点函数FindEventPoints中
#define	EVENT_TYPE_START		0			// 起始事件
#define	EVENT_TYPE_REFLECT		1			// 反射事件
#define	EVENT_TYPE_NONREFLECT	2			// 非反射事件
#define	EVENT_TYPE_END			3			// 结束事件
#define	EVENT_TYPE_FIBER		4			// 光纤事件
#define EVENT_TYPE_PROBABLE     5           // 新增可能的事件
#define EVENT_TYPE_GHOST        90          // 可能的鬼影
#define EVENT_TYPE_FUCK         100         // 让我不爽的事件点
#define	EVENT_TYPE_UNDEF		0xFEDCBA	// 未确定事件，留待以后确定

typedef struct		// 2010-12-8 11:47:39	2010-12-9 17:45:52
{
	Int32	*TotalEvent;		// 指向全部事件点的指针		使用在寻找事件点函数中
	Int32	TotalEventNum;		// 全部事件点的数目			使用在寻找事件点函数中
	Int32	*ReflectEvent;		// 指向反射事件点的指针		使用在寻找事件点函数中
	Int32	ReflectEventNum;	// 反射事件点的数目			使用在寻找事件点函数中
	Int32	*FinalEvent;		// 指向最终事件点的指针		使用在过滤事件点函数中
	Int32	*FinalEventEnd;		// 指向最终事件点末点的指针	使用在过滤事件点函数中
	Int32	FinalEventNum;		// 最终事件点的数目			使用在过滤事件点函数中
	Int32	*EventType;			// 指向最终事件点类型的指针	使用在过滤事件点函数中
	Int32	*SaturateEvent;     // 保存饱和平顶的事件点下标 使用在确定结束事件点函数中
	Int32	SatEventNum;        // 保存饱和平顶的事件点个数 使用在确定结束事件点函数中
}Event_t;

// 最值宏定义
#define	MAX(a, b)	    (((a) > (b)) ? (a) : (b))
#define	MIN(a, b)	    (((a) < (b)) ? (a) : (b))
    
// 32位浮点数转为对应的无符号整数
#define FLOAT2HEX(f)    (*(Uint32 *)&(f))
#define HEX2FLOAT(h)    (*(float  *)&(h))

// 将10为底的指数转换为自然指数
#define Tenth(x)        (exp(2.3025850929940456840179914546844 * (x)))  // 10^x
#define Tenth_div5(x)   (exp(0.4605170185988091368035982909368 * (x)))  // 10^(x/5)
#define DOWN_n_dB(data, ndB)    (data * exp(0.4605170185988091368035982909368 * (-ndB)))    // data * 10^(-ndB/5)
#define UP_n_dB(data, ndB)    	(data * exp(0.4605170185988091368035982909368 * (ndB)))    	// data * 10^(ndB/5)

#define ENLARGE_FACTOR(MeasureTime)    (200*1000 / MAX(MeasureTime, 1000))

// 数据类型定义
#define DATA_TYPE_CHAR      0
#define DATA_TYPE_SHORT     1
#define DATA_TYPE_INT       2
#define DATA_TYPE_LONG      3
#define DATA_TYPE_FLOAT     4
#define DATA_TYPE_DOUBLE    5

// OTDR功率控制表变量类型
typedef const Uint16 (*OtdrPowerLevel_t)[PULSE_WIDTH_NUM];

/*
 **************************************************************************************
 *  OTDR保存的控制参数		2011-12-21 8:18:02
 **************************************************************************************
*/
#define MAX_WL_NUM      10

typedef struct
{
    Uint32  OtdrStartPoint[MEASURE_LENGTH_NUM];     // 为每个量程存储一个曲线起始点值
}OtdrParam_t;

/*
 **************************************************************************************
 *  OTDR控制变量定义		2010-12-28 10:01:17
 **************************************************************************************
*/
typedef struct
{
    // 本次控制变量，开始测试后，不再改变
    Uint32 OtdrMode;           // OTDR工作模式         int
    Uint32 MeasureMode;        // 测试模式             bool
    Uint32 NonReflectThreMode; // 非反射门限设置方式   int
    Uint32 RefreshData;        // 是否需要刷新数据     bool
    Uint32 MeasurePurpose;     // 测试目的，是配置还是监控 int
    Uint32 RawDataLevel;       // 原始数据级别         int
    
    // 全局控制变量，无论何时，都接受来自主机的控制
    Uint32 UserAction;         // 用户动作，继续测试还是取消或中止 int
    
    // 本地测试控制变量，在各个测试中动态修改
    Uint32 FindEvent;          // 是否需要寻找事件点   bool
    Uint32 EnableDeleteJitter;         // 是否使能去除振荡 bool
    Uint32 EnableCapacityCompensate;   // 是否使能电容补偿 bool
    Uint32 EnableCurveConcat;         // 是否使能曲线拼接 bool
    Uint32 EnableAutoPulseWidth;       // 是否使能自动测试时自动匹配脉宽 bool
    Uint32 EnableAutoPower;            // 是否使能拼接时的自动功率控制 bool
    Uint32 CurveConcat;               // 该次测试是否需要数据拼接 bool
    Uint32 Need2RaiseLowPowerCurve;    // 是否需要抬高低功率曲线 bool
    Uint32 LowPowerDataChanged;        // 低功率数据是否被改变 bool
    Uint32 LowPowerDataProcessed;      // 低功率数据是否应该被处理 bool
    
    Uint32 PowerMode;                  // 功率控制模式 int
    
    // 与FPGA命令相关
    float  PulsePeriod_us;     // 脉冲周期             int
    Uint32 AccCountOnce;       // 一次命令的累加数     int
    Uint32 NullReadCount;      // 空读FPGA次数         int
    
    // 状态指示
    Uint32 OtdrDataBusyNow;    // 正在进行数据采集     bool
    Uint32 OtdrDataReadyFlag;  // 数据采集完成标志     int
    Uint32 OtdrAlgoBusyNow;    // 正在进行数据处理     bool
	Uint32 OtdrAlgoReadyFlag;  // 算法处理完成标志     int
	
	Uint32 FpgaTimeOut;        // FPGA通信超时         bool
	Uint32 NetWorkBusyNow;     // 指示网络繁忙         bool
	Uint32 LinkStatus;         // Link指示             int
	
	// 当前无操作时间累计
	Uint32 IdelTime;           // 空闲时间计数         int
	Uint32 ResetWhenSendError; // 当send函数出错时重启 bool
	Uint32 HostConnected;      // 上位机与OTDR是否建立了连接   bool
	Uint32 option;	//wjc 2016-07-04 ch index
}OtdrCtrlVariable_t;

// OTDR工作模式
#define OTDR_MODE_IDLE          0       // 空闲模式
#define OTDR_MODE_AVG           1       // 平均模式
#define OTDR_MODE_REALTIME      2       // 实时模式
#define OTDR_MODE_2LAMBDA       3       // 双波长测试模式
#define OTDR_MODE_RECORRECT     101     // 校正模式
#define OTDR_MODE_CYCLE_TEST    102     // 循环测试模式

// 测试模式
#define MEASURE_MODE_AUTO       0       // 自动测试模式
#define MEASURE_MODE_MANUAL     1       // 手动测试模式

// 非反射门限设置方式
#define NR_MODE_AUTO            0       // 自动设置方式
#define NR_MODE_MANUAL          1       // 手动设置方式

// 用户动作
#define USER_ACTION_NO_ACTION   0       // 无动作，继续测试
#define USER_ACTION_CANCEL      1       // 取消，数据丢弃不处理
#define USER_ACTION_STOP        2       // 终止，处理当前获得的数据

// 测试目的
#define MEASURE_PURPOSE_NONE        0       // 无目的
#define MEASURE_PURPOSE_CFG         1       // 配置目的
#define MEASURE_PURPOSE_WARN        2       // 监控目的
#define MEASURE_PURPOSE_EST         3       // 获取事件点及光纤长度
#define MEASURE_PURPOSE_RECRRECT    100     // 自动校准
#define MEASURE_PURPOSE_OTDR        255     // OTDR算法

// 算法处理完成标志
#define ALGO_READY_FLAG_I_AM_DEAD       0x00000000  // 未开始新的OTDR算法数据处理
#define ALGO_READY_FLAG_START_NEW       0x00000001  // 开始新的OTDR算法数据处理
#define ALGO_READY_FLAG_ONCE_DONE       0x00000002  // 一个刷新周期的数据处理完成
#define ALGO_READY_FLAG_ALL_DONE        0x00000003  // 全部测试时间的数据处理完成

// 事件算法完成标志
#define EVENT_READY_FLAG_I_AM_DEAD      0x00000000  // 未开始新的OTDR事件算法
#define EVENT_READY_FLAG_START_NEW      0x00000001  // 开始新的OTDR事件算法
#define EVENT_READY_FLAG_DONE           0x00000003  // 事件算法完成

// 功率控制模式
#define POWER_MODE_UNDEF                0           // 初始未定义
#define POWER_MODE_LOW                  1           // 使用低功率
#define POWER_MODE_HIGH                 2           // 使用高功率
#define POWER_MODE_COMBINE              3           // 使用混合功率

/*
 **************************************************************************************
 *  OTDR测试参数状态变量定义		2011-3-28 17:42:18
 **************************************************************************************
*/
// 最大平顶个数
#define MAX_SAT_POINTS  16

typedef struct
{
/********************************* 实际的测量参数 ************************************/
    struct
    {
        Uint32  Lambda_nm;          // 波长，单位nm
        Uint32  MeasureLength_m;    // 量程，单位m
    	Uint32	PulseWidth_ns;		// 光脉冲宽度，单位ns
    	Uint32	MeasureTime_ms;		// 测量时间，单位ms
    	float   n;                  // 折射率
    	
    	float   EndThreshold;       // 结束门限
	    float   NonRelectThreshold; // 非反射门限
    }MeasureParam;

/********************************** 曲线饱和平顶 *************************************/
    Uint32  TailLength;             // 平顶拖尾的长度
    struct
    {
        Uint32  SatStart[MAX_SAT_POINTS];
        Uint32  TailStart[MAX_SAT_POINTS];
        Uint32  TailEnd[MAX_SAT_POINTS];
        Uint32  SatNum;
    }OtdrCurveSaturate;

/****************************** 高低功率数据存放地址 *********************************/
    Uint32  TreatAsHighPowerData;   // 是否将数据当作高功率数据对待
    Uint32  PowerLevel;             // 当前功率
    Uint32  AutoPower;              // 自动匹配的功率
    float   HighMinusLow;           // 高低功率曲线相差的dB数

/********************************** 实际测试参数 *************************************/
	Uint32  RealSampleRate_MHz; // 实际采样率，单位MHz
	Uint32  RealSampleRate_Hz;  // 实际采样率，单位Hz

	Uint32  M;                  // 脉宽对应的采样点数
	float   Points_1m;          // 1米对应的采样点数
	float   MinNoiseAmp;        // 最低噪声幅值
	Uint32  SatThreshold;       // 饱和值
	Uint32  sigma;              // 噪声均方根
	Uint32  sigmaLp;            // 低功率曲线噪声均方根
	Uint32  MaxRawData;         // 原始数据的最大值，未去基线

/*********************************** 各个特征点 **************************************/
	Uint32  MeasureLengthPoint;     // 对应量程的点
	Uint32  CurveStartPoint;        // 当前测试曲线的起点
	Uint32  CurveConcatPoint;      // 数据拼接点
	Uint32  HighPowerSatPoint;      // 全功率曲线前端平顶点
    Uint32  SignalEndPoint;         // 所有数据里的信号结束点，应该在滤波前计算
	Uint32  SignalBadPoint;         // 数据负向过冲点

/*********************************** 累加时间量 **************************************/
	Uint32  RefreshPeriod_ms;       // 刷新周期，单位ms，不能小于800
	Uint32  TotalMeasureTime;       // 由于用户可能中止测试，该变量记录实际的测试时间
	Uint32  LowPowerTime;           // 低功率测试时间
	Uint32  HighPowerTime;          // 高功率测试时间

/************************************* 其他量 ****************************************/
	Uint32  RefreshCount;           // 从启动测试以来刷新次数计数
	Uint32  Receiver;               // 接收机
	Uint32  ApdV;                   // APD电压控制
	Uint32  RcvPw;                  // 接收到的脉宽
	Uint32  RcvLambda;              // 接收到的波长
	Uint32  FiberConn;              // 光纤连接状态
	Uint32  EndEventIsReflect;      // 结束事件是否是反射事件
	float   AutoEndLevel;           // 自动测试时，结束点处的曲线dB值
}OtdrStateVariable_t;

/*
 **************************************************************************************
 *  OTDR原始数据级别定义		2012-5-14 14:50:40
 **************************************************************************************
*/
#define DATA_RAW            0       // 最原始的数据，未去基线
#define DATA_ZERO_BASE      1       // 原始数据去基线并放大后
#define DATA_ZERO_MOVE      2       // 原始数据确定起点并前移后
#define DATA_FIR_CMPS       3       // 原始数据数字滤波并电容补偿后
#define DATA_CMB            4       // 原始数据进行拼接后
#define DATA_IIR            5       // 原始数据自适应滤波后
#define DATA_DELTA_LARGE    100     // 增量函数和阈值函数 大事件
#define DATA_DELTA_SMALL    200     // 增量函数和阈值函数 小事件

typedef struct
{
	char ver[32];
	Uint16 otdr_mode;
	Uint16 measure_length;
	Uint16 pulse_width;
	Uint16 measure_time;
	Uint16 done_time;
	Uint8  frame_in;
	Uint8  frame_out;
}otdr_status_t;

#endif  /* _OTDR_H__ */

/*
 **************************************************************************************
 *    End    of    File
 **************************************************************************************
*/
