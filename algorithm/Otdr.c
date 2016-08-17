/*
 **************************************************************************************
 *                        桂林光通电子工程公司
 *
 *  文件描述： Otdr计算主文件。使用在Otdr模块上
 *
 *  文件名  ： OTDR.c
 *  创建者  ： 彭怀敏
 *  创建日期： 2011-4-2 16:30:14
 *  当前版本： v1.0
 * 
 ***** 修改记录 *****
 *  修改者  ： 
 *  修改日期： 
 *  备    注： 
 **************************************************************************************
*/
#include <stdio.h>
#include "Otdr.h"
#include "OtdrEdma.h"
#include "prototypes.h"

// 通道数据变量及测量结果变量
OTDR_ChannelData_t		OtdrData, OtdrEventData;       // 通道数据变量
OTDR_UploadAllData_t    MeasureResult;  // 上传全部测试数据，刷新数据也被包含在内
OTDR_MeasureParam_t      NetWorkMeasureParam;    // 网络控制变量

// 事件组变量
GroupEvent_t            GroupEvent;

// 控制变量及当前测试状态参数
OtdrCtrlVariable_t      OtdrCtrl;	// Otdr控制变量
OtdrStateVariable_t     OtdrState;  // Otdr状态变量

// 保存的参数
OtdrParam_t     OtdrParam;
uint16_t          OtdrLaserMap1550[6];
int16_t           OtdrStartPointOffSet[MEASURE_LENGTH_NUM];
//2016-07-02 wen defined in Otdr.h
/*
#if TR600_A_CLASS
    #undef  MIN_PW_DATA_CONCAT
    #define MIN_PW_DATA_CONCAT     2560
#else
    #undef  MIN_PW_DATA_CONCAT
    #define MIN_PW_DATA_CONCAT     640
#endif
*/
// 判断拼接的条件
#define CC  ((OtdrState.MeasureParam.PulseWidth_ns >= MIN_PW_DATA_CONCAT) && \
            (OtdrState.MeasureParam.MeasureLength_m >= 100000))
            
/*
 **************************************************************************************************
 *  函数  名： SetDefaultOtdrParam
 *  函数描述： 设置默认的 OtdrParam
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-12-21  08:30:35  v1.0
 **************************************************************************************************
*/
static void SetDefaultOtdrParam(void)
{
    int32_t i;

    // 起点使用预先确定的固定值来代替，注意量程 300m 和 1km 是一样的
    for(i = 0; i < MEASURE_LENGTH_NUM; i++)
	{
	    OtdrParam.OtdrStartPoint[i] = OtdrStartPoint[i];// 2015-12-22 9:15   + OtdrStartPointOffSet[i];
	}
}

/*
 **************************************************************************************
 *  函数名称： OtdrDataInit
 *  函数描述： 初始化OtdrCtrl参数，由于在2015-12-12调整了曲线起始点，所以使用新的文件名，使得之前保存的偏移或者起始点都无效
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-3-21 17:05:12
 **************************************************************************************
*/
void OtdrDataInit(void)
{
	extern uint32_t IdleResetPeriod_s;
    uint32_t i, fid, temp;
    
/************************************** 初始化 OtdrCtrl ******************************************/
   	memset(&OtdrCtrl, 0, sizeof(OtdrCtrl));
    OtdrCtrl.OtdrMode           = OTDR_MODE_IDLE;
    OtdrCtrl.MeasureMode        = MEASURE_MODE_AUTO;
    OtdrCtrl.NonReflectThreMode = NR_MODE_AUTO;
    OtdrCtrl.RefreshData        = 0;
    OtdrCtrl.FindEvent          = 0;
    OtdrCtrl.UserAction         = USER_ACTION_NO_ACTION;
    OtdrCtrl.EnableDeleteJitter = 1;
    OtdrCtrl.EnableCapacityCompensate = 1;
    OtdrCtrl.EnableCurveConcat = 1;        // 默认使能大动态拼接
    OtdrCtrl.EnableAutoPulseWidth = 0;      // 默认使能自动测试时自动匹配脉宽
    OtdrCtrl.EnableAutoPower    = 0;        // 默认使能自动功率控制
    OtdrCtrl.RawDataLevel       = 0xffffffff;
    
    OtdrCtrl.NetWorkBusyNow     = 1;    // 初始化成网络繁忙
    OtdrCtrl.ResetWhenSendError = 1;    // 如果发送出错，则重启
    OtdrCtrl.HostConnected      = 0;

/************************************* 初始化 OtdrParam ******************************************/
   	memset(&OtdrParam, 0, sizeof(OtdrParam));
    
    // 打开文件 OtdrStartPointOffSet
	memset(OtdrStartPointOffSet, 0, sizeof(OtdrStartPointOffSet));
	
   	// 打开文件 OtdrParam
	SetDefaultOtdrParam();
	for(i = 0; i < MEASURE_LENGTH_NUM; i++)
	{
	    OtdrParam.OtdrStartPoint[i] += OtdrStartPointOffSet[i];
	}
	
	// 打开文件 OtdrLaserMap1550
	for(i = 0; i < 6; i++)  OtdrLaserMap1550[i] = OtdrDefaultLaserMap1550[i];
}

/*
 **************************************************************************************************
 *  函数  名： AdaptSampleFreq_PulsePeriod
 *  函数描述： 根据光纤长度匹配采样率和脉冲周期
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-05-30  08:42:13  v1.0
 **************************************************************************************************
*/
void AdaptSampleFreq_PulsePeriod(void)
{
    uint32_t MeasureLength_m = OtdrState.MeasureParam.MeasureLength_m;
    uint32_t index, clk_num;
    
    index   = GetMeasureLengthIndex(MeasureLength_m);
    clk_num = OtdrSampleRate[index] / CLK_50MHz;
    clk_num = MAX(clk_num, 1);
    
    OtdrState.RealSampleRate_MHz = OtdrSampleRate[index];
    OtdrState.RealSampleRate_Hz  = OtdrSampleRateHz[index];
    
    OtdrCtrl.PulsePeriod_us      = OtdrPulsePeriod[index];
    OtdrCtrl.AccCountOnce        = (uint32_t)(1000*TIMES_COUNT_ONCE/OtdrCtrl.PulsePeriod_us / clk_num); // 一次FPGA命令对应的累加次数
    OtdrCtrl.NullReadCount       = 2*clk_num;
}

