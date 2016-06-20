/**
 ******************************************************************************
 * @file	ossocket.h
 * @brief	
 *		Windows、Linux网络错误码

 	TODO:introduce 
 *
*/
/*--------------------------------------------------
 * version    |    author    |    date    |    content
 * V1.0 		Nane		2015-1-4	
 ******************************************************************************
*/
#ifndef _OS_SOCKET_H_
#define _OS_SOCKET_H_


#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN32


#include <sys/un.h>  
#include <unistd.h> 


extern int errno;
#define LINUX
#else
//todo
#endif


// *****************************************************************************
// 系统函数

#ifndef WIN32
#define os_error() errno
//todo
#else

#define os_error() WSAGetLastError()
// todo
#endif

// *****************************************************************************
// 错误码
#ifndef WIN32

#define OSE_SUCCESS		 0  /* Success */
#define	OSE_EPERM		 1	/* Operation not permitted */
#define	OSE_ENOENT		 2	/* No such file or directory */
#define	OSE_ESRCH		 3	/* No such process */
#define	OSE_EINTR		 4	/* Interrupted system call */
#define	OSE_EIO			 5	/* I/O error */
#define	OSE_ENXIO		 6	/* No such device or address */
#define	OSE_E2BIG		 7	/* Arg list too long */
#define	OSE_ENOEXEC		 8	/* Exec format error */
#define	OSE_EBADF		 9	/* Bad file number */
#define	OSE_ECHILD		10	/* No child processes */
#define	OSE_EAGAIN		11	/* Try again */
#define	OSE_ENOMEM		12	/* Out of memory */
#define	OSE_EACCES		13	/* Permission denied */
#define	OSE_EFAULT		14	/* Bad address */
#define	OSE_ENOTBLK		15	/* Block device required */
#define	OSE_EBUSY		16	/* Device or resource busy */
#define	OSE_EEXIST		17	/* File exists */
#define	OSE_EXDEV		18	/* Cross-device link */
#define	OSE_ENODEV		19	/* No such device */
#define	OSE_ENOTDIR		20	/* Not a directory */
#define	OSE_EISDIR		21	/* Is a directory */
#define	OSE_EINVAL		22	/* Invalid argument */
#define	OSE_ENFILE		23	/* File table overflow */
#define	OSE_EMFILE		24	/* Too many open files */
#define	OSE_ENOTTY		25	/* Not a typewriter */
#define	OSE_ETXTBSY		26	/* Text file busy */
#define	OSE_EFBIG		27	/* File too large */
#define	OSE_ENOSPC		28	/* No space left on device */
#define	OSE_ESPIPE		29	/* Illegal seek */
#define	OSE_EROFS		30	/* Read-only file system */
#define	OSE_EMLINK		31	/* Too many links */
#define	OSE_EPIPE		32	/* Broken pipe */
#define	OSE_EDOM		33	/* Math argument out of domain of func */
#define	OSE_ERANGE		34	/* Math result not representable */
#define	OSE_EDEADLK		35	/* Resource deadlock would occur */
#define	OSE_ENAMETOOLONG	36	/* File name too long */
#define	OSE_ENOLCK		37	/* No record locks available */
#define	OSE_ENOSYS		38	/* Function not implemented */
#define	OSE_ENOTEMPTY	39	/* Directory not empty */
#define	OSE_ELOOP		40	/* Too many symbolic links encountered */
#define	OSE_EWOULDBLOCK	EAGAIN	/* Operation would block */
#define	OSE_ENOMSG		42	/* No message of desired type */
#define	OSE_EIDRM		43	/* Identifier removed */
#define	OSE_ECHRNG		44	/* Channel number out of range */
#define	OSE_EL2NSYNC	45	/* Level 2 not synchronized */
#define	OSE_EL3HLT		46	/* Level 3 halted */
#define	OSE_EL3RST		47	/* Level 3 reset */
#define	OSE_ELNRNG		48	/* Link number out of range */
#define	OSE_EUNATCH		49	/* Protocol driver not attached */
#define	OSE_ENOCSI		50	/* No CSI structure available */
#define	OSE_EL2HLT		51	/* Level 2 halted */
#define	OSE_EBADE		52	/* Invalid exchange */
#define	OSE_EBADR		53	/* Invalid request descriptor */
#define	OSE_EXFULL		54	/* Exchange full */
#define	OSE_ENOANO		55	/* No anode */
#define	OSE_EBADRQC		56	/* Invalid request code */
#define	OSE_EBADSLT		57	/* Invalid slot */
#define	OSE_EDEADLOCK	58	/* File locking deadlock error */
#define	OSE_EBFONT		59	/* Bad font file format */
#define	OSE_ENOSTR		60	/* Device not a stream */
#define	OSE_ENODATA		61	/* No data available */
#define	OSE_ETIME		62	/* Timer expired */
#define	OSE_ENOSR		63	/* Out of streams resources */
#define	OSE_ENONET		64	/* Machine is not on the network */
#define	OSE_ENOPKG		65	/* Package not installed */
#define	OSE_EREMOTE		66	/* Object is remote */
#define	OSE_ENOLINK		67	/* Link has been severed */
#define	OSE_EADV		68	/* Advertise error */
#define	OSE_ESRMNT		69	/* Srmount error */
#define	OSE_ECOMM		70	/* Communication error on send */
#define	OSE_EPROTO		71	/* Protocol error */
#define	OSE_EMULTIHOP	72	/* Multihop attempted */
#define	OSE_EDOTDOT		73	/* RFS specific error */
#define	OSE_EBADMSG		74	/* Not a data message */
#define	OSE_EOVERFLOW	75	/* Value too large for defined data type */
#define	OSE_ENOTUNIQ	76	/* Name not unique on network */
#define	OSE_EBADFD		77	/* File descriptor in bad state */
#define	OSE_EREMCHG		78	/* Remote address changed */
#define	OSE_ELIBACC		79	/* Can not access a needed shared library */
#define	OSE_ELIBBAD		80	/* Accessing a corrupted shared library */
#define	OSE_ELIBSCN		81	/* .lib section in a.out corrupted */
#define	OSE_ELIBMAX		82	/* Attempting to link in too many shared libraries */
#define	OSE_ELIBEXEC	83	/* Cannot exec a shared library directly */
#define	OSE_EILSEQ		84	/* Illegal byte sequence */
#define	OSE_ERESTART	85	/* Interrupted system call should be restarted */
#define	OSE_ESTRPIPE	86	/* Streams pipe error */
#define	OSE_EUSERS		87	/* Too many users */
#define	OSE_ENOTSOCK	88	/* Socket operation on non-socket */
#define	OSE_EDESTADDRREQ	89	/* Destination address required */
#define	OSE_EMSGSIZE	90	/* Message too long */
#define	OSE_EPROTOTYPE	91	/* Protocol wrong type for socket */
#define	OSE_ENOPROTOOPT	92	/* Protocol not available */
#define	OSE_EPROTONOSUPPORT	93	/* Protocol not supported */
#define	OSE_ESOCKTNOSUPPORT	94	/* Socket type not supported */
#define	OSE_EOPNOTSUPP	95	/* Operation not supported on transport endpoint */
#define	OSE_EPFNOSUPPORT	96	/* Protocol family not supported */
#define	OSE_EAFNOSUPPORT	97	/* Address family not supported by protocol */
#define	OSE_EADDRINUSE	98	/* Address already in use */
#define	OSE_EADDRNOTAVAIL	99	/* Cannot assign requested address */
#define	OSE_ENETDOWN	100	/* Network is down */
#define	OSE_ENETUNREACH	101	/* Network is unreachable */
#define	OSE_ENETRESET	102	/* Network dropped connection because of reset */
#define	OSE_ECONNABORTED	103	/* Software caused connection abort */
#define	OSE_ECONNRESET	104	/* Connection reset by peer */
#define	OSE_ENOBUFS		105	/* No buffer space available */
#define	OSE_EISCONN		106	/* Transport endpoint is already connected */
#define	OSE_ENOTCONN	107	/* Transport endpoint is not connected */
#define	OSE_ESHUTDOWN	108	/* Cannot send after transport endpoint shutdown */
#define	OSE_ETOOMANYREFS	109	/* Too many references: cannot splice */
#define	OSE_ETIMEDOUT	110	/* Connection timed out */
#define	OSE_ECONNREFUSED	111	/* Connection refused */
#define	OSE_EHOSTDOWN	112	/* Host is down */
#define	OSE_EHOSTUNREACH	113	/* No route to host */
#define	OSE_EALREADY	114	/* Operation already in progress */
#define	OSE_EINPROGRESS	115	/* Operation now in progress */
#define	OSE_ESTALE		116	/* Stale NFS file handle */
#define	OSE_EUCLEAN		117	/* Structure needs cleaning */
#define	OSE_ENOTNAM		118	/* Not a XENIX named type file */
#define	OSE_ENAVAIL		119	/* No XENIX semaphores available */
#define	OSE_EISNAM		120	/* Is a named type file */
#define	OSE_EREMOTEIO	121	/* Remote I/O error */
#define	OSE_EDQUOT		122	/* Quota exceeded */

