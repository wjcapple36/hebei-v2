/**
 ******************************************************************************
 * @file	glink.h
 * @brief	glink协议格式

	TODO：详细描述
 *


- 2015-3-31,Menglong Wu,DreagonWoo@163.com
 	- 编写预留接口
*/

#include "glink.h"
#include <arpa/inet.h>
#include <unistd.h>

#include "stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

void glink_HookPrint()
{
	
}




/**
 * @brief	构造帧结构
 * @param[in]	pframd_hdr
 * @param[in]	pbase_hdr
 * @param[in]	cmdid 对于不同版本glink相同的cmdid有不同的意义
 * @param[in]	datalen glink帧data内容长度

 * @retval	null
 * @remarks	构造后的glink首部可以通过glink_Send发送
 * @see	glink_Send
 */

void glink_Build(
		struct glink_base  *pbase_hdr,
		uint32_t cmdid, 
		uint32_t datalen)
{
	pbase_hdr->pklen   = htonl(GLINK_FRAME_H + GLINK_BASE_H + datalen);
	pbase_hdr->version = htonl(FRAME_VERSION);
#ifdef _MANAGE	
	// pbase_hdr->src     = htonl(GLINK_MANAGE_ADDR);
	// pbase_hdr->dst     = htonl(GLINK_CU_ADDR);

	pbase_hdr->src     = htonl(pbase_hdr->src);
	pbase_hdr->dst     = htonl(pbase_hdr->dst);
#else // MCU处理方式
	// pbase_hdr->src     = htonl(GLINK_4412_ADDR);
	// pbase_hdr->dst     = htonl(GLINK_CU_ADDR);

	// printf("build src %x dst %x\n", pbase_hdr->src, pbase_hdr->dst);
	pbase_hdr->src     = htonl(pbase_hdr->src);
	pbase_hdr->dst     = htonl(pbase_hdr->dst);

#endif
	pbase_hdr->type    = 0;//htons(0);
	pbase_hdr->pkid    = htons(pbase_hdr->pkid);//不做任何处理，无实际用途
	pbase_hdr->reserve = 0;
	pbase_hdr->cmdid   = htonl(cmdid);
	pbase_hdr->datalen = htonl(8 + datalen);

	// printf("datalen = %d\n",datalen);
	// printf("total = %d\n",htonl(pbase_hdr->pklen));
}


#include <errno.h>
#include "string.h"
void PrintfMemory(uint8_t *buf, uint32_t len);
int32_t glink_SendSerial(int fd, uint8_t *pdata, uint32_t len) 
{
	int32_t ret = 0;
	uint8_t *pbuf = pdata;
	uint32_t tlen = len;

	// printf("glink_SendSerial %d\n", len);
	// if (len > 20) {
	// 	PrintfMemory(pdata,20);	
	// }
	
	while(tlen > 0) {
		ret = send(fd, pbuf, tlen, 0);	// 
		// usleep(10000);
		// printf("ret = %d\n",ret);
		// ret = write(fd, pbuf, tlen);	// 为保证可移植性最好用这个

		// printf("ret ====== %d\n",ret);
		// perror("glink_SendSerial-------------:");
		// 发送成功部分或全部内容
		if (ret >= 0) {
			pbuf += ret;
			tlen  -= ret;
		}
		
		// 发送不出去表示发送缓存已经满了，释放CPU使用权
		else if (ret == -1){
			// printf("ret = %d fd %d  %d\n", ret, fd, errno);
			// perror("glink_SendSerial:");

			if (errno == EAGAIN) {
				// printf("-----------!!!!glink_SendSerial---------%d %s\n", errno, strerror(errno));
				sleep(0);
				continue;
			}
			else {
				return -1;
			}
		}
		// 套接字已经无效
		else if (ret == 0){
			printf("ret = %d ", ret);
			perror("glink_SendSerial:");
			return 0;
		}
	}
	// perror("glink_SendSerial:");
	return len;
}


