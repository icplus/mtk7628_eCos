
/*
 * ng_pppoe.h
 *
 * Copyright (c) 1996-1999 Whistle Communications, Inc.
 * All rights reserved.
 * 
 * Subject to the following obligations and disclaimer of warranty, use and
 * redistribution of this software, in source or object code forms, with or
 * without modifications are expressly permitted by Whistle Communications;
 * provided, however, that:
 * 1. Any and all reproductions of the source or object code must include the
 *    copyright notice above and the following disclaimer of warranties; and
 * 2. No rights are granted, in any manner or form, to use Whistle
 *    Communications, Inc. trademarks, including the mark "WHISTLE
 *    COMMUNICATIONS" on advertising, endorsements, or otherwise except as
 *    such appears in the above copyright notice or in the software.
 * 
 * THIS SOFTWARE IS BEING PROVIDED BY WHISTLE COMMUNICATIONS "AS IS", AND
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, WHISTLE COMMUNICATIONS MAKES NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, REGARDING THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION, ANY AND ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
 * WHISTLE COMMUNICATIONS DOES NOT WARRANT, GUARANTEE, OR MAKE ANY
 * REPRESENTATIONS REGARDING THE USE OF, OR THE RESULTS OF THE USE OF THIS
 * SOFTWARE IN TERMS OF ITS CORRECTNESS, ACCURACY, RELIABILITY OR OTHERWISE.
 * IN NO EVENT SHALL WHISTLE COMMUNICATIONS BE LIABLE FOR ANY DAMAGES
 * RESULTING FROM OR ARISING OUT OF ANY USE OF THIS SOFTWARE, INCLUDING
 * WITHOUT LIMITATION, ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * PUNITIVE, OR CONSEQUENTIAL DAMAGES, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES, LOSS OF USE, DATA OR PROFITS, HOWEVER CAUSED AND UNDER ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF WHISTLE COMMUNICATIONS IS ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Julian Elischer <julian@freebsd.org>
 *
 * $FreeBSD: src/sys/netgraph/ng_pppoe.h,v 1.7.2.5 2002/06/17 02:19:06 brian Exp $
 * $Whistle: ng_pppoe.h,v 1.7 1999/10/16 10:16:43 julian Exp $
 */

#ifndef _NETGRAPH_PPPOE_H_
#define _NETGRAPH_PPPOE_H_

/********************************************************************
 * Netgraph hook constants etc.
 ********************************************************************/
/* Node type name. This should be unique among all netgraph node types */
#define NG_PPPOE_NODE_TYPE			"pppoe"
#define NGM_PPPOE_COOKIE			939032003

/* Number of active sessions we can handle */
//#define	PPPOE_NUM_SESSIONS			16 /* for now */
#define	PPPOE_SERVICE_NAME_SIZE		64 /* for now */

/* Hook names */
#define NG_PPPOE_HOOK_ETHERNET		"ethernet"
#define NG_PPPOE_HOOK_PADI			"PADI"    /* default PADI requests come here */
#define NG_PPPOE_HOOK_S_LEADIN		"service" /* PADO responses from PADI */
#define NG_PPPOE_HOOK_C_LEADIN		"client"  /* Connect message starts this */
#define NG_PPPOE_HOOK_DEBUG			"debug"

#define M_NETGRAPH  M_TEMP //????

/**********************************************************************
 * Netgraph commands understood by this node type.
 * FAIL, SUCCESS, CLOSE and ACNAME are sent by the node rather than received.
 ********************************************************************/
