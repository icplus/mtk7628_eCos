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

//==============================================================================
//                                INCLUDE FILES
//==============================================================================

//==============================================================================
//                                    MACROS
//==============================================================================
#define MACF_PASS	0
#define MACF_BLOCK	1

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
typedef struct macf_act_s {
	uint32_t tstart;
	uint32_t tend;
	uint16_t days;
	uint16_t mode;
} macf_act_t;


struct macfilter{
	//struct ifnet *ifp;
	char mac[6];
	uint16_t flags;
	struct macfilter *next;
	macf_act_t act;
};
#define MACF_FG_VALID		0x0001
#define MACF_FG_ACTIVE		0x0002

#define macf_tstart	act.tstart
#define macf_tend	act.tend
#define macf_tday	act.days
#define macf_mode	act.mode

#define MACF_MODE_ALWAYS	0
#define MACF_MODE_PERIOD	1
#define MACF_MODE_ONE		2

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern int default_action;


