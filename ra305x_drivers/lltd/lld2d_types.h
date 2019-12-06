/*
 * LICENSE NOTICE.
 *
 * Use of the Microsoft Windows Rally Development Kit is covered under
 * the Microsoft Windows Rally Development Kit License Agreement,
 * which is provided within the Microsoft Windows Rally Development
 * Kit or at http://www.microsoft.com/whdc/rally/rallykit.mspx. If you
 * want a license from Microsoft to use the software in the Microsoft
 * Windows Rally Development Kit, you must (1) complete the designated
 * "licensee" information in the Windows Rally Development Kit License
 * Agreement, and (2) sign and return the Agreement AS IS to Microsoft
 * at the address provided in the Agreement.
 */

/*
 * Copyright (c) Microsoft Corporation 2005.  All rights reserved.
 * This software is provided with NO WARRANTY.
 */

#ifndef LLD2D_TYPES_H
#define LLD2D_TYPES_H




#define	OID_802_11_SSID								0x0509

#define RT_OID_GET_PHY_MODE                         0x761
#define RT_OID_GET_LLTD_ASSO_TABLE                  0x762
#define RT_OID_GET_REPEATER_AP_LINEAGE				0x763


#define uint unsigned int

typedef struct {
    unsigned int   SsidLength;         // length of SSID field below, in bytes;
    unsigned char  Ssid[32];           // SSID information field
} __attribute__ ((packed)) ssid_t;

typedef enum _RT_802_11_PHY_MODE {
	PHY_11BG_MIXED = 0,
	PHY_11B,
	PHY_11A,
	PHY_11ABG_MIXED,
	PHY_11G,
	PHY_11ABGN_MIXED,	// both band   5
	PHY_11N_2_4G,		// 11n-only with 2.4G band   	6
	PHY_11GN_MIXED,	// 2.4G band      7
	PHY_11AN_MIXED,	// 5G  band       8
	PHY_11BGN_MIXED,	// if check 802.11b.      9
	PHY_11AGN_MIXED,	// if check 802.11b.      10
	PHY_11N_5G,			// 11n-only with 5G band		11
} RT_802_11_PHY_MODE;



typedef struct {
    uint8_t a[6];
} __attribute__ ((packed)) etheraddr_t;

typedef struct {
    etheraddr_t bssid[6];
    uint8_t     count;
} __attribute__ ((packed)) aplineage_t;

#define WPM_UNKNOW          0x00
#define WPM_FHSS_24G        0x01
#define WPM_DSSS_24G        0x02
#define WPM_IR_BASEDBAND    0x03
#define WPM_OFDM_5G         0x04
#define WPM_HRDSSS          0x05
#define WPM_ERP             0x06

//snowpin 2006.12.04++

typedef struct {
    etheraddr_t MACaddr;
    uint16_t    MOR;
    uint8_t     PHYtype;
} assn_table_entry_t;

typedef struct {
    unsigned int        Num;
    assn_table_entry_t  Entry[64];
} rt_lltd_assoc_table;

typedef uint32_t ipv4addr_t;

#ifndef CONFIG_IPV6
struct in6_addr
 {
         union 
         {
                 cyg_uint8            u6_addr8[16];
                 cyg_uint16          u6_addr16[8];
                 cyg_uint32          u6_addr32[4];
         } in6_u;
 #define s6_addr                 in6_u.u6_addr8
 #define s6_addr16               in6_u.u6_addr16
 #define s6_addr32               in6_u.u6_addr32
 };
#endif
typedef struct in6_addr ipv6addr_t;

/* our own (hopefully portable) 2-byte char type */
typedef uint16_t ucs2char_t;


/* Process-level event management structure for io & timers */

/* Events invoke functions of this type: */
typedef void (*event_fn_t)(void *state);
/* which get called once, at their firing time. */

struct event_st {
    struct timeval ev_firetime;
    event_fn_t     ev_function;
    void          *ev_state;
    struct event_st *ev_next;
};

/* Process-level events are encapsulated by this opaque type: */
typedef struct event_st event_t;


typedef enum {
    smS_Nascent,
    smS_Pending,
    smS_Temporary,
    smS_Complete
} smS_state_t;


typedef struct {
    bool_t		ssn_is_valid;		/* empty entries in the session table are "invalid" */
    smS_state_t		ssn_state;
    uint                ssn_count;
    uint16_t		ssn_XID;		/* seq number from the last Discover we are associated with */
    etheraddr_t		ssn_mapper_real;	/* mapper we are associated with (Discover BH:RealSrc) */
    etheraddr_t		ssn_mapper_current;	/* mapper we are associated with (curpkt->eh: ethersrc) */
    bool_t		ssn_use_broadcast;	/* TRUE if broadcast need to reach mapper (always, for now) */
    uint8_t		ssn_TypeOfSvc;		/* Discover BH:ToS */
    struct event_st    *ssn_InactivityTimer;
} session_t;


typedef enum {
    smE_Quiescent,
    smE_Pausing,
    smE_Wait
} smE_state_t;


typedef enum {
    smT_Quiescent,
    smT_Command,
    smT_Emit
} smT_state_t;

#include "protocol.h"	// must precede smevent.h, below

/* State machine events, generated from Process-level events */
#include "smevent.h"

/* Process-level-event handler routines */
#include "event.h"

#include "util.h"

#define NUM_SEES 1024	/* how many recvee_desc_t we keep */

#endif	/*** LLD2D_TYPES_H ***/
