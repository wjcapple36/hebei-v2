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
#include "../common/global.h"
#include "../../protocol/SPICommand.h"


#ifdef __cplusplus
extern "C" {
#endif
//spi操作提示信息
const char *spi_msg[] = {
	"spi operate ok!\0",
	"spi send cmd failed!\0",
	"spi cmd confirm failed!\0",
	"spi not rcv otdr data!\0",
	"spi otdr data format error!\0",
	"spi rcv otdr data error!\0"
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
	if (fd == -1) {
		ret = errno;
		printf("%s() %d: open file %s error %d\n",__FUNCTION__, __LINE__,device, ret);
		goto usr_exit;
	}
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mod);
	if (ret == -1) {
		printf("can't set spi mode");
	}
	printf("SPI_IOC_WR_MODE %d\n", mod);

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mod);
	if (ret == -1) {
		printf("can't get spi mode");
	}
	printf("SPI_IOC_RD_MODE %d\n", mod);
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		printf("can't set bits per word");
	}
	printf("SPI_IOC_WR_BITS_PER_WORD %d\n", bits);

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) {
		printf("can't get bits per word");
	}
	printf("SPI_IOC_RD_BITS_PER_WORD %d\n", bits);

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		printf("can't set max speed hz");
	}
	printf("SPI_IOC_WR_MAX_SPEED_HZ %d\n", speed);


	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		printf("can't get max speed hz");
	}
	printf("SPI_IOC_RD_MAX_SPEED_HZ %d\n", speed);

	pspi_dev->fd = fd;
	pspi_dev->delay = delay;
	pspi_dev->bits = 0;
	pspi_dev->speed = 0;
	pspi_dev->mod = mod;
	pspi_dev->len = DATA_LEN*7;
	memset(pspi_dev->buf, 0, pspi_dev->len);
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
	uint8_t res_rang, send_rang;
	is_lock = 0;
	//第一位是ch，最有1B是cmd，先保存下来，有人喜欢直接操作传进去的缓冲区
	send_ch = tx[0]&0x0f;
	send_cmd = tx[tx_bytes - 1]&0x0f;
	send_rang = tx[2]&0x0f;

	array[0].tx_buf = tx;
	array[0].rx_buf = NULL;
	array[0].len = tx_bytes;
	array[0].delay_usecs = dev->delay;
	array[0].speed_hz = dev->speed;
	array[0].bits_per_word = dev->bits;
	array[0].cs_change = 1;

	array[1].tx_buf = NULL;
	array[1].rx_buf = (unsigned long)rx;
	array[1].len = rx_bytes;
	array[1].delay_usecs = dev->delay;
	array[1].speed_hz = dev->speed;
	array[1].bits_per_word = dev->bits;
	array[1].cs_change = 1;

	//上锁
	fd = dev->fd;
	pthread_mutex_lock(&dev->mutex);
	is_lock = 1;
	ret = ioctl(fd, SPI_IOC_MESSAGE(2), &array[0]);
	dev->statis.total_num++;
	if (ret != (tx_bytes + rx_bytes)) {
		dev->statis.send_fail++;
		ret  = SPI_RET_SEND_FAIL;
		goto usr_exit;
	}
	ret = SPI_RET_OP_OK;
	/*
	* 如果验证失败，相关统计量++
	* 告警产生，告警消失，没有回应(2016-07-26)
	*/
	//请求测试回应：ch:rang
	if(send_cmd == SPI_CMD_TESET){
		res_ch = (rx[3]&0xf0) >> 4 ;
		res_rang = rx[3]&0x0f;
		if(res_rang != send_rang || res_ch != send_ch){
			dev->statis.confirm_fail++;
			ret = SPI_RET_CONFIRM_FAIL;
		}
	}
	//查询通道，网段标志回应:cmd:val
	else if(send_cmd > SPI_CMD_RESET){
		res_cmd = (rx[3]&0xf0) >> 4;
		if(res_cmd != send_cmd){
			dev->statis.confirm_fail++;
			ret = SPI_RET_CONFIRM_FAIL;
		}
	
	}

usr_exit:
	if(is_lock)
		pthread_mutex_unlock(&dev->mutex);
	if(ret != SPI_RET_OP_OK){
		printf("%s() %d: %s errno %d \n",\
				__FUNCTION__,__LINE__, spi_msg[ret],errno);
		print_buf(tx, SPI_CMD_LEN,"tx\0");
		print_buf(rx, SPI_CMD_LEN, "rx\0");
	}
	

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
#if OUTPUT_USR_MSG
	print_buf(tx, SPI_CMD_LEN, "tx");	
	print_buf(rx, SPI_CMD_LEN, "rx");	
