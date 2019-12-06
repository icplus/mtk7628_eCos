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
    http_api.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <http_proc.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>                 // INT_MAX

extern	int httpd_write(http_req *req, char *buf, int len);

//==============================================================================
//                                    MACROS
//==============================================================================

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
char *err_string="?";

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

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
static char * strsep( char **stringp, const char *delim)
{
  char *begin, *end;

  begin = *stringp;
  if (begin == NULL)
    return NULL;

  /* A frequent case is when the delimiter string contains only one
     character.  Here we don't need to call the expensive `strpbrk'
     function and instead work using `strchr'.  */
  if (delim[0] == '\0' || delim[1] == '\0')
    {
      char ch = delim[0];

      if (ch == '\0')
	end = NULL;
      else
	{
	  if (*begin == ch)
	    end = begin;
	  else if (*begin == '\0')
	    end = NULL;
	  else
	    end = strchr (begin + 1, ch);
	}
    }
  else
    /* Find the end of the token.  */
    end = strpbrk (begin, delim);

  if (end)
    {
      /* Terminate the token and set *STRINGP past NUL character.  */
      *end++ = '\0';
      *stringp = end;
    }
  else
    /* No more delimiters; this is the last token.  */
    *stringp = NULL;

  return begin;
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
static void unescape(char *si)
{
	unsigned int c;
	char *s=si;
	
	if (s == NULL)return ;
	while ((s = strpbrk(s, "%+")) != NULL) {
		/* Parse %xx */
		if (*s == '%') {
			sscanf(s + 1, "%02x", &c);
			*s++ = (char) c;
			strncpy(s, s + 2, strlen(s) + 1);
		}
		/* Space is special */
		else if (*s == '+')
			*s++ = ' ';
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
void query_init(http_req *req)
{
	char *q, *query, *val;
	int len;
	int idx=0;
	query_var *var;
	
	req->query_tbl = (query_var *)malloc(sizeof(query_var)*NUM_QUERYVAR);
	if (req->query_tbl ==NULL )
		{
		diag_printf("%s:%d malloc fail \n",__FUNCTION__,__LINE__);
		return ;
		}
	memset(req->query_tbl, 0, sizeof(query_var)*NUM_QUERYVAR);
	
	var = req->query_tbl;
	
	query = req->query;
	len = req->query_len;
	
	if ( (!query) || (*query == '\0') ) {
		len = 0;
		return;
	}

	/* Parse into individual assignments */
	//while (strsep(&query, "&;"));
	q = query;
	var->name = q;
	var++;
	//unescape(q); /* Unescape each assignment */
	while((q = strchr(q, '&'))){
		*q = '\0';
		var->name = ++q;
		var++;
	}
#if 0
	/* Unescape each assignment */
	for (q = query; q < (query + len);) {
		unescape(q);
		for (q += strlen(q); q < (query + len) && !*q; q++);
	}
#endif
	
	var = req->query_tbl;
	while( var->name && (idx<NUM_QUERYVAR) ) 
	{
		if ((val = strchr(var->name, '=')) != 0)
		{
			*val = '\0';
			var->value = val+1;
		}
		else
			var->value = err_string;
		
		unescape(var->name);
		unescape(var->value);
		var++;
		idx++;
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
char *query_getvar(http_req *req, char *name)
{
	query_var *var;
	int idx=0;
	
	if(!(req->query_tbl))
		query_init(req);
		
	var = req->query_tbl;
	
	while( var->name && (idx<NUM_QUERYVAR) ) {
		//org, if( strncmp(var->name, name, strlen(name))== 0 )
        if( strcmp(var->name, name)== 0 )
			return var->value;
		var++;
		idx++;
	}
	
	return NULL;
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
void query_set(http_req *req, char *name, char *val)
{
	query_var *var;
	int idx=0;
	
	if(!(req->query_tbl))
		query_init(req);
		
	var = req->query_tbl;
	
	while( var->name && (idx<NUM_QUERYVAR) ) {
		var++;
		idx++;
	}
	
	if( idx == NUM_QUERYVAR)
		printf("Query table was not large enough\n");
		
	var->name = name;
	var->value = val;
	
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
int WEB_write_blk(http_req *req, char *startp, int offset, int len)
{
	return httpd_write(req, startp+offset, len);
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
int WEB_puts(char *s, http_req *req)
{
	return httpd_write(req, s, strlen(s)+1);
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
int WEB_printf( http_req *req, const char *format, ...)
{
	int rc;      // return code
    va_list ap;  // for variable args
	char buf[1024];
	
	
	buf[0] = 0;
	va_start(ap, format); // init specifying last non-var arg
	rc = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap); // end var args
	
	return httpd_write(req, buf, strlen(buf));
}

//add by seen
int WEB_long_printf( http_req *req, const char *format, ...)
{
	int rc;      // return code
	va_list ap;  // for variable args
	char buf[10240];//change this 10240

	buf[0] = 0;
	va_start(ap, format); // init specifying last non-var arg
	rc = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap); // end var args

	return httpd_write(req, buf, strlen(buf));
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
int WEB_getstatus( http_req *req )
{
	return req->status;
}

#if 0
//------------------------------------------------------------------------------
// FUNCTION
//		 WEB_upload_file
//
// DESCRIPTION
//
//  
// PARAMETERS
//		input:	req : http_req struct
//				filename : string pointer of tag name
//				len : variable to restore file length	
//		output:	return pointer of file
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
char *WEB_upload_file(http_req *req, char *filename, int *len)
{
	char *file, *boundary, *endp;
	int rlen;
	
	file = query_getvar( req, filename);
	
	if( !file)
		return NULL;
		
	boundary = strstr(req->content_type, "boundary=");
	boundary += 9;
	
	rlen = req->content_len - (file - req->content);
	if( (endp = check_pattern( file, rlen, boundary)) != NULL) {
		endp -= 4; /* \r\n-- */
		*len = endp - file;
	
		return file;
	}
	else
		return NULL;
}
#endif

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
char *check_pattern(char *sp, int len, char *pattern)
{
	int i;
	if (sp <0x80000000 || pattern <0x80000000)
		{
		diag_printf(" sp =%p,len=%d,pattern=%p\n",sp,len,pattern);
		return NULL;
		}
	for( i=0 ; i<len ; i++) {
		if(memcmp( sp+i, pattern, strlen(pattern)) == 0)
			return sp+i ;
	}
	return NULL;
	
}


