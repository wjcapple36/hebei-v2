#include <stdio.h>
#include <math.h>

#include "Otdr.h"
#include "prototypes.h"

#define OtdrSleepTime   15000
pthread_mutex_t mutex_otdr;

void Thread_OtdrData(void);
void Thread_OtdrAlgo(void);
void Thread_TcpSocket(void);
void Thread_TcpTouch(void);
/*
int32_t main(void)
{
	int tid_otdralgo, tid_otdrdata, tid_tcpsocket, tid_tcptouch;
	OtdrDataInit();
	pthread_create(&tid_otdrdata, NULL, Thread_OtdrData, NULL);
	pthread_create(&tid_otdralgo, NULL, Thread_OtdrAlgo, NULL);
	pthread_create(&tid_tcpsocket, NULL, Thread_TcpSocket, NULL);
	pthread_create(&tid_tcptouch, NULL, Thread_TcpTouch, NULL);
	pthread_join(tid_otdrdata, NULL);
	pthread_join(tid_otdralgo, NULL);
	pthread_join(tid_tcpsocket, NULL);
	pthread_join(tid_tcptouch, NULL);
}
*/
/******************************** ???????? ************************************/
void get_one_piece_of_data(uint32_t powerlevel, uint32_t rcv, uint32_t);
uint32_t AutoMeasureGetFiberLen();
uint32_t MeasureGetFiberConn(void);
void MeasureGetCurveConnect(void);
void MeasureGetAutoPower(void);

