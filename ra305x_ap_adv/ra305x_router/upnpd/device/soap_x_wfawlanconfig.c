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

//#include <net/if.h>

extern unsigned int netif_get_cnt(const char * ifn, int id);
extern UPNP_CONTEXT	upnp_context;
#ifdef CONFIG_WPS
#include <upnp_service.h>
#include "wsc/wsc_common.h"
#include "wsc/wsc_msg.h"
#include "wsc/wsc_upnp_device.h"
#include "wsc/wsc_upnp.h"

extern int
WscDevGetAPSettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int
WscDevGetSTASettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int
WscDevSetAPSettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int
WscDevDelAPSettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int
WscDevSetSTASettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int
WscDevRebootAP(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int 
WscDevResetAP(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int
WscDevRebootSTA(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int
WscDevResetSTA(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int 
WscDevSetSelectedRegistrar(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int 
WscDevPutWLANResponse(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **errorString);
extern int 
WscDevPutMessage(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
extern int
WscDevGetDeviceInfo(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);


/***********************************************************
  WARNNING: PLEASE LEAVE YOUR CODES BETWEEN 
           "<< USER CODE BEGIN" AND ">> USER CODE END"
  AND DON'T REMOVE TAG :
           "<< AUTO GENERATED FUNCTION: "
           ">> AUTO GENERATED FUNCTION"
           "<< USER CODE BEGIN"
           ">> USER CODE END"
************************************************************/

//<< AUTO GENERATED FUNCTION: statevar_WLANEvent()
static int statevar_WLANEvent
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_BIN_BASE64(value) )

//<< USER CODE BEGIN
#ifdef SECOND_WIFI
	if (strcmp("/control", service->control_url) == 0)//"/control" is 2.4G device service.
	{	
		//diag_printf("!!2.4G statevar_WLANEvent\n");
		strncpy(value->str, wscLocalDevice[0].services.StateVarVal[WSC_EVENT_WLANEVENT], sizeof(value->str));
	}	
	else
	{
		//diag_printf("!!5G statevar_WLANEvent\n");
		strncpy(value->str, wscLocalDevice[1].services.StateVarVal[WSC_EVENT_WLANEVENT], sizeof(value->str));
	}	
#else	
	strncpy(value->str, wscLocalDevice.services.StateVarVal[WSC_EVENT_WLANEVENT], sizeof(value->str));
#endif
	value->str[sizeof(value->str) - 1] = '\0';
    return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_APStatus()
static int statevar_APStatus
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI1(value) )

//<< USER CODE BEGIN
#ifdef SECOND_WIFI
	if (strcmp("/control", service->control_url) == 0)//"/control" is 2.4G device service.
	{
		//diag_printf("!!2.4G statevar_APStatus\n");
		UPNP_UI1(value) = wscLocalDevice[0].services.StateVarVal[WSC_EVENT_APSTATUS];
	}	
	else
	{
		//diag_printf("!!5G statevar_APStatus\n");
		UPNP_UI1(value) = wscLocalDevice[1].services.StateVarVal[WSC_EVENT_APSTATUS];
	}	
#else
	UPNP_UI1(value) = wscLocalDevice.services.StateVarVal[WSC_EVENT_APSTATUS];
#endif
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_STAStatus()
static int statevar_STAStatus
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI1(value) )

//<< USER CODE BEGIN
#ifdef SECOND_WIFI
	if (strcmp("/control", service->control_url) == 0)//"/control" is 2.4G device service.
	{
		//diag_printf("!!2.4G statevar_STAStatus\n");
		UPNP_UI1(value) = wscLocalDevice[0].services.StateVarVal[WSC_EVENT_STASTATUS];
	}	
	else
	{
		//diag_printf("!!5G statevar_STAStatus\n");
		UPNP_UI1(value) = wscLocalDevice[1].services.StateVarVal[WSC_EVENT_STASTATUS];
	}	
#else
	UPNP_UI1(value) = wscLocalDevice.services.StateVarVal[WSC_EVENT_STASTATUS];
#endif
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_DelAPSettings()
static int action_DelAPSettings
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewAPSettings = UPNP_IN_ARG("NewAPSettings"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(in_NewAPSettings) )

//<< USER CODE BEGIN
/*
	int retVal = -1;

	retVal = WscDevDelAPSettings(&upnp_context, service, in_argument, out_argument, &error_string);
	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}
	return retVal;
*/
	return 0;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION


//<< AUTO GENERATED FUNCTION: action_GetAPSettings()
static int action_GetAPSettings
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewMessage = UPNP_IN_ARG("NewMessage"); )
	UPNP_HINT( UPNP_VALUE *out_APSettings = UPNP_OUT_ARG("APSettings"); )

	UPNP_HINT( UPNP_BIN_BASE64(in_NewMessage) )
	UPNP_HINT( UPNP_BIN_BASE64(out_APSettings) )
	
//<< USER CODE BEGIN
/*
	UPNP_VALUE *out_APSettings = UPNP_OUT_ARG("APSettings");

	int retVal = -1;

	retVal = WscDevGetAPSettings(&upnp_context, service, in_argument, out_argument, &error_string);

	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}

	return retVal;
*/
	return 0;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetDeviceInfo()
static int action_GetDeviceInfo
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewDeviceInfo = UPNP_OUT_ARG("NewDeviceInfo"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(out_NewDeviceInfo) )

//<< USER CODE BEGIN
	int retVal = -1;

	retVal = WscDevGetDeviceInfo(&upnp_context, service, in_argument, out_argument, &error_string);
	return retVal;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetSTASettings()
static int action_GetSTASettings
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewMessage = UPNP_IN_ARG("NewMessage"); )
	UPNP_HINT( UPNP_VALUE *out_NewSTASettings = UPNP_OUT_ARG("NewSTASettings"); )

	UPNP_HINT( UPNP_BIN_BASE64(in_NewMessage) )
	UPNP_HINT( UPNP_BIN_BASE64(out_NewSTASettings) )
	
//<< USER CODE BEGIN
/*
	UPNP_VALUE *out_NewSTASettings = UPNP_OUT_ARG("NewSTASettings");

	int retVal = -1;

	retVal = WscDevGetSTASettings(&upnp_context, service, in_argument, out_argument, &error_string);

	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}

	return retVal;
*/
	return 0;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_PutMessage()
static int action_PutMessage
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewInMessage = UPNP_IN_ARG("NewInMessage"); )
	UPNP_HINT( UPNP_VALUE *out_NewOutMessage = UPNP_OUT_ARG("NewOutMessage"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(in_NewInMessage) )
	UPNP_HINT( UPNP_BIN_BASE64(out_NewOutMessage) )
	
//<< USER CODE BEGIN
	int retVal = -1;

	retVal = WscDevPutMessage(&upnp_context, service, in_argument, out_argument, &error_string);

	return retVal;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_PutWLANResponse()
static int action_PutWLANResponse
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewMessage = UPNP_IN_ARG("NewMessage"); )
	UPNP_HINT( UPNP_INPUT *in_NewWLANEventType = UPNP_IN_ARG("NewWLANEventType"); )
	UPNP_HINT( UPNP_INPUT *in_NewWLANEventMAC = UPNP_IN_ARG("NewWLANEventMAC"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(in_NewMessage) )
	UPNP_HINT( UPNP_UI1(in_NewWLANEventType) )
	UPNP_HINT( UPNP_STR(in_NewWLANEventMAC) )
	
//<< USER CODE BEGIN
	int retVal = -1;

	retVal = WscDevPutWLANResponse(&upnp_context, service, in_argument, out_argument, &error_string);

	return retVal;	
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_RebootAP()
static int action_RebootAP
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewAPSettings = UPNP_IN_ARG("NewAPSettings"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(in_NewAPSettings) )
	
//<< USER CODE BEGIN
/*
	int retVal = -1;

	retVal = WscDevRebootAP(&upnp_context, service, in_argument, out_argument, &error_string);
	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}

	return retVal;
*/
	return 0;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_RebootSTA()
static int action_RebootSTA
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewSTASettings = UPNP_IN_ARG("NewSTASettings"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(in_NewSTASettings) )
	
//<< USER CODE BEGIN
/*
	int retVal = -1;

	retVal = WscDevRebootSTA(&upnp_context, service, in_argument, out_argument, &error_string);
	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}
	return retVal;
*/
	return 0;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_ResetAP()
static int action_ResetAP
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewMessage = UPNP_IN_ARG("NewMessage"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(in_NewMessage) )
	
//<< USER CODE BEGIN
/*
	int retVal = -1;

	retVal = WscDevResetAP(&upnp_context, service, in_argument, out_argument, &error_string);
	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}
	return retVal;
*/
	return 0;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_ResetSTA()
static int action_ResetSTA
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewMessage = UPNP_IN_ARG("NewMessage"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(in_NewMessage) )
	
//<< USER CODE BEGIN
/*
	int retVal = -1;

	retVal = WscDevResetSTA(&upnp_context, service, in_argument, out_argument, &error_string);
	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}
	return retVal;
*/
	return 0;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetAPSettings()
static int action_SetAPSettings
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_APSettings = UPNP_IN_ARG("APSettings"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(in_APSettings) )
	
//<< USER CODE BEGIN
/*
	int retVal = -1;

	retVal = WscDevSetAPSettings(&upnp_context, service, in_argument, out_argument, &error_string);
	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}

	return retVal;
*/
	return 0;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetSelectedRegistrar()
static int action_SetSelectedRegistrar
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewMessage = UPNP_IN_ARG("NewMessage"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(in_NewMessage) )
	
//<< USER CODE BEGIN
	int retVal = -1;
	/* XXX -- should take action */
	retVal = WscDevSetSelectedRegistrar(&upnp_context, service, in_argument, out_argument, &error_string);
/*
	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}
*/
	return retVal;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetSTASettings()
static int action_SetSTASettings
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewSTASettings = UPNP_OUT_ARG("NewSTASettings"); )
	
	UPNP_HINT( UPNP_BIN_BASE64(out_NewSTASettings) )
	
