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
//spi操作提示信息
const char *spi_msg[] = {
	"spi operate ok!\0",
	"spi send cmd failed!\0",
	"spi cmd confirm failed!\0",
	"spi otdr data format error!\0"
};
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
	memset(&pspi_dev->statis, 0, sizeof(struct _tagSpiStatis));
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
	int32_t fd, ret, is_lock;
	uint8_t res_cmd, res_ch, send_cmd, send_ch;
	is_lock = 0;
	//第一位是ch，最有1B是cmd，先保存下来，有人喜欢直接操作传进去的缓冲区
	send_ch = tx[0];
	send_cmd = tx[tx_bytes - 1];
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
	dev->statis.total_num++;
	if (ret == 1) {
		dev->statis.send_fail++;
		goto usr_exit;
	}
	ret = SPI_RET_OP_OK;
	/*
	* 啐一口，妈的，太恶心了,为什么不能将回应的命令码全部放在高四位呢
	* 你妈的，一会放在高四位，一会放在低四位
	* 如果验证失败，相关统计量++
	*/
	if(send_cmd < SPI_CMD_RESET){
		res_ch = rx[3]&0xff00;
		res_cmd = rx[3]&0x00ff;
		if(res_cmd != send_cmd || res_ch != send_ch){
			dev->statis.confirm_fail++;
			ret = SPI_RET_CONFIRM_FAIL;
		}
	}
	else if(send_cmd > SPI_CMD_RESET){
		res_cmd = rx[3]&0xff00;
		if(res_cmd != send_cmd){
			dev->statis.confirm_fail++;
			ret = SPI_RET_CONFIRM_FAIL;
		}
	
	}

usr_exit:
	if(is_lock)
		pthread_mutex_unlock(&dev->mutex);
	if(ret >= SPI_RET_OP_OK)
		printf("%s() %d: %s errno %d \n",\
			       	__FUNCTION__,__LINE__, spi_msg[ret],errno);
	else
		printf("%s() %d: ret %d errno %d \n",\
			       	__FUNCTION__,__LINE__, ret, errno);

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
int32_t start_otdr_test(
		int32_t ch,
		struct _tagSpiDev *dev,
		const struct _tagCHPara *ptest_para,
		const struct _tagLaserCtrPara *plasr_para)
{
	int32_t ret, res_ch, res_cmd;
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};
	
	CmdSPIStartTest(ch, tx, ptest_para, plasr_para);
	ret = spi_send_usual_cmd(dev, tx, SPI_CMD_LEN, rx, SPI_CMD_LEN);
	
	if(ret != SPI_RET_OP_OK)
		printf("%s() %d: ret %d %s res ch %d cmd %d\n",\
				__FUNCTION__, __LINE__, ret, spi_msg[ret],res_ch, res_cmd);
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
int32_t read_otdr_data(
		int32_t ch,
		struct _tagSpiDev *dev,
		struct _tagCHPara *pch_para,
		struct _tagLaserCtrPara *plaser_para,
		uint8_t buf[],
	       	int32_t rx_bytes)
{
	struct spi_ioc_transfer array[8];
	int32_t fd, is_lock, ret, i, rcv_data, left_bytes;
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};

	CmdSPIGetOtdrTestStatus(ch, tx,pch_para,plaser_para);
	
	// transfer(fd);
	fd = dev->fd;
	for ( i = 0; i < SPI_CMD_LEN; i++) {
		array[i].tx_buf = (unsigned long)tx + i;
		array[i].rx_buf = NULL;//(unsigned long)rx;
		array[i].len = 1;
		array[i].delay_usecs = dev->delay;
		array[i].speed_hz = dev->speed;
		array[i].bits_per_word = dev->bits;
	}
	//读7个字节，看是否读取到数据
	array[7].tx_buf = NULL;
	array[7].rx_buf = (unsigned long)rx;
	array[7].len = SPI_CMD_LEN;
	array[7].delay_usecs = dev->delay;
	array[7].speed_hz = dev->speed;
	array[7].bits_per_word = dev->bits;
	//上锁
	pthread_mutex_lock(&dev->mutex);
	is_lock = 1;
	ret = ioctl(fd, SPI_IOC_MESSAGE(8), &array[0]);
	dev->statis.total_num++;
	//pthread_mutex_unlock(&dev->mutex);
	//发送失败
	if (ret == 1) {
		dev->statis.send_fail++;
		ret = errno;
		printf("%s() %d:can't send spi message errno %d ",\
				__FUNCTION__, __LINE__,errno);
		goto usr_exit;
	}
	rcv_data = 0;
	ret = SPI_RET_NOT_RCV_DATA;
	for(i = 0; i < SPI_CMD_LEN; i++)
	{
		if(rx[i] == 0xfe)
		{
			rcv_data = 1;
			left_bytes = rx_bytes - (SPI_CMD_LEN - i);
			memcpy(buf, rx, SPI_CMD_LEN - i);
			break;
		}
	}
	if(rcv_data)
	{
		array[0].tx_buf = NULL;
		array[0].rx_buf = (unsigned long)(buf + SPI_CMD_LEN - i);
		array[0].len = rx_bytes;
		array[0].delay_usecs = dev->delay;
		array[0].speed_hz = dev->speed;
		array[0].bits_per_word = dev->bits;
		ret = ioctl(fd, SPI_IOC_MESSAGE(1), &array[0]);
		dev->statis.total_num++;

	}

	if(ret != SPI_RET_OP_OK){
		dev->statis.rcv_fail++;
		goto usr_exit;
	}
	
	
