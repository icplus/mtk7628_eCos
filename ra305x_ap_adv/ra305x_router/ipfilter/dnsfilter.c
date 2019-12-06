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
#include <network.h>
#include <stdio.h>
#include <netinet/ip.h>
#include "ip_compat.h"
#include "ip_fil.h"
#include <dnf.h>

//==============================================================================
//                                    MACROS
//==============================================================================

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
struct dnfilter *dnfil_list = NULL;
int dn_default_action=DN_PASS;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int dnfilter_match(int pass, ip_t *ip, fr_info_t *fin)
{
	char *data; 
	int i, data_len;
	struct dnfilter *dn;
	int sip;
	
	if(dnfil_list == NULL)
		return pass |= FR_PASS;
	
	data = (char *)(fin->fin_dp) + 20;
	data_len = fin->fin_dlen - 20;
	//diag_dump_buf(data, 20);
	sip = ntohl(fin->fin_saddr);
	for (dn = dnfil_list; dn ; dn = dn->next) 
	{
		if(dn->sip > sip || dn->eip < sip )
        	continue;
		if(data_len < dn->len)
			continue;
		for(i=0; ((data_len-i) >= dn->len); i++)
		{
			if(((dn->query)[0] == data[i]) && !memcmp(dn->query, data+i, dn->len))
			{
				//diag_printf("DNS FILTER match!!");
				if(dn_default_action == DN_PASS)
					pass |= FR_BLOCK|FR_QUICK;
				else
					pass |= FR_PASS;
					
				//diag_printf("dnfilter_match(match), pass=%x\n", pass);
				return pass;
			}
		}
	}
	if(dn_default_action == DN_DENY)
		pass |= FR_BLOCK|FR_QUICK;
	else
		pass |= FR_PASS;
	//diag_printf("dnfilter_match(no match), pass=%x\n", pass);
	return pass;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int dnfilter_add(struct dnfilter *odn)
{
	struct dnfilter *ndn;
	char *dnsstr;
	char querystr[64];
	int i=0, j;
	
	strcpy(querystr, odn->query);
	while(querystr[i] != '\0')
	{
		if(querystr[i] == '.')
		{
			j = 1;
			while((querystr[j+i] != '.') && (querystr[j+i] != '\0'))
			{
				j++;
			}
			querystr[i] = j-1;
		}
		i++;
	}
	
	if(!(ndn = malloc(sizeof(struct dnfilter))))
		return -1;
		
	memcpy(ndn, odn, sizeof(struct dnfilter));
	
	if(!(dnsstr = malloc(strlen(querystr)+1))) {
		free(ndn);
		return -1;
	}
	strcpy(dnsstr, querystr);
	ndn->query = dnsstr;
	ndn->len = strlen(querystr);
	
	ndn->next = dnfil_list;
	dnfil_list = ndn;
		
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void dnfilter_flush(void)
{
	struct dnfilter *entry, *next_dn;
	
	entry = dnfil_list;
	dnfil_list = NULL;
	while(entry) {
		next_dn = entry->next;
		
		if(entry->query)
			free(entry->query);
		free(entry);
		
		entry = next_dn;
	}
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int dnfilter_del(struct dnfilter *entry)
{
	return 0;
	
}

