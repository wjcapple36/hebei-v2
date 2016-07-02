// tcp-otdr.c
// Access OTDR using TCP
#include "prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define	BUF_SIZE	(128*1024)
char rxbuf[BUF_SIZE];		// Alloc a large buffer to hold data
int32_t Thread_MeasureTx_exit;
pthread_mutex_t mutex_network;

// main function
//int main(int argc, char *argv[])
void Thread_TcpSocket(void)
{
	void Thread_MeasureTx(int);
	char str[64];
	int i, tid_measure_tx, count, Xsock, listen_sock, rxlen, this_rxlen, curlen, totalrxlen, size;
	uint32_t RcvLen, cmd_len, PackID, total_rxlen, TxLen;
	struct sockaddr_in sa_otdr, sa_host;
	struct timeval tv;
	FrameHeader_t * pFS;

	size = sizeof(sa_otdr);
	bzero(&sa_otdr, size);
	sa_otdr.sin_family = AF_INET;
	//sa_otdr.sin_len	= size;
	sa_otdr.sin_port = htons(5000);
	sa_otdr.sin_addr.s_addr = INADDR_ANY;

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock < 0)
	{
		perror("socket");
		printf("errno = %d\n", errno);
		return;
	}
	printf("socket ok\n");

	// set receive buffer size to 256K
	count = 256*1024;
	setsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, &count, sizeof(count));
	
	
	// 绑定IP与socket
	int reuse = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(bind(listen_sock, (struct sockaddr*)&sa_otdr, size) < 0)
	{
		perror("bind");
		printf("errno = %d\n", errno);
		return;
	}
	printf("bind ok\n");

	// 监听
	if(listen(listen_sock, 1) < 0)
	{
		perror("listen");
		printf("errno = %d\n", errno);
		return;
	}
	printf("listen ok\n");

	while(1)
	{
		Xsock = accept(listen_sock, (struct sockaddr*)&sa_host, &size);
		if(Xsock < 0)
	   	{
	   	    perror("accept");
			printf("errno = %d\n", errno);
			return;
	   	}
        pthread_mutex_init(&mutex_network, NULL);
        pthread_create(&tid_measure_tx, NULL, Thread_MeasureTx, Xsock);
	    
/************************************ 建立连接，等待命令 *****************************************/
        // 发送超时设定 5s
        tv.tv_sec  = 5;
        tv.tv_usec = 0;
    	setsockopt(Xsock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    	
        // 超时设定 30ms
        tv.tv_sec  = 0;
        tv.tv_usec = 30000;
        
        // 循环等待上位机命令
    	while(1)
    	{
            // 清空缓冲区
            bzero(rxbuf, 1024);
            {
                // 接收帧头
                this_rxlen = recv(Xsock, (char*)rxbuf, sizeof(FrameHeader_t), 0);
                if(this_rxlen < 0)
                {
                    printf("%d : Thread_TcpSocket recv fail, errno = %d\n", __LINE__, errno);
                    break;
                }
                else if(this_rxlen != sizeof(FrameHeader_t))
                {
                    printf("recv rxbuf Error\n");
                    break;
                }
                // 分析是否帧头
                if(0 != strcmp(rxbuf, FRAME_SYNC_STRING))// 不是帧头，以下不处理
                {
                    ResponsePC(Xsock, STATE_CODE_FRAME_SYNC_ERROR, 0);	// 帧头标志错误
                    printf("STATE_CODE_FRAME_SYNC_ERROR\n");
                    continue;
                }
                
                // 获取帧长度及流水号及剩余长度
                pFS    = (FrameHeader_t *)rxbuf;
                RcvLen = pFS->TotalLength;
                PackID = pFS->PacketID;
                cmd_len  = RcvLen - sizeof(FrameHeader_t);    // 剩余数据长度
                
                if(cmd_len <= 0)
                {
                    ResponsePC(Xsock, STATE_CODE_PACKET_LENTH_ERROR, PackID);
                    continue;
                }
                
                // 接收命令数据
                total_rxlen = 0;
                while(1)
                {
                    this_rxlen = recv(Xsock, (char*)rxbuf+total_rxlen, cmd_len-total_rxlen, 0);
                    if(this_rxlen < 0)
                    {
                        printf("%d : Thread_TcpSocket recv fail, errno = %d\n", __LINE__, errno);
                    	break;
                    }
                    total_rxlen += this_rxlen;
                    if(total_rxlen == cmd_len)   break;
                }
                
                // 处理命令数据并返回
                TxLen = ProcessRcvData(Xsock, rxbuf, cmd_len, PackID);
                if(TxLen < 0)
                {
                    printf("ProcessRcvData Socket Error, ErrorNO : %d\n", errno);
                    break;
                }
            }
        }
        Thread_MeasureTx_exit = 1;
        pthread_join(tid_measure_tx);
		close(Xsock);
	}
}



/*
 **************************************************************************************
 *  函数名称： send_ant
 *  函数描述： 调用send来发送缓冲区数据
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2010-09-09  10:53:28  v1.0
 **************************************************************************************
*/
static uint8_t  response_buf[1024];
FrameHeader_t   *Header;
OTDR_State_t    *State;

