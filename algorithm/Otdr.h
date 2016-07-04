/*
 **************************************************************************************
 *                        ���ֹ�ͨ���ӹ��̹�˾
 *
 *  �ļ������� OTDR����ͷ�ļ��������ǲ�������N�Ͳ���Ƶ��Fs������L����������pp�Ķ�Ӧ��ϵ
 *               N    Fs(MHz)  L(km)   pp(ms)     200ms�ﵥ��ʱ���ۼӴ���
 *             32000    400     5       0.2             200/0.1 / 8
 *             32000    200     10      0.4             200/0.2 / 4
 *             32000    100     30      0.5             200/0.5 / 2
 *             32000    50      60       1              200/1
 *             32000    25      100    200/150          200 / (200/150)
 *             32000    12.5    180    200/75           200 / (200/75)
 *             32000    6.25    300    200/30           200 / (200/30)
 *  �ļ���  �� Otdr.h
 *  ������  �� ����
 *  �������ڣ� 2011-3-28 17:20:02
 *  ��ǰ�汾�� v2.0
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
// �����Ƿ���ͨOTDRģ�飬���Ƕ���ģ��
#define GL3800M_GENERIC     1

// ��ǰ����汾
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
#error "A���Ǳ��C���Ǳ�����ֻ����һ�౻����"
#endif


/*
 **************************************************************************************
 *   ���ݽṹ����
 **************************************************************************************
*/
#define	DATA_LEN		32000       // ���ݳ���
#define	NOISE_LEN		1000        // ��������
#define	CHAN_MASK		0x000FFFFF	// 20 bits data

#define	C				  299792458		// ���� 3e8
#define	TIMES_COUNT_ONCE		200		// 1��������ۼ�ʱ��200ms

// ������� ns
#define PULSE_WIDTH_5NS     0x5555
#define MAX_PULSE_WIDTH     20480   // 20us
// ������� m
#define MAX_MEASURE_LENTH   180000  // 180km

// ���������Լ�ȫ����������
#define MEASURE_LENGTH_NUM  7
#define PULSE_WIDTH_NUM     13
#define LAMBDA_NUM          10

// ����ƴ�ӵ���������
#define MIN_PW_DATA_COMBINE     640    // 5120ns
#define MAX_POWER_LEVEL         4

// OTDRͨ�����ݽṹ
typedef struct
{
	Int32 ChanData[DATA_LEN];
	float Ai[DATA_LEN];
	Int32 Delta[DATA_LEN];
	Int32 LowPowerData[DATA_LEN];
}OTDR_ChannelData_t;

// �¼��㴦�����ݽṹ�����������¼��㺯��FindEventPoints��
#define	EVENT_TYPE_START		0			// ��ʼ�¼�
#define	EVENT_TYPE_REFLECT		1			// �����¼�
#define	EVENT_TYPE_NONREFLECT	2			// �Ƿ����¼�
#define	EVENT_TYPE_END			3			// �����¼�
#define	EVENT_TYPE_FIBER		4			// �����¼�
#define EVENT_TYPE_PROBABLE     5           // �������ܵ��¼�
#define EVENT_TYPE_GHOST        90          // ���ܵĹ�Ӱ
#define EVENT_TYPE_FUCK         100         // ���Ҳ�ˬ���¼���
#define	EVENT_TYPE_UNDEF		0xFEDCBA	// δȷ���¼��������Ժ�ȷ��

typedef struct		// 2010-12-8 11:47:39	2010-12-9 17:45:52
{
	Int32	*TotalEvent;		// ָ��ȫ���¼����ָ��		ʹ����Ѱ���¼��㺯����
	Int32	TotalEventNum;		// ȫ���¼������Ŀ			ʹ����Ѱ���¼��㺯����
	Int32	*ReflectEvent;		// ָ�����¼����ָ��		ʹ����Ѱ���¼��㺯����
	Int32	ReflectEventNum;	// �����¼������Ŀ			ʹ����Ѱ���¼��㺯����
	Int32	*FinalEvent;		// ָ�������¼����ָ��		ʹ���ڹ����¼��㺯����
	Int32	*FinalEventEnd;		// ָ�������¼���ĩ���ָ��	ʹ���ڹ����¼��㺯����
	Int32	FinalEventNum;		// �����¼������Ŀ			ʹ���ڹ����¼��㺯����
	Int32	*EventType;			// ָ�������¼������͵�ָ��	ʹ���ڹ����¼��㺯����
	Int32	*SaturateEvent;     // ���汥��ƽ�����¼����±� ʹ����ȷ�������¼��㺯����
	Int32	SatEventNum;        // ���汥��ƽ�����¼������ ʹ����ȷ�������¼��㺯����
}Event_t;