/**
 * @brief	多个数据从小端头转换成大端头
 * @param	pdata 转换的数据
 * @param	len 转换的长度，len必须是4的倍数
 */

void glink_htonlMore(uint8_t *pdata, uint32_t len) 
{
	uint32_t *pdata32 = (uint32_t *)pdata;
	
	len = len >> 2;
	for (uint32_t i = 0; i < len; i++) {
		*pdata32 = htonl(*pdata32);
		pdata32++;
	}
}
/**
 * @brief	发送数据包，
 * @param[in]	fd
 * @param[out]	pbase_hdr 由glink_Build构造的glink协议首部
 * @param[in]	pdata 数据必须是已经经过“网络字节”转换的，转换由具体应用层处理
 			（网络字节转换多此一举）
 * @param[in]	len data长度
 * @retval	非0 发送成功
 * @retval	-1 发送失败，fd失效
 * @remarks	直到数据发送完毕才返回，fd无效，无论发送多少都返回
 * @see	glink_Build
 */

int32_t glink_Send(
		int fd,  
		struct glink_base  *pbase_hdr,
		uint8_t *pdata,
		uint32_t len)
{


	int ret   = 0;
	int32_t total = 0;
	struct glink_frame frame;

	// 钩子函数
	glink_HookPrint();

	frame.head = htonl(FRAME_HEAD_TAG);
	frame.tail = htonl(FRAME_TAIL_TAG);
	// 发送帧头
	ret = glink_SendSerial(fd, (uint8_t*)&frame.head, GLINK_HREAD_H);
	if (ret >= 0) {
		total += ret;
	}
	else {
		return -1;
	}

	// 发送glink协议首部
	ret = glink_SendSerial(fd, (uint8_t*)pbase_hdr, sizeof(struct glink_base));
	if (ret >= 0) {
		total += ret;
	}
	else {
		return -1;
	}

	// 发送应用层数据
	ret = glink_SendSerial(fd, (uint8_t*)pdata, len);
	
	if (ret >= 0) {
		total += ret;
	}
	else {
		return -1;
	}
	// 发送帧尾
	ret = glink_SendSerial(fd, (uint8_t*)&frame.tail, GLINK_END_H);
	if (ret >= 0) {
		total += ret;
	}
	else {
		return -1;
	}
	return total;
}

/**
 * @brief	发送glink帧头已经帧基本部分，用于处理应用层数据特别长的帧，分批发送
 * @param[in]	fd
 * @param[out]	pbase_hdr 由glink_Build构造的glink协议首部
 * @retval	非0 发送成功
 * @retval	-1 发送失败，fd失效
 * @remarks	
 * @see	
 */

int32_t glink_SendHead(
		int fd,  
		struct glink_base  *pbase_hdr)
{
	int ret   = 0;
	int32_t total = 0;
	struct glink_frame frame;



	frame.head = htonl(FRAME_HEAD_TAG);
	frame.tail = htonl(FRAME_TAIL_TAG);
	// 发送帧头
	ret = glink_SendSerial(fd, (uint8_t*)&frame.head, GLINK_HREAD_H);
	if (ret >= 0) {
		total += ret;
	}
	else {
		return -1;
	}

	// 发送glink协议首部
	ret = glink_SendSerial(fd, (uint8_t*)pbase_hdr, sizeof(struct glink_base));
	if (ret >= 0) {
		total += ret;
	}
	else {
		return -1;
	}

	return total;
}

/**
 * @brief	发送glink帧尾，用于处理应用层数据特别长的帧，分批发送
 * @param[in]	fd
 * @param[out]	pbase_hdr 由glink_Build构造的glink协议首部
 * @retval	非0 发送成功
 * @retval	-1 发送失败，fd失效
 * @remarks	
 * @see	
 */