enum cmd
{
	NGM_PPPOE_SET_FLAG = 1,
	NGM_PPPOE_CONNECT  = 2,	/* Client, Try find this service */
	NGM_PPPOE_LISTEN   = 3,	/* Server, Await a request for this service */
	NGM_PPPOE_OFFER    = 4,	/* Server, hook X should respond (*) */
	NGM_PPPOE_SUCCESS  = 5,	/* State machine connected */
	NGM_PPPOE_FAIL     = 6,	/* State machine could not connect */
	NGM_PPPOE_CLOSE    = 7,	/* Session closed down */
	NGM_PPPOE_SERVICE  = 8,	/* additional Service to advertise (in PADO) */
	NGM_PPPOE_ACNAME   = 9,	/* AC_NAME for informational purposes */
	NGM_PPPOE_GET_STATUS = 10, /* data in/out */
	NGM_PPPOE_SESSIONID  = 11  /* Session_ID for informational purposes */
};

/***********************
 * Structures passed in the various netgraph command messages.
 ***********************/
/* This structure is returned by the NGM_PPPOE_GET_STATUS command */
struct ngpppoestat
{
	u_int   packets_in;	/* packets in from ethernet */
	u_int   packets_out;	/* packets out towards ethernet */
};

/*
 * When this structure is accepted by the NGM_PPPOE_CONNECT command :
 * The data field is MANDATORY.
 * The session sends out a PADI request for the named service.
 *
 *
 * When this structure is accepted by the NGM_PPPOE_LISTEN command.
 * If no service is given this is assumed to accept ALL PADI requests.
 * This may at some time take a regexp expression, but not yet.
 * Matching PADI requests will be passed up the named hook.
 *
 *
 * When this structure is accepted by the NGM_PPPOE_OFFER command:
 * The AC-NAme field is set from that given and a PADI
 * packet is expected to arrive from the session control daemon, on the
 * named hook. The session will then issue the appropriate PADO
 * and begin negotiation.
 */
#define NG_HOOKLEN	15	/* max hook name len (16 with null) */
struct ngpppoe_init_data
{
	char	hook[NG_HOOKLEN + 1];	/* hook to monitor on */
	u_int16_t	data_len;		/* Length of the service name */
	char	data[0];		/* init data goes here */
};

/*
 * This structure is used by the asychronous success and failure messages.
 * (to report which hook has failed or connected). The message is sent
 * to whoever requested the connection. (close may use this too).
 */
struct ngpppoe_sts
{
	char	hook[NG_HOOKLEN + 1]; /* hook associated with event session */
};


/********************************************************************
 * Constants and definitions specific to pppoe
 ********************************************************************/
//#define PPPOE_TIMEOUT_LIMIT		64
#define PPPOE_TIMEOUT_LIMIT		16
#define PPPOE_OFFER_TIMEOUT		16
#define PPPOE_INITIAL_TIMEOUT	2

/* Codes to identify message types */
#define PADI_CODE	0x09
#define PADO_CODE	0x07
#define PADR_CODE	0x19
#define PADS_CODE	0x65
#define PADT_CODE	0xa7

/* Tag identifiers */
#if BYTE_ORDER == BIG_ENDIAN
#define PTT_EOL			(0x0000)
#define PTT_SRV_NAME	(0x0101)
#define PTT_AC_NAME		(0x0102)
#define PTT_HOST_UNIQ	(0x0103)
#define PTT_AC_COOKIE	(0x0104)
#define PTT_VENDOR 		(0x0105)
#define PTT_RELAY_SID	(0x0106)
#define PTT_SRV_ERR     (0x0201)
#define PTT_SYS_ERR  	(0x0202)
#define PTT_GEN_ERR  	(0x0203)

#define ETHERTYPE_PPPOE_DISC	0x8863	/* pppoe discovery packets     */
#define ETHERTYPE_PPPOE_SESS	0x8864	/* pppoe session packets       */
#define ETHERTYPE_PPPOE_STUPID_DISC 0x3c12 /* pppoe discovery packets 3com? */
#define ETHERTYPE_PPPOE_STUPID_SESS 0x3c13 /* pppoe session packets   3com? */
#else
#define PTT_EOL			(0x0000)
#define PTT_SRV_NAME	(0x0101)
#define PTT_AC_NAME		(0x0201)
#define PTT_HOST_UNIQ	(0x0301)
#define PTT_AC_COOKIE	(0x0401)
#define PTT_VENDOR 		(0x0501)
#define PTT_RELAY_SID	(0x0601)
#define PTT_SRV_ERR     (0x0102)
#define PTT_SYS_ERR  	(0x0202)
#define PTT_GEN_ERR  	(0x0302)