int32_t send_ant(int s, char *pbuf, int32_t length, uint32_t FrameType, uint32_t PackID)
{
	int32_t txlen;
	
	if(length <= 0)     return -1;
	
	Header = (FrameHeader_t*)response_buf;
	strcpy((void*)Header->FrameSync, FRAME_SYNC_STRING);
	Header->TotalLength = sizeof(FrameHeader_t) + length;
	Header->Rev         = REV_ID;
	Header->FrameType   = FrameType;
	Header->PacketID    = PackID;
	memcpy(response_buf + sizeof(FrameHeader_t), pbuf, length);
	
	pthread_mutex_lock(&mutex_network);
	txlen = send(s, (char *)response_buf, Header->TotalLength, 0);
	pthread_mutex_unlock(&mutex_network);
	
	if(txlen < 0)	perror("send_ant -> send");
	return txlen - sizeof(FrameHeader_t);
}

// 一次只发1K数据，直到数据发送完
#define SLICE_SIZE  1024     // 每个包长度
#define SLEEP_TIME  1

int32_t send_elephant(int s, char * pbuf, int32_t length, uint32_t FrameType, uint32_t PackID)
{
	int32_t this_txlen, txlen, slice;
	
	if(length <= 0)     return -1;
	
	Header = (FrameHeader_t*)response_buf;
	strcpy((void*)Header->FrameSync, FRAME_SYNC_STRING);
	Header->TotalLength = sizeof(FrameHeader_t) + length;
	Header->Rev         = REV_ID;
	Header->FrameType   = FrameType;
	Header->PacketID    = PackID;
	
	pthread_mutex_lock(&mutex_network);
	send(s, (char *)response_buf, sizeof(FrameHeader_t), 0);
	
	txlen = 0;
	while(1)
	{
	    slice = MIN(SLICE_SIZE, length-txlen);
		this_txlen = send(s, pbuf+txlen, slice, 0);
		if(this_txlen < 0)
		{
			perror("send_elephant -> send");
			txlen = this_txlen;
		    break;
		}

		txlen += this_txlen;
		if(txlen == length)    break;
		usleep(SLEEP_TIME*1000);
	}
	
	pthread_mutex_unlock(&mutex_network);
	
	return txlen;
}

/*
 **************************************************************************************
 *  函数名称： ResponsePC
 *  函数描述： 以相应的错误码响应上位机
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2010-10-25  11:15:11  v1.0
 **************************************************************************************
*/
int32_t ResponsePC(int s, uint32_t StateCode, uint32_t PackID)
{
    int32_t len;
	
	Header = (FrameHeader_t*)response_buf;
	strcpy((void*)Header->FrameSync, FRAME_SYNC_STRING);
	Header->TotalLength = sizeof(FrameHeader_t) + sizeof(OTDR_State_t);
	Header->Rev         = REV_ID;
	Header->FrameType   = FRAMETYPE_TARGET2HOST;
	Header->PacketID    = PackID;
	
	State = (OTDR_State_t*)(response_buf + sizeof(FrameHeader_t));
	State->Cmd          = CMD_RESPONSE_STATE;
	State->PacketLength = 4;
	State->StateCode    = StateCode;
	State->RSVD         = RSVD_VALUE;
	
	pthread_mutex_lock(&mutex_network);
	len = send(s, (char *)response_buf, Header->TotalLength, 0);
	pthread_mutex_unlock(&mutex_network);
	
    printf("ResponsePC : %d\n", StateCode);
	return len;
}

