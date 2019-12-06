#include <sys/types.h>
#include <network.h>
#include <stdio.h>
#include <netinet/if_ether.h>
#include <sys/mbuf.h>
#include <sys/types.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>
#include <ifaddrs.h>
#include <cyg/io/eth/eth_drv_stats.h>
#include <cyg/io/eth/eth_drv.h>
#include <cyg/io/eth/netdev.h>
#include <string.h>

#ifdef DBG
#include <stdarg.h>
#endif // DBG //
#include "ate.h"

#define VERSION_STR "1.2.2"

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#ifndef os_memcpy
#define os_memcpy(d, s, n) memcpy((d), (s), (n))
#endif

#ifndef os_memset
#define os_memset(s, c, n) memset(s, c, n)
#endif

#ifndef os_strlen
#define os_strlen(s) strlen(s)
#endif

#ifndef os_strncpy
#define os_strncpy(d, s, n) strncpy((d), (s), (n))
#endif

#ifndef os_strchr
#define os_strchr(s, c) strchr((s), (c))
#endif

#ifndef os_strcmp
#define os_strcmp(s1, s2) strcmp((s1), (s2))
#endif

static void SanityCheck(u16 command_id, u16 sequence, u16 len);
static void SanityCheckATE(u16 command_id, u16 sequence, u16 len);
void NetReceive(struct ifnet *ifp, u8 *inpkt, int len);
static void SendRaCfgAckFrame(struct ifnet *ifp, struct sockaddr *sa,  int len);
static void Usage(void);

/* 
 * Debugging functions - conditional printf and hex dump. 
 * Driver wrappers can use these for debugging purposes. 
 */

enum { MSG_MSGDUMP, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR };

/* default : ate_debug_level == 2 */
static int ate_debug_level = MSG_MSGDUMP;

#ifdef DBG
static void ate_hexdump(int level, char *str, unsigned char *pSrcBufVA, unsigned long SrcBufLen);
static void ate_printf(int level, char *fmt, ...);

static void ate_hexdump(int level, char *str, unsigned char *pSrcBufVA, unsigned long SrcBufLen)
{
	unsigned char *pt;
	int x;

	if (level < ate_debug_level)
		return;
	
	pt = pSrcBufVA;
	printf("%s: %p, len = %lu\n",str,  pSrcBufVA, SrcBufLen);
	for (x=0; x<SrcBufLen; x++)
	{
		if (x % 16 == 0) 
		{
			printf("0x%04x : ", x);
		}
		printf("%02x ", ((unsigned char)pt[x]));
		if (x%16 == 15) printf("\n");
	}
	printf("\n");
}

/**
 * ate_printf - conditional printf
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 *
 * Note: New line '\n' is added to the end of the text when printing to stdout.
 */
static void ate_printf(int level, char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (level >= ate_debug_level)
	{
		vprintf(fmt, ap);
		printf("\n");
	}
	va_end(ap);
}
#else // DBG //
#define ate_printf(args...) do { } while (0)
#define ate_hexdump(l,t,b,le) do { } while (0)
#endif // DBG //

static const char *ate_daemon_version = "ate daemon v" VERSION_STR "\n";
static const char broadcast_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static unsigned char packet[1536];
static unsigned char buffer[2048];
static int sock = -1;
static int signup_flag = 1;
static char driver_ifname[IFNAMSIZ + 1];
/* respond to QA tool by unicast frames if bUnicast == TRUE */
static boolean bUnicast = FALSE;
/* enable ATE daemon if ralink_ate_enable == 1, and disable if ralink_ate_enable == 0 */
bool ralink_ate_enable = TRUE;

/*
 * The Data field of RaCfgAck Frame always is empty
 * during bootstrapping state
 */