/*
 **************************************************************************************************
 *  函数  名： PowerModeInit
 *  函数描述： 初始化功率控制模式
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2013-01-04  16:17:21  v1.0
 **************************************************************************************************
*/
void PowerModeInit(void)
{
    if(CC)
    {
        if(OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode)
        {
            OtdrCtrl.PowerMode = POWER_MODE_LOW;    // 实时模式功率控制模式为低
        }
        else    OtdrCtrl.PowerMode = POWER_MODE_UNDEF;  // 其他为未定义功率控制模式
    }
    else        OtdrCtrl.PowerMode = POWER_MODE_HIGH;
}

/*
 **************************************************************************************************
 *  函数  名： AdaptMeasureLength_PulseWidth
 *  函数描述： 根据光纤长度自动匹配量程和脉宽
 *  入口参数： FiberLen : 光纤长度
 *  返回参数： 
 *  日期版本： 2011-05-08  21:18:49  v1.0
 **************************************************************************************************
*/
void AdaptMeasureLength_PulseWidth(uint32_t FiberLen)
{
    uint32_t i, j, PulseWidth_ns;
    float   v = OtdrState.AutoEndLevel;
    
    FiberLen = 1.618*FiberLen;    // 2011-9-16 10:13:26
    for(i = 0; i < MEASURE_LENGTH_NUM-1; i++)
    {
        if(FiberLen <= OtdrMeasureLength[i])    break;
    }
    
    OtdrState.MeasureParam.MeasureLength_m = OtdrMeasureLength[i];
    OtdrState.MeasureParam.PulseWidth_ns   = OtdrDefaultPulseWidth[i];
    
    if(OtdrCtrl.EnableAutoPulseWidth)   // 自动匹配脉宽
    {
        if(v <= 5)          j = 0;
        else if(v <= 9)     j = 1;
        else if(v <= 13)    j = 2;
        else if(v <= 17)    j = 3;
        else if(v <= 21)    j = 4;
        else                j = 5;
        
        PulseWidth_ns = OtdrDefaultPulseWidth_AUTO[i][j];
        OtdrState.MeasureParam.PulseWidth_ns = PulseWidth_ns;
    }

    if(MEASURE_MODE_AUTO == OtdrCtrl.MeasureMode)       // 自动测试
    {
        OtdrState.MeasureParam.Lambda_nm = OtdrState.RcvLambda;
        OtdrState.RcvPw     = OtdrState.MeasureParam.PulseWidth_ns;
    }

    // 自动更新采样率和脉冲周期
    AdaptSampleFreq_PulsePeriod();
    
    // 初始化功率控制模式
    PowerModeInit();
}

/*
 **************************************************************************************************
 *  函数  名： ModifyMeasureLength_PulseWidth
 *  函数描述： 修正量程和脉宽
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-07-06  12:01:42  v1.0
 **************************************************************************************************
*/
void ModifyMeasureLength_PulseWidth(void)
{
    uint32_t i, MeasureLength_m, PulseWidth_ns;
    
    MeasureLength_m = OtdrState.MeasureParam.MeasureLength_m;
    PulseWidth_ns   = OtdrState.MeasureParam.PulseWidth_ns;
    
    // 如果脉宽低于10，则定为5；如果脉宽低于20，则定为10；否则必须为20的倍数
    if(PulseWidth_ns < 10)
    {
        PulseWidth_ns = 5;
    }
    else if(PulseWidth_ns < 20)
    {
        PulseWidth_ns = 10;
    }
    else
    {
        PulseWidth_ns = (PulseWidth_ns / 20) * 20;
    }

    // 把量程向上修正为默认量程选项
    MeasureLength_m = MAX(MeasureLength_m, OtdrMeasureLength[0]);
    MeasureLength_m = MIN(MeasureLength_m, OtdrMeasureLength[MEASURE_LENGTH_NUM-1]);
    for(i = 0; i < MEASURE_LENGTH_NUM; i++)
    {
        if(MeasureLength_m <= OtdrMeasureLength[i])
        {
            MeasureLength_m = OtdrMeasureLength[i];
            break;
        }
    }
    
    OtdrState.MeasureParam.MeasureLength_m = MeasureLength_m;
    OtdrState.MeasureParam.PulseWidth_ns   = PulseWidth_ns;
}

/*
 **************************************************************************************************
 *  函数  名： GetReceiverAndPowerLevel
 *  函数描述： 获取功率控制级别
 *  入口参数： index       : 当前已经测试的次数
 *  返回参数： PowerLevel  : 返回的功率级别
 *  日期版本： 2014-7-4 16:58:21
 **************************************************************************************************
*/
void InitApdV(void)
{
    OtdrState.ApdV = APDV_LOW;
    if(OtdrState.MeasureParam.PulseWidth_ns <= 40)
    {
        OtdrState.ApdV = APDV_HIGH;
    }
}