/*
 **************************************************************************************
 *  函数名称 ： ProcessRcvData
 *  函数描述： 处理接收到的数据
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2010-08-13  09:34:39
 **************************************************************************************
*/
int ProcessRcvData(int s, char * pbuf, int length, uint32_t PackID)
{
	int PackLen;
	uint32_t cmd;
    
    printf("\n");
    cmd = *(uint32_t*)pbuf;
    switch(cmd)
    {
/********************************** 配置测试参数并启动测试 ***************************************/
        case CMD_HOST_START_MEASURE :
        case CMD_HOST_MEASURE_FIBER :
        case CMD_HOST_GET_RAWDATA :
        case CMD_HOST_GET_DELTA :
        {
            OTDR_MeasureParam_t *pMP;
            
    		printf("CMD_HOST_START_MEASURE\n");
#if 1
    		// 如果当前OTDR正在测试，则返回“非法请求测试”
    		if(OTDR_MODE_IDLE != OtdrCtrl.OtdrMode)
    		{
    		    printf("OTDR Busy now, Measure Denied!\n");
    		    return ResponsePC(s, STATE_CODE_OTDR_BUSY_MEASURE, PackID);
    		}
    		
    		pMP = (OTDR_MeasureParam_t*)pbuf;
    		if((pMP->PacketLength + 12) != sizeof(OTDR_MeasureParam_t))
    		{
    			return ResponsePC(s, STATE_CODE_PACKET_LENTH_ERROR, PackID);
    		}
    		
    		if((pMP->State.n <= 1) || (pMP->State.n >= 2))
    		{
    			return ResponsePC(s, STATE_CODE_N_ERROR, PackID);
    		}
    		
    		// 将网络命令复制到变量 NetWorkMeasureParam 中
    		memcpy((void*)&NetWorkMeasureParam, (void*)pMP, sizeof(NetWorkMeasureParam));
    		
    		if(0 != NetWorkMeasureParam.State.MeasureLength_m)     // 手动测试，不是自动测试，要检查参数
    		{
        		if(CheckRunParamValid(NetWorkMeasureParam.State.MeasureLength_m, NetWorkMeasureParam.State.PulseWidth_ns) == 0)  // 参数非法
        		{
        			return ResponsePC(s, STATE_CODE_ML_OR_PW_ERROR, PackID);
        		}
        	}
    		
    		if(CMD_HOST_GET_RAWDATA == cmd)     OtdrCtrl.RawDataLevel = NetWorkMeasureParam.Ctrl.RSVD;
    		else if(CMD_HOST_GET_DELTA == cmd)  OtdrCtrl.RawDataLevel = NetWorkMeasureParam.Ctrl.RSVD;//DATA_DELTA;
    		else                                OtdrCtrl.RawDataLevel = 0xffffffff;
    		
    		// 为浙大网新添加一个只计算光纤长度和事件点的测试目的
    		if(CMD_HOST_MEASURE_FIBER == cmd)     OtdrCtrl.MeasurePurpose = CMD_HOST_MEASURE_FIBER;
    		else    OtdrCtrl.MeasurePurpose = 0;
    		
    		if(OTDR_MODE_AVG == NetWorkMeasureParam.Ctrl.OtdrMode)    		    OtdrCtrl.OtdrMode = OTDR_MODE_AVG;
    		else if(OTDR_MODE_REALTIME == NetWorkMeasureParam.Ctrl.OtdrMode)    OtdrCtrl.OtdrMode = OTDR_MODE_REALTIME;
	        
            printf("CMD_HOST_START_MEASURE started\n");
            printf("模式 = %d\n", NetWorkMeasureParam.Ctrl.OtdrMode);
            printf("刷新 = %d\t周期 = %dms\n", NetWorkMeasureParam.Ctrl.EnableRefresh, NetWorkMeasureParam.Ctrl.RefreshPeriod_ms);
            printf("波长 = %dnm\n", NetWorkMeasureParam.State.Lambda_nm);
            printf("量程 = %dm\n", NetWorkMeasureParam.State.MeasureLength_m);
            printf("脉宽 = %dns\n", NetWorkMeasureParam.State.PulseWidth_ns);
            printf("时间 = %dms\n", NetWorkMeasureParam.State.MeasureTime_ms);
#endif
    		return ResponsePC(s, STATE_CODE_CMD_OK, PackID);
    	}
/*********************************** 取消或终止测试命令 ******************************************/
    	case CMD_HOST_STOP_MEASURE :
	    {
	        OTDR_Cancel_t *pCL;
	        
    		printf("CMD_HOST_STOP_MEASURE\n");
#if 1
    		pCL = (OTDR_Cancel_t*)pbuf;
    		if((pCL->PacketLength + 12) != sizeof(OTDR_Cancel_t))
    		{
    			return ResponsePC(s, STATE_CODE_PACKET_LENTH_ERROR, PackID);
    		}
    		
    		if((USER_ACTION_CANCEL == pCL->Cancel_Or_Abort) || (USER_ACTION_STOP == pCL->Cancel_Or_Abort))
    		{
    		    OtdrCtrl.UserAction = pCL->Cancel_Or_Abort;
    		    
    		    // 等待状态变成空闲
                while(OTDR_MODE_IDLE != OtdrCtrl.OtdrMode)
                {
                    usleep(20);
                }
            }
#endif
            // USER_ACTION_ABORT 则不响应，直接上传最终数据， USER_ACTION_CANCEL 才响应 2011-6-28 17:26:08
            if(USER_ACTION_CANCEL == pCL->Cancel_Or_Abort)
            {
                return ResponsePC(s, STATE_CODE_CMD_OK, PackID);
            }
            else	return 1;
	    }

/************************************* 查询电池电量命令 ******************************************/
    	case CMD_HOST_GET_BATTERY :
	    {
	        extern uint32_t GetBatteryLevel(void);
	        OTDR_UploadBattery_t Battery, *pBat;
	        
	        printf("CMD_HOST_GET_BATTERY\n");
#if 1
    		pBat = (OTDR_UploadBattery_t*)pbuf;
    		if(pBat->PacketLength != 0)
    		{
    			return ResponsePC(s, STATE_CODE_PACKET_LENTH_ERROR, PackID);
    		}
    		
    		// 读取电池电量值
    		Battery.Cmd = CMD_DSP_UPLOAD_BATTERY;
    		Battery.PacketLength = sizeof(uint32_t);
    		Battery.Data = 4;
    		Battery.RSVD = RSVD_VALUE;
#endif
            // 上传数据
            return send_ant(s, (char *)&Battery, sizeof(Battery), FRAMETYPE_TARGET2HOST, PackID);
	    }
/**************************************** 网络空闲置位 *******************************************/
        case CMD_HOST_NETWORK_IDLE :
        {
    		printf("CMD_HOST_NETWORK_IDLE, cpu time is \n");
    		
    		// 标记网络空闲
    		OtdrCtrl.NetWorkBusyNow = 0;
            
    		return 1;
    	}
/**************************************** 读取版本信息 *******************************************/
#if 0
        case CMD_HOST_GET_VERSION :
        {
    		OTDR_UploadVersion_t    ver;
    		
    		ver.Cmd = CMD_DSP_UPLOAD_BATTERY;
    		ver.PacketLength = sizeof(ver) - 8;
    		ver.Major_Minor  = (VERSION_MAJOR << 8) | (VERSION_MINOR);
    		
        	// 代码编译日期
        	ver.BuildYear = bt_year;
            ver.BuildMon  = bt_mon;
            ver.BuildDay  = bt_day;
            
            // 代码编译时间     高16位为小时(0~23)，低16位里的高8位为分钟(0~59)，低8位为秒(0~59)
            ver.BuildTime = (bt_hour << 16) | (bt_min << 8) | bt_sec;
            
            memset(ver.Hardware, 0, sizeof(ver.Hardware));
            strcpy(ver.Hardware, "GL3800M");
        
    		ver.RSVD = RSVD_VALUE;
    		
            // 上传数据
            return send_ant(s, (char *)&ver, sizeof(ver), FRAMETYPE_TARGET2HOST, PackID);
    	}
#endif
/******************************************* 非法命令 ********************************************/
	    default :
	    {
	        printf("CMD : Unknown cmd\n");
	        return ResponsePC(s, STATE_CODE_CMD_ID_ERROR, PackID);
	    }
	}
	return 1;
}


