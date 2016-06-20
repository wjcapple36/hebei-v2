/**
 ******************************************************************************
 * @file	bipbuffer.h
 * @brief	The Circular Buffer with a Twist

reference:http://www.codeproject.com/Articles/3479/The-Bip-Buffer-The-Circular-Buffer-with-a-Twist
 *

- 2003,Simon Cooke
 	- raw C++ code BipBuffer.hpp

- 2015-3-1,Menglong Wu,DreagonWoo@163.com
 	- Change to c code
 	- Add find a frame in buffer,bipbuffer_DataData()
 	- Add bipbuffer_FrameLen()
*/

#ifndef _BIP_BUFFER_H_
#define _BIP_BUFFER_H_

// #include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


#define bool int 
#define false 0
#define true 1

struct bipbuffer
{
	char* pBuffer;
	int ixa;
	int sza;
	int ixb;
	int szb;
	int buflen;
	int ixResrv;
	int szResrv;
};
#define OUT

void  bipbuffer_FreeBuffer(struct bipbuffer *bb);
void  bipbuffer_Init(struct bipbuffer *bb);
bool  bipbuffer_AllocateBuffer(struct bipbuffer *bb, int buffersize);
void  bipbuffer_Clear(struct bipbuffer *bb);
void  bipbuffer_FreeBuffer(struct bipbuffer *bb);
char* bipbuffer_Reserve(struct bipbuffer *bb, int size, OUT int *reserved);
void  bipbuffer_Commit(struct bipbuffer *bb, int size);
char* bipbuffer_GetContiguousBlock  (struct bipbuffer *bb,OUT int *size);
char *bipbuffer_GetUnContiguousBlock(struct bipbuffer *bb,OUT int *size);
void  bipbuffer_DecommitBlock(struct bipbuffer *bb,int size);
void  bipbuffer_DecommitMulti(struct bipbuffer *bb,int size);
void  bipbuffer_DecommitTo   (struct bipbuffer *bb,void *ptrto);
bool  bipbuffer_IsSequence(struct bipbuffer *bb, void *pfrom, void *pto);
bool  bipbuffer_IsSequenceAll(struct bipbuffer *bb);
char *bipbuffer_Offset(struct bipbuffer *bb, void *ref, unsigned int offset);
void  bipbuffer_Copy2User(struct bipbuffer *bb, int offset, char *puser, int size);
int   bipbuffer_GetCommittedSize(struct bipbuffer *bb) ;
int   bipbuffer_GetReservationSize(struct bipbuffer *bb);
int   bipbuffer_GetUsedSize(struct bipbuffer *bb);
int   bipbuffer_GetBufferSize(struct bipbuffer *bb) ;
bool  bipbuffer_IsInitialized(struct bipbuffer *bb) ;
char *bipbuffer_DataData(struct bipbuffer *bb, char *sub,int len);
unsigned int bipbuffer_FrameLen(struct bipbuffer *bb, void *saddr, void *eaddr);
void  bipbuffer_PrintMemory(struct bipbuffer *bb);
void bipbuffer_WriteBack(struct bipbuffer *bb, struct bipbuffer *tbb);


#ifdef __cplusplus
}
#endif

#endif