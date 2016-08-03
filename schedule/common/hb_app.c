/**
 * @file hb_app.c
 * @synopsis  
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-07-05
 */

#include <dirent.h>
#include "hb_app.h"
#include "program_run_log.h"
#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif
//通道偏移量
volatile int32_t ch_offset = 0;
//配置文件目录
const char cfg_path[] = "./dev_cfg/\0";
//光纤段配置信息
const char file_fiber_sec[] = "fiber_sec_para\0";
//节点名称
const char file_node_name[] = "state_address.cfg\0";
//通道相关信息
const char file_ch_fpga[] = "ch_fpga.cfg\0";
//光纤段配置信息，里面含有指针，程序结束的时候，需要释放
struct _tagCHFiberSec chFiberSec[CH_NUM];
//通道硬件信息，激光器，波长，动态范围
struct _tagCHInfo chFpgaInfo[CH_NUM];
//杂项，包含了节点名称，通道状态这样比较好管理
struct _tagDevMisc devMisc;

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  initialize_sys_para 从文件中读取相关参数，并设立对应的标志
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t initialize_sys_para()
{
	int32_t ret, slot;
	ret = OP_OK;
	initialize_fiber_sec_cfg(CH_NUM);
	initialize_otdr_dev(otdrDev,CH_NUM);
	memset(&usrOtdrTest, 0, sizeof(struct _tagUsrOtdrTest));
	//dev指针，设备地址，mod,bits,delay,speed
	initial_spi_dev(&spiDev,"/dev/spidev1.0",0,8,0,20000000);
	read_slot();
	read_net_flag();

 	create_usr_tsk();
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  initialize_fiber_sec_cfg 读取配置文件，初始化光纤段参数
 *		初始化光纤段参数之后方可初始化otdrDev
 * @param pFiberSec
 * @param num
 *
 * @returns   0成功，其他失败
 */
/* ----------------------------------------------------------------------------*/
int32_t initialize_fiber_sec_cfg()
{
	int32_t ret, i;
	ret = OP_OK;
	for(i = 0; i < CH_NUM;i++)
	{
#ifdef LOCK_TYPE_SPIN
		pthread_spin_init(&chFiberSec[i].lock, 0);
#else
		pthread_mutex_init(&chFiberSec[i].lock,NULL);
#endif
		memset(&chFiberSec[i].para,0, sizeof(struct _tagFiberSecCfg));		
		read_fiber_sec_para(i,&chFiberSec[i]);
	}

	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  initialize_otdr_dev 用光纤段参数初始化otdrDev
 *
 * @param pOtdrDev
 * @param pFiberSec
 * @param ch_num
 *
 * @returns   0 成功，其他失败
 */
/* ----------------------------------------------------------------------------*/
int32_t initialize_otdr_dev(struct _tagOtdrDev *pOtdrDev,
		int32_t ch_num)
{
	int ret, i;
	struct _tagFiberSecCfg *pFiberSec;

	ret = OP_OK;
	memset(pOtdrDev,0,sizeof(struct _tagOtdrDev)*ch_num);
	for(i = 0; i < ch_num;i++)
	{

		if(chFiberSec[i].para.is_initialize)
		{
			get_ch_para_from_fiber_sec(&pOtdrDev[i].ch_para,&chFiberSec[i]);
			pOtdrDev[i].ch_ctrl.is_cfged = 1;

		}


	}
	return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_ch_para_from_fiber_sec 利用光纤段配置的otdr测量参数初始化
 *		通道里面的测量参数
 * @param pCHPara
 * @param pFiberSec
 *
 * @returns   0 成功，其他失败
 */
/* ----------------------------------------------------------------------------*/
int32_t get_ch_para_from_fiber_sec(struct _tagCHPara *pCHPara, 
		const struct _tagCHFiberSec *pFiberSec)
{
	int ret;
	ret = OP_OK;
	quick_lock(&pFiberSec->lock);
	pCHPara->Lambda_nm = pFiberSec->para.otdr_param.wl;
	pCHPara->PulseWidth_ns = pFiberSec->para.otdr_param.pw;
	pCHPara->MeasureTime_ms = pFiberSec->para.otdr_param.time*1000;
	pCHPara->n = pFiberSec->para.otdr_param.gi;
	pCHPara->EndThreshold = pFiberSec->para.otdr_param.end_threshold;
	pCHPara->NonRelectThreshold = pFiberSec->para.otdr_param.none_reflect_threshold;
	quick_unlock(&pFiberSec->lock);
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  creat_folder 创建文件夹 如果两个线程同时条用这个函数创建同名同
 *		路径下的文件夹就悲催了，在某些情况下回返回错误，难道要加锁?
 * @param folder_path[]	文件夹路径+文件名
 *
 * @returns   0 成功 其他系统给出的错误码
 */
/* ----------------------------------------------------------------------------*/
int32_t creat_folder(const char folder_path[])
{
	int ret;
	DIR *dir_folder;
	ret = 0;
	dir_folder = opendir(folder_path);
	//如果目录为空，就创建
	if(dir_folder == NULL)
	{
		mkdir(folder_path,0775);
	}
	dir_folder = opendir(folder_path);
	//如果打开创建后的目录，仍然失败那么返回
	if(dir_folder == NULL)
	{
		ret = errno;
		goto usr_exit;
	}

usr_exit:
	if(dir_folder != NULL)
		closedir(dir_folder);
	return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  save_fiber_sec_para 用户配置光纤段参数，需要保存文件，同时通知
 *		对应的通道更新参数使用文件锁加锁
 * @param ch
 * @param pfiber_sec
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t save_fiber_sec_para(int ch, struct tms_fibersectioncfg *pfiber_sec)
{
	int ret, counts, tmp;
	char file_path[FILE_PATH_LEN] = {0};
	FILE *fp;
	struct _tagFiberSecHead secHead;

	secHead.sec_num = pfiber_sec->fiber_hdr->count;
	secHead.data_num = pfiber_sec->otdr_hdr->count;
	secHead.event_num = pfiber_sec->event_hdr->count;




	snprintf(file_path,FILE_PATH_LEN , "%s%s_%d.cfg",cfg_path, file_fiber_sec,ch);
	fp = NULL;
	ret = OP_OK;
	//获取日志名字
	fp = fopen(file_path,"wb");
	if(fp == NULL){
		ret = errno;
		goto usr_exit;

	}
	//保存光纤段可变信息
	counts = fwrite(&secHead, sizeof(struct _tagFiberSecHead),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}
	//光纤段头信息
	counts = fwrite(pfiber_sec->fiber_hdr, sizeof( struct tms_fibersection_hdr),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}
	//光纤段具体内容
	tmp = pfiber_sec->fiber_hdr->count;
	counts = fwrite(pfiber_sec->fiber_val, \
			sizeof( struct tms_fibersection_val),tmp,fp);
	if(counts != tmp){
		ret = errno;
		goto usr_exit;
	}
	//测量参数
	counts = fwrite(pfiber_sec->otdr_param, \
			sizeof( struct tms_otdr_param),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}
	//测量结果
	counts = fwrite(pfiber_sec->test_result, \
			sizeof( struct tms_test_result),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}
	//数据点
	counts = fwrite(pfiber_sec->otdr_hdr, \
			sizeof( struct tms_hebei2_data_hdr),1,fp);

	tmp = pfiber_sec->otdr_hdr->count;
	counts = fwrite(pfiber_sec->otdr_val,sizeof( struct tms_hebei2_data_val),\
			tmp, fp);
	if(counts != tmp){
		ret = errno;
		goto usr_exit;
	}
	//事件点信息
	counts = fwrite(pfiber_sec->event_hdr, \
			sizeof( struct tms_hebei2_event_hdr),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}
	tmp = pfiber_sec->event_hdr->count;
	counts = fwrite(pfiber_sec->event_val,sizeof(struct tms_hebei2_event_val),\
			tmp,fp);
	if(counts != tmp){
		ret = errno;
		goto usr_exit;
	}
	ret = OP_OK;
	fclose(fp);
	fp = NULL;

usr_exit:
	if(fp != NULL)
		fclose(fp);
	printf("%s():%d: save fiber sec para ch %d ret %d .\n",\
			__FUNCTION__, __LINE__, ch, ret);
	return ret;

}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  read_fiber_sec_para 从文件中读取光纤段参数，注意与保存光纤段数据
 *		类型一样, 检查读取的光纤段数目，数据点数目，事件点数目是否
 *		与头信息一致，如果不一致，则文件损坏
 * @param ch 通道
 * @param pfiber_sec 指向光纤段配置缓冲区
 *
 * @returns   0 成功，其他失败
 */
/* ----------------------------------------------------------------------------*/
int32_t read_fiber_sec_para(int ch, struct _tagCHFiberSec *pch_fiber_sec)
{

	int32_t ret, counts, tmp;
	uint8_t is_free, is_lock;
	char file_path[FILE_PATH_LEN] = {0};
	FILE *fp;
	struct _tagFiberSecHead secHead;
	struct _tagFiberSecCfg	*pfiber_sec;

	is_free = 0;
	is_lock	= 0;
	pfiber_sec = &(pch_fiber_sec->para);
	snprintf(file_path,FILE_PATH_LEN , "%s%s_%d.cfg",cfg_path, file_fiber_sec,ch);
	fp = NULL;
	ret = OP_OK;
	//获取日志名字
	fp = fopen(file_path,"rb");
	if(fp == NULL){
		ret = errno;
		goto usr_exit;

	}
	counts = fread(&secHead, sizeof(struct _tagFiberSecHead),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}
	quick_lock(&pch_fiber_sec->lock);
	is_lock = 1;
	//为光纤段，数据段，时间点等位置分配空间
	ret = alloc_fiber_sec_buf(secHead, pfiber_sec);
	if(ret != OP_OK)
		goto usr_exit;
	is_free = 1;
	//光纤段头信息
	counts = fread(&pfiber_sec->fiber_hdr, sizeof( struct tms_fibersection_hdr),1,fp);
	if(counts != 1|| pfiber_sec->fiber_hdr.count != secHead.sec_num){
		if(counts != 1)
			ret = errno;
		else
			ret = FILE_RUIN;
		goto usr_exit;
	}
	//光纤段具体内容
	tmp = pfiber_sec->fiber_hdr.count;
	counts = fread(pfiber_sec->fiber_val, \
			sizeof( struct tms_fibersection_val),tmp,fp);
	if(counts != tmp){
		ret = errno;
		goto usr_exit;
	}
	//测量参数
	counts = fread(&pfiber_sec->otdr_param, \
			sizeof( struct tms_otdr_param),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}
	//测量结果
	counts = fread(&pfiber_sec->test_result, \
			sizeof( struct tms_test_result),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}
	//数据点
	counts = fread(&pfiber_sec->otdr_hdr, \
			sizeof( struct tms_hebei2_data_hdr),1,fp);
	if(counts != 1|| pfiber_sec->otdr_hdr.count != secHead.data_num){
		if(counts != 1)
			ret = errno;
		else
			ret = FILE_RUIN;
		goto usr_exit;
	}

	tmp = pfiber_sec->otdr_hdr.count;
	counts = fread(pfiber_sec->otdr_val,sizeof( struct tms_hebei2_data_val),\
			tmp, fp);
	if(counts != tmp){
		ret = errno;
		goto usr_exit;
	}
	//事件点信息
	counts = fread(&pfiber_sec->event_hdr, \
			sizeof( struct tms_hebei2_event_hdr),1,fp);

	if(counts != 1|| pfiber_sec->event_hdr.count != secHead.event_num){
		if(counts != 1)
			ret = errno;
		else
			ret = FILE_RUIN;
		goto usr_exit;
	}

	tmp = pfiber_sec->event_hdr.count;
	counts = fread(pfiber_sec->event_val,sizeof(struct tms_hebei2_event_val),\
			tmp,fp);
	if(counts != tmp){
		ret = errno;
		goto usr_exit;
	}
	ret = OP_OK;
	fclose(fp);
	fp = NULL;

usr_exit:
	if(is_lock)
		quick_unlock(&pch_fiber_sec->lock);
	if(fp != NULL)
		fclose(fp);
	printf("%s():%d: read fiber sec para ch %d ret %d .\n",\
			__FUNCTION__, __LINE__, ch, ret);
	return ret;

}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  alloca_fiber_sec_buf 分配光纤段缓冲区,如果缓冲区已经存在，并且请求的
 *		小于已经存在的缓冲区，则不分配直接使用原来的
 * @param secHead 数据点数目，光纤段数目，事件点数目
 * @param pFiberSecCfg
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t alloc_fiber_sec_buf(struct _tagFiberSecHead secHead, struct _tagFiberSecCfg *pFiberSecCfg)
{
	int32_t ret, count;
	char log[NUM_CHAR_LOG_MSG] = {0};
	ret = OP_OK;
	count = secHead.sec_num;

	//分配存储空间，如果存储空间已经存在并且需要的存储空间小于已分配的，则不再分配
	if(pFiberSecCfg->fiber_val == NULL)
		pFiberSecCfg->fiber_val = (struct tms_fibersection*)\
			malloc(sizeof(struct tms_fibersection_val)*count);
	else if(count > pFiberSecCfg->fiber_hdr.count){
		free(pFiberSecCfg->fiber_val);
		pFiberSecCfg->fiber_val =  (struct tms_fibersection*)\
			malloc(sizeof(struct tms_fibersection_val)*count);
	}

	count = secHead.data_num;
	if(pFiberSecCfg->otdr_val == NULL)
		pFiberSecCfg->otdr_val = (struct tms_hebei2_data_val *)\
			malloc(count * sizeof(struct tms_hebei2_data_val));
	else if(count > pFiberSecCfg->otdr_hdr.count){
		free(pFiberSecCfg->otdr_val);
		pFiberSecCfg->otdr_val = (struct tms_hebei2_data_val *)\
			malloc(count * sizeof(struct tms_hebei2_data_val));

	}

	count = secHead.event_num;
	if(pFiberSecCfg->event_val == NULL)
		pFiberSecCfg->event_val = (struct tms_hebei2_event_val *)\
			malloc(count * sizeof(struct tms_hebei2_event_val));
	else if(count > pFiberSecCfg->event_hdr.count){
		free(pFiberSecCfg->event_val);
		pFiberSecCfg->event_val = (struct tms_hebei2_event_val *)\
			malloc(count * sizeof(struct tms_hebei2_event_val));
	}
	if(!pFiberSecCfg->fiber_val||!pFiberSecCfg->otdr_val || !pFiberSecCfg->event_val){
		free_fiber_sec_buf(pFiberSecCfg);
		ret = NEW_BUF_FAIL;
		snprintf(log, NUM_CHAR_LOG_MSG,"new buf fail !");
		LOGW(__FUNCTION__, __LINE__,LOG_LEV_FATAL_ERRO, log);
		//是否应该自杀，再活下去也没有意思了吧,接下去，程序运行到哪里就不知道了
	}
	else{
		pFiberSecCfg->is_initialize = 1;
	}

	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  free_fiber_sec_buf 释放光纤段配置的缓冲区
 *
 * @param pFiberSecCfg 指向光纤段存储区
 *
 * @returns   0
 */
/* ----------------------------------------------------------------------------*/
int32_t free_fiber_sec_buf(struct _tagFiberSecCfg *pFiberSecCfg)
{
	int32_t ret;
	ret = OP_OK;
	//删除光纤段
	if(pFiberSecCfg->fiber_val != NULL){
		free(pFiberSecCfg->fiber_val);
		pFiberSecCfg->fiber_val = NULL;
	}
	//删除otdr缓冲区
	if(pFiberSecCfg->otdr_val != NULL){
		free(pFiberSecCfg->otdr_val);
		pFiberSecCfg->otdr_val = NULL;
	}
	//事件点缓冲区
	if(pFiberSecCfg->event_val != NULL){
		free(pFiberSecCfg->event_val);
		pFiberSecCfg->event_val = NULL;
	}
	//初始化标志设为0
	pFiberSecCfg->is_initialize = 0;
	pFiberSecCfg->error_num = 0;
	return ret;

}

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  quick_lock 自定义快速锁的上锁函数
 *
 * @param plock
 *
 * @returns   0
 */
/* ----------------------------------------------------------------------------*/
int32_t quick_lock( QUICK_LOCK *plock)
{
	int32_t ret;
	ret = OP_OK;
#ifdef LOCK_TYPE_SPIN
	pthread_spin_lock(plock);
#else
	pthread_mutex_lock(plock);
#endif
	return ret;

}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  quick_unlock 字定义快速锁解锁
 *
 * @param plock
 *
 * @returns   0
 */
/* ----------------------------------------------------------------------------*/
int32_t quick_unlock( QUICK_LOCK *plock)
{
	int32_t ret;
	ret = OP_OK;
#ifdef LOCK_TYPE_SPIN
	pthread_spin_unlock(plock);
#else
	pthread_mutex_unlock(plock);
#endif
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  save_ch_fpga_info 保存fpga信息
 *
 * @param pch_fpga_info
 * @param ch_num
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t save_ch_fpga_info(const struct _tagCHInfo *pch_fpga_info,int32_t ch_num)
{
	int32_t ret, counts, tmp;
	char file_path[FILE_PATH_LEN] = {0};
	FILE *fp;

	snprintf(file_path,FILE_PATH_LEN , "%s%s.cfg",cfg_path, file_ch_fpga);
	fp = NULL;
	ret = OP_OK;

	//获取日志名字
	fp = fopen(file_path,"wb");
	if(fp == NULL){
		ret = errno;
		goto usr_exit;

	}
	//保存光纤段可变信息
	counts = fwrite(pch_fpga_info, ch_num*sizeof(struct _tagCHInfo),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}

usr_exit:
	if(fp != NULL)
		fclose(fp);

	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  read_ch_fpga_info 读取保存在文件里面的信息
 *
 * @param pch_fpga_info
 * @param ch_num
 *
 * @returns 0   
 */
/* ----------------------------------------------------------------------------*/
int32_t read_ch_fpga_info(const struct _tagCHInfo *pch_fpga_info,int32_t ch_num)
{
	int32_t ret, counts, tmp;
	char file_path[FILE_PATH_LEN] = {0};
	FILE *fp;
	//初始化，全部赋值成0
	memset(pch_fpga_info, 0, sizeof(struct _tagCHInfo)*ch_num);

	snprintf(file_path,FILE_PATH_LEN , "%s%s.cfg",cfg_path, file_ch_fpga);
	fp = NULL;
	ret = OP_OK;

	//获取日志名字
	fp = fopen(file_path,"rb");
	if(fp == NULL){
		ret = errno;
		goto usr_exit;

	}
	//保存光纤段可变信息
	counts = fread(pch_fpga_info, ch_num*sizeof(struct _tagCHInfo),1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}

usr_exit:
	if(fp != NULL)
		fclose(fp);

	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  save_node_name_address 保存节点名称，和通道使用状态
 *
 * @param pdev_misc 节点名称，地址，设备信息都保存在杂项里面
 *
 * @returns   0 成功，其他失败
 */
/* ----------------------------------------------------------------------------*/
int32_t save_node_name_address(const struct _tagDevMisc *pdev_misc)
{

	int32_t ret, counts, size;
	char file_path[FILE_PATH_LEN] = {0};
	FILE *fp;

	snprintf(file_path,FILE_PATH_LEN , "%s%s.cfg",cfg_path, file_node_name);
	fp = NULL;
	ret = OP_OK;

	//获取日志名字
	fp = fopen(file_path,"wb");
	if(fp == NULL){
		ret = errno;
		goto usr_exit;

	}
	//保存光纤段可变信息
	size = sizeof(struct _tagDevNameAddr) + sizeof(struct _tagDevCHState);
	counts = fwrite(pdev_misc,size,1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}

usr_exit:
	if(fp != NULL)
		fclose(fp);

	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  reade_node_name_address 读取文件中保存的节点名称和地址
 *	节点名称地址通道使用状态均保存在杂项里面
 * @param pdev_misc
 *
 * @returns   0 成功，其他失败
 */
/* ----------------------------------------------------------------------------*/
int32_t reade_node_name_address(struct _tagDevMisc *pdev_misc)
{

	int32_t ret, counts, size;
	char file_path[FILE_PATH_LEN] = {0};
	FILE *fp;

	snprintf(file_path,FILE_PATH_LEN , "%s%s.cfg",cfg_path, file_node_name);
	fp = NULL;
	ret = OP_OK;

	//获取日志名字
	fp = fopen(file_path,"rb");
	if(fp == NULL){
		ret = errno;
		goto usr_exit;

	}
	//保存光纤段可变信息
	size = sizeof(struct _tagDevNameAddr) + sizeof(struct _tagDevCHState);
	memset(pdev_misc, 0, size);
	counts = fread(pdev_misc,size,1,fp);
	if(counts != 1){
		ret = errno;
		goto usr_exit;
	}

usr_exit:
	if(fp != NULL)
		fclose(fp);

	return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  check_usr_otdr_test_para 对点名测量的参数进行检查
 *
 * @param pget_otdrdata
 *
 * @returns   0 其他非法值
 */
/* ----------------------------------------------------------------------------*/
int32_t check_usr_otdr_test_para(struct tms_get_otdrdata *pget_otdrdata)
{
	int32_t ret;
	ret = CMD_RET_OK;
	if(pget_otdrdata->pipe < ch_offset || \
			pget_otdrdata->pipe > (ch_offset + CH_NUM)){
		ret = CMD_RET_PARA_INVLADE;
		goto usr_exit;
	}
	if(pget_otdrdata->range < 0 || \
			pget_otdrdata->range > MAX_RANG_M){
		ret = CMD_RET_PARA_INVLADE;
		goto usr_exit;
	}


usr_exit:
	return ret;
}

int32_t check_fiber_sec_para(const struct tms_fibersectioncfg *pfiber_sec_cfg)
{
	int32_t ret, i, tmp,ch;
	ret = CMD_RET_OK;
	
	tmp = pfiber_sec_cfg->fiber_hdr->count;
	if(tmp <= 0){
		ret = CMD_RET_PARA_INVLADE;
		printf("%s() %d : sec num error %d ch_offset %d\n",\
				       	__FUNCTION__ ,__LINE__, tmp, ch_offset);
		goto usr_exit;
	}

	//检查通道号
	for(i = 0; i < tmp; i++)
	{
		ch = pfiber_sec_cfg->fiber_val[i].pipe_num; 
		if(ch < ch_offset || ch > (ch_offset + CH_NUM)){
			ret = CMD_RET_PARA_INVLADE;
			printf("%s() %d : ch error %d ch_offset %d\n",\
				       	__FUNCTION__ ,__LINE__, ch, ch_offset);
			goto usr_exit;
		}
	}
	//检查量程
	tmp = pfiber_sec_cfg->otdr_param->range;
	if(tmp <= 0 || tmp > MAX_RANG_M){
		ret = CMD_RET_PARA_INVLADE;
		printf("%s() %d : range over flow %d \n",\
				       	__FUNCTION__ ,__LINE__, tmp);
		goto usr_exit;
	}

usr_exit:
	return ret;
}
extern int32_t tsk_schedule(void * arg);
extern int32_t tsk_OtdrAlgo(void *arg);
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  create_usr_tsk 创建tsk_otdr, tsk_schedule任务
 *
 * @returns   如果失败，直接重启
 */
/* ----------------------------------------------------------------------------*/
int32_t create_usr_tsk()
{
	
	int32_t ret;
	uint8_t log[NUM_CHAR_LOG_MSG] = {0};
	ret = 0; 
	pthread_mutex_init(&mutex_otdr, NULL);
	ret = pthread_create(&tsk_schedule_info.tidp, NULL,\
		       	tsk_schedule,(void *)(&tsk_schedule_info));
	if(ret != 0){

		snprintf(log, NUM_CHAR_LOG_MSG,"creat tsk schedule failed %d!",errno);
		LOGW(__FUNCTION__, __LINE__,LOG_LEV_FATAL_ERRO, log);
		system("sync");
		exit(0);
	}
	
	ret = pthread_create(&tsk_otdr_info.tidp, NULL,\
		tsk_OtdrAlgo,(void *)(&tsk_otdr_info));
	if(ret != 0){
		snprintf(log, NUM_CHAR_LOG_MSG,"creat tsk otdr failed %d!",errno);
		LOGW(__FUNCTION__, __LINE__,LOG_LEV_FATAL_ERRO, log);
		system("sync");
		exit(0);
	}
	return 0;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_context_by_dst 通过目的地址获取context
 *
 * @param dst	目的地址，网管服务器，客户端，节点管理器
 * @param pcontext 获取的context
 *
 * @returns   0成功，其他失败
 */
/* ----------------------------------------------------------------------------*/
int32_t get_context_by_dst(int32_t dst, struct tms_context *pcontext)
{
	int ret;
	ret = 1;
	switch(dst)
	{
		case ADDR_HOST_NODE:
		       ret = tms_SelectNodeMangerContext(pcontext);
		       break;	       
		case ADDR_HOST_SERVER:
		       ret = tms_SelectMangerContext(pcontext);
		       break;
		default:
		       ret = 1;
	}
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  hb_Ret_Ack 封装了向上发送回应的函数
 *
 * @param ack
 *
 * @returns   0 成功 其他失败
 */
/* ----------------------------------------------------------------------------*/
int32_t hb_Ret_Ack(int32_t dst, struct tms_ack ack)
{
	struct tms_context dst_context;
	int32_t ret;
	ret = get_context_by_dst(dst, &dst_context);
	if(!ret)
		ret =  tms_AckEx(dst_context.fd,NULL, &ack);
	
	return ret;
}
//读取槽位号，不成功，就去死
int32_t read_slot()
{
	int32_t ret, slot;
	char msg[NUM_CHAR_LOG_MSG] ={0};
	ret = get_dev_slot(&spiDev, &slot);
	if(ret != OP_OK)
		goto usr_exit;

	if(!slot)
		ch_offset = 0;
	else
		ch_offset = CH_NUM;
usr_exit:
	if(ret != OP_OK){
		snprintf(log, NUM_CHAR_LOG_MSG,"get slot error %d",ret);
		LOGW(__FUNCTION__, __LINE__,LOG_LEV_FATAL_ERRO, log);
		exit(0);
	}
	return ret;

}
//读取网络标志，不成功就去死
int32_t read_net_flag()
{
	int32_t ret, slot;
	char msg[NUM_CHAR_LOG_MSG] ={0};
	ret = get_net_flag(&spiDev, &slot);
usr_exit:
	if(ret != OP_OK){
		snprintf(log, NUM_CHAR_LOG_MSG,"get net flag error %d",ret);
		LOGW(__FUNCTION__, __LINE__,LOG_LEV_FATAL_ERRO, log);
		exit(0);
	}
	return ret;

}

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_test_para_from_algro 将算法模块的测量参数转换成hebei2协议格式
 *
 * @param pHost 指向hebei2协议格式测量参数
 * @param pAlgro 指向算法模块协议格式
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t get_test_para_from_algro(struct tms_ret_otdrparam *pHost,const OTDR_UploadAllData_t *pAlgro)
{
	int32_t ret;
	ret = OP_OK;
	pHost->end_threshold = pAlgro->MeasureParam.EndThreshold;
	pHost->wl = pAlgro->MeasureParam.Lambda_nm;
	pHost->range = pAlgro->MeasureParam.MeasureLength_m;
	pHost->time = pAlgro->MeasureParam.MeasureTime_ms / 1000;
	pHost->none_reflect_threshold = pAlgro->MeasureParam.NonRelectThreshold;
	pHost->pw = pAlgro->MeasureParam.PulseWidth_ns;
	pHost->gi = pAlgro->MeasureParam.n;
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_test_result_from_algro 将算法模块的测量结果转换成hebei2协议格式
 *
 * @param pHost hebei2协议格式的测量结果
 * @param pAlgro 算法模块的测量结果
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t get_test_result_from_algro(struct tms_test_result *pHost,const OTDR_UploadAllData_t *pAlgro)
{
	int ret ;
	ret = OP_OK;
	pHost->atten = pAlgro->MeasureParam.FiberAttenCoef;
	pHost->loss = pAlgro->MeasureParam.FiberLoss;
	pHost->range = pAlgro->MeasureParam.FiberLength;
	
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_test_event_from_algro 将事件点格式从算法模块转换到hebei2协议
 *
 * @param pHost 指向hebei2协议格式的事件点
 * @param pAlgro 指向算法模块全部数据格式
 * @param count 事件个数
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t get_test_event_from_algro(struct tms_hebei2_event_val *pHost,const OTDR_UploadAllData_t *pAlgro, int32_t count)
{
	int32_t ret, i;
	ret = OP_OK;
	for(i = 0; i < count; i++)
	{
		pHost[i].att = pAlgro->Event.EventPoint[i].AttenCoef ;
		pHost[i].distance = pAlgro->Event.EventPoint[i].EventXlabel;
		pHost[i].event_type = pAlgro->Event.EventPoint[i].EventType;
		pHost[i].link_loss= pAlgro->Event.EventPoint[i].EventInsertLoss ;
		pHost[i].loss = pAlgro->Event.EventPoint[i].EventTotalLoss;
		pHost[i].reflect = pAlgro->Event.EventPoint[i].EventReflectLoss;
	}
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  send_otdr_data_host 算法线程完成算法运算后，将数据发送到主机，
 *		适合用户点名测量，配置测量，发送告警的时候发送曲线部分
 * @param pResult 由算法模块完成填充
 * @param pCHInfo 指明了请求点名测量的源地址，cmd命令码等相关信息
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t send_otdr_data_host(OTDR_UploadAllData_t *pResult, struct _tagAlgroCHInfo *pCHInfo )
{
	int32_t ret, offset, event_count;
	OTDR_UploadAllData_t *AllEvent;
	char	*NextAddr;
	ret = OP_OK;
	struct tms_ret_otdrdata hebei2_otdrdata;
	struct tms_ret_otdrparam    ret_otdrparam;
	struct tms_test_result      test_result;
	struct tms_hebei2_data_hdr  hebei2_data_hdr;
	struct tms_hebei2_data_val *hebei2_data_val;
	struct tms_hebei2_event_hdr hebei2_event_hdr;
	struct tms_hebei2_event_val hebei2_event_val[10];
	struct tms_hebei2_event_val *pevent_buf;
	struct tms_context contxt;



	event_count = 0;
	ret = get_context_by_dst(pCHInfo->src_addr, &contxt);
	if(ret){
		printf("%s %d, ask context error \n", __FUNCTION__, __LINE__);
		goto usr_exit;
	}

	pevent_buf = NULL;
	get_test_para_from_algro(&ret_otdrparam, pResult);
	get_test_result_from_algro(&test_result, pResult);
	hebei2_data_hdr.count = pResult->OtdrData.DataNum;
	hebei2_data_val = (struct tms_hebei2_data_val *)pResult->OtdrData.dB_x1000;

	offset = 8 + sizeof(pResult->MeasureParam) + \
		 sizeof(pResult->OtdrData.DataNum)*sizeof(int16_t) + 4;
	NextAddr = (char *)pResult + offset;
	AllEvent = (OTDR_UploadAllData_t*)(NextAddr - 8 - sizeof(AllEvent->MeasureParam) - sizeof(AllEvent->OtdrData));
	event_count = AllEvent->Event.EventNum;

	if(event_count > 10){
		pevent_buf = malloc(sizeof( struct tms_hebei2_event_val)*event_count);
		if(!pevent_buf){
			printf("%s %s() %d, alloc error, event_num %d \n", __FILENAME__, __FUNCTION__,\
					__LINE__, event_count);
			exit(1);
		}
	}
	else
		pevent_buf = hebei2_event_val;
	get_test_event_from_algro(pevent_buf, AllEvent,event_count);
	strcpy(hebei2_data_hdr.dpid,"OTDRData\0");
	strcpy(test_result.result, "OTDRTestResultInfo\0");
	strcpy(hebei2_event_hdr.eventid, "KeyEvents\0");
	hebei2_otdrdata.hebei2_data_hdr = &hebei2_data_hdr;
	hebei2_otdrdata.hebei2_data_val = hebei2_data_val;
	hebei2_otdrdata.hebei2_event_hdr = &hebei2_event_hdr;
	hebei2_otdrdata.hebei2_event_val = pevent_buf;
	hebei2_otdrdata.ret_otdrparam = &ret_otdrparam;
	hebei2_otdrdata.test_result = &test_result;
	hebei2_event_hdr.count = event_count;
	tms_RetOTDRData(contxt.fd, NULL, &hebei2_otdrdata, pCHInfo->cmd);		


	if(event_count> 10)
		free(pevent_buf);
usr_exit:
	printf("%s %s() %d, event_num %d \n", __FILENAME__, __FUNCTION__,\
					__LINE__, event_count);

	return ret;
}
//按照C风格编译
#ifdef __cplusplus
}
#endif