// ��ֵ�궨��
#define	MAX(a, b)	    (((a) > (b)) ? (a) : (b))
#define	MIN(a, b)	    (((a) < (b)) ? (a) : (b))
    
// 32λ������תΪ��Ӧ���޷�������
#define FLOAT2HEX(f)    (*(Uint32 *)&(f))
#define HEX2FLOAT(h)    (*(float  *)&(h))

// ��10Ϊ�׵�ָ��ת��Ϊ��Ȼָ��
#define Tenth(x)        (exp(2.3025850929940456840179914546844 * (x)))  // 10^x
#define Tenth_div5(x)   (exp(0.4605170185988091368035982909368 * (x)))  // 10^(x/5)
#define DOWN_n_dB(data, ndB)    (data * exp(0.4605170185988091368035982909368 * (-ndB)))    // data * 10^(-ndB/5)
#define UP_n_dB(data, ndB)    	(data * exp(0.4605170185988091368035982909368 * (ndB)))    	// data * 10^(ndB/5)

#define ENLARGE_FACTOR(MeasureTime)    (200*1000 / MAX(MeasureTime, 1000))

// �������Ͷ���
#define DATA_TYPE_CHAR      0
#define DATA_TYPE_SHORT     1
#define DATA_TYPE_INT       2
#define DATA_TYPE_LONG      3
#define DATA_TYPE_FLOAT     4
#define DATA_TYPE_DOUBLE    5

// OTDR���ʿ��Ʊ��������
typedef const Uint16 (*OtdrPowerLevel_t)[PULSE_WIDTH_NUM];

/*
 **************************************************************************************
 *  OTDR����Ŀ��Ʋ���		2011-12-21 8:18:02
 **************************************************************************************
*/
#define MAX_WL_NUM      10

typedef struct
{
    Uint32  OtdrStartPoint[MEASURE_LENGTH_NUM];     // Ϊÿ�����̴洢һ��������ʼ��ֵ
}OtdrParam_t;

/*
 **************************************************************************************
 *  OTDR���Ʊ�������		2010-12-28 10:01:17
 **************************************************************************************
*/
typedef struct
{
    // ���ο��Ʊ�������ʼ���Ժ󣬲��ٸı�
    Uint32 OtdrMode;           // OTDR����ģʽ         int
    Uint32 MeasureMode;        // ����ģʽ             bool
    Uint32 NonReflectThreMode; // �Ƿ����������÷�ʽ   int
    Uint32 RefreshData;        // �Ƿ���Ҫˢ������     bool
    Uint32 MeasurePurpose;     // ����Ŀ�ģ������û��Ǽ�� int
    Uint32 RawDataLevel;       // ԭʼ���ݼ���         int
    
    // ȫ�ֿ��Ʊ��������ۺ�ʱ�����������������Ŀ���
    Uint32 UserAction;         // �û��������������Ի���ȡ������ֹ int
    
    // ���ز��Կ��Ʊ������ڸ��������ж�̬�޸�
    Uint32 FindEvent;          // �Ƿ���ҪѰ���¼���   bool
    Uint32 EnableDeleteJitter;         // �Ƿ�ʹ��ȥ���� bool
    Uint32 EnableCapacityCompensate;   // �Ƿ�ʹ�ܵ��ݲ��� bool
    Uint32 EnableCurveConcat;         // �Ƿ�ʹ������ƴ�� bool
    Uint32 EnableAutoPulseWidth;       // �Ƿ�ʹ���Զ�����ʱ�Զ�ƥ������ bool
    Uint32 EnableAutoPower;            // �Ƿ�ʹ��ƴ��ʱ���Զ����ʿ��� bool
    Uint32 CurveConcat;               // �ôβ����Ƿ���Ҫ����ƴ�� bool
    Uint32 Need2RaiseLowPowerCurve;    // �Ƿ���Ҫ̧�ߵ͹������� bool
    Uint32 LowPowerDataChanged;        // �͹��������Ƿ񱻸ı� bool
    Uint32 LowPowerDataProcessed;      // �͹��������Ƿ�Ӧ�ñ����� bool
    
    Uint32 PowerMode;                  // ���ʿ���ģʽ int
    
    // ��FPGA�������
    float  PulsePeriod_us;     // ��������             int
    Uint32 AccCountOnce;       // һ��������ۼ���     int
    Uint32 NullReadCount;      // �ն�FPGA����         int
    
    // ״ָ̬ʾ
    Uint32 OtdrDataBusyNow;    // ���ڽ������ݲɼ�     bool
    Uint32 OtdrDataReadyFlag;  // ���ݲɼ���ɱ�־     int
    Uint32 OtdrAlgoBusyNow;    // ���ڽ������ݴ���     bool
	Uint32 OtdrAlgoReadyFlag;  // �㷨������ɱ�־     int
	
	Uint32 FpgaTimeOut;        // FPGAͨ�ų�ʱ         bool
	Uint32 NetWorkBusyNow;     // ָʾ���緱æ         bool
	Uint32 LinkStatus;         // Linkָʾ             int
	
	// ��ǰ�޲���ʱ���ۼ�
	Uint32 IdelTime;           // ����ʱ�����         int
	Uint32 ResetWhenSendError; // ��send��������ʱ���� bool
	Uint32 HostConnected;      // ��λ����OTDR�Ƿ���������   bool
	Uint32 option;	//wjc 2016-07-04 ch index
}OtdrCtrlVariable_t;