void GetReceiverAndPowerLevel(uint32_t index)
{
    uint32_t i, j, PowerLevel, hp, lp, Rcv, rh, rl;
    OtdrPowerLevel_t OtdrPowerLevel;
    
    OtdrState.TreatAsHighPowerData = 1;
    i  = GetMeasureLengthIndex(OtdrState.MeasureParam.MeasureLength_m);
    j  = GetPulseWidthIndex(OtdrState.MeasureParam.PulseWidth_ns);
    OtdrPowerLevel = GetPowerLevelIndex(OtdrState.MeasureParam.Lambda_nm);
    
    // 大脉宽时，交错使用大小功率测试及大小接收机测试
    PowerLevel = OtdrPowerLevel[i][j];//MAX_POWER_LEVEL;//
    Rcv = OtdrReceiver[i][j];
    if((OtdrState.MeasureParam.PulseWidth_ns >= MIN_PW_DATA_CONCAT) && OtdrCtrl.EnableCurveConcat)
    {
        rh = (Rcv >> 8) & 0xff;
        rl = (Rcv & 0xff);
        if(rh == 0)        rh = rl;        // 如果没有高8位，则以低8位代替
        
        hp = (PowerLevel >> 8) & 0xff;
        lp = (PowerLevel & 0xff);
        if(hp == 0)        hp = lp;        // 如果没有高8位，则以低8位代替
        
        if(OtdrState.MeasureParam.MeasureLength_m >= 100000)
        {
            // 功率控制方式，如果初始未定义或者使用混合控制方式，则分配高低功率时间
            // 如果使用低功率模式或者高功率模式，则分别使用低高功率
            if(POWER_MODE_HIGH == OtdrCtrl.PowerMode)
            {
                PowerLevel = hp;
                Rcv = rh;
            }
            else if(POWER_MODE_LOW == OtdrCtrl.PowerMode)
            {
                PowerLevel = OtdrCtrl.EnableAutoPower ? OtdrState.AutoPower : lp;
                PowerLevel = MAX(PowerLevel, lp);   // 2014-6-20 17:16:19 最小是lp，即功率表里设置的是最低功率值
                Rcv = rl;
            }
            else /* if((POWER_MODE_UNDEF == OtdrCtrl.PowerMode) || (POWER_MODE_COMBINE == OtdrCtrl.PowerMode)) */
            {
                {
                    if(index % 5)
                    {
                        PowerLevel = hp; // 2012-2-9 9:27:24
                        Rcv = rh;
                    }
                    else        // 小功率时，要分别对1310和1550分开进行
                    {
                        PowerLevel = OtdrCtrl.EnableAutoPower ? OtdrState.AutoPower : lp;
                        PowerLevel = MAX(PowerLevel, lp);   // 2014-6-20 17:16:19 最小是lp，即功率表里设置的是最低功率值
                        Rcv = rl;
                        OtdrState.TreatAsHighPowerData = 0;
                        OtdrCtrl.LowPowerDataChanged   = 1;
                    }
                }
            }
        }
        else /* if(OtdrState.MeasureParam.PulseWidth_ns >= MIN_PW_DATA_CONCAT) */
        {
            PowerLevel = OtdrCtrl.EnableAutoPower ? OtdrState.AutoPower : lp;
        }
    }

    OtdrState.Receiver = Rcv;
    OtdrState.PowerLevel = PowerLevel;
    if(OtdrState.Receiver == R_6)           OtdrState.TailLength = 1000;
    else if(OtdrState.Receiver == R_7)      OtdrState.TailLength = 1500;
    else if(OtdrState.Receiver == R_A)      OtdrState.TailLength = 2000;
    else if(OtdrState.Receiver == R_C)      OtdrState.TailLength = 3000;
    else                                    OtdrState.TailLength = 5000;
    
    OtdrState.Receiver = Rcv;
    OtdrState.PowerLevel = PowerLevel;
    InitApdV();
}

/*
 **************************************************************************************************
 *  函数  名： OtdrStateInit
 *  函数描述： 初始化OtdrState里的相关量，它必须在自动测试完成后，即完全确定量程脉宽后调用
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2012-12-27  15:13:33  v1.0
 **************************************************************************************************
*/
void OtdrStateInit(void)
{
    uint32_t i, MeasureLength_m, PulseWidth_ns;
    
    MeasureLength_m = OtdrState.MeasureParam.MeasureLength_m;
    PulseWidth_ns = OtdrState.MeasureParam.PulseWidth_ns;
    i = GetMeasureLengthIndex(MeasureLength_m);
    
    OtdrState.TreatAsHighPowerData = 1;
    OtdrState.PowerLevel = MAX_POWER_LEVEL;

	OtdrState.M = PulseWidthInSampleNum();
	OtdrState.Points_1m = 2*OtdrState.MeasureParam.n * OtdrState.RealSampleRate_Hz /C;
	if(MeasureLength_m > 60000)
    {
        if(PulseWidth_ns > 10240)           OtdrState.MinNoiseAmp = -8;
        else if(PulseWidth_ns > 5120)       OtdrState.MinNoiseAmp = -7;
        else if(PulseWidth_ns > 2560)       OtdrState.MinNoiseAmp = -6;
        else                                OtdrState.MinNoiseAmp = -5;
    }
    else    OtdrState.MinNoiseAmp = -5;
    
    if(OtdrState.MeasureParam.MeasureLength_m == 100000)   OtdrState.MeasureLengthPoint = DATA_LEN - NOISE_LEN;
    else
    {
        OtdrState.MeasureLengthPoint = (int32_t)(OtdrState.MeasureParam.MeasureLength_m * OtdrState.Points_1m);
        OtdrState.MeasureLengthPoint = MIN(OtdrState.MeasureLengthPoint, DATA_LEN-NOISE_LEN);
    }
	OtdrState.CurveStartPoint = OtdrParam.OtdrStartPoint[i];
}