/*************************************************************************************************/
/************************************* 数据发送缓冲帧 ********************************************/
/*************************************************************************************************/
#define MAX_FRAME_COUNT 10
typedef struct
{
    char    *FrameBufAddr;  // 帧起始地址
    uint32_t  FrameLen;       // 帧长度
}MeasureDataFrame_t;

// 定义数据帧变量以记录帧的地址以及长度，它们作为队列的元素
MeasureDataFrame_t DataFrameQ[MAX_FRAME_COUNT];
static int32_t FrameIn, FrameOut, FrameQFull, FrameQEmpty, FrameQBusy, DataFrameQServerOpen;

/*
 **************************************************************************************************
 *  函数  名： PutDataFrame
 *  函数描述： 向队列插入数据帧
 *  入口参数： buf : 数据指针
 *             len : 数据长度
 *  返回参数： 
 *  日期版本： 2011-05-19  21:42:34  v1.0
 **************************************************************************************************
*/
void PutDataFrame(char *buf, uint32_t len)
{
    char *BufTemp;
    int32_t i;
    
    if(!DataFrameQServerOpen)    return;
    
    // 向系统申请内存，如果失败，最多等待250ms
    for(i = 0; i < 5; i++)
    {
        BufTemp = (char*)malloc(len);
        if(NULL == BufTemp)     usleep(50);
        else                    break;
    }
    // 内存不足，出错
    if(NULL == BufTemp)
    {
        printf("malloc fail in PutDataFrame, FrameIn = %d\n", FrameIn);
        return;
    }
    
    // 申请成功，将数据复制到缓冲区
    memcpy(BufTemp, buf, len);
    
    // 查看队列是否有空闲，否则等待队列有空
    for(i = 0; i < 5; i++)
    {
        if(FrameQFull)  usleep(50);    // 如果队列满则等待，最多250ms
        else            break;
    }
    
    // 获取信号量    最多等待250毫秒
    for(i = 0; i < 5; i++)
    {
        if(FrameQBusy)      usleep(50);
        else                break;
    }
    FrameQBusy = 1;
    
    // 如果队列仍然没有空闲，则丢弃最新的那一帧数据
    if(FrameQFull)
    {
        printf("FrameQ full, discard the latest frame. FrameIn = %d\n", FrameIn);
        
        FrameIn--;
        if(FrameIn < 0)     FrameIn = MAX_FRAME_COUNT-1;
        free(DataFrameQ[FrameIn].FrameBufAddr);
        DataFrameQ[FrameIn].FrameBufAddr = NULL;
        DataFrameQ[FrameIn].FrameLen     = 0;
    }
    
    // 插入数据帧
    DataFrameQ[FrameIn].FrameBufAddr = BufTemp;
    DataFrameQ[FrameIn++].FrameLen   = len;
    FrameQEmpty = 0;
    if(FrameIn >= MAX_FRAME_COUNT)  FrameIn -= MAX_FRAME_COUNT;
    if(FrameIn == FrameOut)     FrameQFull = 1;

    FrameQBusy = 0;
}

