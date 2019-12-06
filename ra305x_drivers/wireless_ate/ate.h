
#ifndef __ATE_H__
#define	__ATE_H__

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

//#define SIOCIWFIRSTPRIV 0x00
#define SIOCIWFIRSTPRIV	0x8BE0


#define	IFNAMSIZ	16

/*
 *	IEEE 802.3 Ethernet magic constants.  The frame sizes omit the preamble
 *	and FCS/CRC (frame check sequence). 
 */

#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define ETH_HLEN	14		/* Total octets in header.	 */

/* 
 *	Ethernet Protocol ID used by RaCfg Protocol
 */

#define ETH_P_RACFG	0x2880

/*
 * This is an RaCfg frame header 
 */
 struct racfghdr {
 	u32		magic_no;
	u16		command_type;
	u16		command_id;
	u16		length;
	u16		sequence;
	u8		data[2048];	
}  __attribute__((packed));

/* Total octets before in-band payload.	 */
#define PRE_PAYLOADLEN	(ETH_HLEN + (sizeof(struct racfghdr) - 2048/* u8 data[2048] */))

struct reg_str  {
	u32		address;
	u32		value;
} __attribute__((packed));


struct cmd_id_tbl  {
	u16		command_id;
	u16		length;
} __attribute__((packed));


#define RACFG_MAGIC_NO		0x18142880 /* RALINK:0x2880 */

/* 
 * RaCfg frame Comand Type
 */

/* use in bootstrapping state */

#define RACFG_CMD_TYPE_PASSIVE_MASK 0x7FFF

/* 
 * Bootstrapping command group 
 */

/* command type */
/* for rt28xx and rt2880 host */
/* iNIC does not need this daemon */
#define RACFG_CMD_TYPE_ETHREQ		0x0008

/* command id */
#define RACFG_CMD_RF_WRITE_ALL		0x0000
#define RACFG_CMD_E2PROM_READ16		0x0001
#define RACFG_CMD_E2PROM_WRITE16	0x0002
#define RACFG_CMD_E2PROM_READ_ALL	0x0003
#define RACFG_CMD_E2PROM_WRITE_ALL	0x0004
#define RACFG_CMD_IO_READ				0x0005
#define RACFG_CMD_IO_WRITE			0x0006
#define RACFG_CMD_IO_READ_BULK		0x0007
#define RACFG_CMD_BBP_READ8			0x0008
#define RACFG_CMD_BBP_WRITE8			0x0009
#define RACFG_CMD_BBP_READ_ALL		0x000a
#define RACFG_CMD_GET_COUNTER		0x000b
#define RACFG_CMD_CLEAR_COUNTER		0x000c
#define RACFG_CMD_RSV1					0x000d
#define RACFG_CMD_RSV2					0x000e
#define RACFG_CMD_RSV3					0x000f
#define RACFG_CMD_TX_START			0x0010
#define RACFG_CMD_GET_TX_STATUS		0x0011
#define RACFG_CMD_TX_STOP				0x0012
#define RACFG_CMD_RX_START			0x0013
#define RACFG_CMD_RX_STOP				0x0014
#define RACFG_CMD_GET_NOISE_LEVEL	0x0015

#define RACFG_CMD_ATE_START			0x0080
#define RACFG_CMD_ATE_STOP			0x0081

unsigned short cmd_id_len_tbl[] =
{
	18, 2, 4, 0, 0xffff, 4, 8, 6, 2, 3, 0, 0, 0, 0xffff, 0xffff, 0xffff, 0xffff, 0, 0, 0, 0, 2,
};

#define SIZE_OF_CMD_ID_TABLE    (sizeof(cmd_id_len_tbl) / sizeof(unsigned short) )

#define RACFG_CMD_ATE_START_TX_CARRIER		0x0100
#define RACFG_CMD_ATE_START_TX_CONT			0x0101
#define RACFG_CMD_ATE_START_TX_FRAME		0x0102
#define RACFG_CMD_ATE_SET_BW	            			0x0103
#define RACFG_CMD_ATE_SET_TX_POWER0	        	0x0104
#define RACFG_CMD_ATE_SET_TX_POWER1			0x0105
#define RACFG_CMD_ATE_SET_FREQ_OFFSET		0x0106
#define RACFG_CMD_ATE_GET_STATISTICS			0x0107
#define RACFG_CMD_ATE_RESET_COUNTER			0x0108
#define RACFG_CMD_ATE_SEL_TX_ANTENNA		0x0109
#define RACFG_CMD_ATE_SEL_RX_ANTENNA		0x010a
#define RACFG_CMD_ATE_SET_PREAMBLE			0x010b
#define RACFG_CMD_ATE_SET_CHANNEL			0x010c
#define RACFG_CMD_ATE_SET_ADDR1				0x010d
#define RACFG_CMD_ATE_SET_ADDR2				0x010e
#define RACFG_CMD_ATE_SET_ADDR3				0x010f
#define RACFG_CMD_ATE_SET_RATE				0x0110
#define RACFG_CMD_ATE_SET_TX_FRAME_LEN		0x0111
#define RACFG_CMD_ATE_SET_TX_FRAME_COUNT	0x0112
#define RACFG_CMD_ATE_START_RX_FRAME		0x0113

unsigned short ate_cmd_id_len_tbl[] =
{
	0, 0, 0, 2, 2, 2, 2, 0, 0, 2, 2, 2, 2, 6, 6, 6, 2, 2, 2, 0,
};

#define SIZE_OF_ATE_CMD_ID_TABLE    (sizeof(ate_cmd_id_len_tbl) / sizeof(unsigned short) )

#define RTPRIV_IOCTL_ATE			(SIOCIWFIRSTPRIV + 0x08)

#define RALINK_REG(x)  x

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

// define Linux ioctl relative structure, keep only necessary things
struct iw_point
{
	PVOID		pointer;
	USHORT		length;
	USHORT		flags;
};
	
union iwreq_data
{
	struct iw_point data;
};

struct iwreq {
	union
	{
		char    ifrn_name[IFNAMSIZ];    /* if name, e.g. "eth0" */
	} ifr_ifrn;
	
	union   iwreq_data      u;
};

#endif /* __ATE_H__*/