/*
 **************************************************************************************************
 *  函数  名： OtdrUpdateParam
 *  函数描述： 更新OtdrCtrl和OtdrState参数。它在开始测试前调用，以确保参数的正确性，并复制
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-04-27  10:30:41  v1.0
 **************************************************************************************************
*/
void OtdrUpdateParam(void)
{
    uint32_t temp;
/************************************** 更新 OtdrCtrl ********************************************/
    OtdrCtrl.LowPowerDataChanged     = 0;
    OtdrCtrl.LowPowerDataProcessed   = 0;
    OtdrCtrl.RefreshData        = NetWorkMeasureParam.Ctrl.EnableRefresh;
    OtdrCtrl.UserAction         = USER_ACTION_NO_ACTION;
    OtdrCtrl.FindEvent          = 0;
    
    OtdrCtrl.OtdrDataBusyNow    = 0;
    OtdrCtrl.OtdrDataReadyFlag  = DATA_READY_FLAG_I_AM_DEAD;
    OtdrCtrl.FpgaTimeOut        = 0;
    
    OtdrCtrl.PulsePeriod_us     = 1000;             // 象征性的初始化，因为下面还要初始化
    OtdrCtrl.AccCountOnce       = TIMES_COUNT_ONCE; // 象征性的初始化，因为下面还要初始化
    
    // 模式确定
    if(0 == NetWorkMeasureParam.State.MeasureLength_m)      OtdrCtrl.MeasureMode = MEASURE_MODE_AUTO;
    else                                                    OtdrCtrl.MeasureMode = MEASURE_MODE_MANUAL;
    
    if(0 == NetWorkMeasureParam.State.NonRelectThreshold)   OtdrCtrl.NonReflectThreMode = NR_MODE_AUTO;
    else                                                    OtdrCtrl.NonReflectThreMode = NR_MODE_MANUAL;
    
    // 更新OtdrCtrl的关联量
    if(OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode)      // 实时模式
    {
        OtdrCtrl.RefreshData = 0;                       // 不刷新
    }
    
/************************************** 初始化 OtdrState *****************************************/
   	memset(&GroupEvent, 0, sizeof(GroupEvent));
   	memset(&OtdrState , 0, sizeof(OtdrState) );
   	memcpy(&OtdrState.MeasureParam, &NetWorkMeasureParam.State, sizeof(OtdrState.MeasureParam));
    
    // 记录上位机设定的波长和脉宽作副本
    OtdrState.RcvLambda = OtdrState.MeasureParam.Lambda_nm;
    OtdrState.RcvPw     = OtdrState.MeasureParam.PulseWidth_ns;

    // 更新量程和脉宽
    if(MEASURE_MODE_AUTO == OtdrCtrl.MeasureMode)       // 自动测试
    {
        if((850 == OtdrState.MeasureParam.Lambda_nm) || (1300 == OtdrState.MeasureParam.Lambda_nm))
        {
            OtdrState.MeasureParam.MeasureLength_m = 30000; // 30km
            OtdrState.MeasureParam.PulseWidth_ns   = 640;   // 640ns
        }
        else
        {
            OtdrState.MeasureParam.MeasureLength_m = 180000; // 180km
            OtdrState.MeasureParam.PulseWidth_ns   = 6000;    // 6us
        }
    }
    else
    {
        ModifyMeasureLength_PulseWidth();
    }
    
    // 更新结束门限和非反射门限
    if(NR_MODE_AUTO == OtdrCtrl.NonReflectThreMode)      // 自动
    {
        OtdrState.MeasureParam.EndThreshold = MAX(OtdrState.MeasureParam.EndThreshold, 3);
    }
    else    // 手动，检查合法性
    {
        OtdrState.MeasureParam.EndThreshold = MAX(OtdrState.MeasureParam.EndThreshold, 3);
        OtdrState.MeasureParam.NonRelectThreshold = MAX(OtdrState.MeasureParam.NonRelectThreshold, 0.01);
    }
    
    // 根据光纤长度匹配采样率和脉冲周期
    AdaptSampleFreq_PulsePeriod();
    
    // 调整刷新周期
    temp = NetWorkMeasureParam.Ctrl.RefreshPeriod_ms;
    temp = MAX(temp, 1000);
    temp = MIN(temp, 5000);
    temp = (temp + 999) / 1000 * 1000;    // 向上取整，1000的倍数
    OtdrState.RefreshPeriod_ms = temp;
    
    // 调整测试时间
    // 单次模式下必须大于刷新周期
    if(OTDR_MODE_AVG == OtdrCtrl.OtdrMode)
    {
        temp = OtdrState.MeasureParam.MeasureTime_ms;
        temp = MIN(temp, 180000);
        temp = MAX(temp, 1000);
        temp = (temp + 999) / 1000 * 1000;    // 向上取整，1000的倍数
        OtdrState.MeasureParam.MeasureTime_ms = temp;
    }
    // 实时模式下为刷新周期
    else if(OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode)
    {
        OtdrState.MeasureParam.MeasureTime_ms = OtdrState.RefreshPeriod_ms;//1000;
    }
    else if(OTDR_MODE_CYCLE_TEST != OtdrCtrl.OtdrMode)
    {
        OtdrState.MeasureParam.MeasureTime_ms = 15000;
    }

    // 初始化功率控制模式
    PowerModeInit();
    OtdrStateInit();
}

/*
 **************************************************************************************************
 *  函数  名： CheckIfCurveConcat      CheckIfStartEventAlgo       CheckIfFindSmallEvent
 *  函数描述： 判断是否需要拼接         判断是否启动事件算法        判断是否寻找小事件点
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2012-12-28  17:10:33  v1.0
 **************************************************************************************************
*/
int CheckIfCurveConcat(void)
{
    if((OtdrCtrl.EnableCurveConcat) && (OTDR_MODE_AVG == OtdrCtrl.OtdrMode))
    {
        if(CC)
        {
            OtdrCtrl.CurveConcat = 1;
        }
        else    OtdrCtrl.CurveConcat = 0;
    }
    else        OtdrCtrl.CurveConcat = 0;
    
    return OtdrCtrl.CurveConcat;
}

int CheckIfAutoPower(void)
{
    int AdaptPower = 0;
    
    if((CC) && (OtdrCtrl.EnableAutoPower))
    {
        AdaptPower = 1;
    }
    
    return AdaptPower;
}

/*
 **************************************************************************************************
 *  函数  名： CheckIfLsaFindEnd                    CheckIfFindProbableEndEvent
 *  函数描述： 判断是否使用最小二乘法寻找结束事件   判断是否寻找可能的结束事件
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2012-12-28  17:10:33  v1.0
 **************************************************************************************************
*/
int CheckIfLsaFindEnd(void)
{
    int LsaFindEnd = 0;
    
    // 1310当量程超过100km，脉宽超过5120时才继续
    if(OtdrState.MeasureParam.Lambda_nm == 1310)
    {
        if((OtdrState.MeasureParam.PulseWidth_ns > 2560) && (OtdrState.MeasureParam.MeasureLength_m >= 100000))
        {
            LsaFindEnd = 1;
        }
    }
    
    return LsaFindEnd;
}

int CheckIfFindProbableEndEvent(Event_t* EventTemp)
{
    int i, X1, X2, EventNum, FindEnd = 0;
    float *Ai;
    
    // 只在脉宽和量程比较大的时候才找
    if((OtdrState.MeasureParam.PulseWidth_ns > 2560) && (OtdrState.MeasureParam.MeasureLength_m >= 100000))
    {
        FindEnd = 1;
        
        // 计算区间为结束事件末点及其后8km的长度之内
        Ai = OtdrData.Ai;
        EventNum = EventTemp->FinalEventNum;
        X1 = EventTemp->FinalEventEnd[EventNum-1];
        X2 = X1 + OtdrState.Points_1m * 8000;
        for(i = X1+1; i <= X2; i++)
        {
            if(Ai[i] < 3.5)
            {
                FindEnd = 0;
                break;
            }
        }
    }
    
    return FindEnd;
}