/*
 **************************************************************************************
 *  ???????ƣ? Thread_OtdrData
 *  ?????????? Otdr??ȡ???ݣ???ͨ??OtdrCtrl.OtdrAlgoReadyFlag??��
 *             ????????TskFxn_Otdr��????????
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2011-3-21 21:43:29  v1.0
 **************************************************************************************
*/
void Thread_OtdrData(void)
{
	int32_t   i, FpgaStartCount, RefreshCount, CycleTestTime;
	int32_t   CurrentAccumulateTime, TotalAccumulateTime;
	int32_t   LowPowerTime, HighPowerTime;
	uint32_t  FiberLen;
	
    pthread_mutex_init(&mutex_otdr, NULL);
	for(;;)
	{
Thread_OtdrData_Start:
/********************************** ???????ݲɼ????? *********************************************/
    	OtdrCtrl.OtdrMode        = OTDR_MODE_IDLE;
    	OtdrCtrl.OtdrDataBusyNow = 0;

/*************************** ????OTDR??ģʽΪ????ģʽ?????ȴ? ************************************/
        while(OTDR_MODE_IDLE == OtdrCtrl.OtdrMode)	usleep(OtdrSleepTime);
        printf("Thread_OtdrData Started at time: %010d\n", CurrentDateTime());
        
/******************************* ????OtdrCtrl??OtdrState *****************************************/
        if((OTDR_MODE_AVG == OtdrCtrl.OtdrMode) || (OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode))
        {
            pthread_mutex_lock(&mutex_otdr);
            OtdrUpdateParam();
            pthread_mutex_unlock(&mutex_otdr);
        }
        else
        {
            printf("\nUnknow OtdrMode!!!!!!\n");
            continue;
        }
#if 0 // Wed Apr 27 20:49:07 CST 2016
/****************************** ?????Զ????ԣ???????Ӧ????��· ***********************************/
        if(MEASURE_MODE_AUTO == OtdrCtrl.MeasureMode)
        {
            FiberLen = AutoMeasureGetFiberLen();
            AdaptMeasureLength_PulseWidth(FiberLen);
            OtdrStateInit();
        }
/************************************ ȷ???Ƿ???Ҫƴ?? *******************************************/
        if(CheckIfCurveConnect())
        {
            MeasureGetCurveConnect();
        }
/************************************ ȷ???Ƿ??жϹ??? *******************************************/
//        if(CheckIfAutoPower())
        {
            MeasureGetAutoPower();
        }
#endif
/****************** ֻҪOTDR??ģʽ??Ϊ????ģʽ????Ҫ?Ƚ???һ?β??? *******************************/
        // ????ͨ??????
        memset(OtdrData.ChanData, 0, sizeof(OtdrData.ChanData));
        do
        {
            OtdrState.RefreshCount = 0;
            OtdrCtrl.OtdrDataBusyNow = 1;                               // ???????ݲɼ?æ
            EDMA_Chan_ClrData();                                        // ????????????
            
            // ????????FPGA???????????Լ?ˢ?´???
            FpgaStartCount = OtdrState.MeasureParam.MeasureTime_ms / TIMES_COUNT_ONCE;
            RefreshCount   = OtdrState.MeasureParam.MeasureTime_ms / OtdrState.RefreshPeriod_ms - 1;
    
            // ????FPGA?ɼ?????ȡ????
            CurrentAccumulateTime = 0;  // ??ǰ?ۼ?ʱ??
            TotalAccumulateTime   = 0;  // ??ǰȫ???ۼ?ʱ??
            
            // ??С???????????ر?��????
            LowPowerTime  = 0;
            HighPowerTime = 0;
            
            for(i = 0; i < FpgaStartCount; i++)
            {
/************************************** ????FPGA???в??? *****************************************/
                GetReceiverAndPowerLevel(i);
                get_one_piece_of_data(OtdrState.PowerLevel, OtdrState.Receiver, i);
/******************************* ??ȡFPGA?????ݲ??????ۼ? ****************************************/
                CurrentAccumulateTime += TIMES_COUNT_ONCE;      // ???µ?ǰ???ۼ?ʱ??
                TotalAccumulateTime   += TIMES_COUNT_ONCE;      // ????ȫ?????ۼ?ʱ??
                
                // ???¸ߵ͹????ۼ?ʱ??
                if(OtdrState.TreatAsHighPowerData)  HighPowerTime += TIMES_COUNT_ONCE;
                else                                LowPowerTime  += TIMES_COUNT_ONCE;
/*************************** ?????û????����?????һ???ж? ****************************************/
                if(USER_ACTION_CANCEL == OtdrCtrl.UserAction)       // ȡ???????ݶ??�����??
                {
                    printf("OtdrCtrl.UserAction : Cancel\n");
                    goto Thread_OtdrData_Start;
                }
                else if(USER_ACTION_STOP == OtdrCtrl.UserAction)   // ??ֹ?????���ǰ???õ?????
                {
                    printf("OtdrCtrl.UserAction : Stop\n");
                    break;
                }

/****************************** ??־????һ??ˢ?????ڵ????ݲɼ? ***********************************/
                if(OtdrCtrl.RefreshData && (OTDR_MODE_AVG == OtdrCtrl.OtdrMode))
                {
                    if((CurrentAccumulateTime >= OtdrState.RefreshPeriod_ms) && (RefreshCount > 0))
                    {
                        CurrentAccumulateTime      = 0;
                        OtdrState.TotalMeasureTime = TotalAccumulateTime;
                        OtdrState.LowPowerTime     = LowPowerTime;
                        OtdrState.HighPowerTime    = HighPowerTime;

/********* ????????ƽ??ģʽ?£???????Ҫˢ?????ݣ???????Otdr?㷨???????ݡ?ʵʱģʽ??ˢ?? **********/
                        OtdrCtrl.OtdrAlgoReadyFlag = ALGO_READY_FLAG_START_NEW;
                        OtdrState.RefreshCount++;
                        OtdrCtrl.FindEvent         = 0;             // ??Ѱ???¼???

                        EDMA_Chan_CopyData();
                        RefreshCount--;     // ˢ?´?????1
                        printf("RefreshCount = %d\n", RefreshCount);
                    }
                }
            }
/************** ???????ݲɼ???????Otdr?㷨???????ݣ?Ѱ???¼??㣬??????ʵ???ۼӴ??? ***************/
            while(OtdrCtrl.OtdrAlgoBusyNow)  usleep(OtdrSleepTime);
            EDMA_Chan_CopyData();
            
            if((OTDR_MODE_AVG == OtdrCtrl.OtdrMode) || (OTDR_MODE_RECORRECT == OtdrCtrl.OtdrMode))
            {
                OtdrCtrl.FindEvent = 1;              // Ѱ???¼???
            }
            else if(OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode)
            {
                OtdrCtrl.FindEvent = (USER_ACTION_NO_ACTION != OtdrCtrl.UserAction); // ??ֹ??Ѱ???¼???
            }

            OtdrState.TotalMeasureTime    = TotalAccumulateTime;
            OtdrState.LowPowerTime        = LowPowerTime;
            OtdrState.HighPowerTime       = HighPowerTime;
            printf("Thread_OtdrData Finished at time: %010d\n", CurrentDateTime());
            
/***************************************** ?????㷨 **********************************************/
            OtdrCtrl.OtdrDataBusyNow   = 0;
            OtdrCtrl.OtdrAlgoReadyFlag = ALGO_READY_FLAG_START_NEW;
    
/*********************** ??????ƽ??ģʽ?????ָ??ɿ???ģʽ??ʵʱģʽ???ָ? ************************/
            if(OTDR_MODE_AVG == OtdrCtrl.OtdrMode)
            {
                usleep(OtdrSleepTime);
                while(OtdrCtrl.OtdrAlgoBusyNow)  usleep(OtdrSleepTime);
                OtdrCtrl.OtdrMode = OTDR_MODE_IDLE;
            }
/************** ??????ʵʱģʽ???????û?ȡ??????ֹ?????ȴ??㷨???ɺ??ָ??ɿ???ģʽ ***************/
            if((OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode) && (USER_ACTION_NO_ACTION != OtdrCtrl.UserAction))
            {
                usleep(OtdrSleepTime);
                while(OtdrCtrl.OtdrAlgoBusyNow)  usleep(OtdrSleepTime);
                OtdrCtrl.OtdrMode = OTDR_MODE_IDLE;
            }
        }while(OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode);    // ????Ϊʵʱ??ѭ??ģʽ????????
	}
}