/*
 **************************************************************************************************
 *  函数  名： SendDataFrame
 *  函数描述： 从队列里取得一帧数据并发送
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-05-19  21:58:59  v1.0
 **************************************************************************************************
*/
int32_t SendDataFrame(int s, uint32_t FrameType, uint32_t PackID)
{
    int32_t i, FrameLen, TxLen;
    char *FrameBufAddr;
    
    // 如果当前没有元素，则返回
#if 0   // 检查网络繁忙
    if((FrameQEmpty) || (OtdrCtrl.NetWorkBusyNow))  return 0x7fffffff;  // 返回一个最大的正数指示空
#else   // 不检查网络繁忙
    if(FrameQEmpty)                                 return 0x7fffffff;  // 返回一个最大的正数指示空
#endif

    // 发送当前帧并释放其空间
    //OtdrCtrl.NetWorkBusyNow = 1;    // 标记网络繁忙
    FrameBufAddr  = DataFrameQ[FrameOut].FrameBufAddr;
    FrameLen      = DataFrameQ[FrameOut].FrameLen;
    TxLen         = send_elephant(s, FrameBufAddr, FrameLen, FrameType, PackID);
    free(FrameBufAddr);
    
    // 等待250毫秒
    for(i = 0; i < 5; i++)
    {
        if(FrameQBusy)      usleep(50);
        else                break;
    }
    
	FrameQBusy = 1;
    DataFrameQ[FrameOut].FrameBufAddr = NULL;
    DataFrameQ[FrameOut].FrameLen     = 0;
    FrameQFull = 0;
    FrameOut++;
    if(FrameOut >= MAX_FRAME_COUNT)     FrameOut -= MAX_FRAME_COUNT;
    if(FrameIn == FrameOut)             FrameQEmpty = 1;
    FrameQBusy = 0;

    return TxLen;
}

/*
 **************************************************************************************************
 *  函数  名： FreeFrameQ
 *  函数描述： 释放队列空间
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-05-19  22:12:31  v1.0
 **************************************************************************************************
*/
void FreeFrameQ(void)
{
    int32_t i;
    
    // 等待250毫秒
    for(i = 0; i < 5; i++)
    {
        if(FrameQBusy)      usleep(50);
        else                break;
    }
    FrameQBusy = 1;
    
    if(!FrameQEmpty)
    {
        if(FrameOut < FrameIn)
        {
            for(; FrameOut < FrameIn; FrameOut++)
            {
                if(NULL != DataFrameQ[FrameOut].FrameBufAddr)
                {
                    free(DataFrameQ[FrameOut].FrameBufAddr);
                    DataFrameQ[FrameOut].FrameBufAddr = NULL;
                    DataFrameQ[FrameOut].FrameLen     = 0;
                }
            }
        }
        else if(FrameOut > FrameIn)
        {
            for(; FrameOut < MAX_FRAME_COUNT; FrameOut++)
            {
                if(NULL != DataFrameQ[FrameOut].FrameBufAddr)
                {
                    free(DataFrameQ[FrameOut].FrameBufAddr);
                    DataFrameQ[FrameOut].FrameBufAddr = NULL;
                    DataFrameQ[FrameOut].FrameLen     = 0;
                }
            }
            for(FrameOut = 0; FrameOut < FrameIn; FrameOut++)
            {
                if(NULL != DataFrameQ[FrameOut].FrameBufAddr)
                {
                    free(DataFrameQ[FrameOut].FrameBufAddr);
                    DataFrameQ[FrameOut].FrameBufAddr = NULL;
                    DataFrameQ[FrameOut].FrameLen     = 0;
                }
            }
        }
    }
    memset(DataFrameQ , 0, sizeof(DataFrameQ));
	FrameIn     = 0;
	FrameOut    = 0;
	FrameQFull  = 0;
	FrameQEmpty = 1;
	FrameQBusy  = 0;
}

/*
 **************************************************************************************
 *  函数名称： Thread_MeasureTx
 *  函数描述： 向上位机上传测试结果数据，参看《contest.c》里的函数 ShareTest()
 *  入口参数： 
 *  返回参数：      
 *  日期版本： 2010-12-14  09:14:47  v1.0
 **************************************************************************************
*/
#define     SleepSlice      10000  // 时间片为10ms
void Thread_MeasureTx(int s)
{
	int32_t TxLen;
	struct timeval  tv;
	char tbuf[32];
	
	memset(DataFrameQ , 0, sizeof(DataFrameQ));
	FrameIn     = 0;
	FrameOut    = 0;
	FrameQFull  = 0;
	FrameQEmpty = 1;
	FrameQBusy  = 0;

	printf("Thread_MeasureTx Created\n");
	strcpy(tbuf, "this is the Thread_MeasureTx thread\n");

	// 发送超时设定 2s
    tv.tv_sec  = 2;
    tv.tv_usec = 0;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    DataFrameQServerOpen = 1;
    Thread_MeasureTx_exit = 0;
	while(1)
	{
        // 使用队列缓冲发送
		TxLen = SendDataFrame(s, FRAMETYPE_TARGET2HOST, 0);
		if(TxLen <= 0)
		{
		    //OtdrCtrl.UserAction = USER_ACTION_CANCEL;
			printf("Thread_MeasureTx Socket Error, SendDataFrame failed. ErrorNO = %d\n", errno);
			FreeFrameQ();
			break;
		}

		usleep(SleepSlice);
		
		// 检查网络连接
		if(Thread_MeasureTx_exit)    break;
	}
	
	DataFrameQServerOpen = 0;
	printf("Thread_MeasureTx exit\n");
	pthread_exit();
}