/*
 **************************************************************************************
 *  函数名称： CheckRunParamValid
 *  函数描述： 检查运行参数合法性
 *  入口参数： MeasureLength_m : 测量长度
 *             PulseWidth_ns   : 脉冲宽度
 *  返回参数： 0               : 非法
 *             1               : 合法
 *  日期版本： 2012-12-29 11:44:29
 **************************************************************************************
*/
int32_t CheckRunParamValid(uint32_t MeasureLength_m, Uint32 PulseWidth_ns)
{
    int32_t index, valid = 0;
    
    if((850 == OtdrState.MeasureParam.Lambda_nm) || (1300 == OtdrState.MeasureParam.Lambda_nm))
    {
        if((MeasureLength_m < 32000) && (PulseWidth_ns <= 1280))    valid = 1;
    }
    else
    {
        index = GetMeasureLengthIndex(MeasureLength_m);
        if((PulseWidth_ns >= OtdrMinAllowPulseWidth[index]) && (PulseWidth_ns <= OtdrMaxPulseWidth[index]))    valid = 1;
//        if((PulseWidth_ns >= OtdrMinPulseWidth[index]) && (PulseWidth_ns <= OtdrMaxAllowPulseWidth[index]))              valid = 1;
    }

    return valid;
}

/*
 **************************************************************************************************
 *  函数  名： UseLowPowerMode
 *  函数描述： 丢弃大功率数据，改用小功率曲线代替
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-09-21  15:47:23  v1.0
 **************************************************************************************************
*/
static void UseLowPowerMode(void)
{
    // 不使用高功率数据，所以功率控制模式要变成低功率模式
    OtdrCtrl.CurveConcat = 0;
    OtdrCtrl.PowerMode    = POWER_MODE_LOW;
    OtdrState.CurveConcatPoint = 0;
}

/*
 **************************************************************************************************
 *  函数  名： GetNfirWidth
 *  函数描述： 获取平均滤波的窗口宽度
 *  入口参数： PowerMode : 功率模式，在低功率时使用更强的滤波
 *  返回参数： m         : 滤波窗口宽度
 *  日期版本： 2013-01-11  09:13:02  v1.0
 **************************************************************************************************
*/
uint32_t GetNfirWidth(Uint32 PowerMode)
{
    uint32_t m, PulseWidth_ns;
    
    m = OtdrState.M;
    PulseWidth_ns = OtdrState.MeasureParam.PulseWidth_ns;
    
	if((PulseWidth_ns > 2560) && (PulseWidth_ns <= 10240))      m /= 2;     // 2011-8-23 21:11:11
	if((PulseWidth_ns > 10240) && (PulseWidth_ns <= 20480))     m /= 4;     // 2011-8-23 21:11:13
	if(OtdrState.MeasureParam.MeasureLength_m > 100000)         m *= 2;     // 2012-4-16 11:05:38
	
	// 低功率使用更强的滤波
//	if(POWER_MODE_LOW == PowerMode)     m *= 2;     // 2013-1-11 9:16:15
	
	return m;
}

/*
 **************************************************************************************************
 *  函数  名： ProcessRefreshData
 *  函数描述： 处理刷新数据
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2012-12-28  16:12:59  v1.0
 **************************************************************************************************
*/
#define     DEBUG_LOW_POWER_DATA_ERROR      0
	#if DEBUG_LOW_POWER_DATA_ERROR
	#define LPD_ERROR_1 734
	#define LPD_ERROR_2 9000
	#define LPD_ERROR_THRESHOLD 7.0
	
	int32_t lpdbak1[DATA_LEN], lpdbak2[DATA_LEN], sendlpd = 1;    // 两级缓存低功率数据，未经过处理的
#endif

void ProcessRefreshData(uint32_t RefreshCount)
{
	int32_t   m, i, sigma, ratio;
	int32_t   *An = NULL;
	float   *Ai = NULL;

	printf("\n*********************** OTDR算法处理数据 ***********************\n");
    printf("OtdrState.HighPowerTime = %d, OtdrState.LowPowerTime = %d\n", OtdrState.HighPowerTime, OtdrState.LowPowerTime);
    OtdrState.TotalMeasureTime = OtdrState.HighPowerTime + OtdrState.LowPowerTime;
    
    An              = OtdrData.ChanData;
	Ai              = OtdrData.Ai;
// DATA_RAW
    if(OtdrCtrl.RawDataLevel == DATA_RAW)
    {
//      PutRawData(OtdrData.ChanData);
    }
    
/******************************* 确定是否启动事件算法 **********************************/
//    StartEventAlgo = CheckIfStartEventAlgo();
/********************************** 曲线数据预处理 *************************************/
    ratio = ENLARGE_FACTOR(OtdrState.TotalMeasureTime);
    ratio = MAX(ratio, 1);   // 放大因子至少为1

    RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
    EnlargeData(An, DATA_LEN, ratio);
    AdjustCurve(An, DATA_LEN);
    GetSaturateThreshold(An);
    DeleteOsc(An, DATA_LEN, OtdrState.SatThreshold);
//    InterleaveAverage(An, DATA_LEN, 4);
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    
    // 将太低的负值取绝对值
    AbsTooLowData(An, DATA_LEN, sigma);
    
/******************************** 数字滤波与电容补偿 ***********************************/
	m = GetNfirWidth(OtdrCtrl.PowerMode);   // 2013-1-11 9:17:01
	nfir(An, An, DATA_LEN, m);  // 2011-7-4 11:37:27
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    OtdrState.sigma = sigma;
    printf("Refresh sigma = %d\n", sigma);
    
    CapacityLinearCompensate(An, DATA_LEN, MAX(m, 128), sigma);
    
/************************* 大功率曲线和小功率曲线进行拼接 ******************************/
#if DEBUG_LOW_POWER_DATA_ERROR  // 在处理低功率数据之前把它保存起来，是为第一级缓存
    memcpy(lpdbak1, OtdrData.LowPowerData, DATA_LEN*4);
#endif
    if(OtdrCtrl.CurveConcat)   OtdrConcatCurve(sigma);
/******************************* 计算对数衰减值 ****************************************/
    FastLog10Vector2(&OtdrData, Ai, DATA_LEN, &OtdrState);
    
#if DEBUG_LOW_POWER_DATA_ERROR
    // DEBUG_LOW_POWER_DATA_ERROR 3km and 36.746km
    if((Ai[LPD_ERROR_1] - Ai[LPD_ERROR_2]) < LPD_ERROR_THRESHOLD)     // 出错判定条件，与实际光纤有关
    {
        printf("Refresh Low power COLLAPSE!!!\n");
        if(sendlpd)
        {
            sendlpd = 0;
            PutLowPowerRawData(lpdbak2);    // 第二级缓存是更老的数据，对于第一级缓存刚刚出错时，
            TSK_sleep(10);                  // 它是出错前的好数据，所以上传一次，作为对比
        }
        PutLowPowerRawData(lpdbak1);
    }
    memcpy(lpdbak2, lpdbak1, DATA_LEN*4);   // 在处理之后把低功率数据保存起来，是为第二级缓存
#endif
/******************************** 分析测量结果 *****************************************/
    if(OtdrCtrl.OtdrMode != OTDR_MODE_CYCLE_TEST)
    {
        AnalyseMeasureResult(OtdrData.Ai, NULL, NULL, 0);
    }
	
	// for debug pheigenbaum 2016-03-26 12:18
	GetOtdrStatus();
}

