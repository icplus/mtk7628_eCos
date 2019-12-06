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
    httpd_config.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include <stdlib.h>
#include <ctype.h>
#include <config.h>
#include <network.h>
#include <sys/mbuf.h>
#include <http_proc.h>
#include <http_conf.h> 
#include <cfg_def.h>
#include <cfg_hdr.h>
#include <flash.h>


#define MAX_RECV_CONTENT		(960*1024) /* 1M - 64K boot loader size */

#ifndef	CONFIG_HTTPD_MAX_USERS
	#define USER_NUM_MAX	1
#else
	#if	((CONFIG_HTTPD_MAX_USERS < 1) || (CONFIG_HTTPD_MAX_USERS>4))
		#define USER_NUM_MAX	1
	#else
		#define	USER_NUM_MAX CONFIG_HTTPD_MAX_USERS
	#endif
#endif

struct userinfo_t userinfo[USER_NUM_MAX];
int http_user_num_max = USER_NUM_MAX;

#if (CONFIG_XROUTER_RAM_SIZE > 2)

int add_to_content( http_upload_content** content, char* str, int len )
{
	char *obuf;

	if ( *content == 0 )
	{
		*content = calloc(1, sizeof(http_upload_content));
		if (*content == 0)
		{
			diag_printf("add_to_cont: out of mem_0\n");
			return -1;
		}
		(*content)->size = len + 500;
		(*content)->buf = malloc((*content)->size);
		(*content)->len = 0;
	}
	else if ( (*content)->len + len >= (*content)->size )
	{
		(*content)->size = (*content)->len + len + 500;
		obuf = (*content)->buf ;
		(*content)->buf = realloc((*content)->buf, (*content)->size);
		if ((*content)->buf == NULL) {
			free(obuf);
			diag_printf("add_to_cont: realloc failed 0\n");
		}
	}

	if ( (*content)->buf == (char*) 0 )
	{
		diag_printf("add_to_cont: out of mem_1\n");
		free(*content);
		*content = 0;
		return -1;
	}

	memcpy(&((*content)->buf[(*content)->len]), str, len);
	(*content)->len += len;
	(*content)->buf[(*content)->len] = '\0';

	return 0;
}

void free_request(http_req *req)
{	
	if(req->query_tbl)
		free(req->query_tbl);
	if(req->request)
		free(req->request);
	if(req->response)
		free(req->response);
	if (req->content)
	{
		if (req->content->buf)
			free(req->content->buf);
		free(req->content);
	}
}

int content_length_toobig(int length)
{
	struct mallinfo mem_info;
	int ceiling = 0;
	mem_info = mallinfo();
	ceiling = mem_info.maxfree;
	return (length > MAX_RECV_CONTENT || length > ceiling);
}

void process_multipart(http_req *req)
{
	char *startp, *strp;
	char *boundary;
	char *varname;
	int remain_len;


	boundary = strstr(req->content_type, "boundary=");
	if (boundary == NULL)
	{
		diag_printf("%s :%d boundary == null\n",__FUNCTION__,__LINE__);
		return ;
	}
	boundary += 9;

	startp = req->content->buf;
	remain_len = req->content->len;

	while(remain_len > 0) 
	{
		if((startp = check_pattern(startp, remain_len, boundary)) == NULL )
			break;

		if(startp-4 > req->content->buf)
			*(startp-4) = '\0'; /* \r\n-- */

		startp += strlen(boundary);

		if(*startp=='-' && *(startp+1)=='-')
			break;  /* end */ /* Roger: This check method is right?? */

		startp +=2;

		remain_len = req->content->len - (startp - req->content->buf);
		if (!strncasecmp(startp, "Content-Disposition:", 20) ) 
		{
			if((varname = check_pattern(startp, remain_len, "name=\"")) != NULL ) 
			{

				/* find out begin of file */
				if((strp = check_pattern(startp, remain_len, "\r\n\r\n")) != NULL) 
				{
					startp = strp+4;
				}
				else if((strp = check_pattern(startp, remain_len, "\n\n")) != NULL) 
				{
					startp = strp+2;
				}
				else 
				{
					diag_printf("Can not find value of %s\n", varname);
					return;
				}
				varname += 6;

				strp = strchr( varname, '"');	
				if (strp == NULL)
				{
					diag_printf("%s %d:strp = NULL  Fuck\n",__FUNCTION__,__LINE__);
				}else
					*strp =  '\0';//comment by haitao:replace '"' with '\0',to add end char for name.
				query_set(req, varname, startp);	
			}
		}
		remain_len =  req->content->len -(startp - req->content->buf);
	}
}

