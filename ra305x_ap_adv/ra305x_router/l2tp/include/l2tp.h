/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/

#ifndef L2TP_H
#define L2TP_H

#include <network.h>
#include <arpa/inet.h>


//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define DBG(x) x
#else
#define DBG(x) (void) 0
#endif

#define MD5LEN 16		/* Length of MD5 hash */

/* Debug bitmasks */
#define DBG_TUNNEL             1 /* Tunnel-related events                 */
#define DBG_XMIT_RCV           2 /* Datagram transmission/reception       */
#define DBG_AUTH               4 /* Authentication                        */
#define DBG_SESSION            8 /* Session-related events                */
#define DBG_FLOW              16 /* Flow control code                     */
#define DBG_AVP               32 /* Hiding/showing of AVP's               */
#define DBG_SNOOP             64 /* Snooping in on LCP                    */

/* Maximum size of L2TP datagram we accept... kludge... */
#define MAX_PACKET_LEN   2048//4096

#define MAX_SECRET_LEN   96
#define MAX_HOSTNAME     128

#ifdef CONFIG_L2TP_OVER_PPPOE
#define MAX_RETRANSMISSIONS 8
#else
#define MAX_RETRANSMISSIONS 5
#endif

#define EXTRA_HEADER_ROOM 32

/* Forward declarations */

struct L2tpSelector_t;

//* Callback function */
typedef void (*CallbackFunc)(struct L2tpSelector_t *es,
				 int fd, unsigned int flags,
				 void *data);
/* Handler structure */
typedef struct L2tpHandler_t {
    struct L2tpHandler_t *next; /* Link in list                           */
    int fd;			/* File descriptor for select              */
    unsigned int flags;		/* Select on read or write; enable timeout */
    struct timeval tmout;	/* Absolute time for timeout               */
    CallbackFunc fn;	/* Callback function                       */
    void *data;			/* Extra data to pass to callback          */
} L2tpHandler;

/* Selector structure */
typedef struct L2tpSelector_t {
    L2tpHandler *handlers;	/* Linked list of L2tpHandlers            */
    int nestLevel;		/* Event-handling nesting level            */
    int opsPending;		/* True if operations are pending          */
    int destroyPending;		/* If true, a destroy is pending           */
    void *pInfo;
} L2tpSelector;


/* Flags */
#define EVENT_FLAG_READABLE 1
#define EVENT_FLAG_WRITEABLE 2
#define EVENT_FLAG_WRITABLE EVENT_FLAG_WRITEABLE

/* This is strictly a timer event */
#define EVENT_FLAG_TIMER 4

/* This is a read or write event with an associated timeout */
#define EVENT_FLAG_TIMEOUT 8
#define EVENT_FLAG_DELETED 256

#define EVENT_TIMER_BITS (EVENT_FLAG_TIMER | EVENT_FLAG_TIMEOUT)

/* an L2TP datagram */
typedef struct l2tp_dgram_t {
    uint16_t msg_type;		/* Message type */
    uint8_t bits;		/* Options bits */
    uint8_t version;		/* Version */
    uint16_t length;		/* Length (opt) */
    uint16_t tid;		/* Tunnel ID */
    uint16_t sid;		/* Session ID */
    uint16_t Ns;		/* Ns (opt) */
    uint16_t Nr;		/* Nr (opt) */
    uint16_t off_size;		/* Offset size (opt) */
    unsigned char data[MAX_PACKET_LEN];	/* Data */
    size_t last_random;	        /* Offset of last random vector AVP */
    size_t payload_len;		/* Payload len (not including L2TP header) */
    size_t cursor;		/* Cursor for adding/stripping AVP's */
    size_t alloc_len;		/* Length allocated for data */
    struct l2tp_dgram_t *next;	/* Link to next packet in xmit queue */
} l2tp_dgram;

/* An L2TP peer */
typedef struct l2tp_peer_t {
    struct sockaddr_in addr;	/* Peer's address */
    char secret[MAX_SECRET_LEN]; /* Secret for this peer */
    size_t secret_len;		/* Length of secret */
    int hide_avps;		/* If true, hide AVPs to this peer */
    int retain_tunnel;		/* If true, keep tunnel after last session is
				   deleted.  Otherwise, delete tunnel too. */
    int validate_peer_ip;	/* If true, do not accept datagrams except
				   from initial peer IP address */
} l2tp_peer;