//<< USER CODE BEGIN
/*
	int retVal = -1;

	retVal = WscDevSetSTASettings(&upnp_context, service, in_argument, out_argument, &error_string);

	if(retVal == 0)
	{
		out_argument->next = NULL;
		out_argument->name = NULL;
		out_argument->type = UPNP_TYPE_STR;
	}

	return retVal;
*/
	return 0;
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
static UPNP_EVENTED evented_WLANEvent;
static UPNP_EVENTED evented_APStatus;
static UPNP_EVENTED evented_STAStatus;

//State Variable Table       
UPNP_STATE_VAR statevar_x_wfawlanconfig[] =
{
	{"WLANResponse",                       UPNP_TYPE_BIN_BASE64,       NULL/*&statevar_WLANResponse*/,    0},
	{"Message",                            UPNP_TYPE_BIN_BASE64,       NULL/*&statevar_Message*/,         0},
	{"WLANEventType",                      UPNP_TYPE_UI1,              NULL/*&statevar_WLANEventType*/,   0},
	{"WLANEventMAC",                       UPNP_TYPE_STR,              NULL/*&statevar_WLANEventMAC*/,    0},
	{"WLANEvent",                          UPNP_TYPE_BIN_BASE64,       &statevar_WLANEvent,       &evented_WLANEvent},
	{"DeviceInfo",                         UPNP_TYPE_BIN_BASE64,       NULL/*&statevar_DeviceInfo*/,      0},
	{"STASettings",                        UPNP_TYPE_BIN_BASE64,       NULL/*&statevar_STASettings*/,     0},
	{"APSettings",                         UPNP_TYPE_BIN_BASE64,       NULL/*&statevar_APSettings*/,      0},
	{"APStatus",                           UPNP_TYPE_UI1,              &statevar_APStatus,        &evented_APStatus},
	{"STAStatus",                          UPNP_TYPE_UI1,              &statevar_STAStatus,       &evented_STAStatus},
	{"InMessage",                          UPNP_TYPE_BIN_BASE64,       NULL/*&statevar_InMessage*/,       0},
	{"OutMessage",                         UPNP_TYPE_BIN_BASE64,       NULL/*&statevar_OutMessage*/,      0},
	{0,                                    0,                   0,                                0}
};