#if 0 // 暂时不启用各种服务功能

/*
 **************************************************************************************
 *  函数名称： TskFxn_EchoServer   TskFxn_EchoClient
 *  函数描述： echo服务器，用来检验TCP通信是否仍然工作
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2014-4-21 17:44:45
 **************************************************************************************
*/
void TskFxn_EchoServer(void)
{
#if 1
	struct sockaddr_in  svr_addr, cli_addr;
	int       	    s, stcp;
	int32_t               size, rc;
	char                tempbuf[64];
	
	size = sizeof(struct sockaddr);
	
	bzero(&svr_addr, size);
    svr_addr.sin_family      = AF_INET;
    svr_addr.sin_len         = sizeof(svr_addr);
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port        = htons(5555);
	
	// 打开环境并创建socket
	fdOpenSession(TaskSelf());
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)     // 如果出错不应该退出，而应该等待一定时间后再重新打开本函数
	   	goto _FUCK_ERROR_ECHOSERVER_;             // 如果本函数打开一定次数后仍失败，则重启
	
	// 绑定IP与socket
	if(bind(s, (PSA)&svr_addr, size) == SOCKET_ERROR)
		goto _FUCK_ERROR_ECHOSERVER_;
	
	// 监听
	if(listen(s, 1) == SOCKET_ERROR)
		goto _FUCK_ERROR_ECHOSERVER_;

	// 为了实现自由断开与连接,一直进行accept
    while(1)
    {
        memset(tempbuf, 0, sizeof(tempbuf));
        stcp = accept(s, (PSA)&cli_addr, &size);
		if(stcp == INVALID_SOCKET)
	   	{
	   	    fdClose(stcp);
	   		goto _FUCK_ERROR_ECHOSERVER_;
	   	}
	   	while(1)
	   	{
	   	    OtdrCtrl.IdelTime = 0;      // 用户有操作，空闲时间置0
    	   	rc = recv(stcp, tempbuf, sizeof(tempbuf), 0);
    	   	if(rc <= 0)
    	   	{
    	   	    fdClose(stcp);
    	   	    break;
    	   	}
    	   	
    	   	if(send(stcp, tempbuf, rc, 0) <= 0)
    	   	{
    	   	    fdClose(stcp);
    	   	    break;
    	   	}
    	   	usleep(10);
    	}
    }

_FUCK_ERROR_ECHOSERVER_:
    fdClose(s);
	fdCloseSession(TaskSelf());
#endif
}

void TskFxn_EchoClient(void)
{
#if 0
	struct sockaddr_in  svr_addr;
	struct timeval      tv;
	int       	    s;
	int32_t               result, ec, size, sn = 0;
	char                tempbuf[8];
	
	usleep(1000);
	size = sizeof(struct sockaddr);
	bzero(&svr_addr, size);
    svr_addr.sin_family      = AF_INET;
    svr_addr.sin_len         = sizeof(svr_addr);
    svr_addr.sin_addr.s_addr = inet_addr("192.168.1.249");//inet_addr(LocalIPAddr);  // 目的IP地址是本机的IP地址，即读取本机的信息
    svr_addr.sin_port        = htons(5555);                // 端口为7，即本机的echo端口

	// 打开环境并创建socket
	fdOpenSession(TaskSelf());
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)     // 如果出错不应该退出，而应该等待一定时间后再重新打开本函数
	{
	    printf("socket error in TskFxn_EchoClient\n");
	   	goto _FUCK_ERROR_ECHOCLIENT_;
	}
	
	// connect
	if(connect(s, (PSA)&svr_addr, size) == SOCKET_ERROR)
	{
	    printf("connect error in TskFxn_EchoClient\n");
	   	goto _FUCK_ERROR_ECHOCLIENT_;
	}
	
	// 设置超时 1s
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	// 读取数据
	ec = 0;
	while(1)
	{
	    usleep(1500);
	    memset(tempbuf, 0, sizeof(tempbuf));
	    
	    if(ec >= 3)
	    {
	        printf("FATAL error, program exit\n");
	        c_int00();
	        break; 
	    }
	    
	    result = send(s, "Hello", 5, 0);
	    if(result < 0)
        {
            printf("%d : TskFxn_EchoClient send fail, ErrorNO = %d\n", __LINE__, fdError());
            printf("%d : TskFxn_EchoClient send fail, ErrorNO = %d\n", __LINE__, fdError());
            ec++;
            continue;
        }
        else if(result == 0)
        {
            printf("%d : TskFxn_EchoClient send timeout, ErrorNO = %d\n", __LINE__, fdError());
            ec++;
            continue;
        }
        
        result = recv(s, tempbuf, sizeof(tempbuf), 0);
        if(result < 0)
        {
            printf("%d : TskFxn_EchoClient recv fail, ErrorNO = %d\n", __LINE__, fdError());
            printf("%d : TskFxn_EchoClient recv fail, ErrorNO = %d\n", __LINE__, fdError());
            ec++;
            continue;
        }
        else if(result == 0)
        {
            printf("%d : TskFxn_EchoClient recv timeout, ErrorNO = %d\n", __LINE__, fdError());
            ec++;
            continue;
        }
        else
        {
            OtdrCtrl.IdelTime = 0;
            printf("recv %d: %s\n", sn, tempbuf);
            printf("recv %d : %s\n", sn, tempbuf);
            ec = 0;
            sn++;
        }
	}