#define ETHERTYPE_PPPOE_DISC	0x6388	/* pppoe discovery packets     */
#define ETHERTYPE_PPPOE_SESS	0x6488	/* pppoe session packets       */
#define ETHERTYPE_PPPOE_STUPID_DISC 0x123c /* pppoe discovery packets 3com? */
#define ETHERTYPE_PPPOE_STUPID_SESS 0x133c /* pppoe session packets   3com? */
#endif

struct pppoe_tag
{
	u_int16_t tag_type;
	u_int16_t tag_len;
	char tag_data[0];
}__attribute ((packed));

struct pppoe_hdr
{
	u_int8_t ver:4;
	u_int8_t type:4;
	u_int8_t code;
	u_int16_t sid;
	u_int16_t length;
	struct pppoe_tag tag[0];
}__attribute__ ((packed));

#if 0
#define	ETHER_ADDR_LEN	6
struct	ether_header {
	u_int8_t  ether_dhost[ETHER_ADDR_LEN];
	u_int8_t  ether_shost[ETHER_ADDR_LEN];
	u_int16_t ether_type;
} __attribute__ ((aligned(1), packed));
#endif

struct pppoe_full_hdr
{
	struct ether_header eh;
	struct pppoe_hdr ph;
}__attribute__ ((packed));

union	packet
{
	struct pppoe_full_hdr pkt_header;
	u_int8_t bytes[2048];
};

struct datatag
{
	struct pppoe_tag hdr;
	u_int8_t data[PPPOE_SERVICE_NAME_SIZE];
};


/*
 * Define the order in which we will place tags in packets
 * this may be ignored
 */
/* for PADI */
#define TAGI_SVC		0
#define TAGI_HUNIQ		1
/* for PADO */
#define TAGO_ACNAME		0
#define TAGO_SVC		1
#define TAGO_COOKIE		2
#define TAGO_HUNIQ		3
/* for PADR */
#define TAGR_SVC		0
#define TAGR_HUNIQ		1
#define TAGR_COOKIE		2
/* for PADS */
#define TAGS_ACNAME		0
#define TAGS_SVC		1
#define TAGS_COOKIE		2
#define TAGS_HUNIQ		3
/* for PADT */

/* for netgraph */
#define NGF_RESP		0x0001		/* the message is a response */
#define NG_VERSION		2
/* A netgraph message */
#define NG_CMDSTRLEN	15	/* max command string (16 with null) */
struct ng_mesg
{
	struct	ng_msghdr
	{
		u_char		version;		/* must == NG_VERSION */
		u_char		spare;			/* pad to 2 bytes */
		u_int16_t	arglen;			/* length of data */
		u_int32_t	flags;			/* message status */
		u_int32_t	token;			/* match with reply */
		u_int32_t	typecookie;		/* node's type cookie */
		u_int32_t	cmd;			/* command identifier */
		u_char		cmdstr[NG_CMDSTRLEN+1];	/* cmd string + \0 */
	} header;
	char	data[0];		/* placeholder for actual data */
};

#define NG_MKMESSAGE(msg, cookie, cmdid, len, how)			\
	do {								\
	  MALLOC((msg), struct ng_mesg *, sizeof(struct ng_mesg)	\
	    + (len), M_NETGRAPH, (how));				\
	  if ((msg) == NULL)						\
	    break;							\
	  bzero((msg), sizeof(struct ng_mesg) + (len));			\
	  (msg)->header.version = NG_VERSION;				\
	  (msg)->header.typecookie = (cookie);				\
	  (msg)->header.cmd = (cmdid);					\
	  (msg)->header.arglen = (len);					\
	  strncpy((msg)->header.cmdstr, #cmdid,				\
	    sizeof((msg)->header.cmdstr) - 1);				\
	} while (0)
