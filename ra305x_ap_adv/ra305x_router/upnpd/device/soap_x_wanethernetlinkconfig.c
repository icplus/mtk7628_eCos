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


#include <porting.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <upnp.h>

/***********************************************************
  WARNNING: PLEASE LEAVE YOUR CODES BETWEEN 
           "<< USER CODE BEGIN" AND ">> USER CODE END"
  AND DON'T REMOVE TAG :
           "<< AUTO GENERATED FUNCTION: "
           ">> AUTO GENERATED FUNCTION"
           "<< USER CODE BEGIN"
           ">> USER CODE END"
************************************************************/

//<< AUTO GENERATED FUNCTION: statevar_EthernetLinkStatus()
static int statevar_EthernetLinkStatus
(
	UPNP_SERVICE * service,
	UPNP_STR * pValue,
	char * error_string
)
{
	char *pUp = "Up";
	char *pDown = "Down";
	char *pUnavailable = "Unavailable";

//<< USER CODE BEGIN

	UPNP_UNUSED(pDown);
	UPNP_UNUSED(pUnavailable);
    
	strcpy(pValue, pUp);

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetEthernetLinkStatus()
static int action_GetEthernetLinkStatus
(
	UPNP_SERVICE *service,
	IO_ARGUMENT *in_argument,
	IO_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_OUTBUF *out_NewEthernetLinkStatus = get_UPNP_OUTBUF(out_argument, "NewEthernetLinkStatus");

//<< USER CODE BEGIN

	strcpy(out_NewEthernetLinkStatus, "Up");
    
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< TABLE BEGIN
/***************************************************
  WARNNING: DON'T MODIFY THE FOLLOWING TABLES
  AND DON'T REMOVE TAG :
           "<< TABLE BEGIN"
           ">> TABLE END"
*****************************************************/

//State Variable Table
UPNP_STATE_VAR statevar_x_wanethernetlinkconfig[] =
{
	{"EthernetLinkStatus",                 UPNP_TYPE_STR,       &statevar_EthernetLinkStatus,                 TRUE,      ""},
	{0,                                    0,                   0,                                            0,         0}
};

//Action Table
static UPNP_ARGUMENT arg_out_GetEthernetLinkStatus [] =
{
	{"NewEthernetLinkStatus",         "EthernetLinkStatus",            UPNP_TYPE_STR}
};

UPNP_ACTION action_x_wanethernetlinkconfig[] =
{
	{"GetEthernetLinkStatus",                   0,       0,                                            1,       arg_out_GetEthernetLinkStatus,                &action_GetEthernetLinkStatus},
	{0,                                         0,       0,                                            0,       0,                                            0}
};
//>> TABLE END