//Action Table
static UPNP_ARGUMENT arg_in_DelAPSettings [] =
{
	{"NewAPSettings",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_in_GetAPSettings [] =
{
	{"NewMessage",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_out_GetAPSettings [] =
{
	{"NewAPSettings",  UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_out_GetDeviceInfo [] =
{
	{"NewDeviceInfo",  UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_in_GetSTASettings [] =
{
	{"NewMessage",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_out_GetSTASettings [] =
{
	{"NewSTASettings",  UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_in_PutMessage [] =
{
	{"NewInMessage",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_out_PutMessage [] =
{
	{"NewOutMessage",  UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_in_PutWLANResponse [] =
{
	{"NewMessage",   UPNP_TYPE_BIN_BASE64},
	{"NewWLANEventType",   UPNP_TYPE_UI1},
	{"NewWLANEventMAC",   UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_in_RebootAP [] =
{
	{"NewAPSettings",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_in_RebootSTA [] =
{
	{"NewSTASettings",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_in_ResetAP [] =
{
	{"NewMessage",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_in_ResetSTA [] =
{
	{"NewMessage",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_in_SetAPSettings [] =
{
	{"APSettings",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_in_SetSelectedRegistrar [] =
{
	{"NewMessage",   UPNP_TYPE_BIN_BASE64}
};

static UPNP_ARGUMENT arg_out_SetSTASettings [] =
{
	{"NewSTASettings",  UPNP_TYPE_BIN_BASE64}
};

UPNP_ACTION action_x_wfawlanconfig[] =
{
	{"DelAPSettings",                           1,       arg_in_DelAPSettings,                         0,       0,                                            &action_DelAPSettings},
	{"GetAPSettings",                           1,       arg_in_GetAPSettings,                         1,       arg_out_GetAPSettings,                        &action_GetAPSettings},
	{"GetDeviceInfo",                           0,       0,                                            1,       arg_out_GetDeviceInfo,                        &action_GetDeviceInfo},
	{"GetSTASettings",                          1,       arg_in_GetSTASettings,                        1,       arg_out_GetSTASettings,                       &action_GetSTASettings},
	{"PutMessage",                              1,       arg_in_PutMessage,                            1,       arg_out_PutMessage,                           &action_PutMessage},
	{"PutWLANResponse",                         3,       arg_in_PutWLANResponse,                       0,       0,                                            &action_PutWLANResponse},
	{"RebootAP",                                1,       arg_in_RebootAP,                              0,       0,                                            &action_RebootAP},
	{"RebootSTA",                               1,       arg_in_RebootSTA,                             0,       0,                                            &action_RebootSTA},
	{"ResetAP",                                 1,       arg_in_ResetAP,                               0,       0,                                            &action_ResetAP},
	{"ResetSTA",                                1,       arg_in_ResetSTA,                              0,       0,                                            &action_ResetSTA},
	{"SetAPSettings",                           1,       arg_in_SetAPSettings,                         0,       0,                                            &action_SetAPSettings},
	{"SetSelectedRegistrar",                    1,       arg_in_SetSelectedRegistrar,                  0,       0,                                            &action_SetSelectedRegistrar},
	{"SetSTASettings",                          0,       0,                                            1,       arg_out_SetSTASettings,                       &action_SetSTASettings},
	{0,                                         0,       0,                                            0,       0,                                            0}
};
//>> TABLE END

#endif /* CONFIG_WPS */