void process_multipart_for_upgrade(http_req *req)
{
	char *startp, *strp;
	char *boundary;
	char *varname;
	int remain_len;


	boundary = strstr(req->content_type, "boundary=");
	if (boundary == NULL)
	{
		diag_printf("%s:boundary = NULL\n",__FUNCTION__);
		return ;
	}
	boundary += 9;

	startp = &(req->content->buf[0]);
	remain_len = req->content->len;

	while(remain_len > 0) 
	{
		if((startp = check_pattern(startp, remain_len, boundary)) == NULL )
			break;

		if(startp-4 > &(req->content->buf[0]))
			*(startp-4) = '\0'; /* \r\n-- */	//comment by haitao:add end char '\0'  for value.

		startp += strlen(boundary);

		if(*startp=='-' && *(startp+1)=='-')
			break;  /* end */ /* Roger: This check method is right?? */

		startp +=2;

		remain_len = req->content->len - (startp - &(req->content->buf[0]));
		if (!strncasecmp(startp, "Content-Disposition:", 20) ) 
		{
			if((varname = check_pattern(startp, remain_len, "name=\"")) != NULL ) 
			{

				/* find out begin of file */
				if((strp = check_pattern(startp, remain_len, "\r\n\r\n")) != NULL) 
				{
					startp = strp+4;
				}
				else if((strp = check_pattern(startp, remain_len, "\n\n")) != NULL) 
				{
					startp = strp+2;
				}
				else 
				{
					diag_printf("Can not find value of %s\n", varname);
					return;
				}
				varname += 6;
				strp = strchr( varname, '"');	
				if (strp !=NULL)
					*strp =  '\0';//comment by haitao:replace '"' with '\0',to add end char for name.
				query_set(req, varname, startp);	
			}
		}
		remain_len =  req->content->len -(startp - &(req->content->buf[0]));
	}
}


char *WEB_upload_file(http_req *req, char *filename, int *len)
{
	char *file, *boundary, *endp;
	int rlen;

	file = query_getvar( req, filename);

	if( !file)
		return NULL;

	boundary = strstr(req->content_type, "boundary=");
	if (boundary == NULL)
	{
		diag_printf("%s:boundary = NULL\n",__FUNCTION__);
		return ;
	}
	boundary += 9;

	rlen = req->content->len - (file - req->content->buf);
	if( (endp = check_pattern( file, rlen, boundary)) != NULL) {
		endp -= 4; /* \r\n-- */
		*len = endp - file;

		return file;
	}
	else
		return NULL;
}

char *WEB_check_upload_image_first(http_req *req, char *filename, int *len)
{
	char *file, *boundary, *endp;
	int rlen;

	file = query_getvar( req, filename);

	if( !file)
		return NULL;

	boundary = strstr(req->content_type, "boundary=");
	if (boundary == NULL)
	{
		diag_printf("%s:boundary = NULL\n",__FUNCTION__);
		return ;
	}
	boundary += 9;

	//here,still parse and copy the HTTP post body in request buf.
	rlen = req->content->len - (file - &(req->content->buf[0]));

	if( (endp = check_pattern( file, rlen, boundary)) != NULL) {
		endp -= 4; /* \r\n-- */
		*len = endp - file;
	}
	else
	{
		*len=rlen;
	}
	return file;
}

