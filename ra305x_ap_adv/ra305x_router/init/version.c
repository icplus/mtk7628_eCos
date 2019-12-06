/*
    version.c
*/
#include "build_time.h"
#include <version.h>
#include <cyg/kernel/kapi.h>

const unsigned int sys_build_time=SYS_BUILD_TIME;
const int sys_build_count=SYS_BUILD_COUNT;
const char *sys_fw_chksum;

#define	_v1 (SYS_SW_VER>>24)
#define	_v2 ((SYS_SW_VER>>16)&0xff)
#define	_v3 ((SYS_SW_VER>>8)&0xff)
#define	_v4 (SYS_SW_VER&0xff)

#if (CYG_BYTEORDER == CYG_LSBFIRST)
const unsigned int sys_sw_version=((_v4<<24)|(_v3<<16)|(_v2<<8)|_v1);
#else
const unsigned int sys_sw_version=SYS_SW_VER;
#endif

const char sys_os_name[]="os";
const char sys_os_ver[]="ver 2.0";
#ifndef CONFIG_MODEL_NAME
const char sys_model_name[]="Ralink-Router";
#else
const char sys_model_name[]=CONFIG_MODEL_NAME;
#endif
#ifndef CONFIG_MANUF_NAME
const char sys_vendor_name[] = "Rtr";
#else
const char sys_vendor_name[] = CONFIG_MANUF_NAME;
#endif
const char sys_model_ver[]=SYS_SW_VER_STR;

const char sys_cust_ver[]=SYS_CUST_VER_STR;

