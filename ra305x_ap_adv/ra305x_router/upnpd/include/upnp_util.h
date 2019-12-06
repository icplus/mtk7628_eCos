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


#ifndef __UPNP_UTIL_H__
#define __UPNP_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <upnp_type.h>

#define	UPNP_STR(value)		(value)->str
#define	UPNP_BOOL(value)	(value)->bool
#define	UPNP_I1(value)		(value)->i1
#define	UPNP_I2(value)		(value)->i2
#define	UPNP_I4(value)		(value)->i4
#define	UPNP_UI1(value)		(value)->ui1
#define	UPNP_UI2(value)		(value)->ui2
#define	UPNP_UI4(value)		(value)->ui4
#ifdef CONFIG_WPS
#define	UPNP_BIN_BASE64(value)		(value)->bin_base64
#endif /* CONFIG_WPS */

/* Functions */
int		convert_value	(IN_ARGUMENT *arg);
void	translate_value	(int type, UPNP_VALUE *vaule);
int		gmt_time		(char *time_buf);

UPNP_INPUT	*upnp_get_in_value
(
	IN_ARGUMENT *arguments, 
	char *arg_name
);

UPNP_VALUE	*upnp_get_out_value
(
	OUT_ARGUMENT *arguments, 
	char *arg_name
);

#define	UPNP_HINT(a)
#define	UPNP_IN_ARG(a)		upnp_get_in_value(in_argument, a)
#define	UPNP_OUT_ARG(a)		upnp_get_out_value(out_argument, a)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UPNP_UTIL_H__ */