#endif
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
	struct spi_ioc_transfer array[2];
	int32_t fd, is_lock, ret, i, rcv_data, left_bytes;
	//实际接收字节，总共接收的字节，希望接收的字节
	int32_t rcv_b, total_rcv_b, hope_rcv_b;
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};

	rcv_b = 0;
	total_rcv_b = 0;

	CmdSPIGetOtdrTestStatus(ch, tx,pch_para,plaser_para);

	array[0].tx_buf = tx;
	array[0].rx_buf = NULL;
	array[0].len = SPI_CMD_LEN;
	array[0].delay_usecs = dev->delay;
	array[0].speed_hz = dev->speed;
	array[0].bits_per_word = dev->bits;
	array[0].cs_change = 1;

	array[1].tx_buf = NULL;
	array[1].rx_buf = (unsigned long)rx;
	array[1].len = SPI_CMD_LEN;
	array[1].delay_usecs = dev->delay;
	array[1].speed_hz = dev->speed;
	array[1].bits_per_word = dev->bits;
	array[1].cs_change = 1;

	// transfer(fd);
	fd = dev->fd;
	ret =  SPI_RET_NOT_RCV_DATA;
	//上锁
	pthread_mutex_lock(&dev->mutex);
	is_lock = 1;
	rcv_b  = ioctl(fd, SPI_IOC_MESSAGE(2), &array[0]);
	dev->statis.total_num++;
	//pthread_mutex_unlock(&dev->mutex);
	//发送失败
	if (rcv_b != SPI_CMD_LEN*2) {
		dev->statis.send_fail++;
		ret = SPI_RET_RCV_ERROR;
		printf("%s() %d:can't rcv_bytes  %d errno %d\n ",\
				__FUNCTION__, __LINE__,rcv_b, errno);
		goto usr_exit;
	}
	rcv_data = 0;
	ret = SPI_RET_NOT_RCV_DATA;
	for(i = 0; i < SPI_CMD_LEN; i++)
	{
		if(rx[i] == 0xfe)
		{
			rcv_data = 1;
			total_rcv_b =  SPI_CMD_LEN -i;
			memcpy(buf, rx + i, total_rcv_b);
			break;
		}
	}
	if(!rcv_data)
		goto usr_exit;
	left_bytes = rx_bytes - total_rcv_b;
	rcv_b = 0;
	while(left_bytes > 0){
		//spi每次最大接收4096，为了少占用内核空间，每次接收2048
		if(left_bytes > 2048)
			hope_rcv_b = 2048;
		else 
			hope_rcv_b = left_bytes;

		array[0].tx_buf = NULL;
		array[0].rx_buf = (unsigned long)(buf + total_rcv_b);
		array[0].len = hope_rcv_b;
		array[0].delay_usecs = dev->delay;
		array[0].speed_hz = dev->speed;
		array[0].bits_per_word = dev->bits;
		array[0].cs_change = 1;
		rcv_b = ioctl(fd, SPI_IOC_MESSAGE(1), &array[0]);
		if(rcv_b == -1){
			ret = SPI_RET_RCV_ERROR;
			goto usr_exit;
		}
		total_rcv_b += rcv_b;
		left_bytes = rx_bytes - total_rcv_b;
		dev->statis.total_num++;
	}
	ret = OP_OK;
usr_exit:
	if(ret == SPI_RET_RCV_ERROR){
		dev->statis.rcv_fail++;
		printf("%s() %d:rcv data error, hope bytes %d rcv_btys %d errno %d \n",\
				__FUNCTION__, __LINE__,rx_bytes,total_rcv_b , errno);
	}
	if(is_lock)
		pthread_mutex_unlock(&dev->mutex);
	if(ret != SPI_RET_OP_OK){
		printf("%s() %d: ch %d  ret %d %s \n",\
				__FUNCTION__, __LINE__,ch, ret, spi_msg[ret]);
	}
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
	//LOGW(__FUNCTION__, __LINE__,LOG_LEV_FATAL_ERRO, log);
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
	if(!ret)
		*pnum = rx[3]&0x0f;
	else
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
	else
		*pflag = rx[3]&0x0f;
	return ret;
}
int32_t get_dev_slot(struct _tagSpiDev *dev, int32_t *pslot)
{

	int32_t ret;
	uint8_t tx[SPI_CMD_LEN] = {0}, rx[SPI_CMD_LEN] = {0};

	CmdSPIGetUnitNumber(tx);
	ret = spi_send_usual_cmd(dev, tx, SPI_CMD_LEN, rx, SPI_CMD_LEN);

	if(ret != SPI_RET_OP_OK)
		printf("%s() %d: ret %d %s \n",\
				__FUNCTION__, __LINE__, ret, spi_msg[ret]);
	else
		*pslot = rx[3]&0x0f;
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
	int32_t index_1, index_2;
	//先抽取250个数据进行检查，如果发现错误，则提前退出
	for(i = 0; i < spi_data_num;)
	{
		data_frame = spi_buf[i];
		//低字节存放高位，高字节存放低位
		index_1 = (spi_buf[i + 5]<<8) + spi_buf[i + 6];
		index_2 = (int)(i / 7);
		if((data_frame != 0xfe) ||(index_1 != index_2)){
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
		//首地址fe，7为周期
		for(i = 1; i < spi_data_num;i = i + 7)
		{
			data_buf[j] += ((spi_buf[i]<<24) + (spi_buf[i+1]<<16) +
			       	(spi_buf[i+2]<<8)  + spi_buf[i+3] );
			j++;
		}
	}
	else{
		for(i = 1; i < spi_data_num;i = i + 7)
		{
			data_buf[j] = ((spi_buf[i]<<24) + (spi_buf[i+1]<<16) +
			       	(spi_buf[i+2]<<8)  + spi_buf[i+3] );
			j++;


		}
	}
usr_exit:
	if(ret != SPI_RET_OP_OK)
		printf("%s() %d: ret %d %s \n", __FUNCTION__, __LINE__, ret, spi_msg[ret]);
	return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @synopsis  print_buf 将缓冲区内容输出
 *
 * @param buf[]
 * @param len
 */
/* ----------------------------------------------------------------------------*/
void print_buf(int8_t buf[], int32_t len,int8_t *info)
{
	int32_t i;
	printf("%s\n", info);
	for(i = 0; i < len;i++)
	{
		printf("0x%x ", buf[i]);
		if(i == 10)
			printf("\n");
	}
	printf("\n");
	return;
}

#ifdef __cplusplus
}
#endif