_FUCK_ERROR_ECHOCLIENT_:
    fdClose(s);
	fdCloseSession(TaskSelf());
#endif
}

/*
 **************************************************************************************
 *  函数名称： TskFxn_DayTimeServer   TskFxn_DayTimeClient
 *  函数描述： daytime服务器，用来检验TCP通信是否仍然工作
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2014-4-23 15:32:52
 **************************************************************************************
*/
void TskFxn_DayTimeServer(void)
{
#if 1
	struct sockaddr_in  svr_addr, cli_addr;
	int       	    s, stcp;
	int32_t               size, rc;
	char                tempbuf[16];
	
	size = sizeof(struct sockaddr);
	
	bzero(&svr_addr, size);
    svr_addr.sin_family      = AF_INET;
    svr_addr.sin_len         = sizeof(svr_addr);
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port        = htons(6666);
	
	// 打开环境并创建socket
	fdOpenSession(TaskSelf());
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)
	   	goto _FUCK_ERROR_DAYTIMESERVER_;
	
	// 绑定IP与socket
	if(bind(s, (PSA)&svr_addr, size) == SOCKET_ERROR)
		goto _FUCK_ERROR_DAYTIMESERVER_;
	
	// 监听
	if(listen(s, 1) == SOCKET_ERROR)
		goto _FUCK_ERROR_DAYTIMESERVER_;

	// 为了实现自由断开与连接,一直进行accept
    while(1)
    {
        memset(tempbuf, 0, sizeof(tempbuf));
        stcp = accept(s, (PSA)&cli_addr, &size);
		if(stcp == INVALID_SOCKET)
	   	{
	   	    fdClose(stcp);
	   		goto _FUCK_ERROR_DAYTIMESERVER_;
	   	}
	   	
	   	OtdrCtrl.IdelTime = 0;      // 用户有操作，空闲时间置0
	   	
        rc = sprintf(tempbuf, "%d", CLK_getltime());
        send(stcp, tempbuf, rc, 0 );
        fdClose(stcp);
        usleep(10);
    }

_FUCK_ERROR_DAYTIMESERVER_:
    fdClose(s);
	fdCloseSession(TaskSelf());
#endif
}

void TskFxn_DayTimeClient(void)
{
#if 0
	struct sockaddr_in  svr_addr;
	struct timeval      tv;
	int       	    s;
	int32_t               result, ec, size, sn = 0;
	char                tempbuf[8];
	
	usleep(1000);
	size = sizeof(struct sockaddr);
	bzero(&svr_addr, size);
    svr_addr.sin_family      = AF_INET;
    svr_addr.sin_len         = sizeof(svr_addr);
    svr_addr.sin_addr.s_addr = inet_addr("192.168.1.35");//inet_addr(LocalIPAddr);  // 目的IP地址是本机的IP地址，即读取本机的信息
    svr_addr.sin_port        = htons(6666);

	// 打开环境并创建socket
	fdOpenSession(TaskSelf());
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)     // 如果出错不应该退出，而应该等待一定时间后再重新打开本函数
	{
	    printf("socket error in TskFxn_EchoClient\n");
	   	goto _FUCK_ERROR_DAYTIMECLIENT_;
	}
	
	// connect
	if(connect(s, (PSA)&svr_addr, size) == SOCKET_ERROR)
	{
	    printf("connect error in TskFxn_EchoClient\n");
	   	goto _FUCK_ERROR_DAYTIMECLIENT_;
	}
	
	// 设置超时 1s
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	// 读取数据
	ec = 0;
//	while(1)
	{
	    usleep(1500);
	    
        result = recv(s, tempbuf, sizeof(tempbuf), 0);
        if(result < 0)
        {
            printf("%d : TskFxn_EchoClient recv fail, ErrorNO = %d\n", __LINE__, fdError());
            printf("%d : TskFxn_EchoClient recv fail, ErrorNO = %d\n", __LINE__, fdError());
            ec++;
            continue;
        }
        else if(result == 0)
        {
            printf("%d : TskFxn_EchoClient recv timeout, ErrorNO = %d\n", __LINE__, fdError());
            ec++;
            continue;
        }
        else
        {
            printf("recv %d: %s\n", sn, tempbuf);
            printf("recv %d : %s\n", sn, tempbuf);
            ec = 0;
            sn++;
        }
	}
_FUCK_ERROR_DAYTIMECLIENT_:
    fdClose(s);
	fdCloseSession(TaskSelf());
#endif
}

