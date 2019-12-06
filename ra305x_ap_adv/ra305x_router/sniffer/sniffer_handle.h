
#ifndef __SNIFFER_HANDLE_H__
#define	__SNIFFER_HANDLE_H__

#ifndef GNU_PACKED
#define GNU_PACKED  __attribute__ ((packed))
#endif /* GNU_PACKED */


typedef unsigned char   		boolean;
typedef unsigned char			u8;
typedef int					s32;
typedef unsigned long			u32;
typedef unsigned short			u16;

typedef unsigned char			UINT8;
typedef unsigned short			UINT16;
typedef unsigned int			UINT32;
typedef unsigned long long		UINT64;
typedef int					INT32;
typedef long long 				INT64;

typedef unsigned char			UCHAR;
typedef unsigned short			USHORT;
typedef unsigned int			UINT;
typedef unsigned long			ULONG;

typedef unsigned char *		PUINT8;
typedef unsigned short *		PUINT16;
typedef unsigned int *			PUINT32;
typedef unsigned long long *	PUINT64;
typedef int *					PINT32;
typedef long long * 			PINT64;

typedef char 					STRING;
typedef signed char			CHAR;

typedef signed short			SHORT;
typedef signed int				INT;
typedef signed long			LONG;
typedef signed long long		LONGLONG;	
typedef unsigned long long		ULONGLONG;
 
typedef unsigned char			BOOLEAN;
typedef void					VOID;

typedef char *				PSTRING;
typedef VOID *				PVOID;
typedef CHAR *				PCHAR;
typedef UCHAR * 				PUCHAR;
typedef USHORT *				PUSHORT;
typedef LONG *				PLONG;
typedef ULONG *				PULONG;
typedef UINT *				PUINT;

#ifndef TRUE
#define TRUE              (1)
#define FALSE             (0)
#endif


#define	IFNAMSIZ	16

/*
 *	IEEE 802.3 Ethernet magic constants.  The frame sizes omit the preamble
 *	and FCS/CRC (frame check sequence). 
 */

#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define ETH_HLEN	14		/* Total octets in header.	 */



// Endian byte swapping codes
#define SWAP16(x) \
	((UINT16)( \
	(((UINT16)(x) & (UINT16) 0x00ffU) << 8) | \
	(((UINT16)(x) & (UINT16) 0xff00U) >> 8) ))

#define SWAP32(x) \
	((UINT32)( \
	(((UINT32)(x) & (UINT32) 0x000000ffUL) << 24) | \
	(((UINT32)(x) & (UINT32) 0x0000ff00UL) <<  8) | \
	(((UINT32)(x) & (UINT32) 0x00ff0000UL) >>  8) | \
	(((UINT32)(x) & (UINT32) 0xff000000UL) >> 24) ))

#define SWAP64(x) \
	((UINT64)( \
	(UINT64)(((UINT64)(x) & (UINT64) 0x00000000000000ffULL) << 56) | \
	(UINT64)(((UINT64)(x) & (UINT64) 0x000000000000ff00ULL) << 40) | \
	(UINT64)(((UINT64)(x) & (UINT64) 0x0000000000ff0000ULL) << 24) | \
	(UINT64)(((UINT64)(x) & (UINT64) 0x00000000ff000000ULL) <<  8) | \
	(UINT64)(((UINT64)(x) & (UINT64) 0x000000ff00000000ULL) >>  8) | \
	(UINT64)(((UINT64)(x) & (UINT64) 0x0000ff0000000000ULL) >> 24) | \
	(UINT64)(((UINT64)(x) & (UINT64) 0x00ff000000000000ULL) >> 40) | \
	(UINT64)(((UINT64)(x) & (UINT64) 0xff00000000000000ULL) >> 56) ))

/* Do not modify "big_endian" to "BIG_ENDIAN".*/
/* "BIG_ENDIAN" may be defined when compiling even your target is PC(Little Endian). */
/* "BIG_ENDIAN" ==> "big_endian" */
#ifdef big_endian
#define cpu2le64(x) SWAP64((x))
#define le2cpu64(x) SWAP64((x))
#define cpu2le32(x) SWAP32((x))
#define le2cpu32(x) SWAP32((x))
#define cpu2le16(x) SWAP16((x))
#define le2cpu16(x) SWAP16((x))
#define cpu2be64(x) ((UINT64)(x))
#define be2cpu64(x) ((UINT64)(x))
#define cpu2be32(x) ((UINT32)(x))
#define be2cpu32(x) ((UINT32)(x))
#define cpu2be16(x) ((UINT16)(x))
#define be2cpu16(x) ((UINT16)(x))

#else /* Little Endian */
#define cpu2le64(x) ((UINT64)(x))
#define le2cpu64(x) ((UINT64)(x))
#define cpu2le32(x) ((UINT32)(x))
#define le2cpu32(x) ((UINT32)(x))
#define cpu2le16(x) ((UINT16)(x))
#define le2cpu16(x) ((UINT16)(x))
#define cpu2be64(x) SWAP64((x))
#define be2cpu64(x) SWAP64((x))
#define cpu2be32(x) SWAP32((x))
#define be2cpu32(x) SWAP32((x))
#define cpu2be16(x) SWAP16((x))
#define be2cpu16(x) SWAP16((x))

#endif // big_endian //


#endif /* __SNIFFER_HANDLE_H__*/