usr_exit:
	if(ret != SPI_RET_OP_OK)
		printf("%s() %d: ret %d %s \n",\
				__FUNCTION__, __LINE__, ret, spi_msg[ret]);
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
	
usr_exit:
	if(ret != SPI_RET_OP_OK)
		printf("%s() %d: ret %d %s \n",\
				__FUNCTION__, __LINE__, ret, spi_msg[ret]);
	return ret;

}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  alarm_disappear 告警消失
 *
 * @param dev
 * @param ch
 *
 * @returns   0 成功，其他失败
 */
/* ----------------------------------------------------------------------------*/
int32_t alarm_disappear(struct _tagSpiDev *dev,int32_t ch)
{
	int32_t ret;
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};

	CmdSPIAlarmDisappear(tx, ch);
	ret = spi_send_usual_cmd(dev, tx, SPI_CMD_LEN, rx, SPI_CMD_LEN);
		
	if(ret != SPI_RET_OP_OK)
		printf("%s() %d: ret %d %s \n",\
				__FUNCTION__, __LINE__, ret, spi_msg[ret]);
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
	int32_t ret, res_cmd;
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};

	res_cmd = 0;
	CmdSPIGetPipeNum(tx);
	ret = spi_send_usual_cmd(dev, tx, SPI_CMD_LEN, rx, SPI_CMD_LEN);
	if(ret != SPI_RET_OP_OK)
		printf("%s() %d: ret %d %s res cmd %d\n",\
				__FUNCTION__, __LINE__, ret, spi_msg[ret], res_cmd);
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

	if(ret != SPI_RET_OP_OK)
		printf("%s() %d: ret %d %s \n",\
				__FUNCTION__, __LINE__, ret, spi_msg[ret]);
	return ret;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  CmdSPIStartTest 重写了构造启动Otdr测试时的SPI命令，彭怀敏关于夸阻
 *		，接收机的选择逻辑更细腻
 * @param ch
 * @param pCmdSPI
 * @param ptest_para
 * @param plaser_para
 *
 * @returns 命令字节数  
 */