int32_t glink_SendTail(
		int fd)
{
	int ret   = 0;
	int32_t total = 0;
	struct glink_frame frame;


	frame.tail = htonl(FRAME_TAIL_TAG);
	// 发送帧尾
	ret = glink_SendSerial(fd, (uint8_t*)&frame.tail, GLINK_END_H);
	if (ret >= 0) {
		total += ret;
	}
	else {
		return -1;
	}
	return total;
}

/**
 * @brief	填充目的地址、源地址
 * @param   pgcontext glink协议文件描述符
 * @param	src glink帧源地址
 * @param	dst glink帧目的地址
 * @retval	null
 * @remarks	
 * @see	
 */

int32_t glik_FillAddr(struct glink_context *pgcontext, int32_t src,int32_t dst)
{
	return 0;
}
/**
 * @brief	找到一个协议帧
 * @param[in]	pdata 数据
 * @param[in]	recvlen 数据长度
 * @param[in][out]	retframelen 返回值
 * @retval	0 找到数据帧，帧长度为retframelen
 * @retval	-1 找到帧头，但帧不完整，希望再接收retframelen长度后，再调用glink_FindFrame
 * @retval	-2 错误帧头，retframelen表示需要释放的字节数\n
 		如果整个帧找不到帧头，则建议调用者必须释放pdata前recvlen-4长度\n
 		如果找到帧头，则调用者必须释放，pdata前retframelen长度
 * @retval	-3 没找到帧尾不足，通常是超长帧>1K、100K，希望再接收retframelen长度后，再调用glink_FindFrame
 * @remarks	
 * @see	
 */

void PrintfMemory(uint8_t *buf,uint32_t len);
int32_t glink_FindFrame(int8_t *pdata, int32_t *recvlen, int32_t *retframelen)
{
	// Step 1.是否小于最小帧长
	// printf("sizeof %d %d\n",sizeof(struct glink_frame),sizeof(struct glink_base));
	if ((uint32_t)(*recvlen) <  sizeof(struct glink_frame) + sizeof(struct glink_base) ) {
	// if (sizeof(struct glink_frame) + sizeof(struct glink_base) < *recvlen ) {
		*retframelen = sizeof(struct glink_frame) + sizeof(struct glink_base) - *recvlen;
		// return -3;
		return -2;
	}


	struct glink_base *pbase;
	uint32_t *phead;
	uint32_t *ptail;
	int pklen,datalen;

	pbase = (struct glink_base *)(pdata + sizeof(int32_t));
	phead = (uint32_t*)pdata;
	// Step 2.找到帧头，起始位置找到帧头标志
	if (FRAME_HEAD_TAG != htonl(*phead)) {
		// 计算需要释放的字节帧错误
		int find = 0,i;
		// printf("search......\n");
		for (i = 0;i < *recvlen - 4;i++) {
			phead = (uint32_t*)(pdata + i);
			if (FRAME_HEAD_TAG == htonl(*phead)) {
				find = 1;

				break;
			}
		}
		if (find == 0) {
			// printf("search un find\n");
			*retframelen = *recvlen - 4;
		}
		else {
			// printf("search find\n");
			*retframelen = i;
		}
		return -2;
	}

		// 帧长度匹配
	// 帧长度 <= recvlen
	// 在帧尾处找到帧尾标志
	// pklen与datalen是否存在32字节差距
	pklen   = htonl(pbase->pklen);
	datalen = htonl(pbase->datalen);

	if ( pklen != datalen + 32) {
		*retframelen = 4;//pklen;
		return -2;
	} 



	// Step 3.是否小于该帧长度
	if (*recvlen < pklen) {
		*retframelen = pklen - *recvlen;
		return -3;
	}



	// Step 4.是否在帧末尾存在帧尾
	ptail = (uint32_t*)(pdata + pklen - 4);
	if (FRAME_TAIL_TAG != htonl(*ptail)) {
		*retframelen = pklen;
		return -2;
	}
	*retframelen = pklen;
	return 0;

	

}


#ifdef __cplusplus
}
#endif