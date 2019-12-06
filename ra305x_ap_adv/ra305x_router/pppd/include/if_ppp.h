/*
 * if_ppp.h - Point-to-Point Protocol definitions.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef	__IF_PPP_H__
#define	__IF_PPP_H__

#ifdef  __cplusplus
extern "C" {
#endif

/* includes */


/*
 * Standard PPP header.
 */
struct ppp_header 
{
	u_char	ph_address;	/* Address Field */
	u_char 	ph_control;	/* Control Field */
	u_short	ph_protocol;	/* Protocol Field */
};

#define PPP_HDRLEN  	4	/* sizeof(struct ppp_header) must be 4 */
#define PPP_FCSLEN	2	/* octets for FCS */
#define PPPOE_HDLC_OFFSET    2
#define PPP_HDLC_ACC_LEN     2  /*  HDLC address and control field length  */
#define PPPHEADER_LEN	2

#define PPP_ALLSTATIONS 0xff    /* All-Stations broadcast address */
#define PPP_UI          0x03    /* Unnumbered Information */
#define PPP_FLAG        0x7e    /* Flag Sequence */
#define PPP_ESCAPE      0x7d    /* Asynchronous Control Escape */
#define PPP_TRANS       0x20    /* Asynchronous transparency modifier */

/*
 * Protocol field values.
 */
#define PPP_IP          0x21    /* Internet Protocol */
#define PPP_XNS         0x25    /* Xerox NS */
#define PPP_AT		0x29	/* AppleTalk Protocol */
#define PPP_IPX		0x2b	/* Internetwork Packet Exchange */
#define PPP_VJC_COMP    0x2d    /* VJ compressed TCP */
#define PPP_VJC_UNCOMP  0x2f    /* VJ uncompressed TCP */

#define PPP_IPV6	0x57	/* Internet Protocol Version 6 */
#define PPP_COMP        0xfd    /* compressed packet */

#define PPP_IPCP	0x8021	/* IP Control Protocol */
#define PPP_ATCP	0x8029	/* AppleTalk Control Protocol */
#define PPP_IPXCP	0x802b	/* IPX Control Protocol */
#define PPP_IPV6CP	0x8057	/* IPv6 Control Protocol */
#define PPP_CCP         0x80fd  /* Compression Control Protocol */

#define PPP_LCP		0xc021	/* Link Control Protocol */
#define PPP_PAP		0xc023	/* Password Authentication Protocol */
#define PPP_LQR		0xc025	/* Link Quality Report protocol */
#define PPP_CHAP	0xc223	/* Cryptographic Handshake Auth. Protocol */
#define PPP_CBCP	0xc029	/* Callback Control Protocol */

/*
 * Values for FCS calculations.
 */
#define PPP_INITFCS     0xffff  /* Initial FCS value */
#define PPP_GOODFCS     0xf0b8  /* Good final FCS value */
#define PPP_FCS(fcs, c) (((fcs) >> 8) ^ fcstab[((fcs) ^ (c)) & 0xff])

/*
 * Packet sizes
 */
#define PPP_MTU         1500    /* Default MTU (size of Info field) */
#define PPP_MRU         1500    /* Default MRU (max receive unit) */
#define PPP_MAXMRU      65000   /* Largest MRU we allow */

#define PPP_PROTOCOL(p)	((((u_char *)(p))[0] << 8) + ((u_char *)(p))[1])

/* Extended asyncmap - allows any character to be escaped. */
typedef u_long  ext_accm[8];

#if 1
/*
 * Structure describing each ppp unit.
 */
struct ppp_softc {
	struct 	ifnet sc_if;	/* network-visible interface */
	u_int	sc_flags;	/* see below */
	int	sc_unit;		/* XXX unit number */
	void    *sc_devp;       /* pointer to device-dependent structure */
	int     (*sc_start)();  /* start routine */
	short   sc_mru;         /* max receive unit */
	int	    sc_xfer;        /* used in xferring unit to another dev */
	struct  ifqueue sc_inq; /* TTY side input queue */
	struct  ifqueue sc_fastq; /* IP interactive output packet queue */
	struct	compressor *sc_xcomp;	/* transmit compressor */
	void	*sc_xc_state;		/* transmit compressor state */
	struct	compressor *sc_rcomp;	/* receive decompressor */
	void	*sc_rc_state;		/* receive decompressor state */
#ifdef  VJC
	struct  slcompress sc_comp; /* vjc control buffer */
#endif
	u_int   sc_bytessent;   /* count of octets sent */
	u_int   sc_bytesrcvd;   /* count of octets received */
	u_int	sc_ipsent;	/* count of ip packets sent */
	u_int	sc_iprcvd;	/* count of ip packets received */

        /* Device-dependent part for async lines. */
	ext_accm        sc_asyncmap;   /* async control character map */
	u_long          sc_rasyncmap;   /* receive async control char map */
	struct  mbuf    *sc_outm;  /* mbuf chain being output currently */
	struct  mbuf    *sc_m;     /* pointer to input mbuf chain */
	struct  mbuf    *sc_mc;    /* pointer to current input mbuf */
	char	        *sc_mp;		/* pointer to next char in input mbuf */
	short	        sc_ilen;	/* length of input-packet-so-far */
	u_short         sc_fcs;         /* FCS so far (input) */
	u_short         sc_outfcs;      /* FCS so far for output packet */
	u_char          sc_rawin[16];   /* chars as received */
	int             sc_rawin_count; /* # in sc_rawin */

