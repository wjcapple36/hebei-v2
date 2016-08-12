// tcp-otdr.c
// Access OTDR using TCP
#include "prototypes.h"
#include "Otdr.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>

#if EDMA_GET_DATA_TOUCH

typedef struct
{
	Uint32  Cmd;            // 命令码 0x9000FFFF
	Uint32  PacketLength;   // 数据长度
	
	Uint32	LastOne;		// 是否最后一条曲线
	Int32	TouchData[DATA_LEN];
	Uint32  RSVD;
}OTDR_UploadTouchData_t;

#define	TOUCH_BUF_SIZE	(sizeof(OTDR_UploadTouchData_t)+1024)
char TouchRxBuf[TOUCH_BUF_SIZE];		// Alloc a large buffer to hold data

OTDR_TouchMeasureParam_t tmp;
static OTDR_UploadTouchData_t	 touchdata;
sem_t sem_touch_cmd, sem_touch_data;

/*
 **************************************************************************************
 *  函数名称： CheckDataAvailable
 *  函数描述： 检查FPGA是否已经采集好数据
 *  入口参数： chan_num : 通道号，从1开始
 *  返回参数： 0 : 数据未采集好
 *             1 : 数据已采集好
 *  日期版本： 2011-02-16  11:35:52  v1.0
 **************************************************************************************
*/
Uint32 CheckDataAvailable(void)
{
    return 1;
}

/*
 **************************************************************************************
 *  函数名称： EDMA_Chan_ClrData
 *  函数描述： 将数组数据清零
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2011-5-7 12:19:04  v1.0
 **************************************************************************************
*/
void EDMA_Chan_ClrData(void)
{
	int i;
    memset(&touchdata, 0, sizeof(touchdata));
	for(i = 0; i < DATA_LEN; i++){
		touchdata.TouchData[i] = 1000*(DATA_LEN -i);
	}
}

int EDMA_Chan_GetData(void)
{
	return 1;
}

void EDMA_Chan_CopyData(void)
{
	sem_wait(&sem_touch_data);
    // 大功率曲线
    memcpy((void*)(OtdrData.ChanData), touchdata.TouchData, DATA_LEN*sizeof(Int32));
    
    if(OtdrCtrl.LowPowerDataChanged)    // 小功率曲线
    {
        memcpy((void*)(OtdrData.LowPowerData) , touchdata.TouchData, DATA_LEN*sizeof(Int32));
        OtdrCtrl.LowPowerDataProcessed = 1;
    }
}

// main function
void Thread_TcpTouch(void)
{
	int HandleOtdrData(char * pbuf, int length);
	int count, touch_sock, this_rxlen, size;
	uint32_t RcvLen, cmd_len, total_rxlen, getFinal;
	struct sockaddr_in sa_otdr_module;
	FrameHeader_t * pFS;

	size = sizeof(sa_otdr_module);
	bzero(&sa_otdr_module, size);
	sa_otdr_module.sin_family = AF_INET;
	sa_otdr_module.sin_port = htons(5000);
	sa_otdr_module.sin_addr.s_addr = inet_addr("192.168.1.253");

	touch_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(touch_sock < 0)
	{
		perror("socket");
		printf("errno = %d\n", errno);
		return;
	}
	printf("Thread_TcpTouch socket ok\n");

	// set receive buffer size to 256K
	count = 256*1024;
	setsockopt(touch_sock, SOL_SOCKET, SO_RCVBUF, &count, sizeof(count));
	
	
	// 绑定IP与socket
	if(connect(touch_sock, (struct sockaddr*)&sa_otdr_module, size) < 0)
	{
		perror("Thread_TcpTouch Connecting OTDR server");
		printf("errno = %d\n", errno);
		return;
	}
	printf("Thread_TcpTouch connect ok\n");

	tmp.Cmd = 0x1000FFFF;
	tmp.PacketLength = 32;
	tmp.RefreshPeriod_ms = 1000;

	// init the sem to share between threads of a process, and the value is 0
	sem_init(&sem_touch_cmd, 0, 0);
	sem_init(&sem_touch_data, 0, 0);

	while(1)
	{
		sem_wait(&sem_touch_cmd);
/************************************ 建立连接，等待命令 *****************************************/
		send_ant(touch_sock, &tmp, sizeof(tmp), 1, 0);
		getFinal = 0;
    	while(!getFinal)
    	{
            bzero(TouchRxBuf, 1024);
            {
                // 接收帧头
                this_rxlen = recv(touch_sock, (char*)TouchRxBuf, sizeof(FrameHeader_t), 0);
                if(this_rxlen < 0)
                {
                    printf("%d : Thread_TcpTouch recv fail, errno = %d\n", __LINE__, errno);
                    break;
                }
                else if(this_rxlen != sizeof(FrameHeader_t))
                {
                    printf("recv TouchRxBuf Error\n");
                    break;
                }
                // 分析是否帧头
                if(0 != strcmp(TouchRxBuf, FRAME_SYNC_STRING))// 不是帧头，以下不处理
                {
                    printf("STATE_CODE_FRAME_SYNC_ERROR\n");
                    continue;
                }
                
                // 获取帧长度及流水号及剩余长度
                pFS    = (FrameHeader_t *)TouchRxBuf;
                RcvLen = pFS->TotalLength;
                cmd_len  = RcvLen - sizeof(FrameHeader_t);    // 剩余数据长度
                
                if(cmd_len <= 0)
                {
                    continue;
                }
                
                // 接收命令数据
                total_rxlen = 0;
                while(1)
                {
                    this_rxlen = recv(touch_sock, (char*)TouchRxBuf+total_rxlen, cmd_len-total_rxlen, 0);
                    if(this_rxlen < 0)
                    {
                        printf("%d : Thread_TcpTouch recv fail, errno = %d\n", __LINE__, errno);
                    	break;
                    }
                    total_rxlen += this_rxlen;
                    if(total_rxlen == cmd_len)   break;
                }
                
                // 处理命令数据并返回
                getFinal = HandleOtdrData(TouchRxBuf, cmd_len);
            }
        }
	}
	close(touch_sock);
}

/*
 **************************************************************************************
 *  函数名称 ： HandleOtdrData
 *  函数描述： 处理接收到的数据
 *  入口参数： 
 *  返回参数： 
 *  日期版本： 2010-08-13  09:34:39
 **************************************************************************************
*/
int HandleOtdrData(char * pbuf, int length)
{
	uint32_t cmd;
    
    printf("\n");
    cmd = *(uint32_t*)pbuf;
    switch(cmd)
    {
/********************************** 配置测试参数并启动测试 ***************************************/
        case 0x9000FFFF :
        {
			memcpy(&touchdata, pbuf, sizeof(touchdata));
			printf("get touch data with LastOne = %d\n", touchdata.LastOne);
			sem_post(&sem_touch_data);
            
    		return touchdata.LastOne;
    	}
/******************************************* 非法命令 ********************************************/
	    default :
	    {
	        printf("CMD : Unknown cmd\n");
	        return 1;
	    }
	}
}

#endif // EDMA_GET_DATA_TOUCH
/*
 **************************************************************************************
 *    End    of    File
 **************************************************************************************
*/
