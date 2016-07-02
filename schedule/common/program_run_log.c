/*
 **************************************************************************************
 *  文件描述：该文件实现日志读写文件。日志以2进制写入，通过工具转化成文本
 *                  ：文件。每天一个日志文件，一共保存固定天数的文件，超过固定天数
 *                  ：则删除最老的文件。每个文件固定条目，超过条目，从最老的地方
 *                  ：覆盖.该文件引用./protocol/tmsxx.h,./constan.h中定义的常量
 *  文件名字：program_run_log.c
 *  创建者　：文金朝
 *  创建日期：2016-04-15
 *  当前版本：V1
 *
 ***** 修改记录 *****
 *  修改者　：
 *  修改日期：
 *  备注       ：
 **************************************************************************************
 */
#include <time.h>
#include <sys/stat.h>
#include "program_run_log.h"
#include <dirent.h>


#ifdef __cplusplus
extern "C" {
#endif
        pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;
        const  char log_path[] = "./run_log/";
        const char log_suffix[] = ".log";
#define MSG_BLOCK_SIZE  (sizeof(_tagLogMsg) +  TIME_STR_LEN)
        /*
         **************************************************************************************
         *  函数名称：按照当前格式获取系统时间
         *  函数描述：get_sys_time
         *                ：
         *  入口参数：存放字符串的buf, 时间格式
         *  返回参数：如果成功，返回0，其他值不成功
         *  作者       ：wen
         *  日期       ：2016-04-15
         *  修改日期：
         *  修改内容：
         *                ：
         **************************************************************************************
         */
        int get_sys_time(char time_buf[], int offset, const char format[])
        {
                int ret;
                time_t now, usr_now;
                struct tm tm_now, *ptm;
                ret = LOG_RET_SUCCESS;
                now = time(NULL);
                usr_now = now - offset;
                ptm = (struct tm *)localtime_r((const time_t * )(&usr_now),&tm_now);
                if(ptm == NULL){
                        ret = -1;
                        goto usr_exit;
                }
                strftime(time_buf, 20, format, &tm_now);
                //printf("%s(): Line : %d  %x %x \n",  __FUNCTION__, __LINE__, ptm, &tm_now);
usr_exit:
                return ret;
        }
        /*
         **************************************************************************************
         *  函数名称：construct_dir_path
         *  函数描述：构造路径+文件.运行程序路径 + run_log + 日志文件以日期+.txt命名
         *                ：
         *  入口参数：file_name: 文件名字;log_path 日志路径（含文件名字）
         *  返回参数：
         *  作者       ：
         *  日期       ：
         *  修改日期：
         *  修改内容：
         *                ：
         **************************************************************************************
         */
        /*
           int construct_dir_path(char file_name[], char log_path[], int log_path_bytes)
           {
           int ret;
           char log_folder_path[FILE_PATH_LEN ];
           ret = LOG_RET_SUCCESS;
           strcat(log_folder_path, file_name);
           strcpy(log_path, log_folder_path);
usr_exit:
return ret;

}
*/
/*
 **************************************************************************************
 *  函数名称：init_log_dir
 *  函数描述：初始化日志文件目录。如果日志文件目录不存在，则新建目录
 *                ：
 *  入口参数：
 *  返回参数：
 *  作者       ：
 *  日期       ：
 *  修改日期：
 *  修改内容：
 *                ：
 **************************************************************************************
 */
int init_log_dir()
{
        int ret;
        DIR *dir_log;
        ret = LOG_RET_SUCCESS;
        dir_log = opendir(log_path);
        //如果目录为空，就创建
        if(dir_log == NULL)
        {
                mkdir(log_path,0775);
        }
        dir_log = opendir(log_path);
        //如果打开创建后的目录，仍然失败那么返回
        if(dir_log == NULL)
        {
                ret = errno;
                goto usr_exit;
        }

usr_exit:
        if(dir_log != NULL)
                closedir(dir_log);
        return ret;
}
/*
 **************************************************************************************
 *  函数名称：get_log_file_num
 *  函数描述：获取日志文件数目，同时返回修改日期最古老的文件
 *                ：
 *  入口参数：无
 *  返回参数：日志文件数目，最旧的文件
 *  作者       ：2016-04-22
 *  日期       ：
 *  修改日期：
 *  修改内容：
 *                ：
 **************************************************************************************
 */
int get_log_file_num(char oldest_file_path[])
{
        int log_file_num;
        int name_len,ret;
        DIR *dir_log;
        struct dirent *entry;
        struct stat file_stat;
        time_t oldest_time;
        char file_path[FILE_PATH_LEN + FILE_NAME_LEN] = {0};
        log_file_num = 0;
        oldest_time = time(NULL);
        snprintf(file_path,FILE_PATH_LEN,"%s", log_path);
        //打开文件目录
        dir_log = opendir(log_path);
        if(dir_log == NULL){
                goto usr_exit;
        }
        //开始查看日志文件数目
        while ((entry = readdir (dir_log)) != NULL) {
                name_len = strlen(entry->d_name);
                if ( (name_len > FILE_NAME_LEN) || \
                                (NULL == strstr(entry->d_name, log_suffix) ) )
                        continue;
                else{
                        log_file_num++;
                        snprintf(file_path,FILE_PATH_LEN,"%s", log_path);
                        strcat(file_path,entry->d_name);
                        ret = stat(file_path,&file_stat);
                        if(ret != 0){
                                printf("%s(): Line : %d get file %s stat error %d \n",  \
                                                __FUNCTION__, __LINE__,file_path,errno);
                        }
                        else{
                                //posix2008版本以后，已经没有st_mtime,修改为新类型timespec
                                //如果日志不能删除，检查这里
                                if(oldest_time > file_stat.st_mtime){
                                        oldest_time = file_stat.st_mtime;
                                        strcpy(oldest_file_path,file_path);
                                }
                        }
                }
        }
usr_exit:
        if(dir_log != NULL)
                closedir(dir_log);
        return log_file_num;
}

/*
 **************************************************************************************
 *  函数名称：clear_expiry_log
 *  函数描述：清除超过保存时限的文件
 *                ：
 *  入口参数：无
 *  返回参数：无
 *  作者       ：wen
 *  日期       ：2016-04-18
 *  修改日期：
 *  修改内容：
 *                ：
 **************************************************************************************
 */
int clear_expiry_log()
{
        int ret , log_num;
        char expiry_date[TIME_STR_LEN] = {0};
        char oldest_file_path[FILE_PATH_LEN] = {0};
        ret = LOG_RET_SUCCESS;
        //日志文件以年月日命名
        const char* pFormatDate = "%Y-%m-%d";
        log_num = get_log_file_num(oldest_file_path);
        //如果日志数目没有超过保存期限，则直接返回
        if(log_num <= NUM_DATE_SAVE )
                goto usr_exit;
        ret = remove(oldest_file_path);

usr_exit:
        return ret;
}
/*
 **************************************************************************************
 *  函数名称：check_log_head
 *  函数描述：检查日志文件头信息，如果记录最大数目超过设定的最大条目数，或者
 *                ：已保存的条目数超过最大的条目数或负值，均报错误
 *  入口参数：_tagLogHead log_head，日志头
 *  返回参数：0 检查合法，-1 非法
 *  作者       ：wen
 *  日期       ：2016-04-18
 *  修改日期：
 *  修改内容：
 *                ：
 **************************************************************************************
 */
int check_log_head( _tagLogHead log_head)
{
        int ret;
        ret = LOG_RET_SUCCESS;
        //当前保存的索引号
        if((log_head.cur_index < 0)
                        || (log_head.cur_index >= NUM_LOG_RECORD_MAX
                           )) {
                ret = -1;
                goto usr_exit;
        }
        //日志块字节数 信息部分+时间字符串
        if((log_head.block_size < 0)
                        || (log_head.block_size != MSG_BLOCK_SIZE)) {
                ret = -1;
                goto usr_exit;
        }
        //当前日志数目，不能为负或者大于最大记录数目
        if((log_head.cur_num < 0)
                        || (log_head.cur_num > NUM_LOG_RECORD_MAX)) {
                ret = -1;
                goto usr_exit;
        }
        //记录的最大条目
        if(log_head.record_max != NUM_LOG_RECORD_MAX){
                ret = -1;
                goto usr_exit;
        }
usr_exit:
        return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  outputLogTxt 文本方式写日志，直接刻度
 *
 * @param pmsg 指向日志结构体指针
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int outputLogTxt( _tagLogMsg *pmsg)
{    
        int ret, offset, block_size, head_size;
        int bytes;
        bool is_lock;
        char cur_time[TIME_STR_LEN] = {0};
        char log_name[TIME_STR_LEN] = {0};
        char path[FILE_PATH_LEN] = {0};
        const char* pFormatTime = "%Y-%m-%d %H:%M:%S";
        const char* pFormatLog = "%Y-%m-%d";
        FILE *fp;

        is_lock = false;
        fp = NULL;
        ret = LOG_RET_SUCCESS;
        //获取日志名字
        ret = get_sys_time(log_name, 0, pFormatLog);
        if(ret != LOG_RET_SUCCESS)
                goto usr_exit;
        //获取当前时间
        get_sys_time(cur_time, 0, pFormatTime);
        if(ret != LOG_RET_SUCCESS)
                goto usr_exit;

        strcat(path, log_path);
        strcat(path, log_name);
        strcat(path, log_suffix);
        pthread_mutex_lock(&mutex_log);
        is_lock = true;
        fp = fopen(path,"a+");
        if(fp == NULL){
                fp = fopen(path,"wb+");
        }
        if(fp == NULL){
                ret = errno;
                printf("%s(): Line : %d errno %d\n",  __FUNCTION__, __LINE__,errno);
			goto usr_exit;
		}
	fprintf(fp, "%s %s line:%d lev: %d %s \n",cur_time, pmsg->function,\
			pmsg->line,pmsg->lev,pmsg->log_msg);
        fclose(fp);
        fp = NULL;


usr_exit:
        if(is_lock)
                pthread_mutex_unlock(&mutex_log);
        //    printf("%s(): Line : %d  %s 0x %x \n",  __FUNCTION__, __LINE__,path,fp);
        return ret;
}
/*
 **************************************************************************************
 *  函数名称：outoutLog
 *  函数描述：将msg里面的内容保存到日志文件中.只保存n天之内的文件，首先检查n天
 *                ：前的日志文件是否存在，如果存在，则删除。当前文件如果超过规定条目
 *                 ：则进行覆盖操作
 *  入口参数：消息，函数名，行，信息
 *  返回参数：
 *  作者       ：
 *  日期       ：
 *  修改日期：
 *  修改内容：
 *                ：
 **************************************************************************************
 */
int outoutLog( _tagLogMsg msg)
{    
        int ret, offset, block_size, head_size;
        int bytes;
        bool is_lock;
        char cur_time[TIME_STR_LEN] = {0};
        char log_name[TIME_STR_LEN] = {0};
        char path[FILE_PATH_LEN] = {0};
        const char* pFormatTime = "%Y-%m-%d %H:%M:%S";
        const char* pFormatLog = "%Y-%m-%d";
        _tagLogHead log_head;
        FILE *fp;

        is_lock = false;
        fp = NULL;
        ret = LOG_RET_SUCCESS;
        //获取日志名字
        ret = get_sys_time(log_name, 0, pFormatLog);
        if(ret != LOG_RET_SUCCESS)
                goto usr_exit;
        //获取当前时间
        get_sys_time(cur_time, 0, pFormatTime);
        if(ret != LOG_RET_SUCCESS)
                goto usr_exit;

        block_size =  MSG_BLOCK_SIZE;
        head_size = sizeof(_tagLogHead);
        strcat(path, log_path);
        strcat(path, log_name);
        strcat(path, log_suffix);
        pthread_mutex_lock(&mutex_log);
        is_lock = true;
        fp = fopen(path,"rb+");
        if(fp == NULL){
                fp = fopen(path,"wb+");
        }
        if(fp == NULL){
                ret = errno;
                printf("%s(): Line : %d errno %d\n",  __FUNCTION__, __LINE__,errno);
                goto usr_exit;
        }
        fseek(fp, 0, SEEK_SET);
        bytes = fread(&log_head,sizeof(log_head),1,fp);
        //如果读取头信息长度不正确,如果编译器从右到左执行，只能呵呵
        if((bytes != 1) ||
                        (check_log_head(log_head) != LOG_RET_SUCCESS)
          ){
                memset(&log_head, 0, sizeof(log_head));
                //初始化日志头信息,第一次打开日志，肯定会保存一条记录
                log_head.cur_index = 1;
                log_head.cur_num = 1;
                log_head.record_max = NUM_LOG_RECORD_MAX;
                log_head.block_size = block_size;
                offset = 0;
        }
        else{
                offset = log_head.cur_index * block_size;
                //指向下一个存储位置，
                log_head.cur_index ++;
                if(  log_head.cur_index >=  log_head.record_max)
                        log_head.cur_index = 0;
                if(log_head.cur_num < log_head.record_max)
                        log_head.cur_num ++;

        }
        //保存头信息
        fseek(fp, 0, SEEK_SET);
        //ret = ftell(fp);
        bytes = fwrite(&log_head, sizeof(log_head),1,fp);
        //ret = ftell(fp);
        fseek(fp, offset, SEEK_CUR);
        //ret = ftell(fp);
        fwrite(cur_time, TIME_STR_LEN,1,fp);
        //ret = ftell(fp);
        fwrite(&msg, sizeof(msg),1,fp);
        //ret = ftell(fp);
        fclose(fp);
        fp = NULL;


usr_exit:
        if(is_lock)
                pthread_mutex_unlock(&mutex_log);
        //    printf("%s(): Line : %d  %s 0x %x \n",  __FUNCTION__, __LINE__,path,fp);
        return ret;
}
/*
 **************************************************************************************
 *  函数名称：LOGW
 *  函数描述：写日志，根据用户输入的信息构造日志结构体，然后写日志
 *                ：调用写日志的时候会阻塞
 *  入口参数：调用者，函数名字，函数行，日志级别，日志信息
 *  返回参数：写日志结果
 *  作者       ：wen
 *  日期       ：2016-04-21
 *  修改日期：
 *  修改内容：
 *                ：
 **************************************************************************************
 */
int LOGW(const char function_name[], const int line, int lev, char log_msg[])
{
        int ret;
        _tagLogMsg log;
        log.line = line;
        log.lev =lev;
        strncpy((char *)(&log.function), function_name,NUM_CHAR_LOG_FUN);
        snprintf((char *)(&log.log_msg), NUM_CHAR_LOG_MSG, " %s", log_msg);
#if LOG_FORMAT_BIN
        ret = outoutLog(log);
#else
	ret = outputLogTxt(&log);
#endif
        return ret;
}

/*
 **************************************************************************************
 *  函数名称：convert_log2txt
 *  函数描述：将日志里面的内容从二进制转换成txt格式
 *                ：
 *                 ：
 *  入口参数：日志文件所在路径
 *  返回参数：0成功，其他失败
 *  作者       ：wen
 *  日期       ：2016-04-18
 *  修改日期：
 *  修改内容：
 *                ：
 **************************************************************************************
 */
int convert_log2txt(char file_path[])
{
        int ret, file_path_len;
        int bytes, i;
        bool is_lock;
        char path[FILE_PATH_LEN] = {0};
        char time_buf[TIME_STR_LEN];
        _tagLogHead log_head;
        _tagLogMsg msg;
        FILE *fp_bin, *fp_txt;

        is_lock = false;
        fp_bin = NULL;
        fp_txt = NULL;
        ret = LOG_RET_SUCCESS;

        file_path_len = strlen(file_path);
        strncpy(path, file_path, file_path_len - strlen(log_suffix));
        strcat(path, ".txt");
        pthread_mutex_lock(&mutex_log);
        is_lock = true;
        fp_bin = fopen(file_path,"rb+");
        fp_txt = fopen(path,"w+");
        if( (fp_bin == NULL) || (fp_txt == NULL) ){
                ret = errno;
                printf("%s(): Line : %d errno %d \n",  __FUNCTION__, __LINE__,errno);
                goto usr_exit;
        }
        fseek(fp_bin, 0, SEEK_SET);
        bytes = fread(&log_head,sizeof(log_head),1,fp_bin);
        //如果读取头信息长度不正确,如果编译器从右到左执行，只能呵呵
        if((bytes == 0) ||\
                        (check_log_head(log_head) != LOG_RET_SUCCESS)
          ){
                ret = errno;
                printf("%s(): Line : %d errno %d \n",  __FUNCTION__, __LINE__,errno);
                goto usr_exit;
        }
        for(i = 0; i < log_head.cur_num;i++)
        {
                //读取2进制日志
                bytes = fread(time_buf, TIME_STR_LEN, 1, fp_bin);
                bytes = fread(&msg, sizeof(msg), 1, fp_bin);
                if(bytes != 1){
                        ret = errno;
                        printf("%s(): Line : %d errno %d \n",  __FUNCTION__, __LINE__,errno);
                        break;
                }
                //保存成文本
                fprintf(fp_txt, "%s fuction %s line %d log lev : %d: \n", \
                                time_buf,msg.function, msg.line, msg.lev);
                fprintf(fp_txt, " %s \n", msg.log_msg);
        }

        fclose(fp_bin);
        fp_bin = NULL;
        fclose(fp_txt);
        fp_txt = NULL;


usr_exit:
        if(is_lock)
                pthread_mutex_unlock(&mutex_log);
        //    printf("%s(): Line : %d  %s 0x %x \n",  __FUNCTION__, __LINE__,path,fp);
        return ret;
}


#ifdef __cplusplus
}
#endif
