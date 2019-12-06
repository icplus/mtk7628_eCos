/* 
    version.h 
*/

#include "config.h"

#ifndef SYS_BUILD_TIME
#define SYS_BUILD_TIME  1072349176
#endif

#if defined(CONFIG_WANIF)
	#define _model_num	2
#else
	#define	_model_num 10
#endif


#if	(CONFIG_XROUTER_RAM_SIZE==2)
	#define	CONFIG_MODEL (_model_num+20)
#else
	#define CONFIG_MODEL	_model_num
#endif

#define	_VER_MAJOR	1
#define	_VER_MINOR	2
#define	_VER_PATCH	121

#define xstr(s) str(s)
#define str(s) #s

#define	SYS_SW_VER (((CONFIG_MODEL&0xff)<<24)|((_VER_MAJOR&0xff)<<16)|((_VER_MINOR&0xff)<<8)|(_VER_PATCH&0xff))
#define	SYS_SW_VER_STR	xstr(CONFIG_MODEL) "." xstr(_VER_MAJOR) "." xstr(_VER_MINOR) "." xstr(_VER_PATCH)

#define	SYS_INTERNEL_VER_STR	"V3.1.0.0"

#ifndef	CONFIG_CUST_VER_STR
#define	SYS_CUST_VER_STR	SYS_INTERNEL_VER_STR
#else
#define	SYS_CUST_VER_STR	CONFIG_CUST_VER_STR
#endif

#ifdef RT3050
#define	SYS_INTERNEL_HD_VER_STR	"RT3050"
#else
#ifdef RT3052
#define	SYS_INTERNEL_HD_VER_STR	"RT3052"
#else
#ifdef RT3352
#define	SYS_INTERNEL_HD_VER_STR	"RT3352"
#else
#ifdef RT5350
#define	SYS_INTERNEL_HD_VER_STR	"RT5350"
#else
#ifdef MT7620
#define	SYS_INTERNEL_HD_VER_STR	"MT7620"
#else
#define	SYS_INTERNEL_HD_VER_STR	"Unknown"
#endif //MT7620
#endif //RT5350
#endif //RT3352
#endif //RT3052
#endif //RT3050

#ifndef	CONFIG_HD_CUST_VER_STR
#define	SYS_HD_CUST_VER_STR	SYS_INTERNEL_HD_VER_STR	
#else
#define	SYS_HD_CUST_VER_STR	CONFIG_HD_CUST_VER_STR
#endif

extern const unsigned int sys_build_time;
extern const int sys_build_count;
extern const unsigned int sys_sw_version;
extern const char *sys_fw_chksum;
// upnp reference
extern const char sys_os_name[];
extern const char sys_os_ver[];
extern const char sys_model_name[];
extern const char sys_device_name[];
extern const char sys_vendor_name[];
extern const char sys_model_ver[];
// customize version string
extern const char sys_cust_ver[];
extern const char sys_internel_cust_ver[];
extern const char sys_hd_cust_ver[];
extern const char sys_internel_hd_cust_ver[];