/*
 **************************************************************************************
 *  ???????ƣ? Thread_OtdrAlgo
 *  ?????????? Otdr???ݴ???
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2011-4-10 11:25:07  v1.0
 **************************************************************************************
*/
void Thread_OtdrAlgo(void)
{
	for(;;)
	{
/********************************* ?????㷨???? **************************************************/
		OtdrCtrl.OtdrAlgoBusyNow   = 0;
        
/***************** ?ȴ?OTDR?㷨??ʼ??ˢ?????ݻ?ȫ?????ݶ?Ҫ???м??㴦?? **************************/
		while(ALGO_READY_FLAG_START_NEW != OtdrCtrl.OtdrAlgoReadyFlag)
		{
			usleep(OtdrSleepTime);      // NOT ready
		}

/********************************* ?????㷨æ ****************************************************/
		// ??????ʱ?û?ȡ?????ԣ??򲻴???????
		if(USER_ACTION_CANCEL == OtdrCtrl.UserAction)    continue;      // 2011-8-22 21:46:09
		
		OtdrCtrl.OtdrAlgoBusyNow = 1;
		
/********************************* ?ȴ?ȫ?ֱ?��???? **********************************************/
        pthread_mutex_lock(&mutex_otdr);
/************************************ ??ʼOTDR?㷨???????? ***************************************/
        if((OTDR_MODE_AVG == OtdrCtrl.OtdrMode) || (OTDR_MODE_REALTIME == OtdrCtrl.OtdrMode))
        {
            if(OtdrCtrl.FindEvent == 0)     ProcessRefreshData(OtdrState.RefreshCount);
            else                            ProcessFinalData(MEASURE_PURPOSE_OTDR);
        }

        OtdrCtrl.OtdrAlgoBusyNow = 0;
        OtdrCtrl.OtdrAlgoReadyFlag = ALGO_READY_FLAG_ALL_DONE;
        pthread_mutex_unlock(&mutex_otdr);
	}
}

/*
 **************************************************************************************************
 *  ????  ???? get_one_piece_of_data
 *  ?????????? ??FPGA????һ?????ͬʱ??ȡһ??????
 *  ???ڲ????? powerlevel  : ???ʼ???
 *             rcv         : ʹ?õĽ??ջ?
 *  ???ز????? 
 *  ???ڰ汾?? 2012-12-28  11:09:37  v1.0
 **************************************************************************************************
*/
void get_one_piece_of_data(uint32_t powerlevel, uint32_t rcv, uint32_t index)
{
    int j;
/************************************** ????FPGA???в??? *****************************************/
    FpgaStart(powerlevel, rcv, index==0);
    usleep(1000*TIMES_COUNT_ONCE);
    
    for(j = 0; j < 1000*TIMES_COUNT_ONCE/OtdrSleepTime; j++)    // ?????ȴ? TIMES_COUNT_ONCE ????
    {
        if(CheckDataAvailable())     break;
        usleep(OtdrSleepTime);
    }
/******************************* ??ȡFPGA?????ݲ??????ۼ? ****************************************/
    EDMA_Chan_GetData();
}

/*
 **************************************************************************************************
 *  FunctionName : CurrentDateTime
 *  Description  : current date of Mon-Day, time of HH-MM-SS, in the form of integer
 *  InputParam   : NONE
 *  ReturnValue  : int daytime, such as 1001125324, means the date is Oct 1st, time is 12:53:24
 *  Date & Ver   : 2016-04-29 10:36:48 Ver 1
 **************************************************************************************************
*/
int CurrentDateTime(void)
{
	int t;
	time_t ct = time(NULL);
	struct tm *p = localtime(&ct);
	
	t = (1+p->tm_mon)*100000000 + p->tm_mday*1000000 + p->tm_hour*10000 + p->tm_min*100 + p->tm_sec;
	return t;
}

