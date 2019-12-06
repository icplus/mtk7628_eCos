

#ifndef __WSC_UPNP_DEVICE_H__
#define __WSC_UPNP_DEVICE_H__

#include "upnp_type.h"
#include "upnp_service.h"
#include "wsc_msg.h"
#include "wsc_upnp.h"


#define ACTION_RESPONSE_IDLE 0
#define ACTION_RESPONSE_WAITING_FOR	1
#define ACTION_RESPONSE_FILLED 2

#ifdef SECOND_WIFI
	extern struct upnpDevice wscLocalDevice[2];
#else
	extern struct upnpDevice wscLocalDevice;
#endif
//extern struct upnpDevice wscLocalDevice[WSC_DEVICE_NUM];

int
WscDevGetAPSettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
int
WscDevGetSTASettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
int
WscDevSetAPSettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
int
WscDevDelAPSettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
int
WscDevSetSTASettings(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
int
WscDevRebootAP(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
int 
WscDevResetAP(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
int
WscDevRebootSTA(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
int
WscDevResetSTA(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
int 
WscDevSetSelectedRegistrar(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
#if 0
int
WscDevPutWLANResponse(
	IN struct upnphttp * h,
	IN uint32 ipAddr,
	OUT IXML_Document * out,
	OUT char **errorString);
#else
int 
WscDevPutWLANResponse(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);
#endif
int
WscDevPutMessageResp(
	IN WscEnvelope *envelope,
	IN UPNP_CONTEXT *context);

int 
WscDevPutMessage(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);

int
WscDevGetDeviceInfoResp(
	IN WscEnvelope *envelope,
	IN UPNP_CONTEXT *context);
int
WscDevGetDeviceInfo(
	IN UPNP_CONTEXT *context,
	IN UPNP_SERVICE *service,
	IN IN_ARGUMENT *in_argument,
	OUT OUT_ARGUMENT *out_argument,
	OUT char **error_string);

int
WscDevHandleSubscriptionReq(
	IN char *req_SID,
	IN unsigned long ipaddr);

int 
WscDevCPNodeRemove(
	IN char *SID);

#ifdef __cplusplus
}
#endif

#endif