int WEB_check_upload_image_last(http_req *req, char *buf, int len)
{
	char *boundary, *endp;
	int actualLen;

	boundary = strstr(req->content_type, "boundary=");
	if (boundary == NULL)
	{
		diag_printf("%s:boundary = NULL\n",__FUNCTION__);
		return ;
	}

	boundary += 9;

	if( (endp = check_pattern( buf, len, boundary)) != NULL) 
	{
		endp -= 4; /* \r\n-- */
		actualLen = endp - buf;	
	}
	else
	{
		actualLen=len;
	}
	return actualLen;
}

extern int CFG_put_file(int id, char * file, int len);
int http_put_file(int id, http_upload_content* content, char * file, int len)
{
	return CFG_put_file(id, file, len);
}

void CGI_free_content(http_upload_content *content)
{
	if (content)
	{
		if (content->buf)
			free(content->buf);
		free(content);
	}
}

#else 
/* CONFIG_XROUTER_RAM_SIZE <= 2 */

extern	char *mclrefcnt;
extern  struct	mbstat mbstat;
extern  union	mcluster *mclfree;
int	m_clalloc __P((int, int));

#ifdef CONFIG_HTTP_UPGRADE_SYSMEM
#define MAX_MALLOC_CONTENT_BUF  (CONFIG_HTTP_UPGRADE_SYSMEM*1024)
#else
#define MAX_MALLOC_CONTENT_BUF  (384*1024)
#endif

int add_to_content( http_upload_content** content, char* str, int len )
{
	http_upload_content *current, *next;
	int total_len;
	int copy_len, remain_len = len;
	char *p = str, *obuf;

	if (*content == 0)
	{
		*content = calloc(1, sizeof(http_upload_content));
		if (*content == 0)
		{
			diag_printf("add_to_cont: out of mem_2\n");
			return -1;
		}
	}

	if ((*content)->len + remain_len < MAX_MALLOC_CONTENT_BUF)
	{
		total_len = (*content)->len + remain_len;
		if ((*content)->size == 0)
		{
			(*content)->buf = (char *)malloc(total_len+1);
			if (!(*content)->buf)
			{
				diag_printf("add_to_cont: malloc failed 0\n");
				free(*content);
				*content = 0;
				return -1;
			}
		}
		else
		{
			obuf = (*content)->buf;
			(*content)->buf = (char *)realloc((*content)->buf, total_len+1);
			if (!(*content)->buf)
			{
				free(obuf);	/*  Old buf is not freed if realloc fails  */
				diag_printf("add_to_cont: realloc failed 1\n");
				free(*content);
				*content = 0;
				return -1;
			}
		}
		memcpy(&((*content)->buf[(*content)->len]), p, remain_len);
		(*content)->size = total_len + 1;
		(*content)->len = total_len;
		(*content)->buf[(*content)->len] = '\0';
	}
	else
	{
		if ((*content)->len < MAX_MALLOC_CONTENT_BUF)
		{
			obuf = (*content)->buf;
			(*content)->buf = (char *)realloc((*content)->buf, MAX_MALLOC_CONTENT_BUF);
			if (!(*content)->buf)
			{
				free(obuf);	/*  Old buf is not freed if realloc fails  */
				diag_printf("add_to_cont: realloc failed 2\n");
				free(*content);
				*content = 0;
				return -1;
			}
			(*content)->size = MAX_MALLOC_CONTENT_BUF;
			total_len = (*content)->len;
		}
		else
			total_len = 0;

		current = *content;

		while (remain_len > 0)
		{
			copy_len = current->size - current->len;
			if (copy_len > 0)
			{
				copy_len = (remain_len > copy_len)?copy_len:remain_len;
				memcpy(&(current->buf[current->len]), p, copy_len);
				current->len += copy_len;
				total_len += current->len;
				p += copy_len;
				remain_len -= copy_len;
			}
			else
			{
				total_len += current->len;
			}

			if (remain_len == 0)
				break;

			if (!current->next)
			{
				next = calloc(1, sizeof(http_upload_content));
				if (!next)
				{
					diag_printf("add_to_cont: out of mem_3\n");
					return -1;
				}
				MCLALLOC(next->buf, M_DONTWAIT);
				if (!next->buf)
				{
					diag_printf("add_to_cont: out of cluster 0\n");
					free(next);
					return -1;
				}
				next->type_mcl = 1;    // cluster
				next->size = MCLBYTES;
				current->next = next;
			}
			current = current->next;
		}
	}

	return (total_len);
}

