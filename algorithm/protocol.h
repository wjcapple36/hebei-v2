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
/**************************************** ����궨�� *********************************************/
/*************************************************************************************************/
#define CMD_HOST_START_MEASURE      0x10000000      // ���ò��Բ�������������
#define CMD_HOST_STOP_MEASURE       0x10000001      // ȡ������ֹ����
#define CMD_HOST_SET_IP             0x10000002      // ����IP
#define CMD_HOST_GET_BATTERY        0x10000003      // ��ѯ��ص���
#define CMD_HOST_NETWORK_IDLE       0x10000004      // �������

#define CMD_HOST_SET_RESET_PERIOD	0x1000000A		// ���ÿ���״̬�µĶ�ʱ��λ���ڣ���λΪmin

#define CMD_HOST_MEASURE_FIBER      0x1000000F      // ���ò��Բ������������ԣ�ֻ��ȡ���Ⱥ��¼���

#define CMD_HOST_GETDATA_UNFILTER   0x10000010      // ��ȡδ���˲�������

#define CMD_HOST_GET_VERSION        0x11000000      // ��ȡ�汾�ͱ���ʱ������

#define CMD_HOST_GET_DELTA          0x1FFFFFFE      // ��λ����ȡ������������ֵ����
#define CMD_HOST_GET_RAWDATA        0x1FFFFFFF      // ��λ����ȡ����ԭʼ���ݲ���

#define CMD_OTDR_UPDATE             0x20000000      // OTDR��������
#define CMD_GET_IP                  0x30000000      // ��λ��UDP����IP
#define CMD_DSP_UPLOAD_ALL_DATA     0x90000000      // �ϴ�ȫ����������
#define CMD_DSP_UPLOAD_REF_DATA     0x90000001      // �ϴ�ˢ�²�������
#define CMD_DSP_UPLOAD_BATTERY      0x90000002      // �ϴ���ص���
#define CMD_OTDR_RESET              0x9000000A      // ֪ͨ��λ����OTDR׼����λ
#define CMD_DSP_UPLOAD_PARAM_DATA   0x9000000F      // �ϴ����Բ������ݣ���Ӧ 0x1000000F

#define CMD_DSP_UPLOAD_DATA_UNFILTER   0x90000010   // �ϴ�δ���˲������� 0x10000010

#define CMD_DSP_UPLOAD_VERSION      0x91000000      // �ϴ��汾��Ϣ
#define CMD_DSP_HEART_BEAT          0x92000000      // �ϴ�������

#define CMD_DSP_UPLOAD_DELTA        0x9FFFFFFE      // �ϴ�������������ֵ����
#define CMD_DSP_UPLOAD_RAW          0x9FFFFFFF      // �ϴ�ԭʼ����

#define CMD_RESPONSE_STATE          0xA0000000      // ����״̬��
#define CMD_SEND_IP                 0xB0000000      // �ϴ�IP��ַ

/******************************* ֡��ʽ���� **************************************/
// �汾��
#define REV_ID      0
#define RSVD_VALUE  0xffffeeee

// ֡����
#define FRAMETYPE_HOST2TARGET  0   // ������������
#define FRAMETYPE_TARGET2HOST  1   // ��Ӧ��������

#define	FRAME_SYNC_STRING	"GLinkOtdr-3800M"

// ֡ͷ��־
typedef struct
{
    char    FrameSync[16];  // GLinkOtdr-3800M
    uint32_t  TotalLength;    // ��֡��
    uint32_t  Rev;            // �汾��
    uint32_t  FrameType;      // ֡����
    uint32_t  Src;            // Դ��ַ
    uint32_t  Dst;            // Ŀ�ĵ�ַ
    uint32_t  PacketID;       // ��ˮ��
    uint32_t  RSVD;           // ����
}FrameHeader_t;