#if 0
#define NG_FREE_M(m)							\
	do {								\
		if ((m)) {						\
			m_freem((m));					\
			(m) = NULL;					\
		}							\
	} while (0)
#else
#define NG_FREE_M(m) {if(m) m_freem(m);}
#endif
#define NG_FREE_DATA  NG_FREE_M

#if 0
#define NG_MKRESPONSE(rsp, msg, len, how)				\
	do {								\
	  MALLOC((rsp), struct ng_mesg *, sizeof(struct ng_mesg)	\
	    + (len), M_NETGRAPH, (how)); 				\
	  if ((rsp) == NULL)						\
	    break;							\
	  bzero((rsp), sizeof(struct ng_mesg) + (len));			\
	  (rsp)->header.version = NG_VERSION;				\
	  (rsp)->header.arglen = (len);					\
	  (rsp)->header.token = (msg)->header.token;			\
	  (rsp)->header.typecookie = (msg)->header.typecookie;		\
	  (rsp)->header.cmd = (msg)->header.cmd;			\
	  bcopy((msg)->header.cmdstr, (rsp)->header.cmdstr,		\
	    sizeof((rsp)->header.cmdstr));				\
	  (rsp)->header.flags |= NGF_RESP;				\
	} while (0)
#endif

#define PPPOE_MAX_IDLE_TIME     60
#define PPPOE_MAX_MTU           1492

/*
 * States for the session state machine.
 * These have no meaning if there is no hook attached yet.
 */
enum state
{
    PPPOE_SNONE=0,	/* [both] Initial state */
    PPPOE_LISTENING,	/* [Daemon] Listening for discover initiation pkt */
    PPPOE_SINIT,	/* [Client] Sent discovery initiation */
    PPPOE_PRIMED,	/* [Server] Awaiting PADI from daemon */
    PPPOE_SOFFER,	/* [Server] Sent offer message  (got PADI)*/
    PPPOE_SREQ,		/* [Client] Sent a Request */
    PPPOE_NEWCONNECTED,	/* [Server] Connection established, No data received */
    PPPOE_CONNECTED,	/* [Both] Connection established, Data received */
    PPPOE_DEAD		/* [Both] */
};

#define NUMTAGS 20 /* number of tags we are set up to work with */

/*
 * Information we store for each hook on each node for negotiating the 
 * session. The mbuf and cluster are freed once negotiation has completed.
 * The whole negotiation block is then discarded.
 */

struct sess_neg
{
	struct mbuf *m;			/* holds cluster with last sent packet */
	union packet *pkt;		/* points within the above cluster */
	u_int timeout;			/* 0,1,2,4,8,16 etc. seconds */
	u_int numtags;
	const struct pppoe_tag	*tags[NUMTAGS];
	
	u_int service_len;
	u_int ac_name_len;
	struct datatag service;
	struct datatag ac_name;
};
typedef struct sess_neg *negp;

/*
 * Session information that is needed after connection.
 */
struct sess_con
{
	u_int16_t Session_ID;
	enum state state;
	struct pppoe_full_hdr pkt_hdr;		/* used when connected */
	struct sess_neg neg;				/* used when negotiating */
};
typedef struct sess_con *sessp;


#define IF_NAME_LEN  8
#define PPPOE_MAX_USRNAME_LEN  255
#define PPPOE_MAX_PASSWD_LEN   255