void free_request(http_req *req)
{	
	if(req->query_tbl)
		free(req->query_tbl);
	if(req->request)
		free(req->request);
	if(req->response)
		free(req->response);
	if (req->content)
	{
		http_upload_content *next, *current = req->content;
		while (current)
		{
			next = current->next;
			if (current->buf)
			{
				if (current->type_mcl)
					MCLFREE(current->buf);
				else
					free(current->buf);
			}
			free(current);
			current = next;
		}
	}
}

int content_length_toobig(int length)
{
	return (length > MAX_RECV_CONTENT);
}

char *check_pattern1(http_upload_content **content, http_upload_content **prev_content, char *sp, char *pattern)
{
	http_upload_content *current = *content;
	http_upload_content *next;
	int pattern_len = strlen(pattern), buf_len;
	char *p = sp;
	//char *p1;
	int i;

	while (current)
	{
		if (p >= current->buf && p < current->buf+current->len)
			break;
		*prev_content = current;
		current = current->next;
	}

	while (current)
	{
		buf_len = current->len;

		//for (; (p - current->buf) <= (buf_len - pattern_len); p++)
		for (; (p + pattern_len) <= (current->buf + buf_len); p++)
		{
			if (memcmp(p, pattern, pattern_len) == 0)
			{
				*content = current;
				return p;
			}
		}

		next = current->next;
		if (!next)
			break;

		//p1 = next->buf;
		//buf_len = next->len;

		//for (i=1; i<pattern_len && buf_len >= pattern_len-i; i++, p++)
		for (i=1; i<pattern_len; i++, p++)
		{
			if (next->len < i)
			{
				//char buf[255];
				//
				//i--;
				//p--;
				//
				//diag_printf("pattern=%s\n", pattern);
				//diag_printf("pattern_len = %d, i=%d\n", pattern_len, i);
				//
				//memcpy(buf, p, pattern_len-i);
				//buf[pattern_len-i] = 0;
				//diag_printf("head=%s\n", buf);
				//
				//memcpy(buf, next->buf, next->len);
				//buf[next->len] = 0;
				//diag_printf("tail=%s\n", buf);
				break;
			}

			//if ((memcmp(p, pattern, i) == 0) &&
			//    (memcmp(p1, pattern+i, pattern_len-i) == 0))
			if (memcmp(p, pattern, pattern_len-i) == 0 &&
					memcmp(next->buf, pattern + pattern_len-i, i) == 0)
			{
				*content = current;
				return p;
			}
		}

		*prev_content = current;
		current = next;
		//p = p1;
		p = current->buf;
	}

	*content = current;
	return NULL;
}