static void SendRaCfgAckFrame(struct ifnet *ifp, struct sockaddr *sa, int len)
{
#if 1
	//#define MAX_MBUF_DATA		108		// MAX_MBUF_DATA == MSIZE - sizeof(struct m_hdr)
	//#define MAX_PKTHDR_DATA	96		// MAX_PKTHDR_DATA == MSIZE - sizeof(struct m_hdr) - sizeof(struct pkthdr)
	int MAX_MBUF_DATA = MSIZE - sizeof(struct m_hdr);
	int MAX_PKTHDR_DATA = MSIZE - sizeof(struct m_hdr) - sizeof(struct pkthdr);

	int NUM_MBUF = len/MAX_MBUF_DATA + 1;
	int i = 0, j = 0;
	struct mbuf *m[NUM_MBUF];

	for(i = 0; i< NUM_MBUF; i++)
		m[i]=NULL;
	/*
	 * If the size of the incoming data is larger than 96 bytes, 
	 * the data will be segmented and stored into different mbufs.
	 */
	for(i = 0; i< NUM_MBUF; i++)
	{
		if (i == 0)
		{
			if ((m[i] = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
			{
				ate_printf(MSG_ERROR, "m_gethdr fail!\n");
				return;
			}
			else if (len <= MAX_PKTHDR_DATA) /* if there is only one mbuf, the case will handle this */
			{
				m[i]->m_len = len;
				m[i]->m_pkthdr.len = len;
				MH_ALIGN(m[i], len);
				os_memcpy(m[i]->m_data, &packet[14], len);
			}
			else
			{
				m[i]->m_len = MAX_PKTHDR_DATA;
				m[i]->m_pkthdr.len = MAX_PKTHDR_DATA;
				len -= MAX_PKTHDR_DATA;
				os_memcpy(m[i]->m_data, &packet[14], MAX_PKTHDR_DATA);
				j += MAX_PKTHDR_DATA;
			}
		}
		else
		{
			if (((m[i] = m_get(M_DONTWAIT, MT_DATA)) != NULL) && (len > 0))
			{
				if (len >= MAX_MBUF_DATA) 
				{
					m[i]->m_len = MAX_MBUF_DATA;
					len -= MAX_MBUF_DATA;
					os_memcpy(m[i]->m_data, &packet[14 + j], MAX_MBUF_DATA);
					j += MAX_MBUF_DATA;
				}
				else /* if there are more than one mbuf, this case will handle the last mbuf */
				{
					m[i]->m_len = len;
					//MH_ALIGN(m[i], len);
					os_memcpy(m[i]->m_data, &packet[14 + j], len);
				}
			}
			else
			{
				ate_printf(MSG_ERROR, "m_get fail!\n");
				return;
			}
		}		
	}

	/* chain the segmented mbufs */
	if (NUM_MBUF > 1)
	{
		for(i = 0; i < NUM_MBUF-1; i++)
			m[i]->m_next = m[i+1];
	}

	ifp->if_output(ifp, m[0], sa, (struct rtentry *)0);

#else
	struct mbuf *m;
	struct ether_header *eh;
	//struct sockaddr sa;
	//struct ether_header *p_ehead;
	//p_ehead = (struct ethhdr *)&packet[0];	

	if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
	{
		printf("m_gethdr fail!\n");
		return;
	}

	m->m_len = len;
	m->m_pkthdr.len = len;

	MH_ALIGN(m, len);

	os_memcpy(m->m_data, &packet[14], len);

	ifp->if_output(ifp, m, sa, (struct rtentry *)0);
#endif
}

static void SanityCheck(u16 Command_Id, u16 Sequence, u16 Len)
{
	/* Check for length and ID */
	/* Lengths of these two commands are not in cmd_id_len_tbl.*/
	if (((Command_Id == RACFG_CMD_ATE_START) || (Command_Id == RACFG_CMD_ATE_STOP)) && (Len == 0))
	{
		if (Command_Id == RACFG_CMD_ATE_START)
		{
			ate_printf(MSG_DEBUG, "Cmd:ATE Start\n");
		}
		else
		{
			ate_printf(MSG_DEBUG, "Cmd:ATE Stop\n");
		}
	}
	else if (((Command_Id == RACFG_CMD_ATE_START) || (Command_Id == RACFG_CMD_ATE_STOP)) && (Len != 0))
	{
			ate_printf(MSG_ERROR, "length field error, id = %x, Len = %d, len should be %d\n", Command_Id, Len, 0);
			return;
	}
	else if ((Command_Id < SIZE_OF_CMD_ID_TABLE) && (cmd_id_len_tbl[Command_Id] != 0xffff))
	{
		if (Len != cmd_id_len_tbl[Command_Id])
		{
			ate_printf(MSG_ERROR, "length field or command id error, id = %x, Len = %d, len should be %d\n", Command_Id, Len, cmd_id_len_tbl[Command_Id]);
			return;
		}
	}
	// Len of RACFG_CMD_TX_START will be 0xffff or zero.
	else if ((Command_Id == RACFG_CMD_TX_START) && (Len != 0))/* (cmd_id_len_tbl[Command_Id] == 0xffff) */
	{
		if (Len < 40) // TXWI:20 Count:2 Hlen:2 Header:2+2+6+6
		{
			ate_printf(MSG_ERROR, "Cmd:TxStart, length is too short, len = %d\n", Len);
			return;
		}
	}
	else if ((Command_Id == RACFG_CMD_TX_START) && (Len == 0))/* (cmd_id_len_tbl[Command_Id] == 0x0000) */
	{
		ate_printf(MSG_DEBUG, "Cmd:TxStart, length is zero, either for Carrier test or for Carrier Suppression\n");
	}
	else if (Command_Id == RACFG_CMD_E2PROM_WRITE_ALL)/* (cmd_id_len_tbl[Command_Id] == 0xffff) */
	{
		;
	}
	else
	{
		/* message for debug */
		ate_printf(MSG_ERROR, "command id = %x\n", Command_Id);
		return;
	}
}

static void SanityCheckATE(u16 Command_Id, u16 Sequence, u16 Len)
{
	u16 Command_Id_Offset = 0;

	Command_Id_Offset = Command_Id & (~(1 << 8));

	/* Check for length and ID */
	if ((Command_Id_Offset < SIZE_OF_ATE_CMD_ID_TABLE) && (ate_cmd_id_len_tbl[Command_Id_Offset] != 0xffff))
	{
		if (Len != ate_cmd_id_len_tbl[Command_Id_Offset])
		{
			ate_printf(MSG_ERROR, "length field or command id error, id = %x, Len = %d, len should be %d\n", Command_Id, Len, ate_cmd_id_len_tbl[Command_Id_Offset]);
			return;
		}
	}
	else
	{
		/* message for debug */
		ate_printf(MSG_ERROR, "command id = %x\n", Command_Id);
		return;
	}
	return;
}	

void NetReceive(struct ifnet *ifp, u8 *inpkt, int len)
{
	struct ether_header 	*p_ehead;
	struct racfghdr 		*p_racfgh;
	u16 					Command_Type;
	u16					Command_Id;
	u16					Sequence;
	u16 					Len;
	struct 				iwreq pwrq;
	int    				s;
	struct sockaddr 		sa;
	struct ether_header 	*eh;
    	pid_t               		pid;
	cyg_netdevtab_entry_t 	*t;
	struct eth_drv_sc 		*sc;
	struct iwreq 			param;
	int 					found = 0;

	/* 
	 * Check packet len 
	 */
	if (len < (ETH_HLEN + 12/* sizeof(struct racfghdr) */)) 
	{
		ate_printf(MSG_ERROR, "packet len is too short!\n");
		return;
	}

	p_ehead = (struct ether_header *) inpkt;
	p_racfgh = (struct racfghdr *) &inpkt[ETH_HLEN];

	/*
	 * 1. Check if dest mac is mine or broadcast
	 * 2. Ethernet Protocol ID == ETH_P_RACFG(0x2880)
	 * 3. RaCfg Frame Magic Number
	 */
	if ((p_ehead->ether_type == cpu2be16/*htons*/(ETH_P_RACFG)) && 
	    (be2cpu32/*ntohl*/(p_racfgh->magic_no) == RACFG_MAGIC_NO)) 
	{
		Command_Type = be2cpu16(p_racfgh->command_type);
		if ((Command_Type & RACFG_CMD_TYPE_PASSIVE_MASK) != RACFG_CMD_TYPE_ETHREQ)
		{
			ate_printf(MSG_ERROR, "Command_Type error = %x\n", Command_Type);
			return;
		}
	} 
	else 
	{
		ate_printf(MSG_ERROR, "protocol or magic error\n");
		return;
	}

	Command_Id = be2cpu16(p_racfgh->command_id);
	Sequence = be2cpu16(p_racfgh->sequence);
	Len	= be2cpu16(p_racfgh->length);

	//ate_printf(MSG_MSGDUMP, "NetReceive : Command_Id == %x\n", Command_Id);
	//ate_hexdump(MSG_MSGDUMP, "NetReceive : frame\n", p_ehead->ether_dhost, (PRE_PAYLOADLEN + Len));

	if (((Command_Id & (1 << 8)) == 0))
		SanityCheck(Command_Id, Sequence, Len);
	else
		SanityCheckATE(Command_Id, Sequence, Len);
		
	bzero(&packet[0], 1500);

	/*
	 * Tell the fake pid to ra0 with RACFG_CMD_ATE_STOP command.
	 * It has 4-bytes content containing the fake pid.
	 */
	if (Command_Id == RACFG_CMD_ATE_STOP)
	{		
		pid = 666; 
		/* stuff the fake pid into the content */
		os_memcpy(&p_racfgh->data[0], (u8 *)&pid, sizeof(pid));
		/* have content now */
		Len += sizeof(pid);
		p_racfgh->length = cpu2be16(Len);
		os_memcpy(&packet[14], p_racfgh, Len + 12);
	}

	os_memcpy(&packet[14], p_racfgh, Len + 12);

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, driver_ifname) == 0) 
		{
			found = 1;
			break;
	       }
	}

	if (found == 1)
	{		
		bzero(&pwrq, sizeof(pwrq));
		pwrq.u.data.pointer = (caddr_t) &packet[14];
		pwrq.u.data.length = Len + 12;
#ifdef CONFIG_AP_SUPPORT		
		rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_ATE, (caddr_t)&pwrq);