#if 0 // Wed Apr 27 20:48:13 CST 2016 
/*
 **************************************************************************************************
 *  ????  ???? AutoMeasureGetFiberLen
 *  ?????????? ?Զ?????ʱ??ѧϰ???ˣ???ȡ???³???
 *  ???ڲ????? MeasureLength_m : ѧϰʱʹ?õ?��?̣?Ĭ????60km
 *             PulseWidth_ns   : ѧϰʱʹ?õ????���Ĭ????640ns
 *  ???ز????? FiberLen        : ???˳??ȣ???λΪm
 *  ???ڰ汾?? 2011-05-21  12:02:34  v1.0
 **************************************************************************************************
*/
uint32_t AutoMeasureGetFiberLen(void)
{
    int32_t   i, len, FpgaStartCount, MeasureTime_ms = 400;
    int32_t   trytimes = 2, Rcv = RCV_HIGH, LastRcv = RCV_HIGH;

_AUTO_RETRY_: 
    printf("AutoMeasureGetFiberLen Start\n");
   
    // ????????FPGA??????????????ʹ??180km/6us???????ջ?��????
    FpgaStartCount = MeasureTime_ms / TIMES_COUNT_ONCE;
    EDMA_Chan_ClrData();                                        // ????????????
    InitApdV();
/************************************** ????FPGA???в??? *****************************************/
    for(i = FpgaStartCount; i > 0; i--)
    {
        get_one_piece_of_data(MAX_POWER_LEVEL, Rcv);
    }
    EDMA_Chan_CopyData();
    OtdrState.TotalMeasureTime = MeasureTime_ms;
/****************************** ???????źŵĽ??????Կ??ٻ?ȡ???? *********************************/
    EstimateFiberLen();
    len = (uint32_t)(OtdrState.SignalEndPoint / OtdrState.Points_1m);
    
    printf("AutoMeasureGetFiberLen (m) : %d, v = %.2f\n", len, OtdrState.AutoEndLevel);
    
    if(trytimes && (len * 1.5 < 15000))    // ???????˳?????15km֮?ڣ???ʹ??10km/80ns??С???ջ??ٲ?һ??
    {
        if(LastRcv != RCV_LOW)      // ??????һ???Զ????Բ???ʹ?ñ????ջ????ſ??Լ?????
        {
            OtdrState.MeasureParam.MeasureLength_m = 10000;
            OtdrState.MeasureParam.PulseWidth_ns   = 80;
            Rcv = RCV_LOW;
            LastRcv = RCV_LOW;
            trytimes--;
            AdaptSampleFreq_PulsePeriod();
            OtdrStateInit();
            goto _AUTO_RETRY_;
        }
    }
    else if(trytimes && (len * 1.5 < 62000))     // ???????˳?????60km֮?ڣ???ʹ??60km/640ns???н??ջ??ٲ?һ??
    {
        if(LastRcv != RCV_MIDDLE)      // ??????һ???Զ????Բ???ʹ?ñ????ջ????ſ??Լ?????
        {
            OtdrState.MeasureParam.MeasureLength_m = 60000;
            OtdrState.MeasureParam.PulseWidth_ns   = 640;
            Rcv = RCV_MIDDLE;
            LastRcv = RCV_MIDDLE;
            trytimes--;
            AdaptSampleFreq_PulsePeriod();
            OtdrStateInit();
            goto _AUTO_RETRY_;
        }
    }
    
    return len;
}

