/**
 * @file hb_spi.c
 * @synopsis  定义spi操作函数
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-24
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "hb_spi.h"
#include "../common/program_run_log.h"
#include "../../protocol/SPICommand.h"


#ifdef __cplusplus
extern "C" {
#endif
int32_t initial_spi_dev(struct _tagSpiDev *pspi_dev,
		char device[],
		uint8_t mod,
		uint8_t bits,
		uint16_t delay,
		uint32_t speed)
{
	int32_t ret, fd;
	ret = OP_OK;
	fd = -1;
	fd = open(device, O_RDWR);
	if (fd == NULL) {
		printf("%s() %d: open file %s error\n",__FUNCTION__, __LINE__,device);
		ret = errno;
		goto usr_exit;
	}
	pspi_dev->fd = fd;
	pspi_dev->delay = delay;
	pspi_dev->bits = bits;
	pspi_dev->speed = speed;
	pspi_dev->mod = 0;
	pthread_mutex_init(&pspi_dev->mutex, NULL);
usr_exit:
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  spi_send_usual_cmd 发送spi命令，大部分spi命令的发送都7B，除了命令
 *		内容部分不同外，其他处理流程都是相同的,因此进行了封装.发送完成
 *		之后，马上回读，读多少字节由用户指定，因此可以验证参数发送是否
 *		成功
 * @param dev		dev 设备
 * @param tx[]		发送缓冲区
 * @param tx_bytes	发送字节数
 * @param rx[]		接收缓冲区
 * @param rx_bytes	接收字节数 
 *
 * @returns   0成功，其他错误码
 */
/* ----------------------------------------------------------------------------*/
int32_t spi_send_usual_cmd(
		struct _tagSpiDev *dev, 
		uint8_t tx[], 
		int32_t tx_bytes,
		uint8_t rx[],
		int32_t rx_bytes
		)
{
	struct spi_ioc_transfer array[8];
	int fd, ret;

	
	// transfer(fd);
	fd = dev->fd;
	for (int i = 0; i < tx_bytes;i++) {
		array[i].tx_buf = (unsigned long)tx + i;
		array[i].rx_buf = NULL;//(unsigned long)rx;
		array[i].len = 1;
		array[i].delay_usecs = dev->delay;
		array[i].speed_hz = dev->speed;
		array[i].bits_per_word = dev->bits;
	}


	array[7].tx_buf = NULL;
	array[7].rx_buf = (unsigned long)rx;
	array[7].len = rx_bytes;
	array[7].delay_usecs = dev->delay;
	array[7].speed_hz = dev->speed;
	array[7].bits_per_word = dev->bits;

	//上锁
	pthread_mutex_lock(&dev->mutex);
	ret = ioctl(fd, SPI_IOC_MESSAGE(8), &array[0]);
	pthread_mutex_unlock(&dev->mutex);

	if (ret == 1) {
		ret = errno;
		printf("%s() %d:can't send spi message errno %d ",\
				__FUNCTION__, __LINE__,errno);
		goto usr_exit;
	}

usr_exit:
	return ret;

}


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
 * @synopsis  alarm_find 发现告警，通知FPGA点灯
 *
 * @param dev	spi设备
 * @param ch	发生告警的通道
 *
 * @returns   0
 */
/* ----------------------------------------------------------------------------*/
int32_t alarm_find(struct _tagSpiDev *dev,int32_t ch)
{
	int32_t ret;
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};

	
	CmdSPIAlarmAppear(tx, ch);
	ret = spi_send_usual_cmd(dev, tx, SPI_CMD_LEN, rx, SPI_CMD_LEN);
	if(ret != OP_OK){
		goto usr_exit;
	}

usr_exit:
	return ret;

}
int32_t alarm_disappear(struct _tagSpiDev *dev,int32_t ch)
{
	int32_t ret;
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};

	
	CmdSPIAlarmDisappear(tx, ch);
	ret = spi_send_usual_cmd(dev, tx, SPI_CMD_LEN, rx, SPI_CMD_LEN);
	if(ret != OP_OK){
		goto usr_exit;
	}

usr_exit:
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
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};
	int8_t log[NUM_CHAR_LOG_MSG] = {0};
	CmdSPIRestart(tx);
	ret = spi_send_usual_cmd(dev, tx, SPI_CMD_LEN, rx, SPI_CMD_LEN);
	if(ret != OP_OK){
		goto usr_exit;
	}
	snprintf(log, NUM_CHAR_LOG_MSG,"reset by fpga!");
	LOGW(__FUNCTION__, __LINE__,LOG_LEV_FATAL_ERRO, log);
usr_exit:
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
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};

	
	CmdSPIGetPipeNum(tx);
	ret = spi_send_usual_cmd(dev, tx, SPI_CMD_LEN, rx, SPI_CMD_LEN);
	if(ret != OP_OK){
		goto usr_exit;
	}
	*pnum = rx[3];

usr_exit:
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
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};

	CmdSPIGetNetSegmentFlag(tx);
	ret = spi_send_usual_cmd(dev, tx, SPI_CMD_LEN, rx, SPI_CMD_LEN);
	if(ret != OP_OK)
		goto usr_exit;
	
	*pflag = rx[3];

usr_exit:
	return ret;

}
int32_t switch_osw(struct _tagSpiDev *dev, int32_t ch)
{
	int32_t ret;
	ret = OP_OK;
	return ret;
}



#ifdef __cplusplus
}
#endif