void process_multipart(http_req *req)
{
	http_upload_content *content;
	http_upload_content *prev_content;
	char *startp, *strp;
	char *boundary;
	char *varname;

	boundary = strstr(req->content_type, "boundary=");
	if (boundary == NULL)
	{
		diag_printf("%s:boundary = NULL\n",__FUNCTION__);
		return ;
	}
	boundary += 9;

	content = req->content;
	prev_content = NULL;
	startp = content->buf;

	while (content)
	{
		if ((startp = check_pattern1(&content, &prev_content, startp, boundary)) == NULL)
		{
			break;
		}

		strp = 0;
		if (startp-content->buf > 4)
		{
			strp = startp-4; /* \r\n-- */
		}
		else if (prev_content)
		{
			strp = prev_content->buf + prev_content->len - (4 - (startp - content->buf));
		}

		if (strp)
			*strp = '\0';

		if (startp < content->buf + content->len - strlen(boundary))
			startp += strlen(boundary);
		else
		{
			startp = content->next->buf + strlen(boundary) - (content->buf + content->len - startp);
			content = content->next;
		}

		strp = startp + 1;
		if (strp >= content->buf + content->len)
			strp = content->next->buf;

		if (*startp=='-' && *strp=='-')
		{
			break;  /* end */ /* Roger: This check method is right?? */
		}

		if (startp < content->buf + content->len - 2)
			startp += 2;
		else
		{
			startp = content->next->buf + 2 - (content->buf + content->len - startp);
			content = content->next;
		}

		//
		// XXX -- The following implementation assume the file begins at the first content node
		//
		if (!strncasecmp(startp, "Content-Disposition:", 20) ) 
		{
			if ((varname = check_pattern1(&content, &prev_content, startp, "name=\"")) != NULL)
			{

				/* find out begin of file */
				if ((strp = check_pattern1(&content, &prev_content, startp, "\r\n\r\n")) != NULL)
				{
					startp = strp+4;
				}
				else if ((strp = check_pattern1(&content, &prev_content, startp, "\n\n")) != NULL)
				{
					startp = strp+2;
				}
				else
				{
					diag_printf("Can not find value of %s\n", varname);
					return;
				}
				varname += 6;
				strp = strchr(varname, '"');	
				*strp = '\0';
				query_set(req, varname, startp);
			}
		}
	}
}

char *WEB_upload_file(http_req *req, char *filename, int *len)
{
	http_upload_content *content;
	http_upload_content *prev_content;
	http_upload_content *start_content, *end_content;
	char *file, *boundary, *endp;
	//int rlen;

	file = query_getvar(req, filename);

	if (!file)
		return NULL;

	boundary = strstr(req->content_type, "boundary=");
	boundary += 9;

	content = req->content;
	prev_content = NULL;

	start_content = content;
	while (start_content)
	{
		if (file >= start_content->buf && file < (start_content->buf+start_content->len))
			break;
		start_content = start_content->next;
	}

	end_content = start_content;
	if ((endp = check_pattern1(&end_content, &prev_content, file, boundary)) != NULL)
	{
		if (start_content == end_content)
		{
			*len = endp - file;
		}
		else
		{
			*len = start_content->len - (file - start_content->buf);
			start_content = start_content->next;
			while (start_content != end_content)
			{
				*len += start_content->len;
				start_content = start_content->next;
			}
			*len += (endp - start_content->buf);
		}

		*len -= 4;
		diag_printf("WEB_upload_file: len=%u\n", *len);

		return file;
	}
	else
		return NULL;
}

extern int cfg_chk_header(int id, char * file);
extern void cfg_init_header();
extern int CFG_read_prof(char * file, int size);
extern int cfg_load_static();

static unsigned int unalign_chk_sum2(http_upload_content* content, char *file, int len)
{
	unsigned int sum=0;
	unsigned int d,i;
	http_upload_content *current = content;
	char *p = file;
	char buf[4];
	int remain;

	while (current)
	{
		if (p >= current->buf && p < current->buf+current->len)
			break;
		current = current->next;
	}

	for (i=0; i<len; i+=4)
	{
		if (p + 4 < current->buf+current->len)
		{
			memcpy(&d, p, 4);
			p += 4;
		}
		else
		{
			remain = (current->buf + current->len) - p;
			memcpy(buf, p, remain);
			current = current->next;
			p = current->buf;
			memcpy(buf+remain, p, 4-remain);
			p += (4-remain);
			memcpy(&d, buf, 4);
		}
		sum += d;
	}

	return sum;
}