#define	OSE_ENOMEDIUM	123	/* No medium found */
#define	OSE_EMEDIUMTYPE	124	/* Wrong medium type */

/* Should never be seen by user programs */
#define OSE_ERESTARTSYS	512
#define OSE_ERESTARTNOINTR	513
#define OSE_ERESTARTNOHAND	514	/* restart if no handler.. */
#define OSE_ENOIOCTLCMD	515	/* No ioctl command */

#define OSE__LAST_ERRNO	515

#else



#endif



// *****************************************************************************
// 互斥



// mutex lock
#if defined(LINUX) 
	#include <pthread.h>
#elif defined(WINDOWS)
	#include <windows.h>
	#include <process.h>
#endif

//Data types
#if defined(LINUX)
	#define OS_MUTEX 	pthread_mutex_t
	#define OS_THREAD 	pthread_t
#elif defined(WINDOWS)
	#define OS_MUTEX 		HANDLE
	#define OS_THREAD 	HANDLE
#endif

//Functions
int MUTEX_INIT(OS_MUTEX *mutex);
int MUTEX_LOCK(OS_MUTEX *mutex);
int MUTEX_TRYLOCK(OS_MUTEX *mutex);
int MUTEX_UNLOCK(OS_MUTEX *mutex);








#ifdef __cplusplus
}
#endif


#endif