typedef struct PPPOE_PARAM_STRUCT_S
{
	int enabled;
	int unit;
	char ppp_name[IFNAMSIZ];
	char eth_name[IFNAMSIZ];
	char org_usrname[PPPOE_MAX_USRNAME_LEN+33];
	char username[PPPOE_MAX_USRNAME_LEN+33];
	char password[PPPOE_MAX_PASSWD_LEN+33];
	char service_name[PPPOE_MAX_USRNAME_LEN+33];
	char ac_name[PPPOE_MAX_USRNAME_LEN+33];
	int idle_time;
	int connect_mode;
	int mtu;
	u_long my_ip;
	u_long netmask;
	u_long peer_ip;
	u_long pri_dns;
	u_long snd_dns;
	int	dns_en;
	u_long dns[2];
	int	dflrt;
	int	start_time;
	int end_time;
	int passwd_algorithm;
	char opass[256];
}
PPPOE_PARAM_STRUCT;

#define ENET_LEN   6

#define e_unit				if_ppp.unit           /* PPP unit number */
#define e_ifname			if_ppp.ifname        /* PPP layer interfcae name */
#define e_pppname			if_ppp.pppname        /* PPP layer interfcae name */
#define e_type				if_ppp.encapsulate_type  /* type of interface device */
#define e_user				if_ppp.user         /* authentication username */ 
#define e_password			if_ppp.passwd      /* password for PAP  */
#define e_idle				if_ppp.idle         /* idle time */
#define e_mtu				if_ppp.mtu          /* MTU that we allow */
#define e_mru				if_ppp.mru          /* MRU that we allow */
#define e_netmask			if_ppp.netmask       /* netmask */
#define e_myip				if_ppp.my_ip         /* my IP addrss */
#define e_peerip			if_ppp.peer_ip        /* Peer IP address */
#define	e_pridns			if_ppp.pri_dns
#define e_snddns			if_ppp.snd_dns
#define e_defaultroute		if_ppp.defaultroute /* flag to specify this interfce whether is defaultroute */
#define e_pOutputHook		if_ppp.pOutput      /* output packet function pointer */
#define e_pIfStopHook		if_ppp.pIfStopHook       /* functon called by PPP daemon to close pppoe interface */
#define e_pIfReconnectHook	if_ppp.pIfReconnectHook /* physical layer reconnection hook routine */ 

#define	AUTH_BLOCK_NONE			0
#define	AUTH_BLOCK_CANDIDATE	1
#define	AUTH_BLOCKED			2

#define NUM_AUTH_FAIL_LOG		16
#define	PADO_NEGLECTED_MAX		16
#define	HOST_UNIQ_LEN			8

#define PPPOE_PASSWDALG_AUTO		0
#define PPPOE_PASSWDALG_DEFAULT		1
#define PPPOE_PASSWDALG_DIGEST		2
#define PPPOE_PASSWDALG_XKJSHB		3

struct auth_fail_log
{
	int	state;
	u_char dst[ENET_LEN];
	u_short flag;
	int	count;
};

typedef struct PPPOE_INFO_S
{
	PPP_IF_VAR_T if_ppp;			/* common ppp structures */
	struct ifnet *eth_ifp;
	
	int activated;
	u_char srcEnet[ENET_LEN];		/* Source(my) ethernet address */
	u_char destEnet[ENET_LEN];		/* destination ethernet address */
	
	char unique[HOST_UNIQ_LEN];		/* unique number */
	struct sess_con sp;
	PPPOE_PARAM_STRUCT param;
	struct auth_fail_log fail_log[NUM_AUTH_FAIL_LOG];
	
#define	TIME_CONNECT_FORCE_NONE	0
#define	TIME_CONNECT_FORCE_ON	1
#define	TIME_CONNECT_FORCE_OFF	2
	int force_flag;
	short passalg_idx;
	short last_passalg_idx;
	
	u_char *pconnstate;
}
PPPOE_INFO;


typedef struct pppoe_passalg_s {
	int algid;
	void (*pppoe_passalg_fun)(char *, char *);
} pppoe_passalg_t;


#endif /* _NETGRAPH_PPPOE_H_ */


