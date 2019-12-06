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
    cfg_prof.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================

#ifndef CFG_PROF_C
#define CFG_PROF_C

#include <cyg/kernel/kapi.h> 
#include <cfg_def.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <network.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>

#include <cfg_boot.h>
#include <cfg_hdr.h>
#include <version.h>

#ifdef	CONFIG_LED
#include <led.h>
#endif

#include <flash.h>

#ifdef	CONFIG_DL_COOKIE
#include <dl_cookie.h>
#endif

//==============================================================================
//                                    MACROS
//==============================================================================

#define	FLSH_BASE	FLASH_BASE

#define CFG_ID_END  0xffffffff
#define CFG_SAVE_TIMEOUT   300  // 3 sec

#define NEW_BOOT_CODE

#define EXTERNAL_SIZE		64

#ifdef CFG_DEBUG_LOG
#define cfg_log(fmt, ...)     diag_printf(fmt, ##  __VA_ARGS__)
#else
#define cfg_log(fmt, ...)     do {} while (0)
#endif


#define CFG_INIT_LOCK() 
#define CFG_UNLOCK()  cyg_scheduler_unlock()
#define CFG_LOCK()    cyg_scheduler_lock()



//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
typedef struct cfgobj_t
{
    cfgid   cid;
    int     len;
} cfgobj;

typedef struct cfgobj_list_t
{
    cfgobj * head;  // the first obj
    int num;        // obj number
    int len;        // total size
} cfgobj_list ;

struct cfg_flash_t
{
    int offset;     // where the data is 
    int len;        // how long is it?
};

extern char _binary_profile_txt_start[];
extern char _binary_profile_txt_end[];
extern int _binary_profile_txt_size;
extern char *_cfgfmw_idstr;

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
struct cfgobj_list_t cfg_list;

struct cfgobj_list_t cfg_list_boot;

static int eventcmd;
static unsigned char cfg_factory_blk[CFG_FLSH_BOOT_SZ];

// how many variable has been touch !
static int cfg_update_cnt = 0;
static int in_application = 0;
static struct cfg_flash_t cfg_flash;
static unsigned int flsh_cfg_off;
unsigned int flsh_cfg_boot_off;
unsigned int flsh_cfg_fwm_off;
static unsigned int flsh_cfg_sz;
unsigned int flsh_cfg_fwm_sz;
static int cfg_chk_old_lanip;
static int cfg_verify_flag;
static char cfg_inited = 0;

// extern from WIFI driver, parse the default configuration and store into wlanbuf, by Rorscha
unsigned char *wlanbuf;
//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
void CFG_start(int cmd);
int CFG_set2(int cid, void *val);
int cfg_chk_header(int id, char * file);

static void cfg_verify_mark(void);
static int cfg_verify_chk(void);
static int cid_ok(int id);
inline int CFG_rm(int id);
static int cfg_parse(char *from, int len, struct cfgobj_list_t *cfg_lp);
int CFG_parse_wlan(struct cfgobj_list_t *cfg_lp);

/* 
    just simple do the chksum
*/
static unsigned int chk_sum(int * p, int len)
{
    int wl=len>>2;
    unsigned int sum=0;

   	while (wl-- > 0 )
       	sum += *p++;
    return sum;
}


static unsigned int unalign_chk_sum(char * p, int len)
{
    unsigned int sum=0;
	unsigned int d,i;

	for (i=0; i< len; i+=4)
	{
		memcpy(&d, p+i, 4);
		sum += d;
	}
    return sum;
}


