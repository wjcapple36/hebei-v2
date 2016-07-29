/**
 * @file hb_spi.h
 * @synopsis  spi操作的头文件
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-24
 */
#ifndef _HB_SPI_H_
#define _HB_SPI_H_

#include <pthread.h>
#include "otdr_ch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_CMD_LEN	7
#define SPI_CMD_ALARM_1		0x1	//产生告警
#define SPI_CMD_ALARM_0		0x2	//告警消失
#define SPI_CMD_TESET		0x3	//测试
#define SPI_CMD_ASK_DATA	0x4	//请求数据
#define SPI_CMD_RESET		0x5	//复位
#define SPI_CMD_ASK_CH		0x6	//请求通道数目
#define SPI_CMD_ASK_SLOT	0x7	//请求槽位号
#define SPI_CMD_ASK_NET		0x8	//请求网段标志
//定义SPI操作回应码
#define SPI_RET_OP_OK			0	//spi操作成功
#define SPI_RET_SEND_FAIL		1	//发送失败
#define SPI_RET_CONFIRM_FAIL	2	//确认失败
#define	SPI_RET_NOT_RCV_DATA	3	//未收到otdr数据
#define SPI_RET_DATA_ERROR	4	//OTDR 数据格式错误
#define SPI_RET_RCV_ERROR	5	//接收错误
//描述spi操作设备	
struct _tagSpiStatis
{
	int32_t total_num;
	int32_t send_fail;
	int32_t rcv_fail;
	int32_t confirm_fail;
};
struct _tagSpiDev
{
	pthread_mutex_t mutex;
	struct _tagSpiStatis statis; //统计失败次数
	int32_t fd;
	//char device[64];
	uint8_t mod;		//模式
	uint8_t bits;		//
	uint16_t delay;		//延迟
	uint32_t speed;		//速度
	int32_t len;
	uint8_t buf[DATA_LEN*7];
	
};
//初始化spi设备
int32_t initial_spi_dev(struct _tagSpiDev *pspi_dev,
		char device[],
		uint8_t mod,
		uint8_t bits,
		uint16_t delay,
		uint32_t speed);
//一般spi命令都是7B，发送部分都是非常类似，因此封装起来
int32_t spi_send_usual_cmd(
		struct _tagSpiDev *dev, 
		uint8_t tx[], 
		int32_t tx_bytes,
		uint8_t rx[],
		int32_t rx_bytes
		);
//启动otdr测试
int32_t start_otdr_test(int32_t ch,
		struct _tagSpiDev *dev,
		const struct _tagCHPara *para,
		const struct _tagLaserCtrPara *plaser_para);
//读取fpga累加好的数据
int32_t read_otdr_data(
		int32_t ch,
		struct _tagSpiDev *dev,
		struct _tagCHPara *pch_para,
		struct _tagLaserCtrPara *plaser_para,
		uint8_t buf[],
	       	int32_t rx_bytes);
//通知fpga告警状态
int32_t alarm_find(struct _tagSpiDev *dev, int32_t ch);
//告警消失
int32_t alarm_disappear(struct _tagSpiDev *dev, int32_t ch);
//利用fpga复位
int32_t reset_by_fpfa(struct _tagSpiDev *dev);
//获取通道数目
int32_t get_ch_num(struct _tagSpiDev *dev, int32_t *pnum);
//获取网络标志
int32_t get_net_flag(struct _tagSpiDev *dev, int32_t *pflagi);
//切换光开关
int32_t switch_osw(struct _tagSpiDev *dev, int32_t ch);
//启动otdr测量
int32_t CmdSPIStartOtdrTest(
		int32_t ch,
		uint8_t *pCmdSPI,
		const struct _tagCHPara *ptest_para,
		const struct _tagLaserCtrPara *plaser_para);
//获取otdr测量状态，如果已经完成，则可以读取otdr参数
int32_t CmdSPIGetOtdrTestStatus(
		int32_t ch,
		uint8_t *pCmdSPI,
		const struct _tagCHPara *ptest_para,
		const struct _tagLaserCtrPara *plaser_para);
//从spibuf中获取读取的数据
int32_t get_data_from_spi_buf(
		int32_t data_buf[],
		int32_t data_num,
		uint8_t spi_buf[],
		int32_t spi_data_num,
		int32_t is_accum);
//从缓冲区中输出整形数据
void print_buf(int8_t buf[], int32_t len,int8_t *info);
//获取槽位号
int32_t get_dev_slot(struct _tagSpiDev *dev, int32_t *pslot); 

#ifdef __cplusplus
}
#endif

#endif
