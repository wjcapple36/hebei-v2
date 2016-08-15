/*
 **************************************************************************************
 *                        ���ֹ�ͨ���ӹ��̹�˾
 *
 *  �ļ������� Otdr�������ļ���ʹ����Otdrģ����
 *
 *  �ļ���  �� OTDR.c
 *  ������  �� ����
 *  �������ڣ� 2011-4-2 16:30:14
 *  ��ǰ�汾�� v1.0
 * 
 ***** �޸ļ�¼ *****
 *  �޸���  �� 
 *  �޸����ڣ� 
 *  ��    ע�� 
 **************************************************************************************
*/
#include <stdio.h>
#include "Otdr.h"
#include "OtdrEdma.h"
#include "prototypes.h"

// ͨ�����ݱ����������������
OTDR_ChannelData_t		OtdrData, OtdrEventData;       // ͨ�����ݱ���
OTDR_UploadAllData_t    MeasureResult;  // �ϴ�ȫ���������ݣ�ˢ������Ҳ����������
OTDR_MeasureParam_t      NetWorkMeasureParam;    // ������Ʊ���

// �¼������
GroupEvent_t            GroupEvent;

// ���Ʊ�������ǰ����״̬����
OtdrCtrlVariable_t      OtdrCtrl;	// Otdr���Ʊ���
OtdrStateVariable_t     OtdrState;  // Otdr״̬����

// ����Ĳ���
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
// �ж�ƴ�ӵ�����
#define CC  ((OtdrState.MeasureParam.PulseWidth_ns >= MIN_PW_DATA_CONCAT) && \
            (OtdrState.MeasureParam.MeasureLength_m >= 100000))
            
/*
 **************************************************************************************************
 *  ����  ���� SetDefaultOtdrParam
 *  ���������� ����Ĭ�ϵ� OtdrParam
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2011-12-21  08:30:35  v1.0
 **************************************************************************************************
*/
static void SetDefaultOtdrParam(void)
{
    int32_t i;

    // ���ʹ��Ԥ��ȷ���Ĺ̶�ֵ�����棬ע������ 300m �� 1km ��һ����
    for(i = 0; i < MEASURE_LENGTH_NUM; i++)
	{
	    OtdrParam.OtdrStartPoint[i] = OtdrStartPoint[i];// 2015-12-22 9:15   + OtdrStartPointOffSet[i];
	}
}

/*
 **************************************************************************************
 *  �������ƣ� OtdrDataInit
 *  ���������� ��ʼ��OtdrCtrl������������2015-12-12������������ʼ�㣬����ʹ���µ��ļ�����ʹ��֮ǰ�����ƫ�ƻ�����ʼ�㶼��Ч
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2011-3-21 17:05:12
 **************************************************************************************
*/
void OtdrDataInit(void)
{
	extern uint32_t IdleResetPeriod_s;
    uint32_t i, fid, temp;
    
/************************************** ��ʼ�� OtdrCtrl ******************************************/
   	memset(&OtdrCtrl, 0, sizeof(OtdrCtrl));
    OtdrCtrl.OtdrMode           = OTDR_MODE_IDLE;
    OtdrCtrl.MeasureMode        = MEASURE_MODE_AUTO;
    OtdrCtrl.NonReflectThreMode = NR_MODE_AUTO;
    OtdrCtrl.RefreshData        = 0;
    OtdrCtrl.FindEvent          = 0;
    OtdrCtrl.UserAction         = USER_ACTION_NO_ACTION;
    OtdrCtrl.EnableDeleteJitter = 1;
    OtdrCtrl.EnableCapacityCompensate = 1;
    OtdrCtrl.EnableCurveConcat = 1;        // Ĭ��ʹ�ܴ�̬ƴ��
    OtdrCtrl.EnableAutoPulseWidth = 0;      // Ĭ��ʹ���Զ�����ʱ�Զ�ƥ������
    OtdrCtrl.EnableAutoPower    = 0;        // Ĭ��ʹ���Զ����ʿ���
    OtdrCtrl.RawDataLevel       = 0xffffffff;
    
    OtdrCtrl.NetWorkBusyNow     = 1;    // ��ʼ�������緱æ
    OtdrCtrl.ResetWhenSendError = 1;    // ������ͳ���������
    OtdrCtrl.HostConnected      = 0;

/************************************* ��ʼ�� OtdrParam ******************************************/
   	memset(&OtdrParam, 0, sizeof(OtdrParam));
    
    // ���ļ� OtdrStartPointOffSet
	memset(OtdrStartPointOffSet, 0, sizeof(OtdrStartPointOffSet));
	
   	// ���ļ� OtdrParam
	SetDefaultOtdrParam();
	for(i = 0; i < MEASURE_LENGTH_NUM; i++)
	{
	    OtdrParam.OtdrStartPoint[i] += OtdrStartPointOffSet[i];
	}
	
	// ���ļ� OtdrLaserMap1550
	for(i = 0; i < 6; i++)  OtdrLaserMap1550[i] = OtdrDefaultLaserMap1550[i];
}