int chk_fmw_hdr(char *file, int len)
{
	fmw_hdr hdr;
	int rc=0;
	
	extern int config_dl_cookie;
	extern char dl_cookie[];
	
	if (config_dl_cookie)
	{
		if (memcmp(file+72+EXTERNAL_SIZE, dl_cookie, 20) != 0)
			return -3;
	}
	
	memcpy(&hdr, file, sizeof(fmw_hdr));
	
	switch (hdr.flags.chksum)
	{
	case CHKSUM_NONE:
		break;
		
	case CHKSUM_SUM:
		rc=unalign_chk_sum(file,len);
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


//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern int str2arglist(char *, char **, char, int);
extern char *time2string(int);

// FUNCTION
//
//  cfg_find
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      find the cid locaiton in cfgobj list
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : not found
//------------------------------------------------------------------------------
cfgobj * cfg_find( char * from, int len, int cid)
{
    cfgobj * p, *endp;
    int next_len;

    endp = (cfgobj *) ((unsigned int)from +  len ) ;
    for (p=(cfgobj *)from; ( p < endp ) && (p->cid != CFG_ID_END ); 
            p=(cfgobj*)((unsigned int)(p+1)+next_len ) )
    {
        next_len = p->len & ~3 ;  // to avoid the wrong length field
		if ((unsigned int)next_len > CFG_MAX_OBJ_SZ) break;

        if ( p->cid == cid )
            return p;
    }
    return 0;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_get_from
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      get the config value from the cfgobj list
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : not found
//------------------------------------------------------------------------------
int cfg_get_from(struct cfgobj_list_t *cfg_lp, int cid, void *val)
{
    cfgobj * p;
    int rc=-1;
    int len;

    p=cfg_find( (char*)cfg_lp->head ,  cfg_lp->len, cid);

    if (p)  // found
    {
        cfg_log("cfg_get_from: cid=%08x,@ %08x=%x\n", (int)cid, p+1,*((int*)(p+1)) );
        switch ( CFG_ID_TYPE(cid) )
        {
            case CFG_TYPE_STR:
                len = strlen( (char*)(p+1) );
                if ( len > p->len )
                    len = p->len;
                if (len >= CFG_MAX_OBJ_SZ )
                    len = CFG_MAX_OBJ_SZ-1 ;    // for safe
                *(((char*)val)+len)= '\0' ;
                strncpy(val, (char *)(p+1), len);
                break;

            case CFG_TYPE_MAC:
                len=6;
                break;

            case CFG_TYPE_INT:
            case CFG_TYPE_IP:
                len=4;
                break;

            default:
                len=p->len;
                if (len > CFG_MAX_OBJ_SZ)
                    len = CFG_MAX_OBJ_SZ ;
                break;
        }
        if ( CFG_ID_TYPE(cid) != CFG_TYPE_STR )
            memcpy(val, (p+1), len);

        rc=0;
    }
    else
    {
        cfg_log("CFG_get: No cid:%08x\n", (int)cid);
    }

    return rc;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_get
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      get the config value from the config parameter area
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : not found
//------------------------------------------------------------------------------
int CFG_get(int cid, void *val)
{
	int rc;

	if (cfg_inited == 0)
		CFG_start(0);

	if ( CFG_ID_MOD(cid) >= CFG_ID_HW )
	{
		rc=cfg_get_from(&cfg_list_boot, cid, val);
	}
	else
		rc=cfg_get_from(&cfg_list, cid, val);
	
	return rc;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_get2
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      get the config value from the read-only area
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : not found
//------------------------------------------------------------------------------
int CFG_get2(int cid, void *val)
{
	int rc;

	rc=cfg_get_from(&cfg_list_boot, cid, val);

	return rc;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_get_str
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      get the config value from the cfgobj list, and format in string
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : not found
//------------------------------------------------------------------------------
int CFG_get_str(int id, char *val)
{
    int rc;

    switch ( CFG_ID_TYPE(id) )
    {
        case CFG_TYPE_STR:
            rc=CFG_get(id, val);
            break;

        case CFG_TYPE_MAC:
            {
                struct ether_addr maca;
                rc=CFG_get(id, &maca);
                sprintf(val , "%s", ether_ntoa(&maca));
            }
            break;

        case CFG_TYPE_INT:
            {
                int t;
                rc=CFG_get(id, &t);
                sprintf(val, "%d", t);
            }
            break;

        case CFG_TYPE_IP:
            {
                struct in_addr addr;
                rc=CFG_get(id, &addr);
                sprintf(val , "%s", inet_ntoa(addr));
            }
            break;

        default:
            val[0]='\0';    // null
            rc=CFG_ETYPE;
            break;
    }
    if (!rc)
        cfg_log("CFG_get_str: id=%x, str=%s\n", id, val);
    else
        val[0]='\0';    // default is null

    return rc;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  set_event
//
//------------------------------------------------------------------------------
// DESCRIPTION
//     set event type
//
//------------------------------------------------------------------------------
// PARAMETERS
//      cfgid cid
//
//------------------------------------------------------------------------------
// RETURN
//    
//------------------------------------------------------------------------------
void set_event(cfgid cid)
{
	switch (CFG_ID_MOD(cid))
	{
	case CFG_ID_LAN:
		eventcmd |= MON_CMD_LAN_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_WAN:
		eventcmd |= MON_CMD_WAN_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_WLN:
		eventcmd |= MON_CMD_WL_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_SYS:
		eventcmd |= MON_CMD_SYS_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_NAT:
		eventcmd |= MON_CMD_NAT_UPDTAE|MON_CMD_FW_UPDTAE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_FW:
		eventcmd |= MON_CMD_FW_UPDTAE|MON_CMD_NAT_UPDTAE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_RT:
		eventcmd |= MON_CMD_RIP_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_DNS:
		eventcmd |= MON_CMD_DNS_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_POE:
	case CFG_ID_PTP:
	case CFG_ID_L2T:
		eventcmd |= MON_CMD_WAN_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_UPP:
		eventcmd |= MON_CMD_UPNP_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_DDNS:
		eventcmd |= MON_CMD_DDNS_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	case CFG_ID_LOG:
		eventcmd |= MON_CMD_LOG_UPDATE | MON_CMD_CFGCHANGE;
		break;
		
	default:
		break;
	}	
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_set
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      set the config value to the cfgobj list
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  1 : new val, < 0 : not found
//------------------------------------------------------------------------------
static int cfg_set(int id, void *val, int event)
{
    cfgid cid=(cfgid)id;
    cfgobj * p, *lastp;
    int rc=-1;
    int i;
    int newsz;
    const char * result_str[]={ "same", "insert", "replace" };

    switch ( CFG_ID_TYPE(cid))
    {
        case CFG_TYPE_STR:
            //newsz=CFG_STR_MAX_SZ;
            newsz=strlen(val);
            if (newsz)
            {
                newsz=(newsz+1+3)&~3;   // round to word boundary
                if (newsz > CFG_MAX_OBJ_SZ)
				{
                    //newsz = CFG_MAX_OBJ_SZ ;
					rc=CFG_ERANGE;
					goto out;
				}
            }
            break;

        case CFG_TYPE_MAC:
            newsz=8;
            break;

        default:
			if (id==CFG_LAN_IP)
				cfg_verify_mark();
			newsz=4;
            break;
    }


    lastp = (cfgobj *)( (unsigned int) cfg_list.head +  cfg_list.len ) ;
    for (p=cfg_list.head,i=0; p <= lastp ;
                p=(cfgobj*)((unsigned int)(p+1)+p->len),i++ )
    {
        if ( p == lastp || (unsigned int)cid < (unsigned int)p->cid  )   // insert to the current point
        {
            // move the current point data with 8+newsz bytes later
            // push the current item (p) to 8+newsz bytes later
            if ( p < lastp )
                memmove( (char*)((unsigned int)(p+1)+newsz), p, (unsigned int) lastp - (unsigned int)p  );
            cfg_list.len += ( sizeof(*p) + newsz ); 
            cfg_list.num ++ ;
            p->cid=cid;
            p->len=newsz;
            memcpy( (p+1), val, newsz );
            rc = CFG_INSERT;
            break;
        }
        else
        if ( (int)p->cid==(int)cid ) // over write
        {
            int move_len;

            // move something after the current cfg obj
            move_len = (unsigned int) lastp - ((unsigned int)(p+1)+p->len)  ;
            if (newsz == p->len )
            {
                if (newsz == 0 ) // null string
                {
                    rc=CFG_OK;
                    break;
                }
                else
                if ( CFG_ID_TYPE(cid) == CFG_TYPE_STR )
                {   // do string compare, if same , don't save
                    if (!strcmp((char*)(p+1), (char*)val))
                    {
                        rc=CFG_OK;
                        break;
                    }   
                }
                else
                if ( !memcmp((p+1), val, newsz) )
                {
                    rc=CFG_OK;
                    break;
                }   
                memcpy((p+1), val, newsz);
            }
            else
            if (newsz < p->len )
            {
                memcpy((p+1), val, newsz);                
                // pull the next item (p+8+p->len) to (p->len - newsz) bytes forward
                memcpy( (void *)((unsigned int)(p+1)+newsz), (void *)((unsigned int)(p+1)+p->len), move_len ) ;
            }
            else
            {
                // push the next item (p+8+p->len) to (newsz - p->len) bytes later
                memmove((void *)((unsigned int)(p+1)+newsz), (void *)((unsigned int)(p+1)+p->len), move_len );
                memcpy((p+1), val, newsz);
            }
            cfg_list.len += ( newsz - p->len ); 
            p->len=newsz;
            rc = CFG_CHANGE;
            break;
        }
    }

    if (rc < 0)
        cfg_log("CFG_set:not found cid:%x\n", (int)cid);
    else
    {
        cfg_log("CFG_set: %s [%d] cid=%x,@ %x\n", result_str[rc], i, (int)cid, (p+1)); 
        lastp = (cfgobj *)( (unsigned int) cfg_list.head +  cfg_list.len ) ;
        lastp->cid = CFG_ID_END ;
    }


    if ( CFG_ID_MOD(id) >= CFG_ID_HW )
    {
        CFG_set2(id, val);
    }

out:
    if (rc == CFG_OK)
    {
        cfg_log("CFG_set: same cid:%08x\n", id );
    }
    else
    {
        cfg_log("CFG_set: rc=%d cid:%08x\n", rc, id );
    }

    if (rc > CFG_OK)
	{
		if (event)
    		set_event(cid);
        cfg_update_cnt++;
	}
    else
        return rc;

    return rc;
}


int CFG_set(int id, void *val)
{
	return cfg_set(id,val,1);
}


int CFG_set_no_event(int id, void *val)
{
	return cfg_set(id,val,0);
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_set2
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      set the config value to the boot parameter
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  1 : new val, < 0 : not found
//------------------------------------------------------------------------------
int CFG_set2(int cid, void *val)
{
    cfgobj *p, *lastp;
    int rc=-1;
    int newsz;
 
    if (!cid_ok(cid))
    {
        cfg_log("CFG_set: invalid id: %08x\n", cid);
        return rc;
    }

    switch ( CFG_ID_TYPE(cid))
    {
        case CFG_TYPE_STR:
            newsz=strlen((char*)val);
            newsz= (newsz+3)&~3;
            break;

        case CFG_TYPE_MAC:
            newsz=8;
            break;

        default:
            newsz=4;
            break;
    }
	
    lastp = (cfgobj *)( (unsigned int) cfg_list_boot.head +  cfg_list_boot.len ) ;
    for (p = cfg_list_boot.head; p <=lastp; 
	    p = (cfgobj*)((unsigned int)(p+1) + (p->len & 0xfc))) {
        if ( p == lastp || (unsigned int)cid < (unsigned int)p->cid) {
	        // insert to the current point
            // move the current point data with 8+newsz bytes later
            // push the current item (p) to 8+newsz bytes later
            if ( p < lastp )
                memmove( (char*)((unsigned int)(p+1)+newsz), p, (unsigned int) lastp - (unsigned int)p  );
            cfg_list_boot.len += ( sizeof(*p) + newsz ); 
            cfg_list_boot.num ++ ;
            p->cid=cid;
            p->len=newsz;
            memcpy( (p+1), val, newsz );
            rc = CFG_INSERT;
            break;
        } else if ( (int)p->cid==(int)cid ) {
	        // over write
	        int move_len;

            // move something after the current cfg obj
            move_len = (unsigned int) lastp - ((unsigned int)(p+1)+p->len)  ;
            if (newsz == p->len ) {
		        if (newsz == 0 ) {
        	        // null string
                    rc=CFG_OK;
                    break;
                } else if ( CFG_ID_TYPE(cid) == CFG_TYPE_STR ) {
		            // do string compare, if same , don't save
                    if (!strcmp((char*)(p+1), (char*)val)) {
                        rc=CFG_OK;
                        break;
                    }   
                } else if ( !memcmp((p+1), val, newsz) ) {
                    rc=CFG_OK;
                    break;
                }   
                memcpy((p+1), val, newsz);
            } else if (newsz < p->len ) {
                memcpy((p+1), val, newsz);                
                // pull the next item (p+8+p->len) to (p->len - newsz) bytes forward
                memcpy( (void *)((unsigned int)(p+1)+newsz), (void *)((unsigned int)(p+1)+p->len), move_len ) ;
            } else {
                // push the next item (p+8+p->len) to (newsz - p->len) bytes later
                memmove((void *)((unsigned int)(p+1)+newsz), (void *)((unsigned int)(p+1)+p->len), move_len );
                memcpy((p+1), val, newsz);
            }
            cfg_list_boot.len += ( newsz - p->len ); 
            p->len=newsz;
            rc = CFG_CHANGE;
            break;
        } 
    }

    if (rc == CFG_CHANGE || rc == CFG_INSERT) {
	    /*  Write to flash  */
            CFG_LOCK();
	    flsh_erase(flsh_cfg_boot_off, cfg_list_boot.len);
	    flsh_write(flsh_cfg_boot_off, (unsigned int)cfg_list_boot.head, cfg_list_boot.len);
	    CFG_UNLOCK();
    }

    return rc;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_set_str
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      set the config value to the cfgobj list, from the string
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  1 : new val, < 0 : not found
//------------------------------------------------------------------------------
int CFG_set_str(int id, char *str)
{
    int rc;

    if (str==0)
        return CFG_ETYPE;

	if ((CFG_ID_TYPE(id)!=CFG_TYPE_STR) && strlen(str)==0)
	{	// since it's null, delete this entry
		return CFG_rm(id);
	}

    switch ( CFG_ID_TYPE(id) )
    {
        case CFG_TYPE_STR:
            if (!strcmp(str, "null"))   // this is a special case
                rc=CFG_set(id, "");
            else
                rc=CFG_set(id, str);
            break;

        case CFG_TYPE_MAC:
            {
                struct ether_addr *maca;
                maca=ether_aton(str);
                if (maca)   // if convert ok
                    rc=CFG_set(id, maca);
                else
                    rc=CFG_ETYPE;
            }
            break;

        case CFG_TYPE_INT:
            {
#if 1
                int t=atoi(str);
                rc=CFG_set(id, (void*)&t);
#else
                char *ep;
                unsigned long t = strtoul(str, ep , 10);
                if (ep != str ) // ok
                    rc=CFG_set(id, (void*)&t);
                else
                    rc=CFG_ETYPE;
#endif
            }
            break;

        case CFG_TYPE_IP:
            {
                struct in_addr addr;
                if (inet_aton(str, &addr)) // if convert ok
                    rc=CFG_set(id, &addr);
                else
                    rc=CFG_ETYPE;
            }
            break;

        default:
            rc=CFG_ETYPE; // unknown
            break;
    }
    return rc;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_del
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      delete the config obj from the cfgobj list
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  1 : new val, < 0 : not found
//------------------------------------------------------------------------------
int cfg_del(int id, int adj)
{
    cfgid cid=(cfgid)id;
    cfgobj * p, *endp ;
    int rc = CFG_NFOUND;
    int obsz;

    p=cfg_find((char *)cfg_list.head, cfg_list.len, id);
    if (!p)
        goto err1 ;

    endp = (cfgobj *)( (unsigned int) cfg_list.head +  cfg_list.len) ;
    obsz=sizeof(*p) + p->len ;
    cfg_log("CFG_del: remove cid=%x,@ %x\n", (int)cid, (p+1));
    memcpy( p, (void*)((unsigned int)(p+1)+p->len), (unsigned int)endp - (unsigned int)p - obsz );
    cfg_list.len -= obsz; 
    cfg_list.num-- ;
    rc = CFG_OK;
	cfg_update_cnt++;

    if (adj)
    {
        for ( ;  p < endp ; p=(cfgobj*)((unsigned int)(p+1)+p->len ) )
        {
            if ((p->cid&0xffffff00) == (cid&0xffffff00)) // same group
            {
                if ( CFG_ID_IDX(p->cid) > CFG_ID_IDX(cid) )  // adjust the later's index
                    p->cid--;
            }
            else
                break;
        }
    }
err1:
    if (rc)
        cfg_log("CFG_get:not found cid:%x\n", (int)cid);
	
	set_event(cid);
	
    return rc;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_del
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      delete the config obj from the cfgobj list, and adjust the array index
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  1 : new val, < 0 : not found
//------------------------------------------------------------------------------
int CFG_del(int id)
{
    return cfg_del(id, 1);  // adjust the orginal array index
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_rm
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      remove
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  1 : new val, < 0 : not found
//------------------------------------------------------------------------------
inline int CFG_rm(int id)
{
    return cfg_del(id, 0);  // do'nt adjust the orginal array index
}


static int cid_ok(int id)
{
    int j;
    if (id==0)
        return 0;
    for (j=0; j<CFG_ID_STR_NUM; j++)
    {
        if ((id&0xffffff00) == cfgid_str_table[j].id)
            return 1;
    }
    return 0;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_parse
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      loading the active config obj to cfgobj list
//
//------------------------------------------------------------------------------
// PARAMETERS
//      from: read len bytes from here
//      cfg_lp: write to there
//------------------------------------------------------------------------------
// RETURN
//     >0 : the last position,  < 0 : error
//------------------------------------------------------------------------------
static int cfg_parse(char *from, int len, struct cfgobj_list_t *cfg_lp)
{
    cfgobj * p, *wp, *endp;
    int wlen, num;
    int rc=0;   

    wlen=num=0;

    cfg_log("cfg_parse: read %d bytes\n", len );

    endp = (cfgobj *) ((unsigned int)from +  len ) ;

    for ( p=(cfgobj *)from, wp=(cfgobj *) ((char*)cfg_lp->head+cfg_lp->len);
          p < endp ;
            p=(cfgobj*)((unsigned int)(p+1)+p->len) )
    {
        int tlen=sizeof(cfgobj)+ p->len;
        if ((p->cid == CFG_ID_END ) || (p->len & 3 ) )
        	break;
        else
        if ((cid_ok(p->cid)) || (p->cid == CFG_SYS_FMW_IDSTR))
        {
            if ( tlen > (sizeof(cfgobj)+CFG_MAX_OBJ_SZ ) )
            {
                diag_printf("cfg_parse: cid %x - err len: %d\n", p->cid, tlen - sizeof(cfgobj) );
                break;
            }
            num++;
            wlen+=tlen;
            memcpy(wp, p, tlen);
            wp=(cfgobj*)((unsigned int)wp + tlen );
        }
    }

    cfg_lp->num += num;
    cfg_lp->len += wlen;

    if (wlen==0)
    {
        diag_printf("cfg_parse: null config!\n");
        rc=-1;
    }
    else
        diag_printf("cfg_parse: add cfg item=%d (%d bytes)\n", num, wlen);

    return rc;

}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_parse_wlan
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//      load the cfgobj list and parse WIFI configuration into the array which WIFI driver uses.
//
//------------------------------------------------------------------------------
// PARAMETERS
//     
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : fail
//------------------------------------------------------------------------------
int CFG_parse_wlan(struct cfgobj_list_t *cfg_lp)
{
	cfgobj * p, *wp, *endp;
	int wlen, num;
	char *str;
	int rc = 0; 
	int d;
	char *var;
	int optr = 0;

	if(cfg_lp->head == NULL)
		return -1;

	if (wlanbuf == NULL)
		wlanbuf = malloc(7200);

	optr += sprintf(wlanbuf+optr, "%s", "#The word of \"Default\" must not be removed\nDefault\n");

	endp = (cfgobj *) ((unsigned int)cfg_lp->head + cfg_lp->len) ;

	for (wp = (cfgobj *)(cfg_lp->head); wp < endp; 
		wp= (cfgobj*)((unsigned int)(wp+1)+wp->len))
	{
		if(cid_ok(wp->cid))
		{
			str = CFG_id2str(wp->cid);

			if(strstr(str, "WLN") != NULL)
			{				
				optr += sprintf(wlanbuf+optr, "%s=", str+4);

				switch (CFG_ID_TYPE(wp->cid))
				{	
					case CFG_TYPE_STR:
						if ( wp->len )
                    				optr += sprintf(wlanbuf+optr, "%s\n", (char *)(wp+1));
                				else
                    				optr += sprintf(wlanbuf+optr, "\n" );
                    			break;
					case CFG_TYPE_MAC:
						optr += sprintf(wlanbuf+optr, "%s\n", ether_ntoa( (struct ether_addr *)(wp+1)));
						break;
					case CFG_TYPE_INT:
						optr += sprintf(wlanbuf+optr, "%d\n", *(int *)(wp+1));
						break;
					case CFG_TYPE_IP:
                       			optr += sprintf(wlanbuf+optr, "%s\n", inet_ntoa( *(struct in_addr *)(wp+1)));
						break;
					default:
						optr += sprintf(wlanbuf+optr, "%s\n", (char *)(wp+1));
                				break;
				}		
			}
		}
	}

	diag_printf("cfg_parse_wlan: parse %d bytes!\n", optr);
	
	return rc;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  cfg_init_header
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//  Describe the purpose of this function.
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//  parm1:  description of parameter 1
//
//------------------------------------------------------------------------------
// RETURN
//
//  NONE
//
//------------------------------------------------------------------------------
void cfg_init_header(void)
{
    cfg_list.len = cfg_list.num =  0 ;
    cfg_list.head = (cfgobj *) &cfg_data[0];
    {
        unsigned int val=CFG_HEADER_MAGIC;
        CFG_set(CFG_ID_HEADER , &val  ) ;
        val = 0 ;
        CFG_set(CFG_ID_CHKSUM , &val  ) ;
    }
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  cfg_load_static
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      loading static (read-only) information
//      load read-only status, h/w info
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : not found
//------------------------------------------------------------------------------
int cfg_load_static(void)
{
    char buf[80];
    int o_num=cfg_list.num;
    int o_len=cfg_list.len;
    
    /*  load some static info from flash */
    cfg_parse((char*)cfg_list_boot.head, cfg_list_boot.len, &cfg_list);

    // s/w version
    CFG_set(CFG_STS_FMW_VER, &sys_sw_version);

    // build info
    sprintf(buf, " %s", time2string(sys_build_time));
    CFG_set(CFG_STS_FMW_INFO, buf);

	if (_cfgfmw_idstr != NULL)
		CFG_set(CFG_SYS_FMW_IDSTR, _cfgfmw_idstr);

    diag_printf("cfg_load_static: read-only %d items(%d bytes) into ram\n", cfg_list.num - o_num, cfg_list.len- o_len );
    return 0;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_load
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      loading the active config obj to cfgobj list
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the profile id
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : not found
//------------------------------------------------------------------------------
int CFG_load(int id)
{
	int rc=0;
	int len;
	cfgobj * p;
	cfgobj * ep;
	unsigned int chksum, org_sum;
	unsigned int endp;
	int found;
	unsigned char valbuf[128];
	char *cfgbuf;
	 /*Auto SSID*/
	unsigned char val[255],wlan_mac[7];
	val[0] = '\0';
	char ssid[255];
	
	cfg_list.num = cfg_list.len = 0;
	cfg_list.head = (cfgobj*)&cfg_data[0];   

	if ((cfgbuf = malloc(flsh_cfg_sz)) == NULL)
		return -1;
    
	/*  Load system  configuration to buffer */
	if ((rc = flsh_read(flsh_cfg_off, cfgbuf, flsh_cfg_sz)) != 0) {
		diag_printf("[CFG]: Load sysconf failed. err=%d\n", rc);
		goto errout;
	}                                                  

	p = (cfgobj *)cfgbuf;
	endp = (unsigned int) (cfgbuf + flsh_cfg_sz);
	cfg_flash.offset = flsh_cfg_off ;
	cfg_flash.len = len=flsh_cfg_sz;
	found=0;
	while ( len > 1024 && !found )  // minimum cfg size
	{
		p = cfg_find( (char*)p , len,  CFG_ID_HEADER );
		if ( p==0 )
			break;

		// new search length
		len = endp - (unsigned int) p ;

		if ( p->len == 4  && *(unsigned int*)(p+1) == CFG_HEADER_MAGIC )
		{
			found=1;
			// remember where is the data start
			cfg_flash.offset = (unsigned int) p - (unsigned int)cfgbuf;

			len = endp - (unsigned int) p ;
			ep = cfg_find( (char*)p, len, CFG_ID_CHKSUM );
			if ( ep )
				cfg_flash.len = (unsigned int ) ep - (unsigned int) p + (sizeof(*p) + 4) ;
			else
				cfg_flash.len = endp - (unsigned int ) ep ;
			
			diag_printf("CFG_load: flash offset=%04x, len=%04x\n", cfg_flash.offset, cfg_flash.len );	 
			break;
		}
	}
	
	if (!found) {
		rc = CFG_ETYPE ;
		goto errout;
	}

	rc=cfg_parse( (char*) p , cfg_flash.len, &cfg_list );
	diag_printf("CFG_load: flash read %d items(%d bytes) into ram\n", cfg_list.num, cfg_list.len );
	if (rc != 0) {
		goto errout;
	}

	if (ep)
	{
		len=(unsigned int)ep - (unsigned int)p ;
		chksum = chk_sum( (int*)p, len );
		CFG_get(CFG_ID_CHKSUM, &org_sum);
		if (org_sum != chksum )
		{
			diag_printf("CFG_load: Chksum err,org=%08x, calculated=%08x\n", org_sum, chksum);
		}
	}
	else
	{
		diag_printf("CFG_load: Error, No chksum found\n");
	}

	if (_cfgfmw_idstr != NULL) {
		valbuf[0] = 0;
		if ((CFG_get(CFG_SYS_FMW_IDSTR, valbuf) < 0) || strcmp(_cfgfmw_idstr, valbuf) != 0) {
			diag_printf("CFG: incorrect CFG ID. Force factory default\n");
			rc = CFG_EIDSTR;
			goto errout;
		}
	}

	cfg_load_static();    

	diag_printf("CFG_load: total %d items(%d bytes)\n", cfg_list.num, cfg_list.len );

#ifdef CONFIG_WIRELESS
	CFG_get_str(CFG_WLN_AutoSSID, val);
// 	if (strcmp(val, "") == 0)
// 	{
// 		diag_printf("AutoSSID Faill!\n");
// 		return 0;
// 	}
// 	
	if(atoi(val)==0)
	{
		diag_printf("AutoSSID Start!\n");
		memset(wlan_mac, 0 ,sizeof(wlan_mac));
		CFG_get_mac(2, &wlan_mac);

		diag_printf("wlan_mac:%02x:%02x:%02x:%02x:%02x:%02x\n"
		,wlan_mac[0]
			,wlan_mac[1]
			,wlan_mac[2]
			,wlan_mac[3]
			,wlan_mac[4]
			,wlan_mac[5]);


		memset(val, 0 ,sizeof(val));
		memset(ssid, 0 ,sizeof(ssid));
		memset(valbuf, 0 ,sizeof(valbuf));
		/* get ssid */
		CFG_get_str(CFG_WLN_SSID1, val);
		
		/* format ssid */
		sprintf(valbuf, "%s",val);
		strcat(ssid,valbuf);
		strcat(ssid,"_");
		int i;
		for(i=3;i<=5;i++)
		{
			if(wlan_mac[i]!=0)
			{
			sprintf(valbuf, "%02x",wlan_mac[i]);
			strcat(ssid,valbuf);
			}
			else
			{
			strcat(ssid,"00");
			}

		}
		diag_printf("New SSID=%s!\n",ssid);
		CFG_set(CFG_WLN_SSID1, ssid);
		CFG_set(CFG_WLN_AutoSSID,"1");
		CFG_commit(CFG_DELAY_EVENT);
	}
	
	rc = CFG_parse_wlan(&cfg_list);
	if (rc != 0) 
	{
		diag_printf("Parsing WIFI configuration fails!!\n"); 
      		goto errout;
    	}
	else
		diag_printf("Parsing WIFI configuration succeeds\n");
#endif

errout:
	if (cfgbuf != NULL)
		free(cfgbuf);
	cfg_update_cnt=0;   // reset to zero 

	return rc;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_write_prof
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      set the config value to the cfgobj list
//
//------------------------------------------------------------------------------
// PARAMETERS
//      id : the obj id, val: the return value
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : not found
//------------------------------------------------------------------------------
int CFG_write_prof(char * file, int max)
{
    cfgobj_list * list=&cfg_list;
    cfgobj * p, *endp;
    cfgid cid;
    int i;
	int optr=0;

    p=list->head;
    if (p==0)
    {
        cfg_log("CFG_write_prof:null cfg_data\n");
        return -1;
    }

	if (max < 500)
	{
		cfg_log("CFG_write_prof:buf too small\n");
		return -1;
	}

    optr+=sprintf(file+optr,"%s\n", CFG_PROFILE_HEADER);
    optr+=sprintf(file+optr,"%s\n", CFG_PROFILE_VER);

    endp = (cfgobj *) ((unsigned int)list->head +  list->len ) ;
    for (i=0; (i < list->num) && ( p < endp ) && (optr<max);
            i++,
            p=(cfgobj*)((unsigned int)(p+1)+ p->len ) )
    {
        cid=p->cid;
        
        if ( cid >= CFG_ID_CHKSUM  || cid <= CFG_ID_HEADER )
            continue;

        if (!cid_ok(cid))
            break;

        cfg_log("CFG_write_prof: wr cid=%08x,@ %08x=%08x\n", cid, p+1,*((int*)(p+1)) );

        if (cid == (CFG_DNS_SVR+1))
            optr+=sprintf(file+optr,"DNS_SRV1=");
        else if (cid == (CFG_DNS_SVR+2))
            optr+=sprintf(file+optr,"DNS_SRV2=");
        else
            optr+=sprintf(file+optr,"%s=", CFG_id2str(cid));

        switch ( CFG_ID_TYPE(cid) )
        {
            case CFG_TYPE_MAC:
                optr+=sprintf(file+optr, "%s\n", ether_ntoa( (struct ether_addr *)(p+1)) );
                break;

            case CFG_TYPE_IP:
                optr+=sprintf(file+optr, "%s\n", inet_ntoa( *(struct in_addr *)(p+1)) );
                break;

            case CFG_TYPE_INT:
                optr+=sprintf(file+optr, "%d\n", *(int *)(p+1) );
                break;

            case CFG_TYPE_STR:
                if ( p->len )
                    optr+=sprintf(file+optr, "%s\n", (char *)(p+1) );
                else
                    optr+=sprintf(file+optr, "\n" );
                break;

            default:
                optr+=sprintf(file+optr, "%s\n", (char *)(p+1));
                break;
        }
    }
    optr+=sprintf(file+optr,"\n");

    cfg_log("CFG_write_prof: %d items!, size=%d\n", i, optr);

    return optr;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//  CFG_read_prof
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      read the config variables from the profile
//
//------------------------------------------------------------------------------
// PARAMETERS
//      data: the data buf , file: from this file
//
//------------------------------------------------------------------------------
// RETURN
//     =0 : ok ,  < 0 : not found
//------------------------------------------------------------------------------
int CFG_read_prof(char * file, int size)
{
	char line[300], *sp, *ep;
    char *var, *val;
    cfgid cid;
    int rc=0;
	int len;
	int num=0;
	struct ether_addr *mac;

	for (sp=ep=file;(ep < (file+size));)
	{
		for (sp=ep;(*ep!=0x0d)&&(*ep!=0x0a)&&(ep<(file+size));ep++);
		len=ep-sp;
		if (len>299) break;
		memcpy(line, sp, len);
		line[len]=0;

		while(*ep && (*ep==0x0d || *ep==0x0a)) ep++; //eat to end of line

		var = line;
		/* skip space */
		while(*var && isspace(*var)) var++;
		/* skip comment */
		if(*var == '#' ) continue;

        val=strchr( (const char*)var, '=' );    // found the '='
        if ( val == NULL )
			continue;
        *val=0;                 // cut the var name;
        val++;

        if (strncmp(var, "DNS_SRV1", 8) == 0)
            cid=CFG_DNS_SVR+1;
        else if (strncmp(var, "DNS_SRV2", 8) == 0)	
            cid=CFG_DNS_SVR+2;
        else
	        cid=CFG_str2id(var);

        if (cid==0) continue; // a invalid cfgid, skip

		switch ( CFG_ID_TYPE(cid) )
		{
                case CFG_TYPE_STR:
                    CFG_set(cid, (void*)val);
                    break;

                case CFG_TYPE_MAC:
                    {
			    mac = ether_aton(val);
			    if (mac)
				    CFG_set(cid, mac);
                    }
                    break;

                case CFG_TYPE_INT:
                    {
                        int d=atoi(val);
                        CFG_set(cid, (void*)&d );
                    }
                    break;

                case CFG_TYPE_IP:
                    {
                        struct in_addr addr;
                        if (inet_aton(val, &addr)) // only correct then save
                        	CFG_set(cid,(void*)&addr );
                    }
                    break;

		} //switch
		num++;
	}
    if (rc)
        cfg_log("CFG_read_prof err%x\n", rc);
	else
		diag_printf("CFG_read_prof %d items\n", num);

    return rc;
}


//------------------------------------------------------------------------------
// FUNCTION
//      CFG_str2id
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      mapping the sting to the cfgid
//
//------------------------------------------------------------------------------
// PARAMETERS
//      the config variable
//
//------------------------------------------------------------------------------
// RETURN
//      the cfgid in (int) format.
//
//------------------------------------------------------------------------------
int CFG_str2id(char * var)
{
    int i,dig;
    int len=strlen(var);
    int id=0;
	char tmpc=0;

    if (len==0)
        return 0;

    for (i=0; i<CFG_ID_STR_NUM; i++)
    {
		if (!strcmp(var, cfgid_str_table[i].str))
		{
			id=cfgid_str_table[i].id;
			break;
		}
    }

    return id;
}


//------------------------------------------------------------------------------
// FUNCTION
//  CFG_id2str
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      mapping the cfgid to the config variable string
//
//------------------------------------------------------------------------------
// PARAMETERS
//      the cid in int format.
//
//------------------------------------------------------------------------------
// RETURN
//      the var string
//
//------------------------------------------------------------------------------
char * CFG_id2str(int id)
{
    static char str[32];
    int i;
    int index;
    int found=0;

	for (i = 0; i<CFG_ID_STR_NUM; i++)
	{
		if (id == cfgid_str_table[i].id)
		{
			sprintf(str, "%s", cfgid_str_table[i].str );
			found++;
			break;
		}
	}

    if (!found)
        sprintf(str, "UNKNOWN" );

    return str;
}


static void cfg_flsh_init(void)
{	
	if (cfg_inited)	
		return ;
	flsh_init();
	flsh_cfg_sz = CFG_FLSH_CONF_SZ;

	switch (pFlsh->type)
	{
		case F_BOOT_SEC_BOT:
			flsh_cfg_off = CFG_FLSH_CONF_LOC_BOT ;
			flsh_cfg_boot_off = CFG_FLSH_BOOT_LOC_BOT ;
			flsh_cfg_fwm_off = CFG_FLSH_FMW_LOC_BOT ;
			flsh_cfg_fwm_sz = pFlsh->size - CFG_FLSH_FMW_LOC_BOT ;
			break;
		case F_BOOT_SEC_TOP:
			flsh_cfg_off = pFlsh->size - CFG_FLSH_CONF_SZ; //top most block
			flsh_cfg_boot_off = CFG_FLSH_BOOT_LOC_TOP;
			flsh_cfg_fwm_off = CFG_FLSH_FMW_LOC_TOP ;
			flsh_cfg_fwm_sz = flsh_cfg_off - CFG_FLSH_FMW_LOC_TOP ;
			break;
		default: /* eCos SPI flash configuration */
			flsh_cfg_boot_off = CFG_FLSH_BOOT_LOC_NO_BOOT_SEC ;
			flsh_cfg_off = CFG_FLSH_CONF_LOC_NO_BOOT_SEC; //give default
			flsh_cfg_fwm_off = CFG_FLSH_FMW_LOC_NO_BOOT_SEC;
			flsh_cfg_fwm_sz = pFlsh->size - CFG_FLSH_FMW_LOC_NO_BOOT_SEC ;
			flsh_cfg_sz = CFG_FLSH_CONF_SZ_NO_BOOT_SEC ;
			break;
	}

	cfg_list_boot.head = (cfgobj*)cfg_factory_blk;
	cfg_list_boot.num = 0;

	cfg_inited=1;
	
}


//------------------------------------------------------------------------------
// FUNCTION
//  CFG_start
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
void CFG_start(int cmd)
{
	int err;

	if (cfg_inited)
		return;

	CFG_INIT_LOCK();

	in_application++;
	cfg_flsh_init();
	err=CFG_load(0);

	if (err)    // copy the default
	{
		diag_printf("Load default configuration!\n"); 
		CFG_init_prof();
		eventcmd = 0;		// Don't send event
		//CFG_save(0);
	}

}       

        
//------------------------------------------------------------------------------
// FUNCTION
//  CFG_init_prof
//
//------------------------------------------------------------------------------
// DESCRIPTION
//      reset to default
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
extern unsigned char profile[];
extern int 	profile_len;

int CFG_init_prof(void)
{
	int err;
	int rc = 0;

	CFG_LOCK();

	cfg_init_header();
	err=CFG_read_prof(&_binary_profile_txt_start[0], (int)&_binary_profile_txt_end[0]-(int)&_binary_profile_txt_start[0]);
	cfg_load_static(); 

	CFG_UNLOCK();

#ifdef CONFIG_WIRELESS
#if defined(RT6352) || defined(MT7620)||defined(CONFIG_MT7628_ASIC)
	CFG_set_str(CFG_WLN_HT_TxStream, "2");
	CFG_set_str(CFG_WLN_HT_RxStream, "2");
#endif /* RT6352 || MT7620||defined(CONFIG_MT7628_ASIC) */
	rc = CFG_parse_wlan(&cfg_list);
	if (rc != 0) 
		diag_printf("Default : parsing WIFI configuration fails!!\n"); 
	else
		diag_printf("Default : parsing WIFI configuration succeeds\n");
#endif
	
    return err;
}


static const char profile_header[] = CFG_PROFILE_HEADER ;
static const char fw_header[] = CFG_FW_HEADER ;
static const char fw_header2[] = CFG_FW_HEADER2 ;
static const char fw_header3[] = CFG_FW_HEADER3 ;

extern int FirstImageSegment;
extern unsigned int write_flsh_cfg_fwm_off;
int cfg_chk_header(int id, char * file)
{
    switch (id)
    {
        case 0: //config profile
            if (memcmp(file, profile_header, CFG_PROFILE_HEADER_LEN)==0)
                return 0;
            else
                return -1;
            break;

        case 1: // firmware
            file += EXTERNAL_SIZE;
		cyg_uint32 hdr;
		memcpy(&hdr, file, sizeof(cyg_uint32));
		// we only need b26 to b31
		if ((hdr & 0xfa000000) == 0x08000000)
			return 0;
		else if ((hdr & 0xfa000000) == 0x10000000)
			return 0;
            break;
    }
    return -1;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
int CFG_save(int id)
{
    int rc =-1;
    int len;
    cfgobj * p = NULL;
    unsigned int chksum;
    int savep = -1;

    p=cfg_find((char*)cfg_list.head, cfg_list.len, CFG_ID_CHKSUM );  // this is the first read-only variable

    if (p)
    {
        len=(unsigned int)p - (unsigned int)cfg_list.head;
    }
    else

    {
        diag_printf("CFG_save: Not found CHKSUM!!!\n");
        len=cfg_list.len;
		goto err ;
    }

    if (len > (flsh_cfg_sz-12) )
    {
        diag_printf("CFG_LEN > %d\n", flsh_cfg_sz);
        len = (flsh_cfg_sz-12)  ;
    }
    diag_printf("CFG_save: write %d bytes to flash:", len);

    chksum=chk_sum((int *)cfg_list.head, len);
    *(unsigned int*)(p+1) = chksum ;
    diag_printf("CFG_CHKSUM=%08x, len=%d\n", chksum , len);
    if (p==0)
    {
        p=(cfgobj*)( (unsigned int) cfg_list.head + len );
        p->len=4;
        *(unsigned int*)(p+1)=chksum;
    }
    len += ( sizeof(*p) + 4) ;


    if ( ( cfg_flash.offset +  cfg_flash.len + len ) > flsh_cfg_sz )
    {
        diag_printf("CFG_save: flsh run-out, erase.\n");
        CFG_LOCK();
        rc=flsh_erase(flsh_cfg_off, flsh_cfg_sz );
        CFG_UNLOCK();
        if (rc)
        {
            goto err;
        }
		
        cfg_flash.offset = 0 ;
    }
    else
    {
        savep = cfg_flash.offset ; // remember where to clear the older header
        cfg_flash.offset = cfg_flash.offset + cfg_flash.len ; // new offset
        diag_printf("CFG_save: old offset=%04x, new=%04x\n", savep, cfg_flash.offset );
    }
    cfg_flash.len = len ;

    CFG_LOCK();

    rc=flsh_write(flsh_cfg_off + cfg_flash.offset, (unsigned int)cfg_list.head , len);
    CFG_UNLOCK();
    if (rc)
    {
        diag_printf("CFG_save: flw err (%d)\n", rc);
        goto err;
    }
    if (savep >= 0 )
    {
        int zero = 0;
        // clear the orginal header's id , so it won't effect the searching
        CFG_LOCK();
        flsh_write(flsh_cfg_off + savep, (unsigned int)&zero , 4);
        CFG_UNLOCK();
    }

err:
    // reset the cfg update count
    cfg_update_cnt=0; 

    return rc;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
int CFG_commit(int id)
{
	int rc=0;
	int update_cnt = 0;
	
	diag_printf("CFG_commit: %d update!\n", cfg_update_cnt);
	if (cfg_verify_flag)
		cfg_verify_chk();

	update_cnt = cfg_update_cnt;

	if (cfg_update_cnt)
	{
		untimeout(CFG_save, 0 );    // cancel the last save command, restart again
		if (id & CFG_COM_SAVE)
			rc=CFG_save(0);
		else
			timeout(CFG_save, 0, CFG_SAVE_TIMEOUT  );
	}
	
	if (!(id & CFG_NO_EVENT) && eventcmd )
	{
		if (id & CFG_DELAY_EVENT)
			timeout(mon_snd_cmd, eventcmd, 1000);
		else
			mon_snd_cmd(eventcmd);
	}
    eventcmd = 0;
    
    return update_cnt;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
int CFG_write_image(char * file, int len, int totalLen)
{
    int rc, ok=0;
	unsigned long irqFlags=0;
	static int erase_offset=0;;
	
	if(file == NULL || len <= 0)
		return -3;
	if (erase_offset == 0)
		erase_offset = flsh_cfg_fwm_off;
	
	if(FirstImageSegment)
	{
	    if ( 0!=cfg_chk_header(1, file) )
	    {
	        int i;
	        diag_printf("[CFG] err header id=1, dump:");
	        for (i=0; i<8; i++)
	            diag_printf("%02x ", file[i]&0xff);
	        diag_printf("\n");
	        ok=-3;      // file error
	        return ok;
	    }

		diag_printf("[CFG] FMW sz=%x ,max=%x!\n", totalLen, flsh_cfg_fwm_sz);
	
		if (totalLen > flsh_cfg_fwm_sz )
		{
			return -3;
		}
		
		//??? checksum skip now, todo add checksum in future
		/*if (chk_fmw_hdr(file,len))
		{
			ok=-3;
			return ok;
		}*/
		
	    //diag_printf("[CFG] FMW upload\n");
	    diag_printf("[CFG] FMW upload, sz %d B to flash:\n", totalLen);

		//Here, totalLen use content lenght, it's bigger than actual image length, about 500bytes.
		//For upgrade firmware by segment,we can't get actual image length,until finish write total image.
		//So we use content length from HTTP header to use.
		//HAL_DISABLE_INTERRUPTS(irqFlags);
#if 0		
		    rc=flsh_erase(flsh_cfg_fwm_off, totalLen);
		   diag_printf("\nflsh_erase:ddr[%x],len[%d] Done !!\n",flsh_cfg_fwm_off,totalLen);
		//HAL_RESTORE_INTERRUPTS(irqFlags); 
	        if (rc)
	        {
	            diag_printf("er err %d\n", rc);
	            ok = -3;
				return ok;
	        }	
#endif
	}
	if(write_flsh_cfg_fwm_off < flsh_cfg_fwm_off)//??? todo:add flash range juageemnt to proptect
		panic("write_flsh_cfg_fwm_off error!\n");

	if (erase_offset < write_flsh_cfg_fwm_off + len)
		{
		 rc=flsh_erase(erase_offset, min(65536,totalLen-(erase_offset-flsh_cfg_fwm_off)));
		 if (rc)
	        {
	            diag_printf("er err %d\n", rc);
	            ok = -3;
				return ok;
	        }	
			erase_offset += min(65536,totalLen-(erase_offset-flsh_cfg_fwm_off));
		}
	//HAL_DISABLE_INTERRUPTS(irqFlags);
        rc=flsh_write(write_flsh_cfg_fwm_off, (unsigned int)file, len);
		//diag_printf("flsh_write:next-addr[%x],len[%d]",write_flsh_cfg_fwm_off+len,len);
	//HAL_RESTORE_INTERRUPTS(irqFlags); 

    if (rc)
    {
        diag_printf("wr err %d\n", rc);
        ok = -3;
    }
    else
       ;// diag_printf("OK!\n");
            
    return ok;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
int CFG_put_file(int id, char * file, int len)
{
    int rc, ok=0;
	unsigned long irqFlags=0;
	
	if(file == NULL || len <= 0)
		return -3;
    if ( 0!=cfg_chk_header(id, file) )
    {
        int i;
        diag_printf("[CFG] err header id=%d, dump:", id );
        for (i=0; i<8; i++)
            diag_printf("%02x ", file[i]&0xff);
        diag_printf("\n");
        ok=-3;      // file error
        return ok;
    }

    switch (id)
    {
        case 1:
		diag_printf("[CFG] FMW sz=%x ,max=%x!\n", len, flsh_cfg_fwm_sz);
			if (len > flsh_cfg_fwm_sz )
			{

				return -3;
			}

			if (chk_fmw_hdr(file,len))
			{
				ok=-3;
				return ok;
			}
            diag_printf("[CFG] FMW upload\n");
            printf("[CFG] FMW, sz %d B to flash:", len);

			HAL_DISABLE_INTERRUPTS(irqFlags);
            rc=flsh_erase(flsh_cfg_fwm_off, len);
			HAL_RESTORE_INTERRUPTS(irqFlags); 
            if (rc)
            {
                diag_printf("er err %d\n", rc);
                ok = -3;
                break;
            }

			HAL_DISABLE_INTERRUPTS(irqFlags);
            rc=flsh_write(flsh_cfg_fwm_off, (unsigned int)file, len);
			HAL_RESTORE_INTERRUPTS(irqFlags); 

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
            ok=CFG_read_prof(file, len);     // reload to CFG
            cfg_load_static(); 

            if (ok==0)
			{
				CFG_commit(CFG_DELAY_EVENT);
			}
            break;

        default:
            diag_printf("unknown");
            break;
    }
    return ok;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
void cfg_get_mac(int id, char *dp)
{
    CFG_get_mac(id, dp);
}

#define MAC0_MAC_ADDRESS_OFF    0x28 //0x28, 0x2A, 0x2C
#define MAC1_MAC_ADDRESS_OFF    0x2E //0x2E, 0x30, 0x32
#define MAC2_MAC_ADDRESS_OFF	0x4 //0x4 0x6.0x8

void CFG_get_mac(int id, char *dp)
{
	unsigned char * buf=(unsigned char *)dp;
	int rc;
	int offset;

	if (cfg_inited == 0)
		CFG_start(0);

	if (id == 0)
		offset = MAC0_MAC_ADDRESS_OFF;
	else if (id == 1)
		offset = MAC1_MAC_ADDRESS_OFF;
	else if (id ==2)
		offset=MAC2_MAC_ADDRESS_OFF;
	else
		goto errout;

	if ((rc = flsh_read(flsh_cfg_boot_off + offset, buf, 6)) != 0) {
		diag_printf("[CFG]: Read MAC address failed. err=%d\n", rc);
		goto errout;
	}
	
	if ((buf[0] == 0xff) 
		&& (buf[1] == 0xff)
		&& (buf[2] == 0xff)
        	&& (buf[3] == 0xff)
        	&& (buf[4] == 0xff)        
        	&& (buf[5] == 0xff))
        goto errout;

	diag_printf("cfg_get_mac: id=%d : %02x:%02x:%02x:%02x:%02x:%02x\n"
		,id
		,buf[0]&0xff
        	,buf[1]&0xff
        	,buf[2]&0xff
        	,buf[3]&0xff
        	,buf[4]&0xff
        	,buf[5]&0xff);

	return;
    
errout:
	memset(buf, 0, 5);
	buf[5]=(id+1)&0xff;
	diag_printf("No MAC found: fake one! id=%d : %02x:%02x:%02x:%02x:%02x:%02x\n"
		,id
		,buf[0]&0xff
        	,buf[1]&0xff
        	,buf[2]&0xff
        	,buf[3]&0xff
        	,buf[4]&0xff
        	,buf[5]&0xff);
	
	return;
} 


//------------------------------------------------------------------------------
// FUNCTION
//      Get the wlan channel gain parameters
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
int CFG_get_rf_agc(int ch, int *val)
{
    int rc;

    rc = CFG_get2( CFG_HW_WLN_GAIN+ch, val ) ;
    return rc; 
} 


//------------------------------------------------------------------------------
// FUNCTION
//      Set the wlan channel gain parameters
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
int CFG_set_rf_agc(int ch, int val)
{
    int rc;

    rc = CFG_set2(CFG_HW_WLN_GAIN+ch, &val ) ;
    return rc;
} 


//------------------------------------------------------------------------------
// FUNCTION
//
//
//------------------------------------------------------------------------------
// DESCRIPTION
//
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
void CFG_reset_default()
{
	diag_printf("Reset the default configuration to flash!\n"); 
	CFG_init_prof();

#if defined(RT6352) || defined(MT7620)||defined(CONFIG_MT7628_ASIC)
	CFG_set_str(CFG_WLN_HT_TxStream, "2");
	CFG_set_str(CFG_WLN_HT_RxStream, "2");
#endif /* RT6352 || MT7620 */
	CFG_commit(CFG_DELAY_EVENT);
}
#endif //CFG_PROF_C


//------------------------------------------------------------------------------
// FUNCTION
//
//
//------------------------------------------------------------------------------
// DESCRIPTION
//		since some variable is dependent to LAN ip, we check them 
//		and may need to change them before save
//------------------------------------------------------------------------------
// PARAMETERS
//
//------------------------------------------------------------------------------
// RETURN
//
//------------------------------------------------------------------------------
int cfg_id_chk_list[]=
{	(CFG_NAT_VTS+1),  (CFG_FW_FLT_CLN+1), (CFG_FW_FLT_PKT+1), (CFG_NAT_PTRG+1) ,
	CFG_LAN_DHCPD_START, CFG_LAN_DHCPD_END, (CFG_LAN_DHCPD_SLEASE+1), CFG_NAT_DMZ_HOST
};

static int chk_set(char *buf1, char *buf2,char *ip1,char *ip2)
{
	char *cp1,*cp2,*p;
	int len;
	int len1=strlen(buf1);
	int iplen1=strlen(ip1);
	int iplen2=strlen(ip2);
	int changed=0;
	cp1=buf1;
	cp2=buf2;
	do
	{
		p=strstr(cp1,ip1);
		if (!p) break;
		 // found oip, then replace with new
		len=p-cp1; // how many to copy
		memcpy(cp2,cp1,len);
		cp1+=(len+iplen1);
		cp2+=len;
		memcpy(cp2,ip2,iplen2);
		cp2+=iplen2;
		changed++;
	} while (cp1 < (buf1+len1));
	if (cp1<(buf1+len1)) // copy else data
	{
		len=(buf1+len1)-cp1;
		memcpy(cp2,cp1,len);
		cp2+=len;
	}
	*cp2=0;	// null ending
	return changed;
}


static void cfg_verify_mark(void)
{
	if (!cfg_verify_flag)
	{
		CFG_get(CFG_LAN_IP, &cfg_chk_old_lanip);
	}
	cfg_verify_flag++;
}


static int cfg_verify_chk(void)
{
	int i,j,id,rc;
	char buf1[256];
	char buf2[256];
	char ip1[20];
	char ip2[20];
	int count=0;
	int new_lanip;

	if (!cfg_chk_old_lanip) goto out;
	i=ntohl(cfg_chk_old_lanip);
	sprintf(ip1,"%d.%d.%d.",(i>>24)&0xff,(i>>16)&0xff,(i>>8)&0xff);

	CFG_get(CFG_LAN_IP,&new_lanip);
	i=ntohl(new_lanip);
	sprintf(ip2,"%d.%d.%d.",(i>>24)&0xff,(i>>16)&0xff,(i>>8)&0xff);

	if (!strcmp(ip1,ip2))
	{
		goto out;
	}

	for (i=0;i<sizeof(cfg_id_chk_list)/4;i++)
	{
		id=cfg_id_chk_list[i];
		for (j=0;j<256;j++)
		{
			if (CFG_get_str(id+j, buf1)) break; // not found, skip
			rc=chk_set(buf1,buf2,ip1,ip2);
			if (rc==0) continue; // not thing to change

			CFG_set_str(id+j,buf2);
			count++;
		}
	}

out:
	cfg_verify_flag=0;
	return count;
}