/*
 **************************************************************************************************
 *  函数  名： ProcessFinalData
 *  函数描述： 处理最终数据，其大部分与处理刷新数据的步骤相同
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2012-12-28  17:22:23  v1.0
 **************************************************************************************************
*/
extern int32_t get_curv_start_point(int32_t Sigma,
		OTDR_ChannelData_t *pOtdrData,
	       	OtdrCtrlVariable_t *pOtdrCtrl,
		OtdrStateVariable_t *pOtdrState);

void ProcessFinalData(uint32_t AlgoPurpose)
{
	int32_t   m, i, sigma, FindSmallEvent = 0;
	int32_t   ratio, UploadLen;
	int32_t   *An = NULL;
	float   *Ai = NULL;
	Event_t EventTempLarge, EventTempSmall; // 用于存放事件点信息

#if DEBUG_LOW_POWER_DATA_ERROR
    sendlpd = 1;
#endif
	
	// 分配指针
	An = OtdrData.ChanData;
	Ai = OtdrData.Ai;
	UploadLen = DATA_LEN-NOISE_LEN;

    printf("\n*********************** OTDR算法处理数据 ***********************\n");
    printf("OtdrState.HighPowerTime = %d, OtdrState.LowPowerTime = %d\n", OtdrState.HighPowerTime, OtdrState.LowPowerTime);
    OtdrState.TotalMeasureTime = OtdrState.HighPowerTime + OtdrState.LowPowerTime;
    
    // 如果用户快速停止，则很可能会处于拼接的前200ms时间内，只有低功率数据，所以把它复制过来
    if(OtdrState.HighPowerTime == 0)
    {
        memcpy(An, OtdrData.LowPowerData, DATA_LEN * sizeof(int32_t));
        OtdrCtrl.CurveConcat = 0;
        OtdrState.CurveConcatPoint = 0;
    }
    
// DATA_RAW
    if(OtdrCtrl.RawDataLevel == DATA_RAW)
    {
        PutRawData(OtdrData.ChanData);
    }

    //FindSmallEvent = CheckIfFindSmallEvent();
/********************************** 曲线数据预处理 *************************************/
    RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
// DATA_ZERO_BASE
    if((OtdrCtrl.RawDataLevel == DATA_ZERO_BASE) && OtdrCtrl.FindEvent)
    {
        PutRawData(OtdrData.ChanData);
    }
    
    ratio = ENLARGE_FACTOR(OtdrState.TotalMeasureTime);
    ratio = MAX(ratio, 1);   // 放大因子至少为1
    EnlargeData(An, DATA_LEN, ratio);
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    OtdrState.CurveStartPoint = get_curv_start_point(sigma, OtdrData.ChanData, &OtdrCtrl, &OtdrState);
    AdjustCurve(An, DATA_LEN);
// DATA_ZERO_MOVE
    if((OtdrCtrl.RawDataLevel == DATA_ZERO_MOVE) && OtdrCtrl.FindEvent)
    {
        PutRawData(OtdrData.ChanData);
    }
    
    if(GetSaturateThreshold(An))
    {
        DeleteOsc(An, DATA_LEN, OtdrState.SatThreshold);
    }
//    InterleaveAverage(An, DATA_LEN, 4);
    GetCurveSaturatePoint(&OtdrData, DATA_LEN);

    // 原始数据去除基线后的噪声均方根值
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
	
    // 将太低的负值取绝对值
    AbsTooLowData(An, DATA_LEN, sigma);
    
/******************************** 数字滤波与电容补偿 ***********************************/
	m = GetNfirWidth(OtdrCtrl.PowerMode);   // 2013-1-11 9:17:01
	nfir(An, An, DATA_LEN, m);  // 2011-7-4 11:37:27
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    OtdrState.sigma = sigma;
    
    CapacityLinearCompensate(An, DATA_LEN, MAX(m, 128), sigma);

// DATA_FIR_CMPS
    if((OtdrCtrl.RawDataLevel == DATA_FIR_CMPS) && OtdrCtrl.FindEvent)
    {
        PutRawData(OtdrData.ChanData);
    }
/************************* 大功率曲线和小功率曲线进行拼接 ******************************/
    if(OtdrCtrl.CurveConcat)   OtdrConcatCurve(sigma);
    
// DATA_CMB     只在最后一次上传
    if((OtdrCtrl.RawDataLevel == DATA_CMB) && OtdrCtrl.FindEvent)
    {
        PutRawData(OtdrData.ChanData);
    }
/****************************** 自动寻找事件点 *****************************************/
    if(NR_MODE_AUTO == OtdrCtrl.NonReflectThreMode)
    {
        AllocEventMem(&EventTempLarge);     // 寻找大事件点
        FindLargeEventsBeforeLog(&OtdrData, DATA_LEN, &EventTempLarge, &OtdrState);
    }
/******************************* 计算对数衰减值 ****************************************/
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    OtdrState.sigma = sigma;
    printf("Final sigma = %d\n", sigma);
    FastLog10Vector2(&OtdrData, Ai, UploadLen, &OtdrState);

#if DEBUG_LOW_POWER_DATA_ERROR
    if((Ai[LPD_ERROR_1] - Ai[LPD_ERROR_2]) < LPD_ERROR_THRESHOLD)
    {
        printf("Low power COLLAPSE!!!\n");
        printf("Final Low power COLLAPSE!!!\n");
    }
#endif
    
/******************************** 事件点过滤 *******************************************/
    EventPointsFilter(&OtdrData, &EventTempLarge, "Large", &OtdrState);
    
    // 事件点划分
    i = EventTempLarge.FinalEventNum;
    FindSaturateEvents(An, &EventTempLarge, OtdrState.SatThreshold);  // 寻找饱和事件点
    SplitFinalEvents(OtdrData.Ai, DATA_LEN, &EventTempLarge);
    
	FindSaturateEvents(An, &EventTempLarge, OtdrState.SatThreshold);  // 寻找饱和事件点
	RemoveTailNonReflexEventPoint(&OtdrData, &EventTempLarge);
	RemoveLowLevelEventPoint(&OtdrData, &EventTempLarge);
    RemoveConcatPointEventPoint(&OtdrData, &EventTempLarge);
/******************************** 确定结束事件点 ***************************************/
    FindProbableEndEvent(&OtdrData, &EventTempLarge, &OtdrState);
    FindSaturateEvents(An, &EventTempLarge, OtdrState.SatThreshold);  // 寻找饱和事件点
    FindEndEventPoint(&OtdrData, &EventTempLarge, &OtdrState);
/******************************** 分析测量结果 *****************************************/
    if((MEASURE_PURPOSE_OTDR == AlgoPurpose) && (OtdrCtrl.OtdrMode != OTDR_MODE_CYCLE_TEST))
    {
        AnalyseMeasureResult(Ai, &EventTempLarge, (float*)OtdrData.Delta, 1);
    }

    FreeEventMem(&EventTempLarge);
	
	// for debug pheigenbaum 2016-03-26 12:18
	GetOtdrStatus();
}