// OTDR����ģʽ
#define OTDR_MODE_IDLE          0       // ����ģʽ
#define OTDR_MODE_AVG           1       // ƽ��ģʽ
#define OTDR_MODE_REALTIME      2       // ʵʱģʽ
#define OTDR_MODE_2LAMBDA       3       // ˫��������ģʽ
#define OTDR_MODE_RECORRECT     101     // У��ģʽ
#define OTDR_MODE_CYCLE_TEST    102     // ѭ������ģʽ

// ����ģʽ
#define MEASURE_MODE_AUTO       0       // �Զ�����ģʽ
#define MEASURE_MODE_MANUAL     1       // �ֶ�����ģʽ

// �Ƿ����������÷�ʽ
#define NR_MODE_AUTO            0       // �Զ����÷�ʽ
#define NR_MODE_MANUAL          1       // �ֶ����÷�ʽ

// �û�����
#define USER_ACTION_NO_ACTION   0       // �޶�������������
#define USER_ACTION_CANCEL      1       // ȡ�������ݶ���������
#define USER_ACTION_STOP        2       // ��ֹ������ǰ��õ�����

// ����Ŀ��
#define MEASURE_PURPOSE_NONE        0       // ��Ŀ��
#define MEASURE_PURPOSE_CFG         1       // ����Ŀ��
#define MEASURE_PURPOSE_WARN        2       // ���Ŀ��
#define MEASURE_PURPOSE_EST         3       // ��ȡ�¼��㼰���˳���
#define MEASURE_PURPOSE_RECRRECT    100     // �Զ�У׼
#define MEASURE_PURPOSE_OTDR        255     // OTDR�㷨

// �㷨������ɱ�־
#define ALGO_READY_FLAG_I_AM_DEAD       0x00000000  // δ��ʼ�µ�OTDR�㷨���ݴ���
#define ALGO_READY_FLAG_START_NEW       0x00000001  // ��ʼ�µ�OTDR�㷨���ݴ���
#define ALGO_READY_FLAG_ONCE_DONE       0x00000002  // һ��ˢ�����ڵ����ݴ������
#define ALGO_READY_FLAG_ALL_DONE        0x00000003  // ȫ������ʱ������ݴ������

// �¼��㷨��ɱ�־
#define EVENT_READY_FLAG_I_AM_DEAD      0x00000000  // δ��ʼ�µ�OTDR�¼��㷨
#define EVENT_READY_FLAG_START_NEW      0x00000001  // ��ʼ�µ�OTDR�¼��㷨
#define EVENT_READY_FLAG_DONE           0x00000003  // �¼��㷨���

// ���ʿ���ģʽ
#define POWER_MODE_UNDEF                0           // ��ʼδ����
#define POWER_MODE_LOW                  1           // ʹ�õ͹���
#define POWER_MODE_HIGH                 2           // ʹ�ø߹���
#define POWER_MODE_COMBINE              3           // ʹ�û�Ϲ���

/*
 **************************************************************************************
 *  OTDR���Բ���״̬��������		2011-3-28 17:42:18
 **************************************************************************************
*/
// ���ƽ������
#define MAX_SAT_POINTS  16

