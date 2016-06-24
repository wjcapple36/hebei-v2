#ifndef __STRUCT_H__H__H__
#define __STRUCT_H__H__H__
#include <stdbool.h>
#pragma pack(1) //�ṹ��һ�ֽڶ���
struct _OTDRUnitInfo
{
	char cNodeName[64];
	char cNodeAddress[16];
};

#pragma pack(1)
struct _OTDRPipeStatus
{
	bool bIsPipeConfiged; //�Ƿ����ù����в���
	bool bIsPipeFPGACommunicationSuccess; //��FPGAͨ���Ƿ�����
};

#pragma pack(1)
struct _OTDRPipeInfo
{
	int iPipeSerialNum;   //ͨ�����
	int iLaserWaveLength; //����������
	int iDynamicScope;   //��̬��Χ
	char cWDM[16];   //���ָ���
	int iReserved;  //����λ
};

//OTDRÿͨ·�Ĳ�������
#pragma pack(1)
struct _OTDRPipeTestParam
{
	int iPipeSerialNum; //ͨ�����
	int iRange; //������Χ
	int iWaveLength; //���Ĳ���
	int iPulseWidth; //������
    int iTime; //����ʱ��
    float fRefractive; //������
	float fEndTH; //��������
	float fNoneReflectTH; //�Ƿ�������
	int iFrequency; //����Ƶ��
	int iReserved2; //����λ
	int iReserved3;	 //����λ
};

//���չ��˶�������Ϣ
#pragma pack(1)
struct _PipeFiberSectionInfo  
{
	int iFiberSectionSerialNum;  //���˶α��
	char cFiberRoute[64];        //·��
	char cFiberLineName[64];    //������·����
	int iStartCoordinate;       //�������
	char cStartGeographyInfo[64];  //��������Ϣ
	int iEndCoordinate;        //ĩ������
	char cEndGeographyInfo[64];   //ĩ�������Ϣ
	float fStartAndEndPointLossInitialValue; //���˶���ĩ������ĳ�ʼֵ
	float fFirstAlarmTh; //һ���澯����
	float fSecondAlarmTh; //�����澯����
	float fThirdAlarmTh; //�������澯����
};

#pragma pack(1)
struct _ResultInfo
{
	float fChainLength; //����
   	float fChainLoss; //�����
	float fChainAttenu; //��˥��
};

#pragma pack(1)
struct _OTDRCurveEvent
{
	int iEventLocation; //�¼���λ��
	int iEventType; //�¼�����0,��ʼ�¼���1�����¼���2�Ƿ����¼���3�����¼�
	float fAttenuationCoef; //˥��ϵ��
	float fInsertLoss; //�������
	float fReflectance; //����ֵ
	float fTotalLoss; //�ۼ����
};

#pragma pack(1)
struct _PipeStatisData       //���˶�˥��ͳ������
{
    int iFiberSectionSerialNum; //���˶α��
	float fAttenuation;//˥��ֵ
};

#pragma pack(1)
struct _OperateError
{
	int iPipeNumber;   //ͨ�����
	int iErrorCode;    //������
	int iCmdCode;      //������
	int iOCVMRefreshFlag;  //OCVMˢ�±�־
	int iReserved1;   //����λ
	int iReserved2;   //����λ
	int iReserved3;	  //����λ
};

#pragma pack(1)
struct _PipeAlarmStatus
{
	bool bIsHaveAlarm;  //�Ƿ��и澯
    bool bIsAlarmChange; //�澯�Ƿ�仯
    int iPipeAlarmNum;	 //ͨ���澯��Ŀ
};

#pragma pack(1)
struct _CurAlarmInfo
{
	int iPipeNumber;   //ͨ�����
	int iFiberSectionSerialNum; //���˶α��,��0��ʼ���
	int iAlarmLevel; //�澯����,1��ʾһ���澯��2��ʾ�����澯��3��ʾ�����澯
	int iAlarmType; //�澯���,1��ʾ������·�澯��2��ʾ�豸�澯
	float fInsertLossChangeValue; //�¼�������ı仯ֵ
	int iAlarmPos; //�澯λ��
	int iReserved1; //����λ
	int iReserved2; //����λ
	int iErrorRange;   //�澯λ�������������
};

#pragma pack(1)
struct _AlarmLevelAndPos
{
   	int iAlarmLevel; //�澯����,0��ʾ�޸澯,1��ʾһ���澯��A����2��ʾ�����澯��B��
	int iPointAlarmPos; //��澯λ��
	int iAlarmType; //�澯���
};


#endif