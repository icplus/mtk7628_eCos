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

#define URLF_DENY_MODE		0
#define URLF_ALLOW_MODE		1

#define URLF_KEYWORD_ONLY_DENY		1
#define URLF_KEYWORD_ONLY_ALLOW		2
#define URLF_EXACTLY_ONLY_DENY		3
#define URLF_EXACTLY_ONLY_ALLOW		4

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
typedef struct urlf_act_s {
	uint32_t tstart;
	uint32_t tend;
	uint16_t days;
	uint16_t mode;
} urlf_act_t;

struct urlfilter{
	unsigned int sip; // start of ip
	unsigned int eip; // end of ip
	char *url;
	struct urlfilter *next;
	urlf_act_t act;
	uint16_t flags;
};

#define URLF_FG_VALID		0x0001
#define URLF_FG_ACTIVE		0x0002

#define urlf_tstart	act.tstart
#define urlf_tend	act.tend
#define urlf_tday	act.days
#define urlf_mode	act.mode

#define URLF_MODE_ALWAYS	0
#define URLF_MODE_PERIOD	1
#define URLF_MODE_ONE		2


#define URLF_PASS		0
#define URLF_DENY		1

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern void urlfilter_setmode(int);


