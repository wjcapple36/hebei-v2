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
		memset(&chFiberSec[i].alarm,0, sizeof(struct _tagSecFiberAlarm));		
		memset(&chFiberSec[i].statis,0, sizeof(struct _tagFiberStatisData));		
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
	struct _tagSecFiberAlarm *pAlarm;
	//统计数据，光线段数目有关系
	struct _tagFiberStatisData *pStatis;
	pAlarm = &pch_fiber_sec->alarm;
	pStatis = &pch_fiber_sec->statis;

	is_free = 0;
	is_lock	= 0;
	pfiber_sec = &(pch_fiber_sec->para);
	snprintf(file_path,FILE_PATH_LEN , "%s%s_%d.cfg",cfg_path, file_fiber_sec,ch);
	fp = NULL;
	ret = OP_OK;
	//打开文件
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
	ret = alloc_fiber_sec_buf(secHead, pch_fiber_sec);
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
	pAlarm->ch = ch;
	pAlarm->sec_num = secHead.sec_num;
	pAlarm->alarm_num = 0;
	pStatis->sec_num = secHead.sec_num;
	pStatis->counts = 0;
	pStatis->state = 0;

usr_exit:
	if(is_lock)
		quick_unlock(&pch_fiber_sec->lock);
	if(fp != NULL)
		fclose(fp);
	if(ret != OP_OK)
		free_fiber_sec_buf(pch_fiber_sec);
	printf("%s():%d: read fiber sec para ch %d ret %d .\n",\
			__FUNCTION__, __LINE__, ch, ret);
	return ret;

}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  alloca_fiber_sec_buf 分配光纤段缓冲区,如果缓冲区已经存在，并且请求的
 *		小于已经存在的缓冲区，则不分配直接使用原来的
 * @param secHead	 数据点数目，光纤段数目，事件点数目
 * @param pCHFiberSec	
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t alloc_fiber_sec_buf(struct _tagFiberSecHead secHead, struct _tagCHFiberSec *pCHFiberSec)
{
	int32_t ret, count;
	char log[NUM_CHAR_LOG_MSG] = {0};
	ret = OP_OK;
	//光线段，曲线，告警门限
	struct _tagFiberSecCfg *pFiberSecCfg;
	//告警，和光线段数目有关系
	struct _tagSecFiberAlarm *pAlarm;
	//统计数据，光线段数目有关系
	struct _tagFiberStatisData *pStatis;

	pFiberSecCfg = &pCHFiberSec->para;
	pAlarm = &pCHFiberSec->alarm;
	pStatis = &pCHFiberSec->statis;
	
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
	//分配告警存储空间
	if(pAlarm->buf == NULL)
		pAlarm->buf = malloc(sizeof(struct _tagSecFiberAlarm)*count);
	else if(count > pFiberSecCfg->fiber_hdr.count){
		free(pAlarm->buf);
		pAlarm->buf = malloc(sizeof(struct _tagSecFiberAlarm)*count);
	}
	//分配统计数据存储空间
	if(pStatis->buf == NULL)
		pStatis->buf = malloc(sizeof(struct _tagSecFiberAlarm)*count);
	else if(count > pFiberSecCfg->fiber_hdr.count){
		free(pStatis->buf);
		pStatis->buf = malloc(sizeof(struct _tagSecFiberAlarm)*count);
	}
	//分配采样点数存储空间
	count = secHead.data_num;
	if(pFiberSecCfg->otdr_val == NULL)
		pFiberSecCfg->otdr_val = (struct tms_hebei2_data_val *)\
			malloc(count * sizeof(struct tms_hebei2_data_val));
	else if(count > pFiberSecCfg->otdr_hdr.count){
		free(pFiberSecCfg->otdr_val);
		pFiberSecCfg->otdr_val = (struct tms_hebei2_data_val *)\
			malloc(count * sizeof(struct tms_hebei2_data_val));

	}
	//分配事件点存储空间
	count = secHead.event_num;
	if(pFiberSecCfg->event_val == NULL)
		pFiberSecCfg->event_val = (struct tms_hebei2_event_val *)\
			malloc(count * sizeof(struct tms_hebei2_event_val));
	else if(count > pFiberSecCfg->event_hdr.count){
		free(pFiberSecCfg->event_val);
		pFiberSecCfg->event_val = (struct tms_hebei2_event_val *)\
			malloc(count * sizeof(struct tms_hebei2_event_val));
	}
	//检查分配结果是否正确
	if(!pFiberSecCfg->fiber_val||!pFiberSecCfg->otdr_val || !pFiberSecCfg->event_val\
			|| !pAlarm->buf || !pStatis->buf)
	{
		free_fiber_sec_buf(pFiberSecCfg);
		ret = NEW_BUF_FAIL;
		exit_self(errno,__FUNCTION__, __LINE__,"new buf fail\0");
	}
	else{
		pFiberSecCfg->is_initialize = 1;
	}

	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  free_fiber_sec_buf 释放包括光线段参数，告警缓冲区，统计缓冲区
 *
 * @param pCHFiberSec
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t free_fiber_sec_buf(struct _tagCHFiberSec *pCHFiberSec)
{
	//光线段，曲线，告警门限
	struct _tagFiberSecCfg *pFiberSecCfg;
	//告警，和光线段数目有关系
	struct _tagSecFiberAlarm *pAlarm;
	//统计数据，光线段数目有关系
	struct _tagFiberStatisData *pStatis;

	pFiberSecCfg = &pCHFiberSec->para;
	pAlarm = &pCHFiberSec->alarm;
	pStatis = &pCHFiberSec->statis;

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
	//告警缓冲区
	if(pAlarm->buf != NULL){
		free(pAlarm->buf);
		pAlarm->buf = NULL;
	}
	//释放统计数据缓冲区
	if(pStatis->buf != NULL){
		free(pStatis);
		pAlarm->buf = NULL;
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
	if((pget_otdrdata->pipe - 1) < ch_offset || \
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
	ret = 0; 
	pthread_mutex_init(&mutex_otdr, NULL);
	ret = pthread_create(&tsk_schedule_info.tidp, NULL,\
		       	tsk_schedule,(void *)(&tsk_schedule_info));
	if(ret != 0){
		exit_self(errno, __FUNCTION__, __LINE__, "creat tsk schedule erro\0");
	}
	
	ret = pthread_create(&tsk_otdr_info.tidp, NULL,\
		tsk_OtdrAlgo,(void *)(&tsk_otdr_info));
	if(ret != 0){
		exit_self(errno, __FUNCTION__, __LINE__, "creat tsk schedule erro\0");
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
	ret = 2;
	switch(dst)
	{
		case ADDR_HOST_NODE:
		       ret = tms_SelectNodeMangerContext(pcontext);
		       break;	       
		case ADDR_HOST_SERVER:
		       ret = tms_SelectMangerContext(pcontext);
		       break;
		default:
		       break;
	}
	//大部分函数返回0，代表成功，此处有意外，上述两个调用返回1代表成功
	if(ret == 1)
		ret = 0;
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
		exit_self(errno, __FUNCTION__, __LINE__, "get slot error\0");
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
		exit_self(errno, __FUNCTION__, __LINE__, "get net flag error\0");
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
	int32_t ret_cmd;
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
	ret = get_ret_curv_cmd(pCHInfo->cmd, &ret_cmd);
	if(ret){
		printf("%s %d,requre curv cmd error cmd %d ret %d  \n", __FUNCTION__, __LINE__,\
				pCHInfo->cmd,ret);
		goto usr_exit;
	}

	pevent_buf = NULL;
	get_test_para_from_algro(&ret_otdrparam, pResult);
	get_test_result_from_algro(&test_result, pResult);
	hebei2_data_hdr.count = pResult->OtdrData.DataNum;
	hebei2_data_val = (struct tms_hebei2_data_val *)pResult->OtdrData.dB_x1000;
	offset = pResult->OtdrData.DataNum;
	NextAddr = (char *)(&pResult->OtdrData.dB_x1000[offset]);
	AllEvent = (OTDR_UploadAllData_t*)(NextAddr - 8 - sizeof(AllEvent->MeasureParam) - sizeof(AllEvent->OtdrData));
	event_count = AllEvent->Event.EventNum;

	if(event_count > 10){
		pevent_buf = malloc(sizeof( struct tms_hebei2_event_val)*event_count);
		if(!pevent_buf){
			printf("%s %s() %d, alloc error, event_num %d \n", __FILENAME__, __FUNCTION__,\
					__LINE__, event_count);
			exit_self(errno, __FUNCTION__, __LINE__, "alloc event buf error\0");
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
	tms_RetOTDRData(contxt.fd, NULL, &hebei2_otdrdata, ret_cmd);		


	if(event_count> 10)
		free(pevent_buf);
usr_exit:
	printf("%s %s() %d, event_num %d \n", __FILENAME__, __FUNCTION__,\
					__LINE__, event_count);

	return ret;

}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  exit_self 自我了断， 发现严重错误，那就死吧
 *
 * @param err_code 	错误码
 * @param function[] 	哪个函数把程序搞死了
 * @param line		哪一行出现了问题
 * @param msg[] 	死之前的遗言
 *
 * @returns		无所谓了吧
 */
/* ----------------------------------------------------------------------------*/
int32_t exit_self(int32_t err_code, char function[], int32_t line,  char msg[])
{
	char log[NUM_CHAR_LOG_MSG] = {0};
	snprintf(log, NUM_CHAR_LOG_MSG,"%s %d : errno %d!msg %s",function, line, err_code,msg);
	LOGW(__FUNCTION__, __LINE__,LOG_LEV_FATAL_ERRO, log);
	printf("%s \n", log);
	exit(err_code);
	return 0;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_ret_curv_cmd 根据输入曲线的ID获取返回曲线的ID
 *
 * @param in_cmd	输入曲线id
 * @param ret_cmd	返回曲线id
 *
 * @returns   0 成功，其他没有对应的id
 */
/* ----------------------------------------------------------------------------*/
int32_t get_ret_curv_cmd(int32_t in_cmd, int32_t *ret_cmd)
{
	int32_t ret;
	ret = 0;
	switch(in_cmd)
	{
		case ID_GETOTDRDATA_14:
			*ret_cmd = ID_RETOTDRDATA_16;
			break;
		case ID_GETOTDRDATA_15:
			*ret_cmd = ID_RETOTDRDATA_17;
			break;
		case ID_GETSTANDARDCURV:
			*ret_cmd = ID_RETOTDRDATA_18;
			break;
		case ID_GETCYCLETESTCUV:
			*ret_cmd = ID_RETOTDRDATA_19;
			break;

		default:
			ret = 1;
			break;
	}

	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_start_point 获取曲线的起始点, dsp算法里采用的固定值，hebei
 *		项目里面需要修订一下
 * @param sigma
 * @param pOtdrData
 * @param pOtdrCtrl
 * @param OtdrStateVariable_t
 * @param pOtdrState
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t get_curv_start_point(int32_t Sigma,
		OTDR_ChannelData_t *pOtdrData,
	       	OtdrCtrlVariable_t *pOtdrCtrl,
		OtdrStateVariable_t *pOtdrState)
{
	int iPointDis, i;
	int iMaxValue = 0;
	int iThredhold = 0;
	int32_t iDataLen;
	
	iDataLen = 200;
	iPointDis = 0;
	i = 0;
	
	if(pOtdrState->MeasureParam.MeasureLength_m > 10000)
	{
		iDataLen = 100;
	}
	
	for(int i = 0; i < iDataLen; i++)
	{
		if(pOtdrData->ChanData[i] > iMaxValue)
		{
			iMaxValue = pOtdrData->ChanData[i]; 
		}
	}

	iMaxValue = iMaxValue/2;
	if(iMaxValue > 10 * Sigma )
	{
		iThredhold = iMaxValue;
	}
	else
	{
		iThredhold = 10 * Sigma;
	}
	iThredhold = 5*Sigma;
	for(i = 0; i < iDataLen; i++)
	{
		if (pOtdrData->ChanData[i] > iThredhold)
		{
			iPointDis = i;
			break;
		}
	}

    return iPointDis;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_fiber_alarm_from_event 比较当前事件和标准曲线事件以获取
 *					 告警
 * @param pSecCoord		光纤段起始点
 * @param pstd_event_hdr	标准事件头，含事件数码
 * @param pstd_event_val	标准事件
 * @param AllEvent		当前曲线的事件点
 * @param pOtdrState		当前曲线状态变量
 * @param 			事件告警，含第一个告警和最严重告警
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t get_fiber_alarm_from_event(
		struct _tagFiberSecCoord *pSecCoord,
		struct tms_hebei2_event_hdr *pstd_event_hdr,
		struct tms_hebei2_event_val *pstd_event_val,
		OTDR_UploadAllData_t  *AllEvent,
		OtdrStateVariable_t *pOtdrState,
		struct tms_fibersection_val* pstd_fiber_val,
		struct _tagEventAlarm *pEventAlarm
		)
{
	int32_t std_xlable, cur_xlable, space;
	int32_t alarm_num, i, j;
	int32_t cur_event_num, std_event_num, limit_pt;
	int32_t alarm_lev, ret, is_find_event;
	float cur_inloss, std_inloss, diff_loss;

	limit_pt = pOtdrState->M;
	std_event_num = pstd_event_hdr->count;
	cur_event_num = AllEvent->Event.EventNum;

	//将当前事件点和标准事件点配对，即找到同一个事件点对应的序号
	alarm_num = 0;
	for(i = 0; i < cur_event_num;i++)
	{
		cur_xlable = AllEvent->Event.EventPoint[i].EventXlabel;
		//如果不在本事件段内直接返回 ？
		if(!(cur_xlable >= pSecCoord->start && cur_xlable < pSecCoord->end ))
			continue;
		alarm_lev = 0;	
		for(j = 0; j < std_event_num;j++)
		{
			std_xlable = pstd_event_val[j].distance;
			space = abs(cur_xlable - std_xlable);
			if(space <= limit_pt)
				break;
		}
		//找到相同事件，然后根据插损判别插损差值，如果当前事件的插损不存在，
		//则本函数的处理算法不会返回相关结果	
		if(j < std_event_num)
		{
			std_inloss = pstd_event_val[j].loss;
			cur_inloss = AllEvent->Event.EventPoint[i].EventInsertLoss;
			ret  = get_inser_loss_diff(cur_inloss,std_inloss,&diff_loss);
			if(!ret)
				alarm_lev = get_alarm_lev(diff_loss, pstd_fiber_val);
			if(alarm_lev > FIBER_ALARM_LEV0)
			{
				if(alarm_num == 0)
				{
					pEventAlarm->first.diff = diff_loss;
					pEventAlarm->first.index = i;
					pEventAlarm->first.pos = cur_xlable;
					pEventAlarm->first.lev = alarm_lev;
					memcpy(&pEventAlarm->highest, &pEventAlarm->first,\
							sizeof(struct _tagEventAlarmData));
				}
				else if(alarm_lev < pEventAlarm->highest.lev)
				{
					pEventAlarm->highest.diff = diff_loss;
					pEventAlarm->highest.index = i;
					pEventAlarm->highest.pos = cur_xlable;
					pEventAlarm->highest.lev = alarm_lev;

				}
				alarm_num++;
			}

		}

	}
	return alarm_num;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_inser_loss_diff 获取两个事件点插损差值。如果插损值都存在那么是
 *				  两者差值，如果只有当前值存在，那么就以差值返回
 *				  如果当前事件点没有插损值，则返回失败
 * @param cur_loss	 	当前事件的插损
 * @param std_loss		标准事件插损
 * @param 			返回的插损值
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t get_inser_loss_diff(
		float cur_loss,
		float std_loss,
		float *diff_loss
		)
{
	int32_t ret;
	float loss;
	ret = OP_OK;
	if(cur_loss < RSVD_FLOAT && std_loss < RSVD_FLOAT)
		loss = fabs(cur_loss - std_loss);
	else if(cur_loss < RSVD_FLOAT)
		loss = cur_loss;
	else
		ret = 1;

	return ret;
}
int32_t get_alarm_lev(
		float loss,
		struct tms_fibersection_val *pfiber_sec
		)
{
	int lev;
	//告警级别为0 代表无告警
	lev = 0;

	if(loss >= pfiber_sec->level1)
		lev = 1;
	else if(loss >= pfiber_sec->level2)
		lev = 2;
	else if(loss >= pfiber_sec->listen_level)
		lev = 3;

	return lev;
}



/* --------------------------------------------------------------------------*/
/**
 * @synopsis  find_alarm_on_fiber 测量结束，查找告警，比较事件点，衰减初始值
 *
 * @param ch		通道
 * @param pResult	测试曲线
 * @param pOtdrCtl	otdr控制变量
 * @param pOtdrState	otdr状态变量
 * @param 		本通道光纤段配置参数
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t find_alarm_on_fiber(int32_t ch,
	       	OTDR_UploadAllData_t *pResult,
		OtdrCtrlVariable_t *pOtdrCtl,
		OtdrStateVariable_t *pOtdrState,
		struct _tagCHFiberSec *pFibersec
	       	)
{
	struct _tagFiberSecCfg  *ppara;
	struct _tagFiberStatisData *pstatis;
	struct _tagSecFiberAlarm *palarm;
	
	struct tms_fibersection_hdr *pstd_fiber_hdr; //光纤段头
	struct tms_fibersection_val *pstd_fiber_val;//光纤段信息
	struct tms_hebei2_data_hdr  *pstd_otdr_hdr;//采样点数据头
	struct tms_hebei2_data_val  *pstd_otdr_val;//otdr数据部分
	struct tms_hebei2_event_hdr *pstd_event_hdr;//事件点头
	struct tms_hebei2_event_val *pstd_event_val;//事件缓冲区

	//包含两组告警位置 一个是最严重的告警，一个是第一个告警位置
	struct _tagEventAlarm fiber_alarm, event_alarm;
	struct _tagFiberSecCoord sec_coord;

	int32_t match_num, offset;
	OTDR_UploadAllData_t *AllEvent;
	char *NextAddr;

	float loss_sec, loss_sec_diff; 
	int32_t loss_sec_lev;
	int32_t ret, i, j, sec_start, sec_end;
	int32_t alarm_num, cur_alarm_num;
	char cur_time[20] ={0};
	const char* pFormatTime = "%Y-%m-%d %H:%M:%S";

        //获取当前时间
        get_sys_time(cur_time, 0, pFormatTime);
	//指针初始化	
	ppara = &pFibersec->para;
	palarm = &pFibersec->alarm;
	pstatis = &pFibersec->statis;
	pstd_fiber_hdr = &pFibersec->para.fiber_hdr;
	pstd_fiber_val = pFibersec->para.fiber_val;
	pstd_otdr_hdr = &pFibersec->para.otdr_hdr;
	pstd_otdr_val = pFibersec->para.otdr_val;
	pstd_event_hdr = &pFibersec->para.event_hdr;
	pstd_event_val = pFibersec->para.event_val;

	//指针转换，获取事件点的指针，如果想知道为什么，去问彭怀敏^_^
	offset = pResult->OtdrData.DataNum;
	NextAddr = (char *)(&pResult->OtdrData.dB_x1000[offset]);
	AllEvent = (OTDR_UploadAllData_t*)(NextAddr - 8 - \
			sizeof(AllEvent->MeasureParam) - sizeof(AllEvent->OtdrData));
	
	quick_unlock(&pFibersec->lock);
	if(!pFibersec->para.is_initialize)
		goto usr_exit;
	//开始查找事件点
	alarm_num = 0;
	pstatis->state = 1;
	pstatis->ch = ch + 1;
	pstatis->sec_num = pstd_fiber_hdr->count;
	pstatis->counts++;
	for(i = 0; i < pstd_fiber_hdr->count;i++)
	{
		sec_coord.start = pstd_fiber_val[i].start_coor;
		sec_coord.end = pstd_fiber_val[i].end_coor;
		loss_sec = fabs(pResult->OtdrData.dB_x1000[sec_start]- \
				pResult->OtdrData.dB_x1000[sec_end]);
		loss_sec_diff = fabs(loss_sec - pstd_fiber_val[i].fibe_atten_init);
		//光纤段统计信息
		pstatis->buf[i].attu = loss_sec;
		memcpy(pstatis->buf[i].date, cur_time, sizeof(cur_time));
		pstatis->buf[i].sec = ch + 1;
		//光纤段衰减值产生的告警级别,如果通过事件找不到告警，则判断此值
		loss_sec_lev = get_alarm_lev(loss_sec_diff,&pstd_fiber_val[i]);
		cur_alarm_num = get_fiber_alarm_from_event(
				&sec_coord,
				pstd_event_hdr,
				pstd_event_val,
				AllEvent,
				pOtdrState,
				&pstd_fiber_val[i],
				&event_alarm
				);
		/*
		 * 如果是第一次发现告警，那么全部拷贝，其他的情况下只拷贝最严重的告警
		 * 保存了我们需要的最严重的告警和最先发现的告警
		*/
		if(!alarm_num && cur_alarm_num)
			memcpy(&fiber_alarm, &event_alarm, sizeof(struct _tagEventAlarm));
		else if(cur_alarm_num && event_alarm.highest.lev < fiber_alarm.highest.lev)
			memcpy(&fiber_alarm.highest, &event_alarm.highest,\
				       	sizeof(struct _tagEventAlarmData));
		else if(!cur_alarm_num && alarm_num && loss_sec_lev < fiber_alarm.highest.lev)
		{
			//通过事件找不到告警比较loss_sec_lev与当前最严重的告警
			cur_alarm_num = 1;
			fiber_alarm.highest.diff = loss_sec;
			fiber_alarm.highest.index = -1;
			fiber_alarm.highest.lev = loss_sec_lev;
			fiber_alarm.highest.pos = sec_coord.start;
		}
		else if(!cur_alarm_num && !alarm_num && loss_sec_lev < FIBER_ALARM_LEV0)
		{
			//事件告警为0，总的告警数目为0，且loss_secc_lev有告警
			cur_alarm_num = 1;
			fiber_alarm.highest.diff = loss_sec;
			fiber_alarm.highest.index = -1;
			fiber_alarm.highest.lev = loss_sec_lev;
			fiber_alarm.highest.pos = sec_coord.start;
			memcpy(&fiber_alarm.first, &fiber_alarm.highest,\
				       	sizeof(struct _tagEventAlarmData));
		}
		alarm_num += cur_alarm_num;

	}



	ret = OP_OK;
usr_exit:
	quick_unlock(&pFibersec->lock);
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  refresh_cyc_curv_after_test 将测量结果填入周期性测量缓冲区
 *
 * @param ch		通道
 * @param pResult	测量结果
 * @param pCycCurv	周期性策略曲线缓冲
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t refresh_cyc_curv_after_test(
		int32_t ch,
		OTDR_UploadAllData_t *pResult, 
		struct _tagCycCurv *pCycCurv)
{
	int32_t ret, offset, event_count;
	OTDR_UploadAllData_t *AllEvent;
	char	*NextAddr;
	int32_t ret_cmd;
	ret = OP_OK;
	struct tms_ret_otdrparam    *pret_otdr_para;
	struct tms_test_result      *ptest_result;
	struct tms_hebei2_event_val *pevent;
	pret_otdr_para = (struct tms_ret_otdrparam *) (&pCycCurv->curv.para);
	event_count = 0;
	offset = pResult->OtdrData.DataNum;
	quick_lock(&pCycCurv->lock);
	//获取测量参数和测量结果
	get_test_para_from_algro(pret_otdr_para, pResult);
	get_test_result_from_algro(ptest_result, pResult);
	//数据点
	pCycCurv->curv.data.num = pResult->OtdrData.DataNum;
	memcpy(pCycCurv->curv.data.buf, pResult->OtdrData.dB_x1000, offset*sizeof(int16_t));
	NextAddr = (char *)(&pResult->OtdrData.dB_x1000[offset]);

	AllEvent = (OTDR_UploadAllData_t*)(NextAddr - 8 - sizeof(AllEvent->MeasureParam)\
		       	- sizeof(AllEvent->OtdrData));
	event_count = AllEvent->Event.EventNum;
	
	//获取事件点
	pevent = pCycCurv->curv.event.buf;
	get_test_event_from_algro(pevent, AllEvent,event_count);

	strcpy(pCycCurv->curv.data.id,"OTDRData\0");
	strcpy(pCycCurv->curv.result.id, "OTDRTestResultInfo\0");
	strcpy(pCycCurv->curv.event.id, "KeyEvents\0");
	quick_unlock(&pCycCurv->lock);

	return 0;
}
//按照C风格编译
#ifdef __cplusplus
}
#endif