#endif
#ifdef CONFIG_STA_SUPPORT		
		rt28xx_sta_ioctl(sc, RTPRIV_IOCTL_ATE, (caddr_t)&pwrq);
#endif

	}
	
	/* add ACK bit to command type */	
	p_racfgh = (struct racfghdr *)&packet[14];		
	p_racfgh->command_type = p_racfgh->command_type | cpu2be16/*htons*/(~RACFG_CMD_TYPE_PASSIVE_MASK); 
		
	eh = (struct ether_header *)sa.sa_data;

	if (bUnicast == FALSE)
	{
		/* respond to QA tool by broadcast frames */
		os_memcpy(&eh->ether_dhost[0], broadcast_addr, 6);
	}
	else
	{
		/* respond to QA tool by unicast frames */
		os_memcpy(&eh->ether_dhost[0], p_ehead->ether_shost, 6);
	}
	
	eh->ether_type = cpu2be16/*htons*/(ETH_P_RACFG);
	sa.sa_family = AF_UNSPEC;
	sa.sa_len = sizeof(sa);
	
	/* determine the length and then send ACK */
	{
		u32 length;
		
		length = be2cpu16/*ntohs*/(p_racfgh->length);

		if (length < 46) 
		{
			length = 46;
		}
		else if (length > 1500)
		{
			ate_printf(MSG_ERROR, "response ethernet length is too long\n");
			return;
		}
		
		SendRaCfgAckFrame(ifp, &sa,  length);		
	}
	
}