/*
 **************************************************************************************
 *  函数名称： TskFxn_DiscardServer
 *  函数描述： discard服务器，用来检验TCP通信是否仍然工作
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2014-4-24 14:55:43
 **************************************************************************************
*/
void TskFxn_DiscardServer(void)
{
#if 0
	struct sockaddr_in  svr_addr, cli_addr;
	int       	    s, stcp;
	int32_t               size, rc;
	char                tempbuf[64];
	
	size = sizeof(struct sockaddr);
	
	bzero(&svr_addr, size);
    svr_addr.sin_family      = AF_INET;
    svr_addr.sin_len         = sizeof(svr_addr);
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port        = htons(7777);
	
	// 打开环境并创建socket
	fdOpenSession(TaskSelf());
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)
	   	goto _FUCK_ERROR_DISCARDSERVER_;
	
	// 绑定IP与socket
	if(bind(s, (PSA)&svr_addr, size) == SOCKET_ERROR)
		goto _FUCK_ERROR_DISCARDSERVER_;
	
	// 监听
	if(listen(s, 1) == SOCKET_ERROR)
		goto _FUCK_ERROR_DISCARDSERVER_;

	// 为了实现自由断开与连接,一直进行accept
    while(1)
    {
        memset(tempbuf, 0, sizeof(tempbuf));
        stcp = accept(s, (PSA)&cli_addr, &size);
		if(stcp == INVALID_SOCKET)
	   	{
	   	    fdClose(stcp);
	   		goto _FUCK_ERROR_DISCARDSERVER_;
	   	}
	   	
	   	while(1)
	   	{
	   	    OtdrCtrl.IdelTime = 0;      // 用户有操作，空闲时间置0
    	   	rc = recv(stcp, tempbuf, sizeof(tempbuf), 0);
    	   	if(rc <= 0) break;
    	}
    	
        fdClose(stcp);
        usleep(10);
    }

_FUCK_ERROR_DISCARDSERVER_:
    fdClose(s);
	fdCloseSession(TaskSelf());
#endif
}

/*
 **************************************************************************************
 *  函数名称： TskFxn_ResetServer
 *  函数描述： Reset服务器，只要上位机连接到该端口，则直接reset
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2014-4-23 15:32:52
 **************************************************************************************
*/
void TskFxn_ResetServer(void)
{
	struct sockaddr_in  svr_addr, cli_addr;
	int       	    s, stcp;
	int32_t               size;
	
	size = sizeof(struct sockaddr);
	
	bzero(&svr_addr, size);
    svr_addr.sin_family      = AF_INET;
    svr_addr.sin_len         = sizeof(svr_addr);
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port        = htons(4444);
	
	// 打开环境并创建socket
	fdOpenSession(TaskSelf());
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)
	   	goto _FUCK_ERROR_RESETSERVER_;
	
	// 绑定IP与socket
	if(bind(s, (PSA)&svr_addr, size) == SOCKET_ERROR)
		goto _FUCK_ERROR_RESETSERVER_;
	
	// 监听
	if(listen(s, 1) == SOCKET_ERROR)
		goto _FUCK_ERROR_RESETSERVER_;

	// 为了实现自由断开与连接,一直进行accept
    while(1)
    {
        stcp = accept(s, (PSA)&cli_addr, &size);
		if(stcp == INVALID_SOCKET)
	   	{
	   	    fdClose(stcp);
	   		goto _FUCK_ERROR_RESETSERVER_;
	   	}
	   	
	   	OtdrCtrl.IdelTime = 0;      // 用户有操作，空闲时间置0
	   	
        c_int00();
        fdClose(stcp);
        usleep(10);
    }

_FUCK_ERROR_RESETSERVER_:
    fdClose(s);
	fdCloseSession(TaskSelf());
}

#endif  // 暂时不启用各种服务功能
/*
 **************************************************************************************
 *  函数名称： GetOtdrStatus
 *  函数描述： 读取OTDR当前状态
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2016-03-26 18:32:52
 **************************************************************************************
*/
otdr_status_t status;
void GetOtdrStatus(void)
{
	char buf[128];
	
	memset(&status, 0, sizeof(status));
	strcpy(status.ver, PROJECT_NAME);

#if TR600_A_CLASS
	strcat(status.ver, "-A");
#else
	strcat(status.ver, "-C");
#endif
	
	status.otdr_mode = OtdrCtrl.OtdrMode;
	status.measure_length = OtdrState.MeasureParam.MeasureLength_m;
	status.pulse_width = OtdrState.MeasureParam.PulseWidth_ns;
	status.measure_time = OtdrState.MeasureParam.MeasureTime_ms;
	status.done_time = OtdrState.TotalMeasureTime;
	status.frame_in = FrameIn;
	status.frame_out = FrameOut;
	
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s : Mode(%d), %dkm%dns%ds, %.2f%%, frameq(in %d, out %d)", status.ver, status.otdr_mode, status.measure_length/1000,\
		status.pulse_width, status.measure_time/1000, 100.0f*status.done_time/status.measure_time, status.frame_in, status.frame_out);
	printf("%s\n", buf);
}

/*
 **************************************************************************************
 *    End    of    File
 **************************************************************************************
*/