/* ----------------------------------------------------------------------------*/
int32_t CmdSPIStartTest(
		int32_t ch,
		uint8_t *pCmdSPI,
		const struct _tagCHPara *ptest_para,
		const struct _tagLaserCtrPara *plaser_para)
{
	int iDistanceNumber = GetDistanceNumber(ptest_para->MeasureLength_m);
	int iPulseWidthNumber = GetPulseWidthNumber(ptest_para->PulseWidth_ns);
	
	pCmdSPI[0] = (1 << 4) + ch;
	pCmdSPI[1] = (2 << 4) + iPulseWidthNumber;
	pCmdSPI[2] = (3 << 4) + iDistanceNumber;
	pCmdSPI[3] = (4 << 4) + plaser_para->power;  
	pCmdSPI[4] = (5 << 4) + plaser_para->rcv; 
	pCmdSPI[5] = (6 << 4) + plaser_para->apd; 
	pCmdSPI[6] = (7 << 4) + 3; 

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  CmdSPIGetOtdrTestStatus 重写读取otdr测试状态，使用启动测试时相同的
 *		测试参数
 * @param ch
 * @param pCmdSPI
 * @param ptest_para
 * @param plaser_para
 *
 * @returns  命令字节数
 */
/* ----------------------------------------------------------------------------*/
int32_t CmdSPIGetOtdrTestStatus(
		int32_t ch,
		uint8_t *pCmdSPI,
		const struct _tagCHPara *ptest_para,
		const struct _tagLaserCtrPara *plaser_para)
{
	int iDistanceNumber = GetDistanceNumber(ptest_para->MeasureLength_m);
	int iPulseWidthNumber = GetPulseWidthNumber(ptest_para->PulseWidth_ns);
	
	pCmdSPI[0] = (1 << 4) + ch;
	pCmdSPI[1] = (2 << 4) + iPulseWidthNumber;
	pCmdSPI[2] = (3 << 4) + iDistanceNumber;
	pCmdSPI[3] = (4 << 4) + plaser_para->power;  
	pCmdSPI[4] = (5 << 4) + plaser_para->rcv; 
	pCmdSPI[5] = (6 << 4) + plaser_para->apd; 
	pCmdSPI[6] = (7 << 4) + 4; 

	return FPGA_COMMAND_FRAME_BYTE_NUM;
}
/* --------------------------------------------------------------------------*/
/**
 * @synopsis  get_data_from_spi_buf 数据读取之后按照格式存放于spi_buf缓冲区中，
 *		本函数将其取出，并进行累加或者直接获取
 * @param data_buf[]
 * @param data_num
 * @param spi_buf[]
 * @param spi_data_num
 * @param is_accum
 *
 * @returns   0成功，其他格式不对
 */
/* ----------------------------------------------------------------------------*/
int32_t get_data_from_spi_buf(
		int32_t data_buf[],
		int32_t data_num,
		uint8_t spi_buf[],
		int32_t spi_data_num,
		int32_t is_accum)
{
	int32_t ret, i, j;
	uint8_t data_frame;
	uint16_t *pdata_addr;
	//先抽取250个数据进行检查，如果发现错误，则提前退出
	for(i = 0; i < spi_data_num;)
	{
		data_frame = spi_buf[i];
		pdata_addr = (uint16_t *)(spi_buf + i + 5);
		if(data_frame != 0xfe ||*pdata_addr != i){
			ret = SPI_RET_DATA_ERROR;
			goto usr_exit;
		}
		/*
		 * 假如16000个int类型的数据，格式fe 4B 2B fe 4B 2B
		 *
		*/ 
		i = i + 64*7;
					 	
	}
	ret = SPI_RET_OP_OK;
	j = 0;
	if(is_accum){
		for(i = 0; i < spi_data_num;i = i + 7)
		{
			data_buf[j] += *((int32_t *)(spi_buf + i + 1));
			j++;
		}
	}
	else{
		for(i = 0; i < spi_data_num;i++)
		{
			data_buf[j++] = *((int32_t *)(spi_buf + i + 1));

		}
	}
usr_exit:
	if(ret != SPI_RET_OP_OK)
		printf("%s() %d: ret %d %s \n", __FUNCTION__, __LINE__, ret, spi_msg[ret]);
	return ret;
}



#ifdef __cplusplus
}
#endif