/* A session within a tunnel */
typedef struct l2tp_session_t {
    //hash_bucket hash_by_my_id;	/* Hash bucket for session table  */
    void  *tunnel;	/* Tunnel we belong to            */
    uint16_t my_id;		/* My ID                          */
    uint16_t assigned_id;	/* Assigned ID                    */
    int state;			/* Session state                  */

    /* Some flags */
    unsigned int got_send_accm:1; /* Do we have send_accm?        */
    unsigned int got_recv_accm:1; /* Do we have recv_accm?        */
    unsigned int sequencing_required:1; /* Sequencing required?   */
    unsigned int sent_sli:1;    /* Did we send SLI yet?           */

    uint32_t send_accm;		/* Negotiated send accm           */
    uint32_t recv_accm;		/* Negotiated receive accm        */
    uint16_t Nr;		/* Data sequence number */
    uint16_t Ns;		/* Data sequence number */
    char calling_number[MAX_HOSTNAME]; /* Calling number          */
    void *private;		/* Private data for call-op's use */
} l2tp_session;

#define TUNNEL_NUM 	4

/* An L2TP tunnel */
typedef struct l2tp_tunnel_t {
    //hash_bucket hash_by_my_id;  /* Hash bucket for tunnel hash table */
    //hash_bucket hash_by_peer;   /* Hash bucket for tunnel-by-peer table */
    //hash_table sessions_by_my_id;	/* Sessions in this tunnel */
    struct l2tp_session_t session;
	int  use;
	uint16_t my_id;		/* My tunnel ID */
    uint16_t assigned_id;	/* ID assigned by peer */
    l2tp_peer *peer;		/* The L2TP peer */
    struct sockaddr_in peer_addr; /* Peer's address */
    uint16_t Ns;		/* Sequence of next packet to queue */
    uint16_t Ns_on_wire;	/* Sequence of next packet to be sent on wire */
    uint16_t Nr;		/* Expected sequence of next received packet */
    uint16_t peer_Nr;		/* Last packet ack'd by peer */
    int ssthresh;		/* Slow-start threshold */
    int cwnd;                   /* Congestion window */
    int cwnd_counter;		/* Counter for incrementing cwnd in congestion-avoidance phase */
    int timeout;		/* Retransmission timeout (seconds) */
    int retransmissions;	/* Number of retransmissions */
    int rws;			/* Our receive window size */
    int peer_rws;		/* Peer receive window size */
    L2tpSelector *es;		/* The event selector */
    L2tpHandler *hello_handler; /* Timer for sending HELLO */
    L2tpHandler *timeout_handler; /* Handler for timeout */
    l2tp_dgram *xmit_queue_head; /* Head of control transmit queue */
    l2tp_dgram *xmit_queue_tail; /* Tail of control transmit queue */
    l2tp_dgram *xmit_new_dgrams; /* dgrams which have not been transmitted */
    char peer_hostname[MAX_HOSTNAME]; /* Peer's host name */
    unsigned char response[MD5LEN]; /* Our response to challenge */
    unsigned char expected_response[MD5LEN]; /* Expected resp. to challenge */
    int state;			/* Tunnel state */
} l2tp_tunnel;


/* Bit definitions */
#define TYPE_BIT         0x80
#define LENGTH_BIT       0x40
#define SEQUENCE_BIT     0x08
#define OFFSET_BIT       0x02
#define PRIORITY_BIT     0x01
#define RESERVED_BITS    0x34
#define VERSION_MASK     0x0F
#define VERSION_RESERVED 0xF0

#define AVP_MANDATORY_BIT 0x80
#define AVP_HIDDEN_BIT    0x40
#define AVP_RESERVED_BITS 0x3C

#define MANDATORY     1
#define NOT_MANDATORY 0
#define HIDDEN        1
#define NOT_HIDDEN    0
#define VENDOR_IETF   0

