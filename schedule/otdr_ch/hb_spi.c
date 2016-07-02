/**
 * @file hb_spi.c
 * @synopsis  定义spi操作函数
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-24
 */

#include "hb_spi.h"
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  start_otdr_test 启动otdr测量
 *
 * @param dev
 * @param para
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t start_otdr_test(struct _tagSpiDev *dev, struct _tagFpgaPara *para)
{
	int32_t ret;
	ret = OP_OK;
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  read_otdr_data 读取otdr数据
 *
 * @param dev
 * @param buf[]	缓冲区
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t read_otdr_data(struct _tagSpiDev *dev, uint8_t buf[])
{
	int32_t ret;
	ret = OP_OK;
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  notify_alarm2fpga 通知fpga有告警
 *
 * @param dev
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t notify_alarm2fpga(struct _tagSpiDev *dev)
{
	int32_t ret;
	ret = OP_OK;
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  reset_by_fpfa 通知FPGA复位整个系统
 *
 * @param dev
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t reset_by_fpfa(struct _tagSpiDev *dev)
{
	int32_t ret;
	ret = OP_OK;
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_ch_num	获取fpga通道哦阿数目
 *
 * @param dev
 * @param pnum	通道数目
 *
 * @returns   
 */
/* ----------------------------------------------------------------------------*/
int32_t get_ch_num(struct _tagSpiDev *dev, int32_t *pnum)
{
	int32_t ret;
	ret = OP_OK;
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_net_flag 获取网段标志
 *
 * @param dev	所利用的spi设备
 * @param pflag	网段标志
 *
 * @returns  0 成功 
 */
/* ----------------------------------------------------------------------------*/
int32_t get_net_flag(struct _tagSpiDev *dev, int32_t *pflag)
{
	int32_t ret;
	ret = OP_OK;
	return ret;
}
int32_t switch_osw(struct _tagSpiDev *dev, int32_t ch)
{
	int32_t ret;
	ret = OP_OK;
	return ret;
}
