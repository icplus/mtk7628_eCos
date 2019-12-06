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
    http_proc.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#ifndef HTTP_PROC_H
#define HTTP_PROC_H
//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <stdio.h>
#include <sys/types.h>
#include <network.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <cfg_net.h>
#include <cfg_def.h>
#include <sys_status.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define METHOD_GET 1
#define METHOD_HEAD 2
#define METHOD_POST 3

#define HTTP_REQUEST_OK		200
#define HTTP_BAD_REQUEST	400
#define HTTP_UNAUTHORIZED	401
#define HTTP_FORBIDDEN		403
#define HTTP_NOT_FOUND		404
#define HTTP_METHOD_NA		405
#define HTTP_ERROR			500
#define HTTP_NOT_IMPLEMENT	501

#define LISTEN_SOCK_MAX			(1+PRIMARY_WANIP_NUM)
#define NUM_QUERYVAR			255

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
/* A multi-family sockaddr. */
typedef union {
    struct sockaddr sa;
    struct sockaddr_in sa_in;
} usockaddr;

typedef struct query_var_t{
	char* name;
	char* value;
}query_var;

typedef struct http_upload_content_t
{
    int type_mcl;
    int size;
    int len;
    char *buf;
    struct http_upload_content_t *next;
}
http_upload_content;

/* record environmental variables */
typedef struct http_req_t{
	unsigned char listenif;
	unsigned int ip;
	int conn_fd;
	FILE *conn_fp;
	char* request;
	int request_size, request_len, request_idx;
	int method;
	char* path;
	char* file;
	char* query;
	int query_len;
	int	queryinit;
	query_var *query_tbl;
	char* protocol;
	int status;
	long bytes;
	char* req_hostname;
	char* authorization;
	long content_length;
	char* content_type;
	char* cookie;
	char* host;
	time_t if_modified_since;
	char* referer;
	char* useragent;
	char* remoteuser;
	usockaddr client_addr;
	http_upload_content *content;
	char* response;
	int response_size, response_len;
	char* auth_username;
	int is_ssl;
	void *cp;
	int result;
	char* soap_action;
}http_req;

struct listen_sock{
	unsigned int ip;
	int s;
	unsigned short port;
	int access;
	int is_ssl;
};

struct httpmsg{
	unsigned int ip;
	int fd;
	struct listen_sock *ls;
	unsigned char lif;
};

// typedef void vFunc();

typedef struct{
	char *path;						/* Web page URL path */
	void (*fileFunc)(http_req *, void *);		/* web page function call */
	//unsigned short size;			/* storage size */
	unsigned long size;			/* storage size */
	short flag;						/* flag */
	void *param;					/* parameter to fileFunc() */
	char *(*cgiFunc)(http_req *);		/* cgi function call */
	unsigned long offset;		/* the start offset of file*/
}webpage_entry;

#define	WEBP_COMPRESS		0x0001
#define	WEBP_NORMAL			0x0000
#define NO_AUTH				0x0002
#define CACHE				0x0004
#define	WEBP_LOCKED			0x0008

typedef struct{
	unsigned int magic;				/* magic pattern */
	int size;						/* storage size */
	int ver;						/* version */
	short entry;					/* entry number */
	short flag;						/* flag */
}webpage_head;

struct err_page{
	int code;
	char *filename;
};

struct userinfo_t{
	char username[16];
	char passwd[16];
	int admin_addr;
	int logout_time;
	unsigned char listenif;
};

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
char *check_pattern(char *sp, int len, char *pattern);
void query_init(http_req *req);
char *query_getvar(http_req *req, char *name);
void query_set(http_req *req, char *name, char *val);
int WEB_write_blk(http_req *req, char* startp, int offset, int len);
int WEB_puts(char *s, http_req *req );
int WEB_printf(http_req *req, const char *format, ...);
//add by seen
int WEB_long_printf(http_req *req, const char *format, ...);
///////////////////////////
int WEB_getstatus( http_req *req );
char *WEB_upload_file(http_req *req, char *filename, int *len);

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

#define WEB_getvar(req, var, default)	( query_getvar(req, var) ? : default )
#define WEB_query(req, var) query_getvar(req, var)

void send_error( http_req *req, int s, char* title, char* extra_header, char* text );
void add_headers( http_req *req, int s, char* title, char* extra_header, char* mime_type, long b, time_t mod , int nocache);
void send_response( http_req *req);

#endif /* HTTP_PROC_H */
