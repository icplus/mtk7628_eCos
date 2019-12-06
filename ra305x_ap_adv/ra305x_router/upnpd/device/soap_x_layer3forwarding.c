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

//<< AUTO GENERATED FUNCTION: statevar_DefaultConnectionService()
static int statevar_DefaultConnectionService
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )

//<< USER CODE BEGIN
    UPNP_SERVICE *wan_service = 0;
	
	wan_service = get_service("/control?WANIPConnection");
    if (!wan_service)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    // Construct out string
    sprintf(UPNP_STR(value), "uuid:%s:WANConnectionDevice:1,urn:upnp-org:serviceId:%s",
            wan_service->uuid,
            wan_service->service_id);

    return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetDefaultConnectionService()
static int action_SetDefaultConnectionService
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewDefaultConnectionService = UPNP_IN_ARG("NewDefaultConnectionService"); )
	
	UPNP_HINT( UPNP_STR(in_NewDefaultConnectionService) )

//<< USER CODE BEGIN

    /* modify default connection service */
    return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetDefaultConnectionService()
static int action_GetDefaultConnectionService
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewDefaultConnectionService = UPNP_OUT_ARG("NewDefaultConnectionService"); )
	
	UPNP_HINT( UPNP_STR(out_NewDefaultConnectionService) )

//<< USER CODE BEGIN

	UPNP_VALUE *out_NewDefaultConnectionService = UPNP_OUT_ARG("NewDefaultConnectionService");
	
    return statevar_DefaultConnectionService(service,
                                      out_NewDefaultConnectionService,
                                      error_string);

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

//Evented Variables
static UPNP_EVENTED evented_DefaultConnectionService;

//State Variable Table
UPNP_STATE_VAR statevar_x_layer3forwarding[] =
{
	{"DefaultConnectionService",           UPNP_TYPE_STR,       &statevar_DefaultConnectionService,           &evented_DefaultConnectionService},
	{0,                                    0,                   0,                                            0}
};

//Action Table
static UPNP_ARGUMENT arg_in_SetDefaultConnectionService [] =
{
	{"NewDefaultConnectionService",   UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetDefaultConnectionService [] =
{
	{"NewDefaultConnectionService",   UPNP_TYPE_STR}
};

UPNP_ACTION action_x_layer3forwarding[] =
{
	{"GetDefaultConnectionService",             0,       0,                                            1,       arg_out_GetDefaultConnectionService,          &action_GetDefaultConnectionService},
	{"SetDefaultConnectionService",             1,       arg_in_SetDefaultConnectionService,           0,       0,                                            &action_SetDefaultConnectionService},
	{0,                                         0,       0,                                            0,       0,                                            0}
};
//>> TABLE END