/*
 **************************************************************************************************
 *  ����  ���� AdaptSampleFreq_PulsePeriod
 *  ���������� ���ݹ��˳���ƥ������ʺ���������
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2011-05-30  08:42:13  v1.0
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
    OtdrCtrl.AccCountOnce        = (uint32_t)(1000*TIMES_COUNT_ONCE/OtdrCtrl.PulsePeriod_us / clk_num); // һ��FPGA�����Ӧ���ۼӴ���
    OtdrCtrl.NullReadCount       = 2*clk_num;
}

/*
 **************************************************************************************************
 *  ����  ���� PowerModeInit
 *  ���������� ��ʼ�����ʿ���ģʽ
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2013-01-04  16:17:21  v1.0
 **************************************************************************************************
*/
void PowerModeInit(void)
{
    if(CC)
    {
        if(OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode)
        {
            OtdrCtrl.PowerMode = POWER_MODE_LOW;    // ʵʱģʽ���ʿ���ģʽΪ��
        }
        else    OtdrCtrl.PowerMode = POWER_MODE_UNDEF;  // ����Ϊδ���幦�ʿ���ģʽ
    }
    else        OtdrCtrl.PowerMode = POWER_MODE_HIGH;
}

/*
 **************************************************************************************************
 *  ����  ���� AdaptMeasureLength_PulseWidth
 *  ���������� ���ݹ��˳����Զ�ƥ�����̺�����
 *  ��ڲ����� FiberLen : ���˳���
 *  ���ز����� 
 *  ���ڰ汾�� 2011-05-08  21:18:49  v1.0
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
    
    if(OtdrCtrl.EnableAutoPulseWidth)   // �Զ�ƥ������
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

    if(MEASURE_MODE_AUTO == OtdrCtrl.MeasureMode)       // �Զ�����
    {
        OtdrState.MeasureParam.Lambda_nm = OtdrState.RcvLambda;
        OtdrState.RcvPw     = OtdrState.MeasureParam.PulseWidth_ns;
    }

    // �Զ����²����ʺ���������
    AdaptSampleFreq_PulsePeriod();
    
    // ��ʼ�����ʿ���ģʽ
    PowerModeInit();
}

/*
 **************************************************************************************************
 *  ����  ���� ModifyMeasureLength_PulseWidth
 *  ���������� �������̺�����
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2011-07-06  12:01:42  v1.0
 **************************************************************************************************
*/
void ModifyMeasureLength_PulseWidth(void)
{
    uint32_t i, MeasureLength_m, PulseWidth_ns;
    
    MeasureLength_m = OtdrState.MeasureParam.MeasureLength_m;
    PulseWidth_ns   = OtdrState.MeasureParam.PulseWidth_ns;
    
    // ����������10����Ϊ5������������20����Ϊ10���������Ϊ20�ı���
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

    // ��������������ΪĬ������ѡ��
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
 *  ����  ���� GetReceiverAndPowerLevel
 *  ���������� ��ȡ���ʿ��Ƽ���
 *  ��ڲ����� index       : ��ǰ�Ѿ����ԵĴ���
 *  ���ز����� PowerLevel  : ���صĹ��ʼ���
 *  ���ڰ汾�� 2014-7-4 16:58:21
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
    
    // ������ʱ������ʹ�ô�С���ʲ��Լ���С���ջ�����
    PowerLevel = OtdrPowerLevel[i][j];//MAX_POWER_LEVEL;//
    Rcv = OtdrReceiver[i][j];
    if((OtdrState.MeasureParam.PulseWidth_ns >= MIN_PW_DATA_CONCAT) && OtdrCtrl.EnableCurveConcat)
    {
        rh = (Rcv >> 8) & 0xff;
        rl = (Rcv & 0xff);
        if(rh == 0)        rh = rl;        // ���û�и�8λ�����Ե�8λ����
        
        hp = (PowerLevel >> 8) & 0xff;
        lp = (PowerLevel & 0xff);
        if(hp == 0)        hp = lp;        // ���û�и�8λ�����Ե�8λ����
        
        if(OtdrState.MeasureParam.MeasureLength_m >= 100000)
        {
            // ���ʿ��Ʒ�ʽ�������ʼδ�������ʹ�û�Ͽ��Ʒ�ʽ�������ߵ͹���ʱ��
            // ���ʹ�õ͹���ģʽ���߸߹���ģʽ����ֱ�ʹ�õ͸߹���
            if(POWER_MODE_HIGH == OtdrCtrl.PowerMode)
            {
                PowerLevel = hp;
                Rcv = rh;
            }
            else if(POWER_MODE_LOW == OtdrCtrl.PowerMode)
            {
                PowerLevel = OtdrCtrl.EnableAutoPower ? OtdrState.AutoPower : lp;
                PowerLevel = MAX(PowerLevel, lp);   // 2014-6-20 17:16:19 ��С��lp�������ʱ������õ�����͹���ֵ
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
                    else        // С����ʱ��Ҫ�ֱ��1310��1550�ֿ�����
                    {
                        PowerLevel = OtdrCtrl.EnableAutoPower ? OtdrState.AutoPower : lp;
                        PowerLevel = MAX(PowerLevel, lp);   // 2014-6-20 17:16:19 ��С��lp�������ʱ������õ�����͹���ֵ
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
 *  ����  ���� OtdrStateInit
 *  ���������� ��ʼ��OtdrState�������������������Զ�������ɺ󣬼���ȫȷ��������������
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2012-12-27  15:13:33  v1.0
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
 *  ����  ���� OtdrUpdateParam
 *  ���������� ����OtdrCtrl��OtdrState���������ڿ�ʼ����ǰ���ã���ȷ����������ȷ�ԣ�������
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2011-04-27  10:30:41  v1.0
 **************************************************************************************************
*/
void OtdrUpdateParam(void)
{
    uint32_t temp;
/************************************** ���� OtdrCtrl ********************************************/
    OtdrCtrl.LowPowerDataChanged     = 0;
    OtdrCtrl.LowPowerDataProcessed   = 0;
    OtdrCtrl.RefreshData        = NetWorkMeasureParam.Ctrl.EnableRefresh;
    OtdrCtrl.UserAction         = USER_ACTION_NO_ACTION;
    OtdrCtrl.FindEvent          = 0;
    
    OtdrCtrl.OtdrDataBusyNow    = 0;
    OtdrCtrl.OtdrDataReadyFlag  = DATA_READY_FLAG_I_AM_DEAD;
    OtdrCtrl.FpgaTimeOut        = 0;
    
    OtdrCtrl.PulsePeriod_us     = 1000;             // �����Եĳ�ʼ������Ϊ���滹Ҫ��ʼ��
    OtdrCtrl.AccCountOnce       = TIMES_COUNT_ONCE; // �����Եĳ�ʼ������Ϊ���滹Ҫ��ʼ��
    
    // ģʽȷ��
    if(0 == NetWorkMeasureParam.State.MeasureLength_m)      OtdrCtrl.MeasureMode = MEASURE_MODE_AUTO;
    else                                                    OtdrCtrl.MeasureMode = MEASURE_MODE_MANUAL;
    
    if(0 == NetWorkMeasureParam.State.NonRelectThreshold)   OtdrCtrl.NonReflectThreMode = NR_MODE_AUTO;
    else                                                    OtdrCtrl.NonReflectThreMode = NR_MODE_MANUAL;
    
    // ����OtdrCtrl�Ĺ�����
    if(OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode)      // ʵʱģʽ
    {
        OtdrCtrl.RefreshData = 0;                       // ��ˢ��
    }
    
/************************************** ��ʼ�� OtdrState *****************************************/
   	memset(&GroupEvent, 0, sizeof(GroupEvent));
   	memset(&OtdrState , 0, sizeof(OtdrState) );
   	memcpy(&OtdrState.MeasureParam, &NetWorkMeasureParam.State, sizeof(OtdrState.MeasureParam));
    
    // ��¼��λ���趨�Ĳ���������������
    OtdrState.RcvLambda = OtdrState.MeasureParam.Lambda_nm;
    OtdrState.RcvPw     = OtdrState.MeasureParam.PulseWidth_ns;

    // �������̺�����
    if(MEASURE_MODE_AUTO == OtdrCtrl.MeasureMode)       // �Զ�����
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
    
    // ���½������޺ͷǷ�������
    if(NR_MODE_AUTO == OtdrCtrl.NonReflectThreMode)      // �Զ�
    {
        OtdrState.MeasureParam.EndThreshold = MAX(OtdrState.MeasureParam.EndThreshold, 3);
    }
    else    // �ֶ������Ϸ���
    {
        OtdrState.MeasureParam.EndThreshold = MAX(OtdrState.MeasureParam.EndThreshold, 3);
        OtdrState.MeasureParam.NonRelectThreshold = MAX(OtdrState.MeasureParam.NonRelectThreshold, 0.01);
    }
    
    // ���ݹ��˳���ƥ������ʺ���������
    AdaptSampleFreq_PulsePeriod();
    
    // ����ˢ������
    temp = NetWorkMeasureParam.Ctrl.RefreshPeriod_ms;
    temp = MAX(temp, 1000);
    temp = MIN(temp, 5000);
    temp = (temp + 999) / 1000 * 1000;    // ����ȡ����1000�ı���
    OtdrState.RefreshPeriod_ms = temp;
    
    // ��������ʱ��
    // ����ģʽ�±������ˢ������
    if(OTDR_MODE_AVG == OtdrCtrl.OtdrMode)
    {
        temp = OtdrState.MeasureParam.MeasureTime_ms;
        temp = MIN(temp, 180000);
        temp = MAX(temp, 1000);
        temp = (temp + 999) / 1000 * 1000;    // ����ȡ����1000�ı���
        OtdrState.MeasureParam.MeasureTime_ms = temp;
    }
    // ʵʱģʽ��Ϊˢ������
    else if(OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode)
    {
        OtdrState.MeasureParam.MeasureTime_ms = OtdrState.RefreshPeriod_ms;//1000;
    }
    else if(OTDR_MODE_CYCLE_TEST != OtdrCtrl.OtdrMode)
    {
        OtdrState.MeasureParam.MeasureTime_ms = 15000;
    }

    // ��ʼ�����ʿ���ģʽ
    PowerModeInit();
    OtdrStateInit();
}

/*
 **************************************************************************************************
 *  ����  ���� CheckIfCurveConcat      CheckIfStartEventAlgo       CheckIfFindSmallEvent
 *  ���������� �ж��Ƿ���Ҫƴ��         �ж��Ƿ������¼��㷨        �ж��Ƿ�Ѱ��С�¼���
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2012-12-28  17:10:33  v1.0
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
 *  ����  ���� CheckIfLsaFindEnd                    CheckIfFindProbableEndEvent
 *  ���������� �ж��Ƿ�ʹ����С���˷�Ѱ�ҽ����¼�   �ж��Ƿ�Ѱ�ҿ��ܵĽ����¼�
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2012-12-28  17:10:33  v1.0
 **************************************************************************************************
*/
int CheckIfLsaFindEnd(void)
{
    int LsaFindEnd = 0;
    
    // 1310�����̳���100km��������5120ʱ�ż���
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
    
    // ֻ����������̱Ƚϴ��ʱ�����
    if((OtdrState.MeasureParam.PulseWidth_ns > 2560) && (OtdrState.MeasureParam.MeasureLength_m >= 100000))
    {
        FindEnd = 1;
        
        // ��������Ϊ�����¼�ĩ�㼰���8km�ĳ���֮��
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
 *  �������ƣ� CheckRunParamValid
 *  ���������� ������в����Ϸ���
 *  ��ڲ����� MeasureLength_m : ��������
 *             PulseWidth_ns   : ������
 *  ���ز����� 0               : �Ƿ�
 *             1               : �Ϸ�
 *  ���ڰ汾�� 2012-12-29 11:44:29
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
 *  ����  ���� UseLowPowerMode
 *  ���������� �����������ݣ�����С�������ߴ���
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2011-09-21  15:47:23  v1.0
 **************************************************************************************************
*/
static void UseLowPowerMode(void)
{
    // ��ʹ�ø߹������ݣ����Թ��ʿ���ģʽҪ��ɵ͹���ģʽ
    OtdrCtrl.CurveConcat = 0;
    OtdrCtrl.PowerMode    = POWER_MODE_LOW;
    OtdrState.CurveConcatPoint = 0;
}

/*
 **************************************************************************************************
 *  ����  ���� GetNfirWidth
 *  ���������� ��ȡƽ���˲��Ĵ��ڿ��
 *  ��ڲ����� PowerMode : ����ģʽ���ڵ͹���ʱʹ�ø�ǿ���˲�
 *  ���ز����� m         : �˲����ڿ��
 *  ���ڰ汾�� 2013-01-11  09:13:02  v1.0
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
	
	// �͹���ʹ�ø�ǿ���˲�
//	if(POWER_MODE_LOW == PowerMode)     m *= 2;     // 2013-1-11 9:16:15
	
	return m;
}

/*
 **************************************************************************************************
 *  ����  ���� ProcessRefreshData
 *  ���������� ����ˢ������
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2012-12-28  16:12:59  v1.0
 **************************************************************************************************
*/
#define     DEBUG_LOW_POWER_DATA_ERROR      0
	#if DEBUG_LOW_POWER_DATA_ERROR
	#define LPD_ERROR_1 734
	#define LPD_ERROR_2 9000
	#define LPD_ERROR_THRESHOLD 7.0
	
	int32_t lpdbak1[DATA_LEN], lpdbak2[DATA_LEN], sendlpd = 1;    // ��������͹������ݣ�δ���������
#endif

void ProcessRefreshData(uint32_t RefreshCount)
{
	int32_t   m, i, sigma, ratio;
	int32_t   *An = NULL;
	float   *Ai = NULL;

	printf("\n*********************** OTDR�㷨�������� ***********************\n");
    printf("OtdrState.HighPowerTime = %d, OtdrState.LowPowerTime = %d\n", OtdrState.HighPowerTime, OtdrState.LowPowerTime);
    OtdrState.TotalMeasureTime = OtdrState.HighPowerTime + OtdrState.LowPowerTime;
    
    An              = OtdrData.ChanData;
	Ai              = OtdrData.Ai;
// DATA_RAW
    if(OtdrCtrl.RawDataLevel == DATA_RAW)
    {
//      PutRawData(OtdrData.ChanData);
    }
    
/******************************* ȷ���Ƿ������¼��㷨 **********************************/
//    StartEventAlgo = CheckIfStartEventAlgo();
/********************************** ��������Ԥ���� *************************************/
    ratio = ENLARGE_FACTOR(OtdrState.TotalMeasureTime);
    ratio = MAX(ratio, 1);   // �Ŵ���������Ϊ1

    RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
    EnlargeData(An, DATA_LEN, ratio);
    AdjustCurve(An, DATA_LEN);
    GetSaturateThreshold(An);
    DeleteOsc(An, DATA_LEN, OtdrState.SatThreshold);
//    InterleaveAverage(An, DATA_LEN, 4);
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    
    // ��̫�͵ĸ�ֵȡ����ֵ
    AbsTooLowData(An, DATA_LEN, sigma);
    
/******************************** �����˲�����ݲ��� ***********************************/
	m = GetNfirWidth(OtdrCtrl.PowerMode);   // 2013-1-11 9:17:01
	nfir(An, An, DATA_LEN, m);  // 2011-7-4 11:37:27
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    OtdrState.sigma = sigma;
    printf("Refresh sigma = %d\n", sigma);
    
    CapacityLinearCompensate(An, DATA_LEN, MAX(m, 128), sigma);
    
/************************* �������ߺ�С�������߽���ƴ�� ******************************/
#if DEBUG_LOW_POWER_DATA_ERROR  // �ڴ���͹�������֮ǰ����������������Ϊ��һ������
    memcpy(lpdbak1, OtdrData.LowPowerData, DATA_LEN*4);
#endif
    if(OtdrCtrl.CurveConcat)   OtdrConcatCurve(sigma);
/******************************* �������˥��ֵ ****************************************/
    FastLog10Vector2(&OtdrData, Ai, DATA_LEN, &OtdrState);
    
#if DEBUG_LOW_POWER_DATA_ERROR
    // DEBUG_LOW_POWER_DATA_ERROR 3km and 36.746km
    if((Ai[LPD_ERROR_1] - Ai[LPD_ERROR_2]) < LPD_ERROR_THRESHOLD)     // �����ж���������ʵ�ʹ����й�
    {
        printf("Refresh Low power COLLAPSE!!!\n");
        if(sendlpd)
        {
            sendlpd = 0;
            PutLowPowerRawData(lpdbak2);    // �ڶ��������Ǹ��ϵ����ݣ����ڵ�һ������ոճ���ʱ��
            TSK_sleep(10);                  // ���ǳ���ǰ�ĺ����ݣ������ϴ�һ�Σ���Ϊ�Ա�
        }
        PutLowPowerRawData(lpdbak1);
    }
    memcpy(lpdbak2, lpdbak1, DATA_LEN*4);   // �ڴ���֮��ѵ͹������ݱ�����������Ϊ�ڶ�������
#endif
/******************************** ����������� *****************************************/
    if(OtdrCtrl.OtdrMode != OTDR_MODE_CYCLE_TEST)
    {
        AnalyseMeasureResult(OtdrData.Ai, NULL, NULL, 0);
    }
	
	// for debug pheigenbaum 2016-03-26 12:18
	GetOtdrStatus();
}

/*
 **************************************************************************************************
 *  ����  ���� ProcessFinalData
 *  ���������� �����������ݣ���󲿷��봦��ˢ�����ݵĲ�����ͬ
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2012-12-28  17:22:23  v1.0
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
	Event_t EventTempLarge, EventTempSmall; // ���ڴ���¼�����Ϣ

#if DEBUG_LOW_POWER_DATA_ERROR
    sendlpd = 1;
#endif
	
	// ����ָ��
	An = OtdrData.ChanData;
	Ai = OtdrData.Ai;
	UploadLen = DATA_LEN-NOISE_LEN;

    printf("\n*********************** OTDR�㷨�������� ***********************\n");
    printf("OtdrState.HighPowerTime = %d, OtdrState.LowPowerTime = %d\n", OtdrState.HighPowerTime, OtdrState.LowPowerTime);
    OtdrState.TotalMeasureTime = OtdrState.HighPowerTime + OtdrState.LowPowerTime;
    
    // ����û�����ֹͣ����ܿ��ܻᴦ��ƴ�ӵ�ǰ200msʱ���ڣ�ֻ�е͹������ݣ����԰������ƹ���
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
/********************************** ��������Ԥ���� *************************************/
    RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
// DATA_ZERO_BASE
    if((OtdrCtrl.RawDataLevel == DATA_ZERO_BASE) && OtdrCtrl.FindEvent)
    {
        PutRawData(OtdrData.ChanData);
    }
    
    ratio = ENLARGE_FACTOR(OtdrState.TotalMeasureTime);
    ratio = MAX(ratio, 1);   // �Ŵ���������Ϊ1
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

    // ԭʼ����ȥ�����ߺ������������ֵ
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
	
    // ��̫�͵ĸ�ֵȡ����ֵ
    AbsTooLowData(An, DATA_LEN, sigma);
    
/******************************** �����˲�����ݲ��� ***********************************/
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
/************************* �������ߺ�С�������߽���ƴ�� ******************************/
    if(OtdrCtrl.CurveConcat)   OtdrConcatCurve(sigma);
    
// DATA_CMB     ֻ�����һ���ϴ�
    if((OtdrCtrl.RawDataLevel == DATA_CMB) && OtdrCtrl.FindEvent)
    {
        PutRawData(OtdrData.ChanData);
    }
/****************************** �Զ�Ѱ���¼��� *****************************************/
    if(NR_MODE_AUTO == OtdrCtrl.NonReflectThreMode)
    {
        AllocEventMem(&EventTempLarge);     // Ѱ�Ҵ��¼���
        FindLargeEventsBeforeLog(&OtdrData, DATA_LEN, &EventTempLarge, &OtdrState);
    }
/******************************* �������˥��ֵ ****************************************/
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
    
/******************************** �¼������ *******************************************/
    EventPointsFilter(&OtdrData, &EventTempLarge, "Large", &OtdrState);
    
    // �¼��㻮��
    i = EventTempLarge.FinalEventNum;
    FindSaturateEvents(An, &EventTempLarge, OtdrState.SatThreshold);  // Ѱ�ұ����¼���
    SplitFinalEvents(OtdrData.Ai, DATA_LEN, &EventTempLarge);
    
	FindSaturateEvents(An, &EventTempLarge, OtdrState.SatThreshold);  // Ѱ�ұ����¼���
	RemoveTailNonReflexEventPoint(&OtdrData, &EventTempLarge);
	RemoveLowLevelEventPoint(&OtdrData, &EventTempLarge);
    RemoveConcatPointEventPoint(&OtdrData, &EventTempLarge);
/******************************** ȷ�������¼��� ***************************************/
    FindProbableEndEvent(&OtdrData, &EventTempLarge, &OtdrState);
    FindSaturateEvents(An, &EventTempLarge, OtdrState.SatThreshold);  // Ѱ�ұ����¼���
    FindEndEventPoint(&OtdrData, &EventTempLarge, &OtdrState);
/******************************** ����������� *****************************************/
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
 *  ����  ���� EstimateFiberLen
 *  ���������� ���ݲ��Ե����������ƹ��˳���
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2012-12-28  15:23:19  v1.0
 **************************************************************************************************
*/
float EstimateFiberLen(void)
{
	int32_t   m, i, sigma, *An = OtdrData.ChanData;
	
	OtdrState.M = PulseWidthInSampleNum();
	OtdrState.Points_1m = 2*OtdrState.MeasureParam.n * OtdrState.RealSampleRate_Hz /C;
/********************************** ��������Ԥ���� *************************************/
    RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
    EnlargeData(An, DATA_LEN, 128);
    AdjustCurve(An, DATA_LEN);
    GetSaturateThreshold(An);
/******************************** �����˲�����ݲ��� ***********************************/
	m = GetNfirWidth(OtdrCtrl.PowerMode);   // 2013-1-11 9:17:01
	nfir(An, An, DATA_LEN, m);  // 2011-7-4 11:37:27
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    
    CapacityLinearCompensate(An, DATA_LEN, MAX(m, 128), sigma);
/******************************** �����źŽ����� ***************************************/
	OtdrState.SignalEndPoint = FastEstimateFiber(&OtdrData, DATA_LEN, sigma);
	
	// ���ؽ����㴦��dBֵ
	i = MAX(OtdrState.M, OtdrState.SignalEndPoint - OtdrState.M);
	MeanValue(An, i, OtdrState.SignalEndPoint, &m, DATA_TYPE_INT);
	OtdrState.AutoEndLevel = 5*FastLog10((float)m / sigma);
	return OtdrState.AutoEndLevel;
}

/*
 **************************************************************************************************
 *  ����  ���� EstimateCurveConcat
 *  ���������� ���ݲ��Ե������������Ƿ���Ҫ��������ƴ��
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2012-12-31  14:50:02  v1.0
 **************************************************************************************************
*/
void EstimateCurveConcat(void)
{
	int32_t   m, i, avg, sigma, Cn, accept, FrontFlat = 0, fiberlessthan30km = 0;
	int32_t   *An = NULL;
	uint32_t  PulseWidth_ns;
	float   v, k;

	// ����ָ��
	An = OtdrData.ChanData;

    PulseWidth_ns = OtdrState.MeasureParam.PulseWidth_ns;
/********************************** ��������Ԥ���� *************************************/
    RemoveBaseLine(An, DATA_LEN, NOISE_LEN);
    EnlargeData(An, DATA_LEN, 128);
    AdjustCurve(An, DATA_LEN);
    i = GetSaturateThreshold(An);
    //debug
    if(i == 0)
    {
        printf("NOT found saturation\n");
    }
/******************************** �����˲�����ݲ��� ***********************************/
	m = GetNfirWidth(OtdrCtrl.PowerMode);   // 2013-1-11 9:17:01
	nfir(An, An, DATA_LEN, m);  // 2011-7-4 11:37:27
    sigma = RootMeanSquare(An, DATA_LEN, NOISE_LEN-OtdrState.M);
    OtdrState.sigma = sigma;
    
    CapacityLinearCompensate(An, DATA_LEN, MAX(m, 128), sigma);
/****************************** �жϴ����ź��Ƿ���Ҫƴ�� *****************************/
    // ���ȼ������Ƿ���30km
    {
        int32_t Pos5dB, tmp;
        
        An = OtdrData.ChanData;
        Cn = (int32_t)(OtdrState.Points_1m * 30000);
        MeanValue(An, Cn-20, Cn+20, &avg, DATA_TYPE_INT);
        
        // ��ǰ���Ѱ�ҵ���5dB�ĵ�
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
        
        if((avg < sigma*Tenth_div5(5)) || (Cn > Pos5dB))   // ����5dB
        {
            fiberlessthan30km = 1;
        }
    }
    
    // Ȼ���жϴ������ߵ�ǰ���Ƿ�ƽ��
    {
        int32_t i1, i2, temp, cc = 0;
        
        // �����Ե����ɹ��Ƶ�ǰ�����ä������ת���ɵ���
        Cn = GetPulseWidthSaturateBlindZone(OtdrState.MeasureParam.Lambda_nm, PulseWidth_ns);
        Cn = (int32_t)(OtdrState.Points_1m * Cn);
        
        // ���õ�֮���Ƿ���ڱ���ֵ
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
        
        // ���ǰ��ƽ������Ѱ��ƴ�ӵ�
        if(FrontFlat)
        {
            // ǰ��ƽ����������˳��ȳ���30km������Ҫƴ�ӣ�����ʹ�õ͹��ʣ�ͬʱ�͹���������Ҫ��̧5dB
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
                        // ƽ����ʼ��λ����Cn+5�����������ڵ�16λ�У�ƽ��������λ����i�����������ڸ�16λ��
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
                        OtdrState.CurveConcatPoint = i + (int32_t)(OtdrState.Points_1m * 4000);  // ����ƴ�ӵ㣬�ڵ�ǰ���4���ﴦ
                        OtdrState.CurveConcatPoint = MAX(OtdrState.CurveConcatPoint, i+3*m);  // ���Ҳ��ܶ���3m
                        
                        // �õ㲻��̫Զ
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
        else    // ǰ��δƽ����ʹ�ô���ģʽ
        {
            OtdrCtrl.CurveConcat = 0;
            OtdrCtrl.PowerMode = POWER_MODE_HIGH;
            printf("front not saturate, use high power mode\n");
            return;
        }
    }
/*************************** ��һ�������ƴ�ӵ��Ƿ���� ********************************/
    // ƴ�ӵ㴦���ܴ����¼���
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
            	
            	if(fabs(v) > 0.5)     // �����������̫�󣬳���0.5dB����������¼��㣬������m��
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
    
    // ƴ�ӵ㴦��ǿ�ȱ������ 15dB
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
    
    // ����������������Ҫƴ�ӵ���������������
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
 *  ����  ���� CheckIfFrontSaturate
 *  ���������� ���ݲ��Ե��������ж������Ƿ���ָ��������ƽ��
 *  ��ڲ����� 
 *  ���ز����� 
 *  ���ڰ汾�� 2013-12-6 14:41:35
 **************************************************************************************************
*/
int32_t CheckIfFrontSaturate(uint32_t FrontLen)
{
	int32_t   i, cc, FrontFlat = 0;
	int32_t   *An = NULL;

	// ����ָ��
	An = OtdrData.ChanData;
/********************************** ��������Ԥ���� *************************************/
    AdjustCurve(An, DATA_LEN);
    i = GetSaturateThreshold(An);
    
    if(i == 0)  // δ�ҵ�ƽ�����������߲�ƽ��
    {
        printf("NOT found saturation\n");
    }
    else
    {
/********************************** �ж�����ǰ���Ƿ�ƽ�� *******************************/
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