int flsh_write2(unsigned int addr, http_upload_content* content, char *file, int len)
{
	http_upload_content *current = content;
	char *src = file;
	char *dst = (char *)addr;
	int remain_len = len;
	int write_len;
	static int need_align = 0;
	char buf[2];

	while (current)
	{
		if (src >= current->buf && src < current->buf+current->len)
			break;
		current = current->next;
	}

	while (remain_len > 0)
	{
		if (need_align)
		{
			buf[1] = *src++;
			remain_len--;
			flsh_write((unsigned int)dst, (unsigned int)buf, 2);
			dst += 2;
			if (remain_len == 0)
				break;
		}
		if (current->buf + current->len - src < remain_len)
		{
			write_len = current->buf + current->len - src;
			if (write_len % 2 != 0)
			{
				write_len = write_len - 1;
				buf[0] = *(src+write_len);
				need_align = 1;
			}
			flsh_write((unsigned int)dst, (unsigned int)src, write_len);
			dst += write_len;
			current = current->next;
			src = current->buf;
		}
		else
		{
			write_len = remain_len;
			flsh_write((unsigned int)dst, (unsigned int)src, write_len);
		}
		remain_len -= write_len;
	}

	return 0;
}

int chk_fmw_hdr2(http_upload_content* content, char *file, int len)
{
	fmw_hdr hdr;
	int rc=0;
	extern int config_dl_cookie;
	extern char dl_cookie[];

	if (config_dl_cookie)
	{
		if (memcmp(file+68, dl_cookie, 20) != 0)
		{
			diag_printf("invalid cookie\n");
			return -3;
		}
	}

	memcpy(&hdr, file, sizeof(fmw_hdr));

	if (hdr.size < sizeof(fmw_hdr))	{
		diag_printf("Image length too short\n");
		return -3;
	}

	switch (hdr.flags.chksum)
	{
		case CHKSUM_NONE:
			break;

		case CHKSUM_SUM:
			rc=unalign_chk_sum2(content, file, len);
			if (rc)
				diag_printf("Error chksum=%x\n",rc);
			if (rc!=0) 
				rc=-3;
			break;

		default:
			rc=-3;
			break;
	}

	return rc;
}

int http_put_file(int id, http_upload_content* content, char * file, int len)
{
	int rc, ok=0;
	extern unsigned int flsh_cfg_fwm_off;
	extern unsigned int flsh_cfg_fwm_sz;

	if (content == NULL || file == NULL || len <= 0)
	{
		ok = -3;
		goto err_out;
	}

	if (0 != cfg_chk_header(id, file))
	{
		int i;
		diag_printf("[CFG] err header id=%d, dump:", id );
		for (i=0; i<8; i++)
			diag_printf("%02x ", file[i]&0xff);
		diag_printf("\n");
		ok = -3;      // file error
		goto err_out;
	}

	switch (id)
	{
		case 1:
			diag_printf("[CFG] FMW sz=%x ,max=%x!\n", len, flsh_cfg_fwm_sz);
			if (len > flsh_cfg_fwm_sz )
			{
				ok = -3;
				break;
			}

			if (chk_fmw_hdr2(content, file, len))
			{
				ok = -3;
				break;
			}
			diag_printf("[CFG] FMW upload\n");
			diag_printf("[CFG] FMW, sz %d B to flash:", len);

			cyg_scheduler_lock();
			rc = flsh_erase(flsh_cfg_fwm_off, len);
			cyg_scheduler_unlock();
			if (rc)
			{
				diag_printf("er err %d\n", rc);
				ok = -3;
				break;
			}

			cyg_scheduler_lock();
			rc=flsh_write2(flsh_cfg_fwm_off, content, file, len);
			cyg_scheduler_unlock();

			if (rc)
			{
				diag_printf("wr err %d\n", rc);
				ok = -3;
				break;
			}
			else
				diag_printf("OK!\n");
			break;

		case 0:
			diag_printf("[CFG] update profile!len=%d\n",len);
			cfg_init_header();
			ok = CFG_read_prof(file, len);     // reload to CFG
			cfg_load_static(); 
			if (ok == 0)
			{
				CFG_commit(CFG_DELAY_EVENT);
			}
			break;

		default:
			diag_printf("unknown");
			break;
	}

err_out:
	return ok;
}

void CGI_free_content(http_upload_content *content)
{
	http_upload_content *next;

	while (content)
	{
		next = content->next;
		if (content->buf)
		{
			if (content->type_mcl)
				MCLFREE(content->buf);
			else
				free(content->buf);
		}
		free(content);
		content = next;
	}
}

#endif