/*
 **************************************************************************************************
 *  ????  ???? MeasureGetCurveConnect
 *  ?????????? ???????ж??Ƿ???Ҫƴ??
 *  ???ڲ????? 
 *  ???ز????? 
 *  ???ڰ汾?? 2012-12-28  11:02:40  v1.0
 **************************************************************************************************
*/
void MeasureGetCurveConnect(void)
{
    int32_t   i, j, Rcv, FpgaStartCount, MeasureTime_ms = 400;
    
    printf("MeasureGetCurveConnect Start\n");
    
    // ????????FPGA??????????
    FpgaStartCount = MeasureTime_ms / TIMES_COUNT_ONCE;
    EDMA_Chan_ClrData();    // ????????????
    
    // ȷ??ʹ?õĽ??ջ?
    i  = GetMeasureLengthIndex(OtdrState.MeasureParam.MeasureLength_m);
    j  = GetPulseWidthIndex(OtdrState.MeasureParam.PulseWidth_ns);
    Rcv = OtdrReceiver[i][j];
    Rcv = (Rcv >> 8) & 0xff;    // ȡ?߹??ʲ??ֵĽ??ջ?
    InitApdV();
/************************************** ????FPGA???в??? *****************************************/
    for(i = FpgaStartCount; i > 0; i--)
    {
        get_one_piece_of_data(MAX_POWER_LEVEL, Rcv);
    }
    EDMA_Chan_CopyData();
    OtdrState.TotalMeasureTime = MeasureTime_ms;
/********************************** ?жϴ??????????Ƿ???Ҫƴ?? ***********************************/
    EstimateCurveConnect();
    
    // ??????ƴ?ӣ???ƴ?ӵ?Ϊ0
    if(!OtdrCtrl.CurveConnect)     OtdrState.CurveConnectPoint = 0;
#if 0
    else if(OtdrCtrl.EnableAutoPower)   // ??????Ҫƴ?ӣ???ʹ???н??ջ???ȫ?????ٲ?һ?Σ????жϳ??н??ջ?Ӧ??ʹ?õĹ???????
    {
        extern int32_t CheckIfFrontSaturate(uint32_t FrontLen);
        uint32_t  j, Rcv, FrontSat, n, power = MAX_POWER_LEVEL;
        
        i  = GetMeasureLengthIndex(OtdrState.MeasureParam.MeasureLength_m);
        j  = GetPulseWidthIndex(OtdrState.MeasureParam.PulseWidth_ns);
        Rcv = OtdrReceiver[i][j] & 0xff;
        while(power > 1)
        {
            printf("MeasureGetCurveConnect Estimate Power Using Rcv = 0x%02x with power = %d\n", Rcv, power);
            EDMA_Chan_ClrData();    // ????????????
/************************************** ????FPGA???в??? *****************************************/
            get_one_piece_of_data(power, Rcv);
            EDMA_Chan_CopyData();
            OtdrState.TotalMeasureTime = TIMES_COUNT_ONCE;
/********************************** ?жϴ??????????Ƿ???Ҫƴ?? ***********************************/
            n = 4*PulseWidthInSampleNum();
            FrontSat = CheckIfFrontSaturate(n);
            if(FrontSat)    power--;
            else            break;
        }
        power = MAX(power, 1);
        OtdrState.AutoPower = power;
    }
#endif
}

void MeasureGetAutoPower(void)
{
    extern int CheckIfAutoPower(void);
#if TR600_A_CLASS
    int minPower = 1;
#else   // TR600_C_CLASS
    int minPower = 2;
#endif

    if(CheckIfAutoPower())
    {
        extern int32_t CheckIfFrontSaturate(uint32_t FrontLen);
        uint32_t  i, j, Rcv, FrontSat, n;
#if TR600_A_CLASS
        int power = 2;
#else   // TR600_C_CLASS
        int power = MAX_POWER_LEVEL;
#endif
        
        // ȷ??ʹ?õĽ??ջ?
        i  = GetMeasureLengthIndex(OtdrState.MeasureParam.MeasureLength_m);
        j  = GetPulseWidthIndex(OtdrState.MeasureParam.PulseWidth_ns);
        Rcv = OtdrReceiver[i][j];
        Rcv &= 0xff;    // ȡ?͹??ʲ??ֵĽ??ջ?
        InitApdV();
        while(power > minPower)
        {
            printf("MeasureGetAutoPower using RCV = %d with power = %d\n", Rcv, power);
            EDMA_Chan_ClrData();    // ????????????
/************************************** ????FPGA???в??? *****************************************/
            get_one_piece_of_data(power, Rcv);
            EDMA_Chan_CopyData();
            OtdrState.TotalMeasureTime = TIMES_COUNT_ONCE;
/********************************** ?жϴ??????????Ƿ???Ҫƴ?? ***********************************/
            n = 4*PulseWidthInSampleNum();
            FrontSat = CheckIfFrontSaturate(n);
            if(FrontSat)    power--;
            else            break;
        }
        power = MAX(power, minPower);
        OtdrState.AutoPower = power;
    }
    else    OtdrState.AutoPower = MAX_POWER_LEVEL;
}
#endif

/*
 **************************************************************************************
 *    End    of    File
 **************************************************************************************
*/