// ���ò��Բ�������������
typedef struct
{
    uint32_t  Cmd;            // ������ 0x10000000    0x1FFFFFFF
    uint32_t  PacketLength;   // ���ݳ���
    
    // ���Ʋ���
    struct
    {
        uint32_t  OtdrMode;           // OTDRģʽ
        uint32_t  OtdrOptMode;        // OTDR�Ż�ģʽ
        uint32_t  RSVD;               // ����ȡԭʼ����ʱ����������ݵļ��𣬼� DATA_LEVEL
        uint32_t  EnableRefresh;      // ʹ�ܻ��ֹˢ��
        uint32_t  RefreshPeriod_ms;   // ˢ�����ڣ���λms������С��800
    }Ctrl;
    
    // ״̬����
    struct
    {
        uint32_t	Lambda_nm;			// �����Ⲩ������λnm
    	uint32_t	MeasureLength_m;	// �������ȣ���λm
    	uint32_t	PulseWidth_ns;		// �������ȣ���λns
    	uint32_t	MeasureTime_ms;		// ����ʱ�䣬��λms
    	float   n;                  // ������
    	
    	float   EndThreshold;       // ��������
    	float   NonRelectThreshold; // �Ƿ�������
    }State;
    
    uint32_t  RSVD;
}OTDR_MeasureParam_t;

// ȡ������
typedef struct
{
    uint32_t  Cmd;            // ������ 0x10000001
    uint32_t  PacketLength;   // ���ݳ���
    
    // ���Ʒ�ʽ
    uint32_t  Cancel_Or_Abort;    // ȡ��������ֹ����
    
    uint32_t  RSVD;
}OTDR_Cancel_t;

// IP��ַ���������롢Ĭ������
typedef struct
{
    uint32_t  Cmd;            // ������ 0x10000002
	uint32_t  PacketLength;   // ���ݳ���
	
	char    LocalIPAddr[16];
	char    LocalIPMask[16];
	char    GatewayIP[16];

	uint32_t  RSVD;
}OTDR_IP_t;

// OTDR�ϴ�ȫ����������
#define	MAX_EVENT_NUM		500
#define RSVD_FLOAT          8192.0      // ����������ֵ��������ֵ�޷�ȷ��������ȷ��
typedef struct
{
	uint32_t  Cmd;            // ������ 0x90000000
	uint32_t  PacketLength;   // ���ݳ���
	
	// ����������Ϣ
	struct
	{
	    uint32_t	SampleRate_Hz;
		uint32_t	MeasureLength_m;
		uint32_t	PulseWidth_ns;
		uint32_t	Lambda_nm;
		uint32_t	MeasureTime_ms;
		float	n;
		float	FiberLength;	    // ��������
		float	FiberLoss;		    // �����
		float	FiberAttenCoef;	    // ��˥��ϵ��
		
    	float   NonRelectThreshold; // �Ƿ�������
    	float   EndThreshold;       // ��������
    	
    	uint32_t  OtdrMode;           // OTDRģʽ�� ʵʱ������
        uint32_t  MeasureMode;        // ����ģʽ�� �Զ����ֶ�
	}MeasureParam;
	
	// ����������Ϣ
	struct
	{
		uint32_t	DataNum;
		uint16_t	dB_x1000[DATA_LEN];
	}OtdrData;
	
	// �¼�����Ϣ��Ԫ
	struct
	{
		uint32_t	EventNum;			// �¼�����Ŀ
		// �¼���
		struct
		{
			uint32_t	EventXlabel;		// �¼������ϴ����������е����
			uint32_t	EventType;			// �¼�������
			float	EventReflectLoss;	// ������� / �ز����
			float	EventInsertLoss;    // �������
			float	AttenCoef;	        // ����һ�¼���֮��˥��ϵ��
			float	EventTotalLoss;	    // �¼����ۼ����
		}EventPoint[MAX_EVENT_NUM];
	}Event;
	
	uint32_t  RSVD;
}OTDR_UploadAllData_t;