/*
 **************************************************************************************************
 *  函数  名： EstimateFiberLen
 *  函数描述： 根据测试的数据来估计光纤长度
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2012-12-28  15:23:19  v1.0
 **************************************************************************************************
*/
float EstimateFiberLen(void)
{
	int32_t   m, i, sigma, *An = OtdrData.ChanData;
	
	OtdrState.M = PulseWidthInSampleNum();
	OtdrState.Points_1m = 2*OtdrState.MeasureParam.n * OtdrState.RealSampleRate_Hz /C;
/********************************** 曲线数据预处理 *************************************/
    RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
    EnlargeData(An, DATA_LEN, 128);
    AdjustCurve(An, DATA_LEN);
    GetSaturateThreshold(An);
/******************************** 数字滤波与电容补偿 ***********************************/
	m = GetNfirWidth(OtdrCtrl.PowerMode);   // 2013-1-11 9:17:01
	nfir(An, An, DATA_LEN, m);  // 2011-7-4 11:37:27
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    
    CapacityLinearCompensate(An, DATA_LEN, MAX(m, 128), sigma);
/******************************** 计算信号结束点 ***************************************/
	OtdrState.SignalEndPoint = FastEstimateFiber(&OtdrData, DATA_LEN, sigma);
	
	// 返回结束点处的dB值
	i = MAX(OtdrState.M, OtdrState.SignalEndPoint - OtdrState.M);
	MeanValue(An, i, OtdrState.SignalEndPoint, &m, DATA_TYPE_INT);
	OtdrState.AutoEndLevel = 5*FastLog10((float)m / sigma);
	return OtdrState.AutoEndLevel;
}

