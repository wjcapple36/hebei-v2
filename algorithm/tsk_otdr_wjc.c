/**
 * @file tsk_otdr.c
 * @synopsis  运行otdr算法，查找事件点的任务
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-07-19
 */
#include <stdio.h>
#include <math.h>

#include "algorithm/Otdr.h"
#include "prototypes.h"
#include "sys/ioctl.h"
#include "../schedule/otdr_ch/otdr_ch.h"
#ifdef __cplusplus
extern "C" {
#endif

extern OTDR_ChannelData_t OtdrData;
extern OtdrCtrlVariable_t OtdrCtrl; 
extern OtdrStateVariable_t OtdrState; 
extern pthread_mutex_t mutex_otdr;

struct _tagThreadInfo tsk_otdr_info;

int32_t tsk_OtdrAlgo(void * arg)
{
	int32_t sleep_time, ch;
	struct _tagThreadInfo *ptsk_info;
	sleep_time = 50000;

	ptsk_info = (struct _tagThreadInfo *)arg;
	//htop 命令显示的就是下面这个东西
	ptsk_info->htop_id = (long int)syscall(224);
	ch = ptsk_info->ch;

	for(;;)
	{
		OtdrCtrl.OtdrAlgoBusyNow   = 0;
        
		while(ALGO_READY_FLAG_START_NEW != OtdrCtrl.OtdrAlgoReadyFlag)
		{
			usleep(sleep_time);      // NOT ready
		}

		if(USER_ACTION_CANCEL == OtdrCtrl.UserAction)    continue;      
		
		OtdrCtrl.OtdrAlgoBusyNow = 1;
		
        pthread_mutex_lock(&mutex_otdr);
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



#ifdef __cplusplus
}
#endif