// OTDR�ϴ�ˢ�²�������
typedef struct
{
	uint32_t  Cmd;            // ������ 0x90000001
	uint32_t  PacketLength;   // ���ݳ���
	
	// ����������Ϣ
	struct
	{
		uint32_t	DataNum;
		uint16_t	dB_x1000[DATA_LEN];
	}OtdrData;

	uint32_t  RSVD;
}OTDR_UploadRefData_t;

// OTDR�ϴ�ԭʼ����
typedef struct
{
	uint32_t  Cmd;            // ������ 0x9FFFFFFF
	uint32_t  PacketLength;   // ���ݳ���
	
	// ����������Ϣ
	struct
	{
	    uint32_t	SampleRate_Hz;
		uint32_t	MeasureLength_m;
		uint32_t	PulseWidth_ns;
		uint32_t	Lambda_nm;
		uint32_t	MeasureTime_ms;
		float	n;
	}MeasureParam;
	
	// ����������Ϣ
	uint32_t	DataNum;
	int32_t	RawData[DATA_LEN];

	uint32_t  RSVD;
}OTDR_UploadRawData_t;

// OTDR�ϴ�δ���˲��Ķ�����������
typedef struct
{
	uint32_t  Cmd;            // ������ 0x90000010
	uint32_t  PacketLength;   // ���ݳ���
	
	// ����������Ϣ
	struct
	{
	    uint32_t	SampleRate_Hz;
		uint32_t	MeasureLength_m;
		uint32_t	PulseWidth_ns;
		uint32_t	Lambda_nm;
		uint32_t	MeasureTime_ms;
		float	n;
	}MeasureParam;
	
	// ����������Ϣ
	uint32_t	DataNum;
	uint16_t	dB_x1000[DATA_LEN];

	uint32_t  RSVD;
}OTDR_UploadUnFilterData_t;

// OTDR�ϴ�������������ֵ����
typedef struct
{
	uint32_t  Cmd;            // ������ 0x9FFFFFFE
	uint32_t  PacketLength;   // ���ݳ���

	// ����������Ϣ
	float	t1;
	float   t2;
	int32_t   sigma;
	int32_t	Delta[DATA_LEN];
	int32_t	Threshold[DATA_LEN];

	uint32_t  RSVD;
}OTDR_UploadDelta_t;

// OTDR�ϴ���ص���
typedef struct
{
	uint32_t  Cmd;            // ������ 0x90000002
	uint32_t  PacketLength;   // ���ݳ���
	
	uint32_t  Data;

	uint32_t  RSVD;
}OTDR_UploadBattery_t, HostSetIdleResetPeriod_t;

/************************************** OTDR�ϴ��汾��Ϣ *****************************************/
typedef struct
{
	uint32_t  Cmd;            // ������ 0x91000000
	uint32_t  PacketLength;   // ���ݳ���
	
	// ��8λΪ���汾�ţ���8λΪ�ΰ汾��
	uint16_t  Major_Minor;
	
	// �����������
	uint16_t  BuildYear;
    uint16_t  BuildMon;
    uint16_t  BuildDay;
    
    // �������ʱ��     ��16λΪСʱ(0~23)����16λ��ĸ�8λΪ����(0~59)����8λΪ��(0~59)
    uint32_t  BuildTime;
    char    Hardware[16];   // TR600-0126-A     ����    TR600-0126-C
    
	uint32_t  RSVD;
}OTDR_UploadVersion_t;

/************************************** OTDR�ϴ��汾��Ϣ *****************************************/
typedef struct
{
	uint32_t  Cmd;            // ������ 0x92000000
	uint32_t  PacketLength;   // ���ݳ���
	
	uint32_t  MeasureState;   // 0 : OTDR_MODE_IDLE   1 : OTDR_MODE_AVG   2 : OTDR_MODE_REALTIME
	uint32_t  CpuTime;        // ��ǰCPU����ʱ�䣬ms
	uint32_t  RSVD;
}OTDR_HeartBeat_t;

