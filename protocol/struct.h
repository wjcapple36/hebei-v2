#ifndef __STRUCT_H__H__H__
#define __STRUCT_H__H__H__
#include <stdbool.h>
#pragma pack(1) //结构体一字节对齐
struct _OTDRUnitInfo
{
	char cNodeName[64];
	char cNodeAddress[16];
};

#pragma pack(1)
struct _OTDRPipeStatus
{
	bool bIsPipeConfiged; //是否配置过运行参数
	bool bIsPipeFPGACommunicationSuccess; //与FPGA通信是否正常
};

#pragma pack(1)
struct _OTDRPipeInfo
{
	int iPipeSerialNum;   //通道编号
	int iLaserWaveLength; //激光器波长
	int iDynamicScope;   //动态范围
	char cWDM[16];   //波分复用
	int iReserved;  //保留位
};

//OTDR每通路的测量参数
#pragma pack(1)
struct _OTDRPipeTestParam
{
	int iPipeSerialNum; //通道编号
	int iRange; //测量范围
	int iWaveLength; //中心波长
	int iPulseWidth; //脉冲宽度
    int iTime; //测量时间
    float fRefractive; //折射率
	float fEndTH; //结束门限
	float fNoneReflectTH; //非反射门限
	int iFrequency; //采样频率
	int iReserved2; //保留位
	int iReserved3;	 //保留位
};

//接收光纤段配置信息
#pragma pack(1)
struct _PipeFiberSectionInfo  
{
	int iFiberSectionSerialNum;  //光纤段编号
	char cFiberRoute[64];        //路由
	char cFiberLineName[64];    //光纤线路名称
	int iStartCoordinate;       //起点坐标
	char cStartGeographyInfo[64];  //起点地理信息
	int iEndCoordinate;        //末点坐标
	char cEndGeographyInfo[64];   //末点地理信息
	float fStartAndEndPointLossInitialValue; //光纤段起末两点损耗初始值
	float fFirstAlarmTh; //一级告警门限
	float fSecondAlarmTh; //二级告警门限
	float fThirdAlarmTh; //反侦听告警门限
};

#pragma pack(1)
struct _ResultInfo
{
	float fChainLength; //链长
   	float fChainLoss; //链损耗
	float fChainAttenu; //链衰减
};

#pragma pack(1)
struct _OTDRCurveEvent
{
	int iEventLocation; //事件点位置
	int iEventType; //事件类型0,开始事件，1反射事件，2非反射事件，3结束事件
	float fAttenuationCoef; //衰减系数
	float fInsertLoss; //插入损耗
	float fReflectance; //反射值
	float fTotalLoss; //累计损耗
};

#pragma pack(1)
struct _PipeStatisData       //光纤段衰减统计数据
{
    int iFiberSectionSerialNum; //光纤段编号
	float fAttenuation;//衰减值
};

#pragma pack(1)
struct _OperateError
{
	int iPipeNumber;   //通道编号
	int iErrorCode;    //错误码
	int iCmdCode;      //命令码
	int iOCVMRefreshFlag;  //OCVM刷新标志
	int iReserved1;   //保留位
	int iReserved2;   //保留位
	int iReserved3;	  //保留位
};

#pragma pack(1)
struct _PipeAlarmStatus
{
	bool bIsHaveAlarm;  //是否有告警
    bool bIsAlarmChange; //告警是否变化
    int iPipeAlarmNum;	 //通道告警数目
};

#pragma pack(1)
struct _CurAlarmInfo
{
	int iPipeNumber;   //通道编号
	int iFiberSectionSerialNum; //光纤段编号,从0开始编号
	int iAlarmLevel; //告警级别,1表示一级告警，2表示二级告警，3表示三级告警
	int iAlarmType; //告警类别,1表示光纤线路告警，2表示设备告警
	float fInsertLossChangeValue; //事件插入损耗变化值
	int iAlarmPos; //告警位置
	int iReserved1; //保留位
	int iReserved2; //保留位
	int iErrorRange;   //告警位置允许的误差距离
};

#pragma pack(1)
struct _AlarmLevelAndPos
{
   	int iAlarmLevel; //告警级别,0表示无告警,1表示一级告警（A），2表示二级告警（B）
	int iPointAlarmPos; //点告警位置
	int iAlarmType; //告警类别
};


#endif