typedef struct
{
/********************************* ʵ�ʵĲ������� ************************************/
    struct
    {
        Uint32  Lambda_nm;          // ��������λnm
        Uint32  MeasureLength_m;    // ���̣���λm
    	Uint32	PulseWidth_ns;		// �������ȣ���λns
    	Uint32	MeasureTime_ms;		// ����ʱ�䣬��λms
    	float   n;                  // ������
    	
    	float   EndThreshold;       // ��������
	    float   NonRelectThreshold; // �Ƿ�������
    }MeasureParam;

/********************************** ���߱���ƽ�� *************************************/
    Uint32  TailLength;             // ƽ����β�ĳ���
    struct
    {
        Uint32  SatStart[MAX_SAT_POINTS];
        Uint32  TailStart[MAX_SAT_POINTS];
        Uint32  TailEnd[MAX_SAT_POINTS];
        Uint32  SatNum;
    }OtdrCurveSaturate;

/****************************** �ߵ͹������ݴ�ŵ�ַ *********************************/
    Uint32  TreatAsHighPowerData;   // �Ƿ����ݵ����߹������ݶԴ�
    Uint32  PowerLevel;             // ��ǰ����
    Uint32  AutoPower;              // �Զ�ƥ��Ĺ���
    float   HighMinusLow;           // �ߵ͹�����������dB��

/********************************** ʵ�ʲ��Բ��� *************************************/
	Uint32  RealSampleRate_MHz; // ʵ�ʲ����ʣ���λMHz
	Uint32  RealSampleRate_Hz;  // ʵ�ʲ����ʣ���λHz

	Uint32  M;                  // �����Ӧ�Ĳ�������
	float   Points_1m;          // 1�׶�Ӧ�Ĳ�������
	float   MinNoiseAmp;        // ���������ֵ
	Uint32  SatThreshold;       // ����ֵ
	Uint32  sigma;              // ����������
	Uint32  sigmaLp;            // �͹�����������������
	Uint32  MaxRawData;         // ԭʼ���ݵ����ֵ��δȥ����

/*********************************** ���������� **************************************/
	Uint32  MeasureLengthPoint;     // ��Ӧ���̵ĵ�
	Uint32  CurveStartPoint;        // ��ǰ�������ߵ����
	Uint32  CurveConcatPoint;      // ����ƴ�ӵ�
	Uint32  HighPowerSatPoint;      // ȫ��������ǰ��ƽ����
    Uint32  SignalEndPoint;         // ������������źŽ����㣬Ӧ�����˲�ǰ����
	Uint32  SignalBadPoint;         // ���ݸ�������

/*********************************** �ۼ�ʱ���� **************************************/
	Uint32  RefreshPeriod_ms;       // ˢ�����ڣ���λms������С��800
	Uint32  TotalMeasureTime;       // �����û�������ֹ���ԣ��ñ�����¼ʵ�ʵĲ���ʱ��
	Uint32  LowPowerTime;           // �͹��ʲ���ʱ��
	Uint32  HighPowerTime;          // �߹��ʲ���ʱ��

/************************************* ������ ****************************************/
	Uint32  RefreshCount;           // ��������������ˢ�´�������
	Uint32  Receiver;               // ���ջ�
	Uint32  ApdV;                   // APD��ѹ����
	Uint32  RcvPw;                  // ���յ�������
	Uint32  RcvLambda;              // ���յ��Ĳ���
	Uint32  FiberConn;              // ��������״̬
	Uint32  EndEventIsReflect;      // �����¼��Ƿ��Ƿ����¼�
	float   AutoEndLevel;           // �Զ�����ʱ�������㴦������dBֵ
}OtdrStateVariable_t;

/*
 **************************************************************************************
 *  OTDRԭʼ���ݼ�����		2012-5-14 14:50:40
 **************************************************************************************
*/
#define DATA_RAW            0       // ��ԭʼ�����ݣ�δȥ����
#define DATA_ZERO_BASE      1       // ԭʼ����ȥ���߲��Ŵ��
#define DATA_ZERO_MOVE      2       // ԭʼ����ȷ����㲢ǰ�ƺ�
#define DATA_FIR_CMPS       3       // ԭʼ���������˲������ݲ�����
#define DATA_CMB            4       // ԭʼ���ݽ���ƴ�Ӻ�
#define DATA_IIR            5       // ԭʼ��������Ӧ�˲���
#define DATA_DELTA_LARGE    100     // ������������ֵ���� ���¼�
#define DATA_DELTA_SMALL    200     // ������������ֵ���� С�¼�

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