	int	            sc_fd;		/* file descriptor */
	short	        sc_qlen;	/* length of tty input queue */
	int 	        pkt_len;	/* length of current packet */
    PPP_IF_VAR_T    *pPppIf;    /* back pointer to PPP_IF_VAR_T */
};
#endif

/* flags */
#define SC_COMP_PROT    0x00000001      /* protocol compression (output) */
#define SC_COMP_AC      0x00000002      /* header compression (output) */
#define SC_COMP_TCP     0x00000004      /* TCP (VJ) compression (output) */
#define SC_NO_TCP_CCID  0x00000008      /* disable VJ connection-id comp. */
#define SC_REJ_COMP_AC  0x00000010      /* reject adrs/ctrl comp. on input */
#define SC_REJ_COMP_TCP 0x00000020      /* reject TCP (VJ) comp. on input */
#define SC_CCP_OPEN		0x00000040		/* Look at CCP packets */
#define SC_CCP_UP		0x00000080		/* May send/recv compressed packets */
#define SC_ENABLE_IP    0x00000100      /* IP packets may be exchanged */
#define SC_DEBUG        0x00010000      /* enable debug messages */
#define SC_LOG_INPKT    0x00020000      /* log contents of good pkts recvd */
#define SC_LOG_OUTPKT   0x00040000      /* log contents of pkts sent */
#define SC_LOG_RAWIN    0x00080000      /* log all chars received */
#define SC_LOG_FLUSH    0x00100000      /* log all chars flushed */
#define SC_MASK         0x0fffffff      /* bits that user can change */

/* state bits */
#define SC_ESCAPED      0x80000000      /* saw a PPP_ESCAPE */
#define SC_FLUSH        0x40000000      /* flush input until next PPP_FLAG */
#define SC_DISCARD_PKT  0x20000000      /* Discard packet at interrupt level */
#define SC_RCV_B7_0     0x01000000      /* have rcvd char with bit 7 = 0 */
#define SC_RCV_B7_1     0x02000000      /* have rcvd char with bit 7 = 0 */
#define SC_RCV_EVNP     0x04000000      /* have rcvd char with even parity */
#define SC_RCV_ODDP     0x08000000      /* have rcvd char with odd parity */
#define SC_COMP_RUN		0x00001000		/* compressor has been inited */
#define SC_DECOMP_RUN	0x00002000		/* decompressor has been inited */
#define SC_DC_ERROR		0x00004000		/* non-fatal decomp error detected */
#define SC_DC_FERROR	0x00008000		/* fatal decomp error detected */
#define SC_VJ_RESET	    0x00000800		/* need to reset VJ decomp */

/* this stuff doesn't belong here... */
#define	PPPIOCGSEM	_IOR('t', 92, int)	/* get ppp read semaphore */
#define	PPPIOCGFD	_IOR('t', 91, int)	/* get ppp underlying fd */
#define PPPIOCGFLAGS    _IOR('t', 90, int)      /* get configuration flags */
#define PPPIOCSFLAGS    _IOW('t', 89, int)      /* set configuration flags */
#define PPPIOCGASYNCMAP _IOR('t', 88, int)      /* get async map */
#define PPPIOCSASYNCMAP _IOW('t', 87, int)      /* set async map */
#define PPPIOCGUNIT     _IOR('t', 86, int)      /* get ppp unit number */
#define PPPIOCGRASYNCMAP _IOR('t', 85, int)     /* get receive async map */
#define PPPIOCSRASYNCMAP _IOW('t', 84, int)     /* set receive async map */
#define PPPIOCGMRU      _IOR('t', 83, int)      /* get max receive unit */
#define PPPIOCSMRU      _IOW('t', 82, int)      /* set max receive unit */
#define PPPIOCSMAXCID   _IOW('t', 81, int)      /* set VJ max slot ID */
#define PPPIOCGXASYNCMAP _IOR('t', 80, ext_accm) /* get extended ACCM */
#define PPPIOCSXASYNCMAP _IOW('t', 79, ext_accm) /* set extended ACCM */
#define PPPIOCXFERUNIT  _IO('t', 78)            /* transfer PPP unit */
#define PPPIOCSCOMPRESS	_IOW('t', 77, struct ppp_option_data)

#ifdef ultrix
#define ifr_mtu ifr_ifru.ifru_metric
#endif

/* Structure describing a CCP configuration option, for PPPIOCSCOMPRESS */
struct ppp_option_data {
	u_char	*ptr;
	u_int	length;
	int	transmit;
};

struct compstat {
    u_int	unc_bytes;	/* total uncompressed bytes */
    u_int	unc_packets;	/* total uncompressed packets */
    u_int	comp_bytes;	/* compressed bytes */
    u_int	comp_packets;	/* compressed packets */
    u_int	inc_bytes;	/* incompressible bytes */
    u_int	inc_packets;	/* incompressible packets */
    u_int	ratio;		/* recent compression ratio << 8 */
};

/* old copies of PPP may have defined this */
#if !defined(ifr_mtu)
#define ifr_mtu	ifr_metric
#endif

/* function declarations */
#ifdef  __cplusplus
}
#endif

#endif	/* __IF_PPP_H__ */


