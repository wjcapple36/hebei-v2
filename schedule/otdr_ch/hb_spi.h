/**
 * @file hb_spi.h
 * @synopsis  spi操作的头文件
 * @author wen wjcapple@163.com
 * @version 2.0
 * @date 2016-06-24
 */
#ifndef _HB_SPI_
#define _HB_SPI_H_

#include <pthread.h>
#include "otdr_ch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_CMD_LEN	7
//描述spi操作设备	
struct _tagSpiDev
{
	pthread_mutex_t mutex;
	int32_t fd;
	//char device[64];
	uint8_t mod;
	uint8_t bits;
	uint16_t delay;
	uint32_t speed;
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
int32_t start_otdr_test(struct _tagSpiDev *dev, struct _tagFpgaPara *para);
//读取fpga累加好的数据
int32_t read_otdr_data(struct _tagSpiDev *dev, uint8_t buf[]);
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




#ifdef __cplusplus
}
#endif

#endif