// OTDR��֡�������ݽṹ
#define	STATE_CODE_CMD_OK                   0       // �ɹ���������
#define	STATE_CODE_FRAME_SYNC_ERROR         1       // �Ƿ�֡��ʼ�ַ���
#define	STATE_CODE_REV_ERROR                2       // �Ƿ��汾��
#define	STATE_CODE_FRAME_TYPE_ERROR         3       // �Ƿ�֡����
#define	STATE_CODE_CMD_ID_ERROR             4       // �Ƿ������ʶ
#define	STATE_CODE_PACKET_LENTH_ERROR       5       // �Ƿ����ݳ���

#define	STATE_CODE_ML_OR_PW_ERROR           16      // ���̻�����Ƿ�
#define	STATE_CODE_N_ERROR                  17      // Ⱥ�����ʷǷ�
#define	STATE_CODE_NR_ERROR                 18      // �Ƿ������޷Ƿ�
#define STATE_CODE_OTDR_BUSY_MEASURE        19      // �Ƿ��������
#define STATE_CODE_IP_ERROR                 20      // �Ƿ�IP��ַ
#define STATE_CODE_NO_UNFILTERDATA          21      // ������δ�˲�������

// ��������״̬��
#define STATE_CODE_FILE_CONTENT_ERROR       100     // �����ļ����ݳ���
#define STATE_CODE_OTDR_UPDATE_START        101     // ��ʼ����
#define STATE_CODE_OTDR_UPDATE_FAIL         102     // ����ʧ��
#define STATE_CODE_OTDR_UPDATE_DONE         103     // �������

typedef struct
{
	uint32_t	Cmd;		    // ������ 0xA0000000
	uint32_t  PacketLength;   // ���ݳ���
	uint32_t	StateCode;		// ��Ӧ��
	uint32_t  RSVD;
}OTDR_State_t;

// ���������ļ����ݽṹ����
// �����ļ��ĽṹΪ EncData(n) + DSP_CODE_ID(16) + CodeLen(4) + Checksum(4)
typedef struct
{
    uint32_t  Cmd;            // ������ 0x20000000
    uint32_t  PacketLength;   // ���ݳ���
//	uint8_t   content[0];
	uint32_t  CodeLen;
	uint32_t  Checksum;
}OTDR_DspCodeFile_t;

/*************************************************************************************************/
/****************************************** Ĭ�ϵ�IP��ַ *****************************************/
/*************************************************************************************************/
#define	DEFAULT_IP_ADDR		"192.168.1.249"
#define	DEFAULT_IP_MASK		"255.255.255.0"
#define	DEFAULT_IP_GATEWAY	"192.168.1.1"
#define	DEFAULT_PORT_ADDR	5000

/*************************************************************************************************/
/******************************************** �¼����� *******************************************/
/*************************************************************************************************/
#define MAX_GROUP_NUM   5
typedef struct
{
    int32_t   ValidGroupNum;      // �м����¼���������Ч��
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
	Uint32  Cmd;            // ������ 0x1000FFFF
	Uint32  PacketLength;   // ���ݳ���

	Uint32  RefreshPeriod_ms;   // ˢ�����ڣ���λms������С��800
	struct
	{
		Uint32	Lambda_nm;			// �����Ⲩ������λnm
		Uint32	MeasureLength_m;	// �������ȣ���λm
		Uint32	PulseWidth_ns;		// �������ȣ���λns
		Uint32	MeasureTime_ms;		// ����ʱ�䣬��λms
		Uint32  R;					// ���ջ�����
		Uint32  P;					// ����������
		Uint32  ApdV;				// APD��ѹ
	}MeasureParam;
	
	Uint32  RSVD;
}OTDR_TouchMeasureParam_t;

#endif	// _TCP_OTDR_H
