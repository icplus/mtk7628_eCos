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
    httpd.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <network.h>
#include <http_proc.h>
#include <http_conf.h>
#include <http_mime.h>
#include <auth_check.h>
#include <cfg_def.h>
#include <eventlog.h>

#if 0
#define LEA_DBG(_fmt, _args...) \
	diag_printf("[ %s:%u ] " _fmt, __func__, __LINE__, ##_args)
#else 
#define LEA_DBG(_fmt, _args...)
#endif
//==============================================================================
//                                    MACROS
//==============================================================================
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
int	(*http_access_check)(struct sockaddr *client_address);

int HTTPD_running = 0;
int HTTPD_reconf = 0;
struct listen_sock listener[LISTEN_SOCK_MAX];

static cyg_mbox httpd_conn_mbox;
static cyg_handle_t httpd_conn_mbox_hdl;

static cyg_handle_t httpd_daemon_hdl, httpd_proc_hdl;
static char http_daemon_stack[HTTPD_DAEMON_STACKSIZE];
static char http_proc_stack[HTTPD_PROCESS_STACKSIZE];
static cyg_thread thread_obj[2];


//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
void HTTPD_daemon(cyg_addrword_t data);
void HTTPD_process(cyg_addrword_t data);
static void handle_request(struct httpmsg *msg);
static void de_dotdot( char *file );
static void do_file( http_req *req );
//void send_error( http_req *req, int s, char* title, char* extra_header, char* text );
static void send_error_body( http_req *req, int s, char *title,
			     char *text );
static int send_error_file( http_req *req, int err_code );
//void add_headers( http_req *req, int s, char* title, char* extra_header, char* mime_type, long b, time_t mod , int nocache);
static char *get_request_line( http_req *req );
static void start_response( http_req *req);
static void add_to_response( http_req *req, char *str, int len );
//void send_response( http_req *req);
static void request_init(http_req *req );
extern void free_request(http_req *req);
int get_file_handle(char *filename);
int read_file(webpage_entry *entry);
int close_file(void *file);
static char *get_method_str( int m );
static char *get_mime_type( char *name );
static void strdecode( char *to, char *from );
static int hexit( char c );
static int match( const char *pattern, const char *string );
static int match_one( const char *pattern, int patternlen,
		      const char *string );
static void add_to_buf( char **bufP, int *bufsizeP, int *buflenP,
			char *str, int len );
//void process_multipart(http_req *req);


//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern time_t tdate_parse( char *str );
extern webpage_entry webpage_table[];
extern time_t time_pages_created;
extern int zweb_location;

extern int add_to_content( http_upload_content **content, char *str,
			   int len );
extern int content_length_toobig(int length);
extern void process_multipart(http_req *req);


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
int HTTPD_init( void )
{
	int i;

	if (HTTPD_running != 0)
		return -1;

	cyg_mbox_create(&httpd_conn_mbox_hdl, &httpd_conn_mbox );

	bzero(listener, sizeof(struct listen_sock)*LISTEN_SOCK_MAX);

	for (i = 0; i < LISTEN_SOCK_MAX; i++)
		listener[i].s = -1;

	cyg_thread_create(HTTPD_DAEMON_PRIORITY,
			  HTTPD_daemon ,
			  (cyg_addrword_t)0,
			  "HTTPD_daemon",
			  (void *)http_daemon_stack,
			  HTTPD_DAEMON_STACKSIZE,
			  &httpd_daemon_hdl,
			  &thread_obj[0]);
	cyg_thread_resume(httpd_daemon_hdl);

	cyg_thread_create(HTTPD_PROCESS_PRIORITY,
			  HTTPD_process ,
			  (cyg_addrword_t)0,
			  "HTTPD_proc",
			  (void *)http_proc_stack,
			  HTTPD_PROCESS_STACKSIZE,
			  &httpd_proc_hdl,
			  &thread_obj[1]);
	cyg_thread_resume(httpd_proc_hdl);

	HTTPD_running++;
	HTTPD_reconf++;
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
int get_sock(char *ifname, unsigned short port)
{
	int s, n;
	struct sockaddr_in server_address;
	unsigned int addr;

	if (ifname == NULL)
		addr = INADDR_ANY;
	else if(read_interface(ifname, &addr, NULL, NULL) < 0)
		return -1;

	memset( &server_address, 0, sizeof(server_address) );
	server_address.sin_family = AF_INET;
	server_address.sin_len = sizeof(server_address);
	server_address.sin_addr.s_addr = addr;
	server_address.sin_port = htons(port);

	n = 1;
	s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if (s < 0)
		return -1;

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &n,
		       sizeof(n)) == -1) {
		close(s);
		return -1;
	}

	if ((bind(s, (struct sockaddr *)&server_address,
		  sizeof(server_address)) < 0) ||
	    (listen(s, SOMAXCONN ) < 0)) {
		close(s);
		return -1;
	}

	return s;
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
void http_config(void)
{
	int val, rm_ip = 0;
	int lan_updated = 0, wan_updated = 0;
	int ssl_enabled;
	int i;

	ssl_enabled = 0;

	diag_printf("zweb_location:%x\n", zweb_location);

	/* LAN */
	if (listener[0].ip != SYS_lan_ip || listener[0].is_ssl != ssl_enabled) {
		if (listener[0].s >= 0) {
			close(listener[0].s);
			listener[0].s = -1;
		}

		listener[0].is_ssl = ssl_enabled;

		if (ssl_enabled)
			listener[0].port = DEFAULT_HTTPS_PORT;
		else
			listener[0].port = DEFAULT_HTTP_PORT;

		if ((listener[0].s = get_sock(SYS_lan_if, listener[0].port)) < 0)
			diag_printf("[HTTPD]: create LAN socket error\n");

		listener[0].ip = SYS_lan_ip;
		listener[0].access = 0;
		lan_updated = 1;
	}

	/* WAN*/
	if (CFG_get(CFG_SYS_RM_EN, &val) != -1 && val == 1) {
		CFG_get(CFG_SYS_RM_IP, &rm_ip);

		if (CFG_get(CFG_SYS_RM_PORT, &val) != -1) {
			for (i = 1; i <= PRIMARY_WANIP_NUM; i++) {
				if (listener[i].s >= 0) {
					close(listener[i].s);
					listener[i].s = -1;
				}

				if (primary_wan_ip_set[i - 1].up != CONNECTED)
					continue;

				listener[i].is_ssl = ssl_enabled;

				if ((listener[i].s = get_sock(primary_wan_ip_set[i - 1].ifname, val)) < 0)
					diag_printf("[HTTPD]: create WAN socket error\n");

				listener[i].ip = primary_wan_ip_set[i - 1].ip;
				listener[i].port = val;
				listener[i].access = rm_ip;
			}

			wan_updated = 1;
		}

	} else {
		for (i = 1; i <= PRIMARY_WANIP_NUM; i++)
			if (listener[i].s >= 0) {
				close(listener[i].s);
				listener[i].s = -1;
			}
	}

	user_config(lan_updated, wan_updated);
}


//------------------------------------------------------------------------------
// FUNCTION
//		HTTPD_daemon
//
// DESCRIPTION
//		create daemon thread to listen connection
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
void HTTPD_daemon(cyg_addrword_t data)
{
	struct sockaddr_in client_address;
	int conn_fd, maxfd;
	int sz;
	int i, n;
	struct httpmsg *msg;
	fd_set  fds;
	struct  timeval tv;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	while(1) {
		if(HTTPD_reconf) {
			http_config();
			HTTPD_reconf = 0;
		}

		maxfd = 0;
		FD_ZERO(&fds);

		for(i = 0; i < LISTEN_SOCK_MAX; i++) {
			if (listener[i].s >= 0)
				FD_SET(listener[i].s, &fds);

			if(listener[i].s >= maxfd)
				maxfd = listener[i].s;
		}

		n = select(maxfd + 1, &fds, (fd_set *)NULL, (fd_set *)NULL, &tv);

		if( n > 0 ) {
			for(i = 0; i < LISTEN_SOCK_MAX; i++) {
				if (listener[i].s == -1 || !FD_ISSET(listener[i].s, &fds))
					continue;

				sz = sizeof(client_address);
				conn_fd = accept( listener[i].s, (struct sockaddr *)&client_address, &sz );

				if (conn_fd  == -1)
					continue;

				LEA_DBG("New connection from %s:%u\n",
					inet_ntoa(((struct sockaddr_in *)&client_address)->sin_addr),
					((struct sockaddr_in *)&client_address)->sin_port);

				if (listener[i].access
				    && (listener[i].access != ((struct sockaddr_in *)
							       &client_address)->sin_addr.s_addr)) {
					close(conn_fd);
					continue;
				}

				if (http_access_check
				    && (*http_access_check)((struct sockaddr *)&client_address) != 0) {
					close(conn_fd);
					continue;
				}

				msg = (struct httpmsg *)malloc(sizeof(struct httpmsg));

				if (msg == 0) {
					diag_printf("No memory of httpmsg\n");
					close(conn_fd);
					continue;
				}

				/* Send message to listen socket */
				msg->ip = ntohl(((struct sockaddr_in *)&client_address)->sin_addr.s_addr);
				msg->fd = conn_fd;
				msg->ls = &listener[i];
				msg->lif = i;

				if (!cyg_mbox_tryput(httpd_conn_mbox_hdl, (void *)msg)) {
					/* fail to put message */
					diag_printf("[HTTPD]: cyg_mbox_tryput error!!\n");
					close(conn_fd);
					free(msg);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
// FUNCTION
//		HTTPD_process
//
// DESCRIPTION
//		create process thread to process request
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
void HTTPD_process(cyg_addrword_t data)
{
	struct httpmsg *msg;

	while(1) {
		msg = (struct httpmsg *)cyg_mbox_get(httpd_conn_mbox_hdl);
		handle_request(msg);

		free(msg);
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
int	httpd_read(http_req *req, char *buf, int len)
{
	{
		int fd = req->conn_fd;
		fd_set rfd;
		struct timeval timeout;

		timeout.tv_sec = 5;     /*  no.  of seconds  */
		timeout.tv_usec = 0;    /*  no. of micro seconds  */

		FD_ZERO(&rfd);
		FD_SET(fd, &rfd);

		if (select(fd + 1, &rfd, 0, 0, &timeout) <= 0) {
			diag_printf("[HTTPD]: select error\n");
			return -1;
		}

		if (FD_ISSET(fd, &rfd))
			return read( fd, buf, len);
		else
			return -1;
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
int httpd_write(http_req *req, char *buf, int len)
{
	if (len <= 0)
		return 0;

	fwrite(buf, 1, len, req->conn_fp);
	fflush(req->conn_fp );
	return len;
}

//------------------------------------------------------------------------------
// FUNCTION
//		handle_request
//
// DESCRIPTION
//		process request
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
//int IsUpgrade =0;

extern unsigned int flsh_cfg_fwm_off;

int ImageSegmentLen = 0;
int FirstImageSegment = 0;
unsigned int write_flsh_cfg_fwm_off = 0;

#define ImageSegmentMax (4*1024) //
static void handle_request(struct httpmsg *msg)
{
	char *method_str;
	char *line;
	char *cp;
	http_req request, *req;

	char buf[2048];//old value[1024]
	int r;
	//added for web UI upgrade.
	int IsUpgrade = 0;
	char *cmd = NULL;
	char *image_segment_buf = NULL;
	char *image_segment_buf_offset = NULL;
	int  process_multipart_done = 0;

	LEA_DBG("New http request...\n");
	/* Initialize the request variables. */
	req = &request;

	req->ip = msg->ip;
	req->conn_fd = msg->fd;
	req->listenif = msg->lif;
	req->is_ssl = msg->ls->is_ssl;

	if (req->is_ssl == 0) {
		req->conn_fp = fdopen( req->conn_fd, "r+");
		req->cp = NULL;

		if (req->conn_fp == NULL) {
			diag_printf("%s:%d fdopen fail\n", __FUNCTION__, __LINE__);
			return ;
		}
	}

	request_init(req);

	for (;;) {
		r = httpd_read(req, buf, sizeof(buf));

		if (r <= 0)
			goto err;

		add_to_buf( &(req->request), &(req->request_size), &(req->request_len),
			    buf, r);

		if (req->request == (char *) 0)
			goto err;

		if ( strstr( req->request, "\r\n\r\n" ) != (char *) 0 ||
		     strstr( req->request, "\n\n" ) != (char *) 0 )

			break;
	}

	/* Parse the first line of the request. */
	method_str = get_request_line(req);

	if ( method_str == NULL) { //error handle for klocwork
		send_error( req, HTTP_BAD_REQUEST, "Bad Request", (char *) 0,
			    "Can't parse request." );
		goto err;
	}

	req->path = strpbrk( method_str, " \t\n\r" );

	if ( req->path == (char *) 0 ) {
		send_error( req, HTTP_BAD_REQUEST, "Bad Request", (char *) 0,
			    "Can't parse request." );
		goto err;
	}

	*(req->path)++ = '\0';
	req->path += strspn( req->path, " \t\n\r" );
	req->protocol = strpbrk( req->path, " \t\n\r" );

	if ( req->protocol == (char *) 0 ) {
		send_error(req, HTTP_BAD_REQUEST, "Bad Request", (char *) 0,
			   "Can't parse request." );
		goto err;
	}

	*(req->protocol)++ = '\0';
	req->query = strchr( req->path, '?' );

	if ( req->query == (char *) 0 )
		req->query = "";
	else
		*(req->query)++ = '\0';

	req->query_len = strlen(req->query);

	/* Parse the rest of the request headers. */
	while ((line = get_request_line(req)) != (char *)0) {
		if (line[0] == '\0') {
			break;
		} else if (strncasecmp(line, "Authorization:", 14) == 0) {
			cp = &line[14];
			cp += strspn(cp, " \t");
			req->authorization = cp;
		} else if (strncasecmp(line, "Content-Length:", 15) == 0) {
			cp = &line[15];
			cp += strspn(cp, " \t");
			req->content_length = atol(cp);
		} else if (strncasecmp(line, "Content-Type:", 13) == 0) {
			cp = &line[13];
			cp += strspn(cp, " \t");
			req->content_type = cp;
		} else if (strncasecmp(line, "Cookie:", 7) == 0) {
			cp = &line[7];
			cp += strspn(cp, " \t");
			req->cookie = cp;
		} else if (strncasecmp(line, "Host:", 5) == 0) {
			cp = &line[5];
			cp += strspn(cp, " \t");
			req->host = cp;
		} else if (strncasecmp(line, "If-Modified-Since:", 18) == 0) {
			cp = &line[18];
			cp += strspn(cp, " \t" );
			req->if_modified_since = tdate_parse(cp);
		} else if (strncasecmp(line, "Referer:", 8) == 0) {
			cp = &line[8];
			cp += strspn(cp, " \t");
			req->referer = cp;
		} else if (strncasecmp(line, "User-Agent:", 11) == 0) {
			cp = &line[11];
			cp += strspn(cp, " \t");
			req->useragent = cp;
		} else if (strncasecmp(line, "SOAPAction:", 11) == 0) {
			cp = &line[11];
			cp += strspn(cp, " \t");
			req->soap_action = cp;
		}
	}

	if ( strcasecmp( method_str, get_method_str( METHOD_GET ) ) == 0 )
		req->method = METHOD_GET;
	else if ( strcasecmp( method_str, get_method_str( METHOD_HEAD ) ) == 0 )
		req->method = METHOD_HEAD;
	else if ( strcasecmp( method_str, get_method_str( METHOD_POST ) ) == 0 )
		req->method = METHOD_POST;
	else {
		send_error( req, HTTP_NOT_IMPLEMENT, "Not Implemented", (char *) 0,
			    "That method is not implemented." );
		goto err;
	}

	strdecode( req->path, req->path );

	if ( req->path[0] != '/' ) {
		send_error( req, HTTP_BAD_REQUEST, "Bad Request", (char *) 0,
			    "Bad filename." );
		goto err;
	}

	req->file = &(req->path[1]);
	//if ( request.file[0] == '\0' )
	//	request.file = "./";
	de_dotdot( req->file );

	if ( req->file[0] == '/' ||
	     ( req->file[0] == '.' && req->file[1] == '.' &&
	       ( req->file[2] == '\0' || req->file[2] == '/' ) ) ) {
		send_error( req, HTTP_BAD_REQUEST, "Bad Request", (char *) 0,
			    "Illegal filename." );
		goto err;
	}

#if 0

	if ( vhost )
		file = virtual_file( file );

#endif
	/* add check file or directory in here */

	/* process POST */
	if (req->method == METHOD_POST) {
		int c;
		int total_len;
		//read again.
		total_len = req->content_length;
		c = req->request_len - req->request_idx;

		if (c > 0 && (add_to_content(&(req->content),
					     &(req->request[req->request_idx]), c) < 0)) {
			diag_printf("%s:%d goto err\n", __FUNCTION__, __LINE__);
			goto err;
		}

		if(total_len > c) {
			r = httpd_read(req, buf, MIN(sizeof(buf), total_len - c));

			if (r <= 0) {
				diag_printf("process METHOD_POST error	! r = %d\n", r);
				goto err;
			}

			if (add_to_content(&(req->content), buf, r) < 0) {
				diag_printf("%s:%d goto err\n", __FUNCTION__, __LINE__);
				goto err;
			}

			//	add_to_buf( &(req->request), &(req->request_size), &(req->request_len), buf, r);
			if (req->request == (char *) 0)
				goto err;

			c = req->request_len - req->request_idx + r;
		}

		if ( c > 0 ) {
			//add_to_buf(&(req->content), &(req->content_size), &(req->content_len), &(req->request[req->request_idx]), c);
			// if (add_to_content(&(req->content), &(req->request[req->request_idx]), c) < 0)
			//   goto err;
			//webUI upgrade, content will free in free_request() in the end.
			if ((strstr(req->content_type, "multipart/form-data") != NULL)
			    && (strstr(req->content->buf, "SYS_UPG") != NULL)) {
				process_multipart_for_upgrade(
					req);//??? should check first segment end is ok???
				process_multipart_done = 1;

				cmd = WEB_query(req, "CMD");

				if((cmd) && (!(strcmp(cmd, "SYS_UPG")))) { //webUI upgrade case.

					IsUpgrade = 1;
					FirstImageSegment = 1;
					ImageSegmentLen = 0;
					write_flsh_cfg_fwm_off = flsh_cfg_fwm_off;
					image_segment_buf = malloc(ImageSegmentMax + 2048);
					image_segment_buf_offset = image_segment_buf;

					if(image_segment_buf) {
						char *file;
						int len;
						//find files content(image)
						file = WEB_check_upload_image_first(req, "files", &len);

						if(!file) {
							diag_printf("Can't find files!\n");
							goto err;
						}

						memcpy(image_segment_buf_offset, file, len);

						image_segment_buf_offset += len;
						ImageSegmentLen += len;
					} else {
						diag_printf("Can't alloc memory for firmware upgrade!\n");
						goto err;
					}
				}


			}

			//webUI upgrade

		}

		/* we only read 1024 bytes of content to parse command while content length exceed maximum */
		/*if (content_length_toobig(req->content_length))
		  {
		  diag_printf("content length over maximum limit\n");
		  total_len = 2048; //old value 1024
		  }
		  else*/
		//total_len = req->content_length;

		while (c < total_len) {
			r = httpd_read(req, buf, MIN(MIN(sizeof(buf), total_len - c),
						     ImageSegmentMax - ImageSegmentLen));

			if (r <= 0) {
				diag_printf("process METHOD_POST error	! r =%d\n", r);
				goto err;
			}

			//webUI upgrade
			if(IsUpgrade == 1) {
				int actualLen = 0;
				//parse the end boundary for the last segment
				actualLen = WEB_check_upload_image_last(req, buf,
									r); //here we direct parse the temp buf
				memcpy(image_segment_buf_offset, buf, actualLen);

				ImageSegmentLen += actualLen;
				image_segment_buf_offset += actualLen;

				if(ImageSegmentLen >= ImageSegmentMax) {
					int result = 0;
					//write flash by segment.
					//diag_printf("write addr: [%x]\n",write_flsh_cfg_fwm_off);
					result = CFG_write_image(image_segment_buf, ImageSegmentLen, total_len);

					if(result)
						diag_printf("***Write segment error!***\n");

					write_flsh_cfg_fwm_off += ImageSegmentLen;

					ImageSegmentLen = 0;
					image_segment_buf_offset = image_segment_buf;

					if(FirstImageSegment)
						FirstImageSegment = 0;
				}
			}//webUI upgrade
			else {	//other case.
				//add_to_buf(&(req->content), &(req->content_size), &(req->content_len), buf, r);
				if (add_to_content(&(req->content), buf, r) < 0)
					goto err;

			}

			c += r;
		}

		//write the last segment
		if((IsUpgrade == 1) && (ImageSegmentLen > 0)) {
			int result = 0;

			diag_printf("***check sum total image size:[%d]***\n",
				    write_flsh_cfg_fwm_off + ImageSegmentLen - flsh_cfg_fwm_off);
			result = CFG_write_image(image_segment_buf, ImageSegmentLen, total_len);

			if(result)
				diag_printf("***Write last segment error!***\n");

			IsUpgrade = 0;
			ImageSegmentLen = 0;
			write_flsh_cfg_fwm_off = flsh_cfg_fwm_off;
		}

		//last free temp buf image_segment_buf.
		if(image_segment_buf) {
			free(image_segment_buf);
			image_segment_buf = NULL;
		}

		/* read remaining content to let client close socket smoothly */
		if (total_len != req->content_length) {
			while ( c < req->content_length) {
				r = httpd_read(req, buf, MIN(sizeof(buf), req->content_length - c));

				if (r <= 0)
					break;

				c += r;
			}
		}

		if (process_multipart_done == 0
		    && (strstr(req->content_type, "multipart/form-data") != NULL)) {
			process_multipart(req);
		} else {

			//req->query = req->content;
			//req->query_len = req->content_len;
			if (req->content) {
				req->query = req->content->buf;
				req->query_len = req->content->len;
			} else {
				req->query = 0;
				req->query_len = 0;
			}
		}
	}

	do_file(req);

err:

	if(image_segment_buf) {
		free(image_segment_buf);
		image_segment_buf = NULL;
	}

	free_request(req);
	fclose(req->conn_fp);
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
static void request_init(http_req *req)
{
	req->remoteuser = (char *) 0;
	req->method = -1;
	req->path = (char *) 0;
	req->file = (char *) 0;
	req->query = "";
	req->query_len = 0;
	req->query_tbl = NULL;
	req->queryinit = 0;
	req->protocol = "HTTP/1.0";
	req->status = 0;
	req->bytes = -1;
	req->req_hostname = (char *) 0;

	req->authorization = (char *) 0;
	req->content_type = (char *) 0;
	req->content_length = -1;
	req->cookie = (char *) 0;
	req->host = (char *) 0;
	req->if_modified_since = (time_t) - 1;
	req->referer = "";
	req->useragent = "";

	req->content = NULL;
	//req->content_size = 0;
	//req->content_len = 0;

	req->request = NULL;
	req->request_size = 0;
	req->request_len = 0;
	req->request_idx = 0;

	req->response = NULL;
	req->response_size = 0;
	req->response_len = 0;
	req->result = 0;

	req->soap_action = NULL;
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
static void de_dotdot( char *file )
{
	char *cp;
	char *cp2;
	int l;

	/* Elide any xxx/../ sequences. */
	while ( ( cp = strstr( file, "/../" ) ) != (char *) 0 ) {
		for ( cp2 = cp - 1; cp2 >= file && *cp2 != '/'; --cp2 )
			continue;

		if ( cp2 < file )
			break;

		(void) strcpy( cp2, cp + 3 );
	}

	/* Also elide any xxx/.. at the end. */
	while ( ( l = strlen( file ) ) > 3 &&
		strcmp( ( cp = file + l - 3 ), "/.." ) == 0 ) {
		for ( cp2 = cp - 1; cp2 >= file && *cp2 != '/'; --cp2 )
			continue;

		if ( cp2 < file )
			break;

		*cp2 = '\0';
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
static void do_file(http_req *req)
{
	char idx[255];
	char fixed_type[255];
	char header[255];
	char *type;
	webpage_entry *fileptr;
	int last_mod = time_pages_created;
	char *new_file = NULL;
	int	err_code = HTTP_REQUEST_OK;
	int result = 0;
	char *cmd;
	char *old_file = NULL;

	extern char *basic_realm;
	extern char *config_default_page(void);

	LEA_DBG("%s %s\n",
		req->method == METHOD_POST ? "POST" : (req->method == METHOD_GET ? "GET" : 
		(req->method == METHOD_HEAD ? "HEAD" : "UNKOWN")), req->file);
		
	/* if no page was specified, redirect to default page */
	if( req->file[strlen(req->file) - 1] == '/' ) {
		(void) snprintf( idx, sizeof(idx), "%s%s", req->file,
				 config_default_page());
		old_file = req->file;
		req->file = idx;
	}

	if(req->if_modified_since != -1) {
		/* we don't hope browser get pages from cache */
		last_mod = (req->if_modified_since < time_pages_created) ?
			   time_pages_created : req->if_modified_since + 1;
	}

	/* Get handle */
	fileptr = (webpage_entry *)get_file_handle(req->file);

	if (fileptr) {
		switch (req->method) {
			case METHOD_GET:
				if (!(fileptr->flag & NO_AUTH)) {
					err_code = auth_check(req);

					if (err_code != 0)
						goto err_out;
				}

				cmd = WEB_query(req, "CMD");

				if (cmd) {
					result = CGI_do_cmd(req);
				}

				break;

			case METHOD_POST:
				if (fileptr->flag & NO_AUTH) {
					err_code = HTTP_BAD_REQUEST;
					goto err_out;
				}

				err_code = auth_check(req);

				if(err_code != 0)
					goto err_out;

				new_file = cgiFunc(fileptr, req);

				if (new_file) {
					fileptr = get_file_handle(new_file);

					if (!fileptr) {
						err_code = HTTP_ERROR;
						goto err_out;
					}
				}

				break;

			default:
				err_code = HTTP_METHOD_NA;
				goto err_out;
		}

#if 0

		// Now read the file
		if (!read_file(fileptr)) {
			// Read file fail
			close_file(fileptr);
			err_code = HTTP_ERROR;
			goto err_out;
		}

#endif
		// Send file out
		type = get_mime_type(req->file );
		(void) snprintf( fixed_type, sizeof(fixed_type), type, CHARSET );

#if 1

		if (fileptr->flag & CACHE) {
//			diag_printf("http: cache %x, %x\n", req->if_modified_since, time_pages_created);
			if ((req->if_modified_since != (time_t) - 1)
			    && (req->if_modified_since >= time_pages_created) ) {
				add_headers(req, 304, "Not Modified", (char *) 0, fixed_type, -1,
					    time_pages_created, 0);
				send_response(req);
				close_file(fileptr);

				if (old_file !=
				    NULL)//if old_file !=NULL, req->file is a local variable address
					req->file = old_file;//klocwork fix warning,

				return;
			}
		}

#endif

		// Now read the file
		if (!read_file(fileptr)) {
			// Read file fail
			close_file(fileptr);
			err_code = HTTP_ERROR;
			goto err_out;
		}
		
		{
			cmd = WEB_query(req, "CMD");
			/* Wireless SiteSurvey result will be returned manually */
			if (!(cmd && !strcmp(cmd, "WIRELESS_SITESURVEY"))) {
				add_headers( req, HTTP_REQUEST_OK, "Ok", (char *) 0, fixed_type, -1,
					     last_mod, !(fileptr->flag & CACHE));
				send_response(req);
				(*fileptr->fileFunc)(req, fileptr->param);	
			}
		}
		close_file(fileptr);
		
		return;
	} else {
		err_code = auth_check(req);

		if (err_code != 0)
			goto err_out;

		/* if webpage location has been changed, we hope it can do reboot at least */
		//if(!strcmp(req->file, CMD_PAGE))
		/*if(!strncmp(req->file, CMD_PAGE, 3))
		{
				add_headers( req, HTTP_REQUEST_OK, "Ok", (char*) 0, fixed_type, -1, last_mod, 1);
			send_response(req);
			CGI_do_cmd(req);
				WEB_printf(req, RESTART_MSG_PAGE);
			}
		else*/
		if (cgiFileMiss(req) != 0)
			err_code = HTTP_NOT_FOUND;
	}

err_out:

	switch (err_code) {
		case HTTP_UNAUTHORIZED:
			snprintf(header, sizeof(header), "WWW-Authenticate: Basic realm=\"%s\"",
				 basic_realm);
			send_error(req, HTTP_UNAUTHORIZED, "Unauthorized", header,
				   "Authorization required." );
			break;

		case HTTP_FORBIDDEN:
			send_error( req, HTTP_FORBIDDEN, "Forbidden", (char *) 0, "Forbidden." );
			break;

		case HTTP_BAD_REQUEST:
			send_error(req, HTTP_BAD_REQUEST, "Bad Request", (char *) 0,
				   "Bad request.");
			break;

		case HTTP_NOT_FOUND:
			send_error( req, HTTP_NOT_FOUND, "Not Found", (char *) 0,
				    "File not found.");
			break;

		case HTTP_METHOD_NA:
			send_error( req, HTTP_METHOD_NA, "Not Support", (char *) 0,
				    "Method not supported.");
			break;

		case HTTP_ERROR:
			send_error( req, HTTP_ERROR, "Server Error", (char *) 0,
				    "Internal server error.");
			break;

		default:
			break;
	}

	if (old_file !=
	    NULL)//if old_file !=NULL, req->file is a local variable address
		req->file = old_file;//klocwork fix warning,

	return;
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
int get_file_handle(char *filename)
{
	webpage_entry *entry;
	char *p;
	int offset;

	offset = 0;
	entry = &webpage_table[0];

	while (entry->path) {
		p = entry->path;

		while (*p == '/' || *p == '\\')
			p++;

		if (strcmp(filename, p) == 0)
			break;

		offset += entry->size;
		entry++;
	}

	if (entry->path) {
		if (zweb_location == 0 && (entry->flag & WEBP_LOCKED) == 0)
			return 0;

		entry->offset = offset;
		return (int)entry;
	}

	if (strcmp(filename, "favicon.ico") !=
	    0) //favicon.ico icons, that get displayed in the address bar of every browser.
		diag_printf("[http]: file \"%s\" not found\n", filename);

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
int read_file(webpage_entry *entry)
{
	if (entry->path) {
		if (entry->flag & WEBP_COMPRESS) {
			int datalen;
			void *dataptr = NULL;
			extern	int zweb_page_size_max;

			if ((entry->flag & WEBP_LOCKED) ||
			    entry->param != NULL) {
				return (int)entry;
			}

			if (zweb_location == 0)
				return 0;

			//datalen = gzip_org_len(zweb_location+entry->offset, entry->size);
			datalen = lzma_org_len(zweb_location + entry->offset);

			//diag_printf("**length:[%d]**\n",datalen);
			//datalen = zweb_page_size_max;
			if(datalen ==
			    0) { //means this file just has c file, not have html data,such as:profile.bin,syslog.bin
				return (int)entry; //so we just return.not do anything.
			}

			if( datalen > zweb_page_size_max) {
				diag_printf("%s, file size exceed %d\n", entry->path, zweb_page_size_max);
				goto perr;
			}

			dataptr = malloc(datalen);

			if (dataptr) {
				int try_counter = 0;
				int error = 0;
try_again:
				try_counter++;
				error = uncompress(dataptr, &datalen, zweb_location + entry->offset,
						   entry->size);

				//if (decompress(dataptr, &datalen, zweb_location+entry->offset, entry->size) < 0)
				if (error != 0 && error != 6) {
					diag_printf("\nunzip %s, offset=%d, size=%d, datalen=%d, error![%d]\n",
						    entry->path,
						    entry->offset,
						    entry->size,
						    datalen, error);

					if(try_counter < 2)
						goto try_again;

					free(dataptr);
					goto perr;
				}
			} else {
				diag_printf("%d, No memory for zip\n", entry->offset);
				goto perr;
			}

			entry->param = dataptr;
		} else	// no compression
			entry->param = (void *)(zweb_location + entry->offset);

		return (int)entry;
	}

perr:
	return 0;
}

int query_webfile_len(webpage_entry *entry)
{
	if (entry->path) {
		if (entry->flag & WEBP_COMPRESS) {
			//return gzip_org_len(zweb_location+entry->offset, entry->size);
			return lzma_org_len(zweb_location + entry->offset);
		} else
			return entry->size;
	}

	return -1;
}

int copy_webfile(webpage_entry *entry, char *dst, int len)
{
	int flen;

	if ((dst == NULL) || (len == 0))
		return 0;

	if (read_file(entry) == 0)
		return 0;

	flen = query_webfile_len(entry);

	if (len <= flen)
		flen = len;

	memcpy(dst, entry->param, flen);
	return flen;
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
int close_file(void *file)
{
	webpage_entry *entry = (webpage_entry *)file;

	if (entry &&
	    (entry->flag & WEBP_COMPRESS) &&
	    (entry->flag & WEBP_LOCKED) == 0) {
		if (entry->param) {
			free(entry->param);
			entry->param = 0;
		}
	}

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
void	lock_web_file(char *filename)
{
	webpage_entry *entry;

	entry = (webpage_entry *)get_file_handle(filename);

	if (!entry)
		return;

	if ((entry->flag & WEBP_LOCKED) == 0) {
		int TeRs = read_file(entry);
		entry->flag |= WEBP_LOCKED;
	}
}

void	unlock_web_file(char *filename)
{
	webpage_entry *entry = (webpage_entry *)get_file_handle(filename);

	if (entry) {
		entry->flag &= ~WEBP_LOCKED;
		close_file(entry);
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
void HTTPD_update(void)
{
	HTTPD_reconf = 1;
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
#if 0
static char *virtual_file( char *file )
{

	char *cp;
	static char vfile[10000];

	/* Use the request's hostname, or fall back on the IP address. */
	if ( host != (char *) 0 )
		req_hostname = host;
	else {
		usockaddr usa;
		int sz = sizeof(usa);

		if ( getsockname( conn_fd, &usa.sa, &sz ) < 0 )
			req_hostname = "UNKNOWN_HOST";
		else
			req_hostname = ntoa( &usa );
	}

	/* Pound it to lower case. */
	for ( cp = req_hostname; *cp != '\0'; ++cp )
		if ( isupper( *cp ) )
			*cp = tolower( *cp );

	(void) snprintf( vfile, sizeof(vfile), "%s/%s", req_hostname, file );
	return vfile;
}
#endif /* 0 */

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
void send_error( http_req *req, int s, char *title, char *extra_header,
		 char *text )
{
	add_headers( req, s, title, extra_header, "text/html", -1, -1, 1);

	send_error_body( req, s, title, text );

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
static void send_error_body( http_req *req, int s, char *title,
			     char *text )
{
	char buf[500];
	int buflen;

	/* Try server-wide custom error page. */
	if ( send_error_file(req, s) )
		return;

	/* Send built-in error page. */
	buflen = snprintf(
			 buf, sizeof(buf),
			 "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\n<BODY BGCOLOR=\"#cc9999\"><H4>%d %s</H4>\n",
			 s, title, s, title );
	add_to_response( req, buf, buflen );
	buflen = snprintf( buf, sizeof(buf), "%s\n", text );
	add_to_response( req, buf, buflen );

	if ( match( "**MSIE**", req->useragent ) ) {
		int n;
		buflen = snprintf( buf, sizeof(buf), "<!--\n" );
		add_to_response( req, buf, buflen );

		for ( n = 0; n < 6; ++n ) {
			buflen = snprintf( buf, sizeof(buf),
					   "Padding so that MSIE deigns to show this error instead of its own canned one.\n" );
			add_to_response( req, buf, buflen );
		}

		buflen = snprintf( buf, sizeof(buf), "-->\n" );
		add_to_response( req, buf, buflen );
	}

	buflen = snprintf( buf, sizeof(buf), "</BODY></HTML>\n");
	add_to_response( req, buf, buflen );
	send_response(req);
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
static int send_error_file( http_req *req, int err_code )
{
	char *filename = NULL;
	webpage_entry *fileptr;
	struct err_page *errpg;

	for(errpg = &error_files[0]; errpg->filename; errpg++) {
		if(errpg->code == err_code) {
			filename = errpg->filename;
			break;
		}
	}

	if ( !filename || !(fileptr = (webpage_entry *)get_file_handle(filename)) )
		return 0;

	if(read_file(fileptr))
		return 0;

	send_response(req); /* send head first */
	fileptr->fileFunc(req, fileptr->param);
	close_file(fileptr);

	return 1;
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
void add_headers( http_req *req, int s, char *title, char *extra_header,
		  char *mime_type, long b, time_t mod, int nocache )
{
	time_t now;
	char timebuf[100];
	char buf[1000];
	int buflen;
	const char *rfc1123_fmt = "%a, %d %b %Y %H:%M:%S GMT";

	req->status = s;
	req->bytes = b;
	start_response(req);
	buflen = snprintf( buf, sizeof(buf), "%s %d %s\r\n", req->protocol,
			   req->status, title );
	add_to_response( req, buf, buflen );
	buflen = snprintf( buf, sizeof(buf), "Server: %s\r\n", SERVER_NAME );
	add_to_response( req, buf, buflen );
	now = time( (time_t *) 0 );
	(void) strftime( timebuf, sizeof(timebuf), rfc1123_fmt, gmtime( &now ) );
	buflen = snprintf( buf, sizeof(buf), "Date: %s\r\n", timebuf );
	add_to_response( req, buf, buflen );

	if ( extra_header != (char *) 0 ) {
		buflen = snprintf( buf, sizeof(buf), "%s\r\n", extra_header );
		add_to_response( req, buf, buflen );
	}

	if(nocache) {
		buflen = snprintf( buf, sizeof(buf),
				   "Pragma: no-cache\r\nCache-Control: no-cache\r\n");
		add_to_response( req, buf, buflen );
	}

	if ( mime_type != (char *) 0 ) {
		buflen = snprintf( buf, sizeof(buf), "Content-Type: %s\r\n", mime_type );
		add_to_response( req, buf, buflen );
	}

	if ( req->bytes >= 0 ) {
		buflen = snprintf( buf, sizeof(buf), "Content-Length: %ld\r\n",
				   req->bytes );
		add_to_response( req, buf, buflen );
	}

	if ( mod != (time_t) - 1 ) {
		(void) strftime( timebuf, sizeof(timebuf), rfc1123_fmt, gmtime( &mod ) );
		buflen = snprintf( buf, sizeof(buf), "Last-Modified: %s\r\n", timebuf );
		add_to_response( req, buf, buflen );
	}

	buflen = snprintf( buf, sizeof(buf), "Connection: close\r\n\r\n" );
	add_to_response( req, buf, buflen );
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
static char *get_request_line( http_req *req )
{
	int i;
	char c;

	for ( i = req->request_idx; req->request_idx < req->request_len;
	      ++req->request_idx ) {
		c = req->request[req->request_idx];

		if ( c == '\n' || c == '\r' ) {
			req->request[req->request_idx] = '\0';
			++req->request_idx;

			if ( c == '\r' && req->request_idx < req->request_len &&
			     req->request[req->request_idx] == '\n' ) {
				req->request[req->request_idx] = '\0';
				++(req->request_idx);
			}

			return &(req->request[i]);
		}
	}

	return (char *) 0;
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
static void start_response( http_req *req)
{
	req->response_len = 0;
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
static void add_to_response( http_req *req, char *str, int len )
{
	add_to_buf( &(req->response), &(req->response_size), &(req->response_len),
		    str, len );
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
void send_response(http_req *req)
{
	httpd_write(req, req->response, req->response_len);
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
static void add_to_buf( char **bufP, int *bufsizeP, int *buflenP,
			char *str, int len )
{
	if ( *bufP == 0 ) {
		if(*bufsizeP == 0)
			*bufsizeP = len + 2000;

		*buflenP = 0;
		*bufP = (char *) malloc( *bufsizeP );
	} else if ( *buflenP + len >= *bufsizeP ) {
		*bufsizeP = *buflenP + len + 2000;
		*bufP = (char *) realloc( (void *) *bufP, *bufsizeP );
	}

	if ( *bufP == (char *) 0 ) {
		printf("add_to_buf: out of memory\n");
		return; /* 15/09/03 Roger,  fix me*/
	}

	(void) memcpy( &((*bufP)[*buflenP]), str, len );
	*buflenP += len;
	(*bufP)[*buflenP] = '\0';
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
static char *get_method_str( int m )
{
	switch ( m ) {
		case METHOD_GET:
			return "GET";

		case METHOD_HEAD:
			return "HEAD";

		case METHOD_POST:
			return "POST";

		default:
			return (char *) 0;
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
static char *get_mime_type( char *name )
{
	int fl = strlen( name );
	int i;

	for ( i = 0; i < sizeof(mime_table) / sizeof(*mime_table); ++i ) {
		int el = strlen( mime_table[i].ext );

		if ( strcasecmp( &(name[fl - el]), mime_table[i].ext ) == 0 )
			return mime_table[i].type;
	}

	return "text/plain; charset=%s";
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//		Copies and decodes a string.  It's ok for from and to to be the same string.
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
static void strdecode( char *to, char *from )
{
	for ( ; *from != '\0'; ++to, ++from ) {
		if ( from[0] == '%' && isxdigit( from[1] ) && isxdigit( from[2] ) ) {
			*to = hexit( from[1] ) * 16 + hexit( from[2] );
			from += 2;
		} else
			*to = *from;
	}

	*to = '\0';
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
static int hexit( char c )
{
	if ( c >= '0' && c <= '9' )
		return c - '0';

	if ( c >= 'a' && c <= 'f' )
		return c - 'a' + 10;

	if ( c >= 'A' && c <= 'F' )
		return c - 'A' + 10;

	return 0;           /* shouldn't happen, we're guarded by isxdigit() */
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//		Simple shell-style filename matcher.  Only does ? * and **, and multiple
//	patterns separated by |.  Returns 1 or 0.
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
int match( const char *pattern, const char *string )
{
	const char * or;

	for (;;) {
		or = strchr( pattern, '|' );

		if ( or == (char *) 0 )
			return match_one( pattern, strlen( pattern ), string );

		if ( match_one( pattern, or - pattern, string ) )
			return 1;

		pattern = or + 1;
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
static int match_one( const char *pattern, int patternlen,
		      const char *string )
{
	const char *p;

	for ( p = pattern; p - pattern < patternlen; ++p, ++string ) {
		if ( *p == '?' && *string != '\0' )
			continue;

		if ( *p == '*' ) {
			int i, pl;
			++p;

			if ( *p == '*' ) {
				/* Double-wildcard matches anything. */
				++p;
				i = strlen( string );
			} else
				/* Single-wildcard matches anything but slash. */
				i = strcspn( string, "/" );

			pl = patternlen - ( p - pattern );

			for ( ; i >= 0; --i )
				if ( match_one( p, pl, &(string[i]) ) )
					return 1;

			return 0;
		}

		if ( *p != *string )
			return 0;
	}

	if ( *string == '\0' )
		return 1;

	return 0;
}


