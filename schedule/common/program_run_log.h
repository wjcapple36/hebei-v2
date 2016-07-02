/*
 **************************************************************************************
 *  文件描述：声明程序运行写读写日志以及将日志从二进制转换成文本文件的头文件
 *                  ：该文件引用./protocol/tmsxx.h ./constant.h文件中定义的常量，移植的时候
 *                  ：须注意
 *  文件名字：program_run_log.h
 *  创建者　：文金朝
 *  创建日期：2016-04-14
 *  当前版本：
 *
  ***** 修改记录 *****
 *  修改者　：
 *  修改日期：
 *  备注       ：
 **************************************************************************************
*/

#ifndef PROGRAM_RUN_LOG_H
#define PROGRAM_RUN_LOG_H
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include<errno.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 *2016-04-14 处理逻辑 保存固定条目，如果超过固定条目，循环覆盖
 *日志头里面保存了允许保存的最大条目，当前保存的位置，以及已
 *经保存的数目
*/
//2015-04-14 日志信息
//文件字符长度
#define FILE_PATH_LEN	256
//文件名字长度
#define FILE_NAME_LEN	64
//时间字符串长度
#define TIME_STR_LEN	20	
#define NUM_CHAR_LOG_MSG  128
#define NUM_CHAR_LOG_FUN 64
#define LOG_FORMAT_BIN	0 /*如果等于1，日志文件以2进制保存，0 为文本*/
#define NUM_LOG_RECORD_MAX          5000/*2进制文件的时候一个日志文件日志条目*/
#define NUM_DATE_SAVE           5           //保存几天的日志
#define NUM_SECOND_LOG_SAVE      (NUM_DATE_SAVE *24*60*60)  //将NUM_DATE_SAVE转换成秒
#define LOG_RET_SUCCESS	0
//日志正常提示信息，比如登录，配置等操作
#define LOG_LEV_USUAL_MSG          0
//严重错误信息，比如分配内存失败之类的特别严重的影响程序运行的错误
#define LOG_LEV_FATAL_ERRO    1
//程序运行中的异常，参数检查发现问题类似
#define LOG_LEV_USUAL_ERRO   2
typedef struct
{
    //char time[TIME_STR_LEN]; //时间，封装到函数内部
    char function[NUM_CHAR_LOG_FUN]; //函数名字
    int line;//行数
    int lev;//日志级别
    char log_msg[NUM_CHAR_LOG_MSG]; //日志信息
}_tagLogMsg;
//日志头
typedef struct
{
    int record_max;
    int cur_index;
    int cur_num;
    int block_size;
}_tagLogHead;
extern pthread_mutex_t mutex_log;
//写日志
int outoutLog( _tagLogMsg msg);
//初始化日志目录：检查目录是否存在，如果不存在则新建
int init_log_dir();
//清除过期的日志
int clear_expiry_log();
//写日志，另一接口
int LOGW(const char function_name[], const int line, int lev,  char log_msg[]);


#ifdef __cplusplus
}
#endif

#endif // PROGRAM_RUN_LOG_H
