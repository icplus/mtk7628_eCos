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
 ***************************************************************************

    Module Name:
	http_mime.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef HTTP_MIME_H
#define HTTP_MIME_H

struct {
		char* ext;
		char* type;
} mime_table[] = {
	{ ".html", "text/html; charset=%s" },
	{ ".htm", "text/html; charset=%s" },
	{ ".asp", "text/plain; charset=%s" },
	{ ".txt", "text/plain; charset=%s" },
	{ ".css", "text/css" },
	{ ".xml", "text/xml" },
	{ ".gif", "image/gif" },
	{ ".jpg", "image/jpeg" },
	{ ".jpeg", "image/jpeg" },
	{ ".js", "application/x-javascript" },
//	{ ".bin", "application/octet-stream" },
	{ ".bin", "application/x-unknown" },
};

#endif /* HTTP_MIME_H */