static void Usage(void)
{
	printf("%s\n\n\n"
			"usage:\n"
		       "ated [start|stop|-v|-u|-h]"
		       "\n",
	       ate_daemon_version);

	printf("options:\n"
	       "  start = ATE daemon start\n"
	       "  stop = ATE daemon stop\n"
	       "  -u = respond to QA by unicast frames\n"
	       "  -v = show version\n"
	       "  -h = show help\n");

	printf("example 1:\n"
	       "  ated start\n");
	
	printf("example 2:\n"
	       "  ated stop\n");

	printf("example 3:\n"
	       "  ated -u\n");

	printf("example 4:\n"
	       "  ated -v\n");
}

int CmdATE(int argc, char* argv[])
{
	/* initialize interface */
	os_memset(driver_ifname, 0, IFNAMSIZ + 1);
	/* set default interface name */
	os_memcpy(driver_ifname, "ra0", 4);

	if((argc == 1) && (os_strcmp(argv[0], "start") == 0))
	{
		ralink_ate_enable = TRUE;
		printf("ATE daemon enabled : %d\n", ralink_ate_enable);
	}
	else if((argc == 1) && (os_strcmp(argv[0], "stop") == 0))
	{
		ralink_ate_enable = FALSE;
		printf("ATE daemon disabled : %d\n", ralink_ate_enable);
	}
	else if((argc == 1) && (os_strcmp(argv[0], "-v") == 0))
	{
		printf("%s\n", ate_daemon_version);
	}
	else if((argc == 1) && (os_strcmp(argv[0], "-u") == 0))
	{
		bUnicast = TRUE; /* respond to QA tool by unicast frames */
	}
	else if((argc == 1) && (os_strcmp(argv[0], "-h") == 0))
	{
		Usage();
	}
	else
	{
		Usage();
		return FALSE;
	}
	
	return TRUE;
	
}