/*
 **************************************************************************************************
 *  函数  名： EstimateCurveConcat
 *  函数描述： 根据测试的数据来估计是否需要进行曲线拼接
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2012-12-31  14:50:02  v1.0
 **************************************************************************************************
*/
void EstimateCurveConcat(void)
{
	int32_t   m, i, avg, sigma, Cn, accept, FrontFlat = 0, fiberlessthan30km = 0;
	int32_t   *An = NULL;
	uint32_t  PulseWidth_ns;
	float   v, k;

	// 分配指针
	An = OtdrData.ChanData;

    PulseWidth_ns = OtdrState.MeasureParam.PulseWidth_ns;
/********************************** 曲线数据预处理 *************************************/
    RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
    EnlargeData(An, DATA_LEN, 128);
    AdjustCurve(An, DATA_LEN);
    i = GetSaturateThreshold(An);
    //debug
    if(i == 0)
    {
        printf("NOT found saturation\n");
    }
/******************************** 数字滤波与电容补偿 ***********************************/
	m = GetNfirWidth(OtdrCtrl.PowerMode);   // 2013-1-11 9:17:01
	nfir(An, An, DATA_LEN, m);  // 2011-7-4 11:37:27
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    OtdrState.sigma = sigma;
    
    CapacityLinearCompensate(An, DATA_LEN, MAX(m, 128), sigma);
/****************************** 判断大功率信号是否需要拼接 *****************************/
    // 首先检测光纤是否长于30km
    {
        int32_t Pos5dB, tmp;
        
        An = OtdrData.ChanData;
        Cn = (int32_t)(OtdrState.Points_1m * 30000);
        MeanValue(An, Cn-20, Cn+20, &avg, DATA_TYPE_INT);
        
        // 从前向后寻找低于5dB的点
        Pos5dB = DATA_LEN;
        tmp = Tenth_div5(6.0) * sigma;
        for(i = OtdrState.M; i < DATA_LEN; i++)
        {
            if(An[i] <= tmp)
            {
                Pos5dB = i;
                break;
            }
        }
        
        if((avg < sigma*Tenth_div5(5)) || (Cn > Pos5dB))   // 高于5dB
        {
            fiberlessthan30km = 1;
        }
    }
    
    // 然后判断大功率曲线的前端是否平顶
    {
        int32_t i1, i2, temp, cc = 0;
        
        // 按线性递增律估计当前脉宽的盲区，并转换成点数
        Cn = GetPulseWidthSaturateBlindZone(OtdrState.MeasureParam.Lambda_nm, PulseWidth_ns);
        Cn = (int32_t)(OtdrState.Points_1m * Cn);
        
        // 看该点之后是否存在饱和值
        An = OtdrData.ChanData;
        temp = OtdrState.SatThreshold;
        cc = 0;
        for(i = Cn+5; i < Cn+200; i++)
        {
            if(An[i] >= temp)
            {
                if(++cc > 10)
                {
                    FrontFlat = 1;
                    break;
                }
            }
        }
        
        // 如果前端平顶，则寻找拼接点
        if(FrontFlat)
        {
            // 前端平顶，如果光纤长度长于30km，则需要拼接，否则使用低功率，同时低功率曲线需要上抬5dB
            if(fiberlessthan30km)
            {
                UseLowPowerMode();
                printf("front saturate and fiberlessthan30km, use low power mode\n");
                return;
            }
            else
            {
                m = PulseWidthInSampleNum();
                for(i = Cn+10; i < DATA_LEN; i++)
                {
                    if(An[i] < OtdrState.SatThreshold)
                    {
                        // 平顶开始的位置是Cn+5，把它保存在低16位中，平顶结束的位置是i，把它保存在高16位中
                        i1 = Cn+5;
						i2 = i;
						OtdrState.HighPowerSatPoint = (i2 << 16) + i1;
						if(i2 < i1+10)	// ERROR
						{
							printf("*** i2 < i1+10 *** in EstimateCurveConcat!!!");
							if(NetWorkMeasureParam.Ctrl.OtdrOptMode == 0x12345678)
							{
								printf(" And Putting the hpd Data!\n");
								PutRawData(An);
							}else{printf("\n");}
						}
                        OtdrState.CurveConcatPoint = i + (int32_t)(OtdrState.Points_1m * 4000);  // 记下拼接点，在当前点后4公里处
                        OtdrState.CurveConcatPoint = MAX(OtdrState.CurveConcatPoint, i+3*m);  // 而且不能短于3m
                        
                        // 该点不能太远
                        if((OtdrState.CurveConcatPoint >= OtdrState.MeasureLengthPoint*8/10) ||
                           (OtdrState.CurveConcatPoint >= DATA_LEN - 6*m))
                        {
                            UseLowPowerMode();
                            printf("concatenate point is too far away, use low power mode\n");
                            return;
                        }
                        break;
                    }
                }
            }
        }
        else    // 前端未平顶，使用大功率模式
        {
            OtdrCtrl.CurveConcat = 0;
            OtdrCtrl.PowerMode = POWER_MODE_HIGH;
            printf("front not saturate, use high power mode\n");
            return;
        }
    }
/*************************** 进一步考察该拼接点是否合理 ********************************/
    // 拼接点处不能存在事件点
    if(OtdrCtrl.CurveConcat)
    {
        An = OtdrData.ChanData;
        Cn = OtdrState.CurveConcatPoint;
        GetEventFindingParamBeforeLog(NULL, NULL, &k, &m, "Large", 0);
        k = k*m;
        while((Cn < OtdrState.MeasureLengthPoint*8/10) && (Cn < DATA_LEN - 6*m))
        {
            accept = 1;
            for(i = Cn-m/2; i < Cn+m/2; i++)
            {
                if((An[i] < sigma) || (An[i+m] < sigma))
                {
                    OtdrCtrl.CurveConcat = 0;
                    accept                = -1;
                    break;
                }
                v = (float)An[i] / An[i+m];
            	v = 5* FastLog10(v) - k;
            	
            	if(fabs(v) > 0.5)     // 如果增量函数太大，超过0.5dB，则表明有事件点，往后走m点
            	{
            	    accept = 0;
            	    break;
            	}
            }
            if(1 == accept)
            {
                OtdrState.CurveConcatPoint = Cn;
                break;
            }
            else if(-1 == accept)
            {
                UseLowPowerMode();
                printf("can't find a proper connection point, use low power mode\n");
                return;
            }
            else    Cn += m;
        }
    }
    
    // 拼接点处的强度必须高于 15dB
    if(OtdrCtrl.CurveConcat)
    {
    #if TR600_A_CLASS
        float mindB = 7;
    #else   // TR600_C_CLASS
        float mindB = 15;
    #endif
        MeanValue(An, OtdrState.CurveConcatPoint-m, OtdrState.CurveConcatPoint+m, &avg, DATA_TYPE_INT);
        if(avg < sigma*Tenth_div5(mindB))
        {
            UseLowPowerMode();     // 2012-12-12 10:19:42
            printf("concatenate point lower than 15dB, use low power mode\n");
            return;
        }
    }
    
    // 如果到达这里，表明需要拼接的所有条件都满足
    if(OtdrCtrl.CurveConcat)
    {
//        OtdrState.CurveConcatPoint = 6200; // debug
        OtdrCtrl.PowerMode = POWER_MODE_COMBINE;
        printf("concatenate point = %d(%.3fkm)\n", OtdrState.CurveConcatPoint,
                       (float)OtdrState.CurveConcatPoint/OtdrState.Points_1m/1000);
    }
}

/*
 **************************************************************************************************
 *  函数  名： CheckIfFrontSaturate
 *  函数描述： 根据测试的数据来判断曲线是否在指定长度内平顶
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2013-12-6 14:41:35
 **************************************************************************************************
*/
int32_t CheckIfFrontSaturate(uint32_t FrontLen)
{
	int32_t   i, cc, FrontFlat = 0;
	int32_t   *An = NULL;

	// 分配指针
	An = OtdrData.ChanData;
/********************************** 曲线数据预处理 *************************************/
    AdjustCurve(An, DATA_LEN);
    i = GetSaturateThreshold(An);
    
    if(i == 0)  // 未找到平顶，表明曲线不平顶
    {
        printf("NOT found saturation\n");
    }
    else
    {
/********************************** 判断曲线前端是否平顶 *******************************/
        cc = 0;
        for(i = 0; i < FrontLen; i++)
        {
            if(An[i] >= OtdrState.SatThreshold)
            {
                if(++cc > 10)
                {
                    FrontFlat = 1;
                    break;
                }
            }
        }
    }
    return FrontFlat;
}

/*
 **************************************************************************************
 *    End    of    File
 **************************************************************************************
*/
