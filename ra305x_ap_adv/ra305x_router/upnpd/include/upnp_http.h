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
#ifndef __UPNP_HTTP_H__
#define __UPNP_HTTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <upnp_type.h>

/* 
 * Constants
 */
#define UPNP_HTTP_DEFPORT           1980
#define MAX_WAITS                   10      /* http socket maximum connection queue */
#define MAX_FIELD_LEN	            256
 

/* request status */
#define R_SILENT                    999		

#define R_REQUEST_OK                200
#define R_CREATED                   201
#define R_ACCEPTED                  202
#define R_NON_AUTHOR_INFORMATION    203	
#define R_NO_CONTENT                204
#define R_RESET_CONTENT             205
#define R_PARTIAL_CONTENT           206

#define R_MULTIPLE                  300	
#define R_MOVED_PERM                301
#define R_MOVED_FOUND               302
#define R_SEE_OTHER                 303
#define R_NOT_MODIFIED              304
#define R_USE_PROXY                 305
#define R_TEMPORARY_REDIRECT        307

#define R_BAD_REQUEST               400
#define R_UNAUTHORIZED              401
#define R_PAYMENT                   402	
#define R_FORBIDDEN                 403
#define R_NOT_FOUND                 404
#define R_METHOD_NA                 405
#define R_NONE_ACC                  406
#define R_PROXY                     407
#define R_REQUEST_TO                408
#define R_CONFLICT                  409
#define R_GONE                      410
#define R_LENGTH_REQUIRED           411
#define R_PRECONDITION_FAIL         412
#define R_REQUEST_ENTITY_LARGE      413
#define R_REQUEST_URI_LONG          414
#define R_UNSUPPORTED_MEDIA_TYPE    415
#define R_REQUEST_RANGE_NOT_SATIS   416
#define R_EXPECTATION_FAIL          417

#define R_ERROR                     500
#define	R_NOT_IMP                   501
#define	R_BAD_GATEWAY               502	
#define R_SERVICE_UNAV              503
#define	R_GATEWAY_TO                504
#define	R_HTTP_VERSION_NOT_SUPPORT  505

/* request methods */
enum UPNP_HTTP_REQUEST_METHOD_E
{
    METHOD_GET = 0,
    METHOD_SUBSCRIBE,
    METHOD_UNSUBSCRIBE,
    METHOD_POST,
    METHOD_MPOST,
    METHOD_MSEARCH
};

struct upnp_method
{
    char *str;
    int len;
    int method;
    int (*dispatch)(UPNP_CONTEXT *);
};

struct upnp_state
{
    int (*func)(UPNP_CONTEXT *, struct upnp_method *);
    void *parm;
};

/*
 * Functions
 */
int	upnp_context_init   (UPNP_CONTEXT *, struct upnp_method *);
int     parse_method        (UPNP_CONTEXT *, struct upnp_method *);
int     parse_uri           (UPNP_CONTEXT *, struct upnp_method *);
int     parse_msghdr        (UPNP_CONTEXT *, struct upnp_method *);
int     dispatch            (UPNP_CONTEXT *, struct upnp_method *);

int     upnp_http_engine    (UPNP_CONTEXT *, struct upnp_state *);
int     get_msghdr          (UPNP_CONTEXT *);
void 	upnp_http_process   (UPNP_CONTEXT *, int);
void	upnp_http_shutdown  (UPNP_CONTEXT *);
int     upnp_http_init      (UPNP_CONTEXT *);
 
#ifdef __cplusplus
}
#endif

#endif /* __UPNP_HTTP_H__ */

