// MD5.h: interface for the CMD5 class.
// BigLee 2006.03.30 Shanghai
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MD5_H__EE5FF5CA_A974_455A_9FB1_A9E466ECACE1__INCLUDED_)
#define AFX_MD5_H__EE5FF5CA_A974_455A_9FB1_A9E466ECACE1__INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define uchar unsigned char
typedef unsigned char *POINTER;
typedef unsigned short int UINT2;
typedef unsigned long int UINT4;
#define PROTO_LIST(list) list

// #pragma pack(1)
//MD5 context.
typedef struct md5_ctx {
  UINT4 state[4];                                   // state (ABCD)
  UINT4 count[2];        // number of bits, modulo 2^64 (lsb first)
  unsigned char buffer[64];                         // input buffer
} MD5_CTX;
// #pragma pack(4)
/* Constants for MD5Transform routine.*/

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21


// static unsigned char PADDING[64] = {
//   0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* F, G, H and I are basic MD5 functions.*/
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits.*/
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
Rotation is separate from addition to prevent recomputation.
*/
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }


#ifdef  USE_MD5
class CMD5  
{
public:
	static void MD5_memcpy (POINTER output, POINTER input, unsigned int len);
	static void MD5_memset (POINTER output, int value, unsigned int len);
	static void Decode (UINT4 *output, unsigned char *input, unsigned int len);
	static void Encode (unsigned char *output, UINT4 *input, unsigned int len);
	static char * MD5(unsigned char * src,unsigned int inlen,char * pszHex);
	static unsigned char * MD5Int(unsigned char *src, unsigned int inlen, unsigned char *pszHex);
	static unsigned char * MD5Int2Str(unsigned char *pint, unsigned char *pstr);
	static void MD5Final(unsigned char *digest,MD5_CTX *context);
	static void MD5Final (MD5_CTX *context);
	static void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int inputLen);
	static void MD5Init(MD5_CTX *context);
	static void MD5Transform (UINT4 state[4], unsigned char block[64]);
	CMD5();
	virtual ~CMD5();

};
#endif


#ifdef __cplusplus
}
#endif

#endif // !defined(AFX_MD5_H__EE5FF5CA_A974_455A_9FB1_A9E466ECACE1__INCLUDED_)
