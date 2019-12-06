/*
	This file will copy to $TOPDIR/web/cgi at runtime.
	Include file should base on $TOPDIR/web/cgi
*/

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <network.h>
#include <http_proc.h>
#include <http_conf.h>
#include <auth_check.h>
#include <cfg_def.h>
#include <eventlog.h>

void HTTP_logout(http_req *req);

static void logout(struct userinfo_t *user);
//static void send_authenticate( http_req *req, char* realm );


//static unsigned int auth_ip = 0;
//struct userinfo_t userinfo[USER_NUM_MAX];
extern struct userinfo_t userinfo[];

//void send_error( http_req *req, int s, char* title, char* extra_header, char* text );
int base64_decode(char*, char*, size_t);

void user_config(int lan_updated, int wan_updated)
{
	int i;
	char username[50], passwd[50];
	for (i=0; i<http_user_num_max; i++)
	{
		username[0] = '\0';
		passwd[0] = '\0';
		CFG_get(CFG_SYS_USERS+i, username);
		CFG_get(CFG_SYS_ADMPASS+i, passwd);
		
		if (strcmp(username, userinfo[i].username) || strcmp(passwd, userinfo[i].passwd) ||
			(lan_updated && userinfo[i].listenif ==0)||(wan_updated && userinfo[i].listenif == 1))
		{
			bzero(&(userinfo[i]), sizeof(struct userinfo_t));
			strncpy(userinfo[i].username, username,sizeof(userinfo[i].username));
			strncpy(userinfo[i].passwd, passwd,sizeof(userinfo[i].passwd));
		}
	}
}

int auth_check( http_req *req)
{
    char authinfo[50];
    char* authpass;
    int l, i;
    int idletime;
    int addr;
#ifdef CONFIG_LOG_CUSTOMIZED_LINKSYS
    int cfg_log_item;
#endif
	
    CFG_get(CFG_SYS_IDLET, &idletime);
	addr = ntohl(req->ip);
	
	
    /* Does this request contain authorization info? */
    if ( (req->authorization == (char*) 0) || 
    	( strncmp( req->authorization, "Basic ", 6 ) != 0 ) ) 
    {
		/* Nope, return a 401 Unauthorized. */
		goto send_auth;
	}

    /* Decode it. */
    l = base64_decode( &(req->authorization[6]), authinfo, sizeof(authinfo) );
	 if (l <sizeof (authinfo) && l > 0)
       authinfo[l] = '\0';
	 else 
	 	{
		diag_printf("%s:%d out of array size\n",__FUNCTION__,__LINE__);
		authinfo[sizeof(authinfo) - 1 ]='\0';
	 	}
    /* Split into user and password. */
    authpass = strchr( authinfo, ':' );
    if ( authpass == (char*) 0 )
	/* No colon?  Bogus auth info. */
		goto send_auth;
    *authpass++ = '\0';
    
    /* Is this the right user? */
	for(i=0; i< http_user_num_max; i++)
	{
		if((strcmp( authinfo, userinfo[i].username) == 0 ) && 
			( strcmp( authpass, userinfo[i].passwd) == 0 )) 
			break;	
	}
	/* no match username and password */
	if(i == http_user_num_max)
	{
#ifdef CONFIG_LOG_CUSTOMIZED_LINKSYS
        CFG_get(CFG_LOG_ITEM, &cfg_log_item);
        if (cfg_log_item & LOG_ITEM_UNAUTH)
        {
            SysLog(LOG_AUTH|SYS_LOG_NOTICE|LOGM_HTTPD|LOG_LINKSYS, STR_authentication_failed " (%s)", NSTR(addr));
        }
#else
		SysLog(LOG_AUTH|SYS_LOG_NOTICE|LOGM_HTTPD, STR_authentication_failed " (%s)", NSTR(addr));
#endif
		goto send_auth;
	}
	
#if 0	
#ifndef CONFIG_WITHOUT_CHECK_DUP_LOGIN
	if((auth_ip!= req->ip) && (userinfo[i].admin_addr != req->ip))
		goto send_auth;
#endif
	auth_ip = 0;
#endif

   	/* idletime equal 0 mean without log out automatically */
	if(idletime == 0) 
	{
#ifndef CONFIG_WITHOUT_CHECK_DUP_LOGIN
		if(userinfo[i].admin_addr && (userinfo[i].admin_addr != req->ip))
			logout(&(userinfo[i]));
#endif
	}
	else
	{
		if(userinfo[i].logout_time > time(0))
		{
#ifndef CONFIG_WITHOUT_CHECK_DUP_LOGIN
			if( userinfo[i].admin_addr && (userinfo[i].admin_addr != req->ip))
			{
				//send_error( req, HTTP_FORBIDDEN, "Forbidden", (char*) 0, "Forbidden." );
				SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_HTTPD, STR_duplicated " " STR_login " (%s)", NSTR(addr));
				return HTTP_FORBIDDEN;
			}
#endif
		}
		else if(userinfo[i].logout_time)
		{
			logout(&(userinfo[i]));
			goto send_auth;
		}
	}
	
	
	if(userinfo[i].logout_time == 0)	
	{
#ifdef CONFIG_LOG_CUSTOMIZED_LINKSYS
        CFG_get(CFG_LOG_ITEM, &cfg_log_item);
        if (cfg_log_item & LOG_ITEM_AUTH)
            SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_HTTPD|LOG_LINKSYS, STR_login " (%s)", NSTR(addr));
#else
		SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_HTTPD, STR_login " (%s)", NSTR(addr));
#endif
	}
	
	userinfo[i].admin_addr = req->ip;
	userinfo[i].logout_time = time(0)+idletime;
	userinfo[i].listenif = req->listenif;
	req->auth_username = userinfo[i].username;
	return 0;
	
send_auth:
	for(i=0; i< http_user_num_max; i++)
	{
		if(userinfo[i].admin_addr == req->ip)
		{
			userinfo[i].admin_addr = 0;
			userinfo[i].logout_time = 0;
			userinfo[i].listenif = 0;
		}
	}
	
	//send_authenticate(req, req->file );
	//auth_ip = req->ip;
	return HTTP_UNAUTHORIZED;
}

/*
static void send_authenticate(http_req *req, char* realm)
{
    char header[255];
	extern char *basic_realm;
	
	snprintf(header, sizeof(header), "WWW-Authenticate: Basic realm=\"%s\"", basic_realm);
	
    send_error(req, HTTP_UNAUTHORIZED, "Unauthorized", header, "Authorization required." );
    auth_ip = req->ip;
}
*/

static void logout(struct userinfo_t *user)
{
	int addr;
	
	if(user->admin_addr != 0)
	{
		addr = ntohl(user->admin_addr);
		SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_HTTPD, STR_logout " (%s)", NSTR(addr));
	}
	user->admin_addr = 0;
	user->logout_time = 0;
	user->listenif = 0;
}

void HTTP_logout(http_req *req)
{
	int i,l;
	char *ch;
	char authinfo[50];

	if (req==0)
	{
		for(i=0; i<http_user_num_max; i++)
			logout(&(userinfo[i]));
		return ;
	}	
	l = base64_decode( &(req->authorization[6]), authinfo, sizeof(authinfo) );
	if (l >= sizeof(authinfo) ||l <0)return ;
	authinfo[l] = '\0';
    /* Split into user and password. */
    ch = strchr( authinfo, ':' );
	if (ch != NULL)
      *ch = '\0';
	for(i=0; i<http_user_num_max; i++)
		if(!strcmp(userinfo[i].username, authinfo))
			break;
	if(i == http_user_num_max)
		return;
	logout(&(userinfo[i]));
}