#define AVP_MESSAGE_TYPE               0
#define AVP_RESULT_CODE                1
#define AVP_PROTOCOL_VERSION           2
#define AVP_FRAMING_CAPABILITIES       3
#define AVP_BEARER_CAPABILITIES        4
#define AVP_TIE_BREAKER                5
#define AVP_FIRMWARE_REVISION          6
#define AVP_HOST_NAME                  7
#define AVP_VENDOR_NAME                8
#define AVP_ASSIGNED_TUNNEL_ID         9
#define AVP_RECEIVE_WINDOW_SIZE       10
#define AVP_CHALLENGE                 11
#define AVP_Q931_CAUSE_CODE           12
#define AVP_CHALLENGE_RESPONSE        13
#define AVP_ASSIGNED_SESSION_ID       14
#define AVP_CALL_SERIAL_NUMBER        15
#define AVP_MINIMUM_BPS               16
#define AVP_MAXIMUM_BPS               17
#define AVP_BEARER_TYPE               18
#define AVP_FRAMING_TYPE              19
#define AVP_CALLED_NUMBER             21
#define AVP_CALLING_NUMBER            22
#define AVP_SUB_ADDRESS               23
#define AVP_TX_CONNECT_SPEED          24
#define AVP_PHYSICAL_CHANNEL_ID       25
#define AVP_INITIAL_RECEIVED_CONFREQ  26
#define AVP_LAST_SENT_CONFREQ         27
#define AVP_LAST_RECEIVED_CONFREQ     28
#define AVP_PROXY_AUTHEN_TYPE         29
#define AVP_PROXY_AUTHEN_NAME         30
#define AVP_PROXY_AUTHEN_CHALLENGE    31
#define AVP_PROXY_AUTHEN_ID           32
#define AVP_PROXY_AUTHEN_RESPONSE     33
#define AVP_CALL_ERRORS               34
#define AVP_ACCM                      35
#define AVP_RANDOM_VECTOR             36
#define AVP_PRIVATE_GROUP_ID          37
#define AVP_RX_CONNECT_SPEED          38
#define AVP_SEQUENCING_REQUIRED       39

#define HIGHEST_AVP                   39

#define MESSAGE_SCCRQ                  1
#define MESSAGE_SCCRP                  2
#define MESSAGE_SCCCN                  3
#define MESSAGE_StopCCN                4
#define MESSAGE_HELLO                  6

#define MESSAGE_OCRQ                   7
#define MESSAGE_OCRP                   8
#define MESSAGE_OCCN                   9

#define MESSAGE_ICRQ                  10
#define MESSAGE_ICRP                  11
#define MESSAGE_ICCN                  12

#define MESSAGE_CDN                   14
#define MESSAGE_WEN                   15
#define MESSAGE_SLI                   16

/* A fake type for our own consumption */
#define MESSAGE_ZLB                32767

/* Result and error codes */
#define RESULT_GENERAL_REQUEST          1
#define RESULT_GENERAL_ERROR            2
#define RESULT_CHANNEL_EXISTS           3
#define RESULT_NOAUTH                   4
#define RESULT_UNSUPPORTED_VERSION      5
#define RESULT_SHUTTING_DOWN            6
#define RESULT_FSM_ERROR                7

#define ERROR_OK                        0
#define ERROR_NO_CONTROL_CONNECTION     1
#define ERROR_BAD_LENGTH                2
#define ERROR_BAD_VALUE                 3
#define ERROR_OUT_OF_RESOURCES          4
#define ERROR_INVALID_SESSION_ID        5
#define ERROR_VENDOR_SPECIFIC           6
#define ERROR_TRY_ANOTHER               7
#define ERROR_UNKNOWN_AVP_WITH_M_BIT    8

/* Tunnel states */
enum {
    TUNNEL_IDLE,
    TUNNEL_WAIT_CTL_REPLY,
    TUNNEL_WAIT_CTL_CONN,
    TUNNEL_ESTABLISHED,
    TUNNEL_RECEIVED_STOP_CCN,
    TUNNEL_SENT_STOP_CCN
};

/* Session states */
enum {
    SESSION_IDLE,
    SESSION_WAIT_TUNNEL,
    SESSION_WAIT_REPLY,
    SESSION_WAIT_CONNECT,
    SESSION_ESTABLISHED
};

/* Constants and structures for parsing config file */
typedef struct l2tp_opt_descriptor_t {
    char const *name;
    int type;
    void *addr;
} l2tp_opt_descriptor;

/* Structures for option-handlers for different sections */
typedef struct option_handler_t {
    struct option_handler_t *next;
    char const *section;
    int (*process_option)(L2tpSelector *, char const *, char const *);
} option_handler;

#define OPT_TYPE_BOOL       0
#define OPT_TYPE_INT        1
#define OPT_TYPE_IPADDR     2
#define OPT_TYPE_STRING     3
#define OPT_TYPE_CALLFUNC   4
#define OPT_TYPE_PORT       5   /* 1-65535 */


struct l2tp_cfg
{
    char pppname[16];
    char tunnel_ifname[16];
    char username[256];
    char password[256];
    struct in_addr serverip;
    struct in_addr myip;
    struct in_addr mymask;
    int idle_time;
    int auto_reconnect;
    u_short cid;
    u_short mtu;
    struct l2tp_cfg *next;
};

#endif
