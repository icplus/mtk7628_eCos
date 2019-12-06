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
	rt_ecos.c

    Abstract:
    Common functions for eCos Platform
 
    Revision History:
    Who          When          What
    --------    ----------      ------------------------------------------
*/

#include "rt_config.h"

#ifdef BRANCH_ADV
#include <eventlog.h>
#endif /* BRANCH_ADV */

ULONG RTDebugLevel = DBG_LVL_ERROR;
ULONG RTDebugFunc = 0;
ULONG DebugCategory = DBG_CAT_ALL;


/*UCHAR   RALINK_OUI[]  = {0x00, 0x0c, 0x43}; */
#ifdef BRANCH_ADV
UCHAR   DbgBuffer[2048];
#define is_digit(c) ((c >= '0') && (c <= '9'))

#ifdef VENDOR_FEATURE4_SUPPORT
ULONG	OS_NumOfMemAlloc = 0, OS_NumOfMemFree = 0;
#endif /* VENDOR_FEATURE4_SUPPORT */
#ifdef VENDOR_FEATURE2_SUPPORT
ULONG	OS_NumOfPktAlloc = 0, OS_NumOfPktFree = 0;
#endif /* VENDOR_FEATURE2_SUPPORT */

static int
_cvt(unsigned long long val, char *buf, long radix, char *digits)
{
    char temp[80];
    char *cp = temp;
    int length = 0;

    if (val == 0) {
        /* Special case */
        *cp++ = '0';
    } else {
        while (val) {
            *cp++ = digits[val % radix];
            val /= radix;
        }
    }
    while (cp != temp) {
        *buf++ = *--cp;
        length++;
    }
    *buf = '\0';
    return (length);
}

static cyg_bool diag_check_string( const char *str )
{
    cyg_bool result = true;
    const char *s;

    if( str == NULL ) return false;
    
    for( s = str ; result && *s ; s++ )
    {
        char c = *s;

        /* Check for a reasonable length string. */
        
        if( s-str > 2048 ) result = false;

        /* We only really support CR, NL, tab and backspace at present.
	 * If we want to use other special chars, this test will
         * have to be expanded.  */

        if( c == '\n' || c == '\r' || c == '\b' || c == '\t' || c == '\033' )
            continue;

        /* Check for printable chars. This assumes ASCII */
        
        if( c < ' ' || c > '~' )
            result = false;

    }

    return result;
}

static int _vprintf(char *DbgBuffer, const char *fmt, va_list ap)
{
    char buf[sizeof(long long)*8];
    char c, sign, *cp=buf;
    int left_prec, right_prec, zero_fill, pad, pad_on_right, 
        i, islong, islonglong;
    long long val = 0;
    int res = 0, length = 0;

    if (!diag_check_string(fmt)) {
        diag_write_string("<Bad format string: ");
        diag_write_hex((cyg_uint32)fmt);
        diag_write_string(" :");
        for( i = 0; i < 8; i++ ) {
            diag_write_char(' ');
            val = va_arg(ap, unsigned long);
            diag_write_hex(val);
        }
        diag_write_string(">\n");
        return 0;
    }
    while ((c = *fmt++) != '\0') {
        if (c == '%') {
            c = *fmt++;
            left_prec = right_prec = pad_on_right = islong = islonglong = 0;
            if (c == '-') {
                c = *fmt++;
                pad_on_right++;
            }
            if (c == '0') {
                zero_fill = true;
                c = *fmt++;
            } else {
                zero_fill = false;
            }
            while (is_digit(c)) {
                left_prec = (left_prec * 10) + (c - '0');
                c = *fmt++;
            }
            if (c == '.') {
                c = *fmt++;
                zero_fill++;
                while (is_digit(c)) {
                    right_prec = (right_prec * 10) + (c - '0');
                    c = *fmt++;
                }
            } else {
                right_prec = left_prec;
            }
            sign = '\0';
            if (c == 'l') {
                /* 'long' qualifier */
                c = *fmt++;
		islong = 1;
                if (c == 'l') {
                    /* long long qualifier */
                    c = *fmt++;
                    islonglong = 1;
                }
            }
            if (c == 'z') {
                c = *fmt++;
		islong = sizeof(size_t) == sizeof(long);
            }
            /* Fetch value [numeric descriptors only] */
            switch (c) {
            case 'p':
		islong = 1;
            case 'd':
            case 'D':
            case 'x':
            case 'X':
            case 'u':
            case 'U':
            case 'b':
            case 'B':
                if (islonglong) {
                    val = va_arg(ap, long long);
	        } else if (islong) {
                    val = (long long)va_arg(ap, long);
		} else{
                    val = (long long)va_arg(ap, int);
                }
                if ((c == 'd') || (c == 'D')) {
                    if (val < 0) {
                        sign = '-';
                        val = -val;
                    }
                } else {
                    /* Mask to unsigned, sized quantity */
                    if (islong) {
                        val &= ((long long)1 << (sizeof(long) * 8)) - 1;
                    } else if (!islonglong) { /* no need to mask longlong */
                        val &= ((long long)1 << (sizeof(int) * 8)) - 1;
                    }
                }
                break;
            default:
                break;
            }
            /* Process output */
            switch (c) {
            case 'p':  /* Pointer */
                DbgBuffer[res] = '0';
                DbgBuffer[res+1] = 'x';
                zero_fill = true;
                left_prec = sizeof(unsigned long)*2;
                res += 2;  /* Account for "0x" leadin */
            case 'd':
            case 'D':
            case 'u':
            case 'U':
            case 'x':
            case 'X':
                switch (c) {
                case 'd':
                case 'D':
                case 'u':
                case 'U':
                    length = _cvt(val, buf, 10, (char *)"0123456789");
                    break;
                case 'p':
                case 'x':
                    length = _cvt(val, buf, 16, (char *)"0123456789abcdef");
                    break;
                case 'X':
                    length = _cvt(val, buf, 16, (char *)"0123456789ABCDEF");
                    break;
                }
                cp = buf;
                break;
            case 's':
            case 'S':
                cp = va_arg(ap, char *);
                if (cp == NULL) 
                    cp = (char *)"<null>";
                else if (!diag_check_string(cp)) {
                    diag_write_string("<Not a string: 0x");
                    diag_write_hex((cyg_uint32)cp);
                    cp = (char *)">";
                }
                length = 0;
                while (cp[length] != '\0') length++;
                break;
            case 'c':
            case 'C':
                c = va_arg(ap, int /*char*/);
                DbgBuffer[res] = c;
                res++;
                continue;
            case 'b':
            case 'B':
                length = left_prec;
                if (left_prec == 0) {
                    if (islonglong)
                        length = sizeof(long long)*8;
                    else if (islong)
                        length = sizeof(long)*8;
                    else
                        length = sizeof(int)*8;
                }
                for (i = 0;  i < length-1;  i++) {
                    buf[i] = ((val & ((long long)1<<i)) ? '1' : '.');
                }
                cp = buf;
                break;
            case '%':
                DbgBuffer[res] = '%';
                res++;
                continue;
            default:
                DbgBuffer[res] = '%';
                DbgBuffer[res+1] = c;
                res += 2;
                continue;
            }
            pad = left_prec - length;
            if (sign != '\0') {
                pad--;
            }
            if (zero_fill) {
                c = '0';
                if (sign != '\0') {
                    DbgBuffer[res] = sign;
                    res++;
                    sign = '\0';
                }
            } else {
                c = ' ';
            }
            if (!pad_on_right) {
                while (pad-- > 0) {
                    DbgBuffer[res] = c;
                    res++;
                }
            }
            if (sign != '\0') {
                DbgBuffer[res] = sign;
                res++;
            }
            while (length-- > 0) {
                c = *cp++;
                DbgBuffer[res] = c;
                res++;
            }
            if (pad_on_right) {
                while (pad-- > 0) {
                    DbgBuffer[res] = ' ';
                    res++;
                }
            }
        } else {
            DbgBuffer[res] = c;
            res++;
        }
    }
    return (res);
}

int printUI(const char *fmt, ...)
{
    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    ret = _vprintf(DbgBuffer, fmt, ap);
    va_end(ap);
    DbgBuffer[ret] = '\0';
    if (ret > 0)
        SysLog(LOG_USER|LOG_INFO|LOGM_WLAN, DbgBuffer);	
    return (ret);
}
#endif /* BRANCH_ADV */

/*
	This function copy from target/usr/src/libdl/strsep.c
*/
char *strsep(register char **stringp, register const char *delim)
{
	register char *s;
	register const char *spanp;
	register int c, sc;
	char *tok;

	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}


unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
        unsigned long result = 0,value;

        if (!base) {
                base = 10;
                if (*cp == '0') {
                        base = 8;
                        cp++;
                        if ((*cp == 'x') && isxdigit(cp[1])) {
                                cp++;
                                base = 16;
                        }
                }
        }
        while (isxdigit(*cp) &&
               (value = isdigit(*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < base) {
                result = result*base + value;
                cp++;
        }
        if (endp)
                *endp = (char *)cp;
        return result;
}

long simple_strtol(const char *cp,char **endp,unsigned int base)
{
        if(*cp=='-')
                return -simple_strtoul(cp+1,endp,base);
        return simple_strtoul(cp,endp,base);
}
void hex_dump(char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen)
{
	unsigned char *pt;
	int x;

	if (RTDebugLevel < DBG_LVL_TRACE)
		return;
	
	pt = pSrcBufVA;
	printf("%s: %p, len = %d\n",str,  pSrcBufVA, SrcBufLen);
	for (x=0; x<SrcBufLen; x++)
	{
		if (x % 16 == 0) 
			printf("0x%04x : ", x);
		printf("%02x ", ((unsigned char)pt[x]));
		if (x%16 == 15) printf("\n");
	}
    printf("\n");
}



/***********************************************************************************
 *
 *	Definition of timer and time related functions
 * ***********************************************************************************/ 
/* timeout -- ms */
VOID RTMP_SetPeriodicTimer(
	IN	NDIS_MINIPORT_TIMER *pTimer, 
	IN	unsigned long timeout)
{
	if (pTimer->Valid)
	{
		timeout = (timeout * OS_HZ) / 1000;
		if (timeout > 0) {
		cyg_alarm_initialize(pTimer->alarm_hdl, cyg_current_time() + timeout, timeout);
                        cyg_alarm_enable(pTimer->alarm_hdl);
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("RTMP_SetPeriodicTimer:timeout less then 10ms!\n"));
		}
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("RTMP_SetPeriodicTimer: Timer is not initial!\n"));
	} /* End of if */
}

VOID RTMP_OS_Init_Timer(
	IN	VOID 					*pReserved,
	IN	NDIS_MINIPORT_TIMER		*pTimer, 
	IN	TIMER_FUNCTION			function,
	IN	PVOID					data,
	IN	LIST_HEADER				*pTimerList)
{
	if (!pTimer->Valid)
	{	
		cyg_clock_to_counter(cyg_real_time_clock(), &pTimer->counter_hdl);
		pTimer->data = data;
		cyg_alarm_create(pTimer->counter_hdl,
			function,
 			(cyg_addrword_t) pTimer->data,
			&pTimer->alarm_hdl,
			&pTimer->alarm_obj);        

		if (!pTimer->alarm_hdl)
			DBGPRINT(RT_DEBUG_ERROR, ("init_Timer failed!\n"));
		else
			pTimer->Valid = TRUE;
	}
}

VOID RTMP_OS_Add_Timer(
	IN	NDIS_MINIPORT_TIMER		*pTimer,
	IN	unsigned long timeout)
{
	if ((pTimer->Valid) && (pTimer->alarm_hdl))
	{
	    cyg_alarm_disable(pTimer->alarm_hdl);
		timeout = (timeout * OS_HZ)  / 1000;
		if (timeout > 0) {
		cyg_alarm_initialize(pTimer->alarm_hdl, cyg_current_time() + timeout, 0);
                        cyg_alarm_enable(pTimer->alarm_hdl);
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("RTMP_OS_Add_Timer:timeout less then 10ms!\n"));
		}
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("RTMP_OS_Add_Timer: Timer is not initial!\n"));
	} /* End of if */
}

VOID RTMP_OS_Mod_Timer(
	IN	NDIS_MINIPORT_TIMER		*pTimer,
	IN	unsigned long timeout)
{
	if ((pTimer->Valid) && (pTimer->alarm_hdl))
	{
	    cyg_alarm_disable(pTimer->alarm_hdl);
		timeout = (timeout * OS_HZ) / 1000;
		if (timeout > 0) {
            cyg_alarm_initialize(pTimer->alarm_hdl, cyg_current_time() + timeout, 0);
			cyg_alarm_enable(pTimer->alarm_hdl);
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("RTMP_OS_Mod_Timer: timeout less then 10ms!\n"));
		}
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("RTMP_OS_Mod_Timer: Timer is not initial!\n"));
    } /* End of if */
}

VOID RTMP_OS_Del_Timer(
	IN	NDIS_MINIPORT_TIMER		*pTimer,
	OUT	BOOLEAN					*pCancelled)
{
	if ((pTimer->Valid) && (pTimer->alarm_hdl))
	{
	    cyg_alarm_disable(pTimer->alarm_hdl);
		*pCancelled = TRUE;
	} else {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("CancelTimer failed!\n"));
		*pCancelled = FALSE;
    } /* End of if */
}

VOID RTMP_OS_Release_Timer(
	IN	NDIS_MINIPORT_TIMER		*pTimer)
{
}

/* Unify all delay routine by using udelay */
VOID RtmpusecDelay(
	IN	ULONG	usec)
{
    CYGACC_CALL_IF_DELAY_US((UINT32)usec);
}

VOID RtmpOsMsDelay(
	IN	ULONG	msec)
{
	mdelay(msec);
}

void RTMP_GetCurrentSystemTime(LARGE_INTEGER *NowTime)
{
	NowTime->u.HighPart = 0;
	time((time_t *)&NowTime->u.LowPart);
}
void RTMP_GetCurrentSystemTick(ULONG *pNow)
{
	*pNow = cyg_current_time();
}


/*******************************************************************************

	Flash Read/Write related functions.
	
 *******************************************************************************/
//#ifdef RTMP_RBUS_SUPPORT		//sht:RTMP_FLASH_SUPPORT not limit by RTMP_RBUS_SUPPORT again.
#ifndef CONFIG_RALINK_FLASH_API
void RtmpFlashWrite(UCHAR *p, ULONG offset, ULONG size)
{
	cyg_flashaddr_t addr = (cyg_flashaddr_t)offset;
	int status;
	unsigned long irqFlags=0;
	HAL_DISABLE_INTERRUPTS(irqFlags);
	status = flsh_erase (addr, size);
	HAL_RESTORE_INTERRUPTS(irqFlags);
	if (status) 
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Flash erase error : %s\n", cyg_flash_errmsg (status)));
	HAL_DISABLE_INTERRUPTS(irqFlags);
	status = flsh_write(addr, p, size);
	HAL_RESTORE_INTERRUPTS(irqFlags);
	if (status)
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("FlashWrite error : %s\n", cyg_flash_errmsg (status)));
}


void RtmpFlashRead(UCHAR *p, ULONG a, ULONG b)
{
	cyg_flashaddr_t err_addr;
	cyg_flashaddr_t flsbase = (cyg_flashaddr_t)a;
	size_t len = (size_t)b;
	void *rambase = (void *)p;
	int status;
	
	status = cyg_flash_read(flsbase, rambase, len, &err_addr);
	if (status != CYG_FLASH_ERR_OK)
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("FlashRead error : %s\n", cyg_flash_errmsg (status)));

}
#endif /* CONFIG_RALINK_FLASH_API */
//#endif /* RTMP_RBUS_SUPPORT */


/*******************************************************************************

	Normal memory allocation related functions.
	
 *******************************************************************************/
NDIS_STATUS os_alloc_mem(
	IN	VOID *pAd,
	OUT	UCHAR **mem,
	IN	ULONG  size)
{	
	*mem = (PUCHAR) kmalloc(size, 0);
	if (*mem)
		return (NDIS_STATUS_SUCCESS);
	else
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("malloc failed size = %d\n", (INT) size));
		return (NDIS_STATUS_FAILURE);
	}
}

NDIS_STATUS os_alloc_mem_suspend(
	IN	VOID					*pReserved,
	OUT	UCHAR					**mem,
	IN	ULONG					size)
{
	return os_alloc_mem(pReserved, mem, size);
}


/* pAd MUST allow to be NULL */
NDIS_STATUS os_free_mem(
	IN	VOID *pAd,
	IN	PVOID mem)
{
	
	ASSERT(mem);
	kfree(mem);
	return (NDIS_STATUS_SUCCESS);
}


/*******************************************************************************

	Device IRQ related functions.
	
 *******************************************************************************/
int RtmpOSIRQRequest(IN PNET_DEV pNetDev)
{
	return 0; 
}

int RtmpOSIRQRelease(
	IN	PNET_DEV				pNetDev,
	IN	UINT32					infType,
	IN	PPCI_DEV				pci_dev,
	IN	BOOLEAN					*pHaveMsi)
{
	return 0;
}


/*******************************************************************************

	File open/close related functions.
	
 *******************************************************************************/
/*EddyTODO */
RTMP_OS_FD RtmpOSFileOpen(char *pPath,  int flag, int mode)
{
    return -1;
/*	return open(pPath, flag, mode); */
}

int RtmpOSFileClose(RTMP_OS_FD osfd)
{
    return -1;
/*	return close(osfd); */
}

void RtmpOSFileSeek(RTMP_OS_FD osfd, int offset)
{
/*	lseek(osfd, offset, SEEK_SET); */
}

int RtmpOSFileRead(RTMP_OS_FD osfd, char *pDataPtr, int readLen)
{
    return -1;
/*	return read(osfd, pDataPtr, readLen); */
}

int RtmpOSFileWrite(RTMP_OS_FD osfd, char *pDataPtr, int writeLen)
{
    return -1;
/*	return write(osfd, pDataPtr, writeLen); */
}

void RtmpOSFSInfoChange(RTMP_OS_FS_INFO *pOSFSInfo, BOOLEAN bSet)
{
	return;
}

/*
========================================================================
Routine Description:
	Get wifi_dev from the priv info linked to OS network interface data structure.

Arguments:
	pDev			- the device

Return Value:
	sys_handle

Note:
========================================================================
*/
VOID *RtmpOsGetNetDevPriv(VOID *pDev)
{
	return ((struct mt_dev_priv *) ((PNET_DEV) pDev)->driver_private)->sys_handle;
}

VOID RtmpOsSetNetDevWdev(VOID *pDev, VOID *wdev)
{
	struct mt_dev_priv *priv_info;

	priv_info = (struct mt_dev_priv *) ((PNET_DEV) pDev)->driver_private;
	priv_info->wifi_dev = wdev;
}


VOID *RtmpOsGetNetDevWdev(VOID *pDev)
{
	return ((struct mt_dev_priv *) ((PNET_DEV) pDev)->driver_private)->wifi_dev;
}



/*******************************************************************************

	Task create/management/kill related functions.
	
 *******************************************************************************/
#ifndef NET_TASK_PRIORITY
#define NET_TASK_PRIORITY       4
#endif
NDIS_STATUS RtmpOSTaskAttach(
	IN RTMP_OS_TASK *pTask,
	IN RTMP_OS_TASK_CALLBACK fn, 
	IN ULONG arg)
{
	NDIS_STATUS status;

    cyg_thread_create(NET_TASK_PRIORITY, (FUNCPTR)fn, (cyg_addrword_t) arg,
		    pTask->taskName, (void *) pTask->stack, sizeof(pTask->stack),
		    &pTask->threadID, &pTask->thread_obj);
	if (pTask->threadID == NULL) 
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Attach task(%s) failed!\n", pTask->taskName));
		pTask->taskPID = -1;
		status = NDIS_STATUS_FAILURE;
	}
	else {
		status = NDIS_STATUS_SUCCESS;
		
		pTask->taskComplete.done = 0;
		cyg_semaphore_init(&pTask->taskComplete.wait, 0);
		
		cyg_thread_resume(pTask->threadID);        
    }
	
	return status;
}

NDIS_STATUS RtmpOSTaskInit(
	IN RTMP_OS_TASK *pTask,
	RTMP_STRING *pTaskName,
	VOID			 *pPriv,
	LIST_HEADER				*pTaskList,
	LIST_HEADER				*pSemList)
{
	int len;
	
	ASSERT(pTask);

	NdisZeroMemory((PUCHAR)(pTask), sizeof(RTMP_OS_TASK));

	len = strlen(pTaskName);
	len = len > (RTMP_OS_TASK_NAME_LEN -1) ? (RTMP_OS_TASK_NAME_LEN-1) : len;
	NdisMoveMemory(&pTask->taskName[0], pTaskName, strlen(pTaskName)); 
	
	RTMP_SEM_EVENT_INIT_LOCKED(&(pTask->taskSema), NULL);

	pTask->priv = pPriv;
	
	return NDIS_STATUS_SUCCESS;
}

INT RtmpOSTaskNotifyToExit(
	IN RTMP_OS_TASK *pTask)
{
	pTask->taskComplete.done = 1;
	cyg_semaphore_post(&pTask->taskComplete.wait);
	return 0;
}

void RtmpOSTaskCustomize(
	IN RTMP_OS_TASK *pTask)
{
	return;
}

NDIS_STATUS RtmpOSTaskKill(
	IN RTMP_OS_TASK *pTask)
{
	UINT retVal;
	int cnt = 0;
	
	pTask->task_killed = 1;
	RTMP_OS_TASK_WAKE_UP(&pTask->taskSema);
	retVal = cyg_thread_delete(pTask->threadID);

	do
	{
		cyg_semaphore_trywait(&pTask->taskComplete.wait);
		
		if (pTask->taskComplete.done == 1)
			break;
		else
		{
			cyg_thread_info info;
			cyg_uint16 id;
			cyg_bool_t rt;
			int state = -1;

			id = cyg_thread_get_id(pTask->threadID);
			rt = cyg_thread_get_info(pTask->threadID, id, &info);
			if (rt == 1)
				state = info.state;
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Killed task(%s) donebit=%d, status=%d, round=%d!\n",
				pTask->taskName, pTask->taskComplete.done, state, cnt)); 
			cyg_thread_yield();
		}
		cnt++;
	}while(cnt< 1000);
	
	if (retVal == 0)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Kill task(%s) failed!\n", pTask->taskName)); 
		return NDIS_STATUS_FAILURE;
	}	
	else
	{
	       pTask->task_killed = 0;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Kill task(%s) succeed!\n", pTask->taskName)); 
		return NDIS_STATUS_SUCCESS;
	}
}


/*
	change OS packet DataPtr and DataLen
*/
void  update_os_packet_info(
	IN	PRTMP_ADAPTER	pAd, 
	IN	RX_BLK			*pRxBlk,
	IN  UCHAR			wdev_idx)
{
    PECOS_PKT_BUFFER pkt;
	PNET_DEV	pNetDev;

	ASSERT(pRxBlk->pRxPacket);
	pkt = RTPKT_TO_OSPKT(pRxBlk->pRxPacket);
	if(pkt)
	{
		pNetDev = get_netdev_from_bssid(pAd, wdev_idx);
		SET_OS_PKT_NETDEV(pkt, pNetDev);

		SET_OS_PKT_DATAPTR(pkt, pRxBlk->pData);
		SET_OS_PKT_LEN(pkt, pRxBlk->DataSize);
	}
}


void wlan_802_11_to_802_3_packet(
	IN	PNET_DEV				pNetDev,
	IN	UCHAR					OpMode,
	IN	USHORT					VLAN_VID,
	IN	USHORT					VLAN_Priority,
	IN	PNDIS_PACKET			pRxPacket,
	IN	UCHAR					*pData,
	IN	ULONG					DataSize,
	IN	PUCHAR					pHeader802_3,
	IN	UCHAR					*TPID)
{
    PECOS_PKT_BUFFER pkt;

	ASSERT(pRxPacket);
	ASSERT(pHeader802_3);

	pkt = RTPKT_TO_OSPKT(pRxPacket);

	SET_OS_PKT_NETDEV(pkt, pNetDev);
	SET_OS_PKT_DATAPTR(pkt, pData);
	SET_OS_PKT_LEN(pkt, DataSize);

	/* copy 802.3 header */
#ifdef CONFIG_AP_SUPPORT
	RT_CONFIG_IF_OPMODE_ON_AP(OpMode)
	{
		/* maybe insert VLAN tag to the received packet */
		UCHAR VLAN_Size = 0;
		UCHAR *data_p;

		/* VLAN related */
		if (VLAN_VID != 0)
			VLAN_Size = LENGTH_802_1Q;

		data_p = skb_push(pkt, LENGTH_802_3+VLAN_Size);

		VLAN_8023_Header_Copy(VLAN_VID, VLAN_Priority,
								pHeader802_3, LENGTH_802_3,
								data_p, TPID);
	}
#endif /* CONFIG_AP_SUPPORT */

}

static VOID RTMP_NetPktInput(
	IN PNDIS_PACKET pPacket)
{
	PRTMP_ADAPTER       pAd = NULL;
    PNET_DEV            pNetDev = NULL;
    PECOS_PKT_BUFFER    pBuffer = NULL;
	struct ifnet        *ifp = NULL;
    struct mbuf         *pMBuf = NULL;
    struct ether_header *eh = NULL;
 
	pNetDev = GET_OS_PKT_NETDEV(pPacket);
    if (GET_OS_PKT_TOTAL_LEN(pPacket) < LENGTH_802_3) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: packet size %lu < LENGTH_802_3\n", __FUNCTION__, GET_OS_PKT_TOTAL_LEN(pPacket)));
        return;
    }

	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(pNetDev);
	ifp = &pNetDev->sc_arpcom.ac_if;
    pBuffer = (PECOS_PKT_BUFFER) pPacket;

    pMBuf = pBuffer->pDataMBuf;
    ifp->if_ipackets++;
    pMBuf->m_pkthdr.rcvif = ifp;
    pMBuf->m_data = pBuffer->pDataPtr;
    eh = (struct ether_hdr *) pMBuf->m_data;
    pMBuf->m_data += sizeof(struct ether_header);
    pMBuf->m_pkthdr.len = pBuffer->pktLen - sizeof(struct ether_header);
    pMBuf->m_len = pMBuf->m_pkthdr.len;

    /* Push data into protocol stacks */
    ether_input(ifp, eh, pMBuf);
	MEM_DBG_PKT_FREE_INC(pAd);
    pBuffer->pDataMBuf = NULL;
    RTMPFreeNdisPacket(pAd, pPacket);
}


#ifdef CONFIG_SNIFFER_SUPPORT
VOID RTMP_SnifferPktInput(
	IN PNDIS_PACKET pPacket)
{
	PRTMP_ADAPTER       pAd = NULL;
    PNET_DEV            pNetDev = NULL;
    PECOS_PKT_BUFFER    pBuffer = NULL;
	struct ifnet        *ifp = NULL;
    struct mbuf         *pMBuf = NULL;
    struct ether_header *eh = NULL;
 
	pNetDev = GET_OS_PKT_NETDEV(pPacket);
    if (GET_OS_PKT_TOTAL_LEN(pPacket) < LENGTH_802_3) {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: packet size %lu < LENGTH_802_3\n", __FUNCTION__, GET_OS_PKT_TOTAL_LEN(pPacket)));
        return;
    }

	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(pNetDev);
	ifp = &pNetDev->sc_arpcom.ac_if;
    pBuffer = (PECOS_PKT_BUFFER) pPacket;
	SNI_DBGPRINTF(RT_DEBUG_INFO,"ifp:%s\n",ifp->if_name);
    pMBuf = pBuffer->pDataMBuf;
    ifp->if_ipackets++;
    pMBuf->m_pkthdr.rcvif = ifp;
    pMBuf->m_data = pBuffer->pDataPtr;
    eh = (struct ether_hdr *) pMBuf->m_data;
    //pMBuf->m_data += sizeof(struct ether_header);
    //pMBuf->m_pkthdr.len = pBuffer->pktLen - sizeof(struct ether_header);
    //pMBuf->m_len = pMBuf->m_pkthdr.len;

    /* Push data into protocol stacks */
    ether_input(ifp, eh, pMBuf);
	MEM_DBG_PKT_FREE_INC(pAd);
    pBuffer->pDataMBuf = NULL;
    RTMPFreeNdisPacket(pAd, pPacket);
}
#endif 

 
void announce_802_3_packet(
        IN VOID *pAdSrc,
        IN PNDIS_PACKET	pPacket,
        IN UCHAR OpMode)
{
	PRTMP_ADAPTER	pAd = (PRTMP_ADAPTER)pAdSrc;

	pAd = pAd; /* avoid compile warning */

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	PNET_DEV		pNetDev = GET_OS_PKT_NETDEV(pPacket);

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (MATPktRxNeedConvert(pAd, pNetDev))
			MATEngineRxHandle(pAd, pPacket, 0);
	}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */


	RTMP_NetPktInput(pPacket);
}


VOID IAPP_L2_UpdatePostCtrl(RTMP_ADAPTER *pAd, UINT8 *mac_p, INT wdev_idx)
{
}




static UINT32 RtmpOSWirelessEventTranslate(
	IN UINT32		eventType)
{
	switch(eventType)
	{
		case RT_WLAN_EVENT_CUSTOM:
		case IWEVCUSTOM:
			eventType = IWEVCUSTOM;
			break;

		default:
			printk("Unknown event: %x\n", eventType);
			break;
	}

	return eventType;
}

int RtmpOSWrielessEventSend(
	IN PNET_DEV		pNetDev,
	IN UINT32		eventType,
	IN INT			flags,
	IN PUCHAR		pSrcMac,
	IN PUCHAR		pData,
	IN UINT32		dataLen)
{
	RTMP_ADAPTER *pAd = NULL;


	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(pNetDev);
	/* translate event type */
	eventType = RtmpOSWirelessEventTranslate(eventType);

#ifdef BRANCH_ADV
#ifdef WSC_INCLUDED
        if ((pData!= NULL) 
                && (dataLen > 0) 
                && (flags != RT_WSC_UPNP_EVENT_FLAG)) {
	        SysLog(LOG_USER|LOG_INFO|LOGM_WLAN, pData);	
        }
#endif /* WSC_INCLUDED */
#endif /* BRANCH_ADV */
    
	if (eventType == IWEVCUSTOM)
	{
		POS_COOKIE  pObj;
		pObj = (POS_COOKIE)pAd->OS_Cookie;
		switch(flags)
		{
#ifdef WSC_AP_SUPPORT
			case RT_WSC_UPNP_EVENT_FLAG:
				if (pObj->WscMsgCallBack)
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("%s(RT_WSC_UPNP_EVENT_FLAG)\n", __FUNCTION__));
					pObj->WscMsgCallBack(pData, dataLen);
				}
				break;
#endif /* WSC_AP_SUPPORT */
			default:
				break;
		}
	}
	return 0;
}

void RTMP_IndicateMediaState(	
	IN	PRTMP_ADAPTER		pAd,
	IN  NDIS_MEDIA_STATE	media_state)
{


}

extern INT wscNLSockRecv(char *pData, UINT32 dataLen);

/*EddyTODO */
/*
========================================================================
Routine Description:
	Check if the network interface is up.

Arguments:
	*pDev			- Network Interface

Return Value:
	None

Note:
========================================================================
*/
BOOLEAN RtmpOSNetDevIsUp(VOID *pDev)
{
	PNET_DEV pNetDev = (PNET_DEV)pDev;
	struct ifnet *ifp = &pNetDev->sc_arpcom.ac_if;

	if ((pNetDev == NULL) || !(ifp->if_flags & IFF_UP))
		return FALSE;

	return TRUE;
}

/*
========================================================================
Routine Description:
	Assign sys_handle data pointer (pAd) to the priv info structured linked to
	the OS network interface.

Arguments:
	pDev			- the os net device data structure
	pPriv			- the sys_handle want to assigned

Return Value:
	None

Note:
========================================================================
*/
VOID RtmpOsSetNetDevPriv(VOID *pDev, VOID *pPriv)
{
	struct mt_dev_priv *priv_info = NULL;

	priv_info = (struct mt_dev_priv *) ((PNET_DEV) pDev)->driver_private;

	priv_info->sys_handle = (VOID *)pPriv;
	priv_info->priv_flags = 0;
}


/*
========================================================================
Routine Description:
	Get private flags from the network interface.

Arguments:
	pDev			- the device

Return Value:
	pPriv			- the pointer

Note:
========================================================================
*/
VOID RtmpDevPrivFlagsSet(VOID *pDev, USHORT PrivFlags)
{
	struct mt_dev_priv *priv_info;

	priv_info = (struct mt_dev_priv *) ((PNET_DEV) pDev)->driver_private;
	priv_info->priv_flags = PrivFlags;
}

/*
========================================================================
Routine Description:
	Get network interface name.

Arguments:
	pDev			- the device

Return Value:
	the name

Note:
========================================================================
*/
char *RtmpOsGetNetDevName(VOID *pDev)
{
	return ((PNET_DEV) pDev)->dev_name;
}

/*
  *	Assign the network dev name for created Ralink WiFi interface.
  */
static int RtmpOSNetDevRequestName(
	IN RTMP_ADAPTER *pAd, 
	IN RTMP_STRING *pPrefixStr, 
	IN INT	devIdx,
	OUT INT	*avilNum)
{
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Eddy Notice\n", __FUNCTION__));
    return 0;
}


void RtmpOSNetDevClose(
	IN PNET_DEV pNetDev)
{
}


void RtmpOSNetDevFree(PNET_DEV pNetDev)
{
}

int RtmpOSNetDevAddrSet(
	IN UCHAR					OpMode,
	IN PNET_DEV 				pNetDev,
	IN PUCHAR					pMacAddr,
	IN PUCHAR					dev_name)
{
    if (NULL != pMacAddr)
        bcopy(pMacAddr, &pNetDev->sc_arpcom.ac_enaddr, MAC_ADDR_LEN);

	return 0;
}

/*EddyTODO */
void RtmpOSNetDevDetach(
	IN PNET_DEV pNetDev)
{
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Eddy Notice\n", __FUNCTION__));
}
int RtmpOSNetDevAttach(
	IN UCHAR					OpMode,
	IN PNET_DEV					pNetDev, 
	IN RTMP_OS_NETDEV_OP_HOOK	*pDevOpHook)
{
	RtmpOSNetDevAddrSet(OpMode, pNetDev, &pDevOpHook->devAddr[0], NULL);
	ether_ifsetmac(&pNetDev->sc_arpcom.ac_if);
	return NDIS_STATUS_SUCCESS;
}

/*========================================================================
Routine Description:
	Get private flags from the network interface.

Arguments:
	pDev			- the device

Return Value:
	pPriv			- the pointer

Note:
========================================================================
*/
USHORT RtmpDevPrivFlagsGet(VOID *pDev)
{
	return ((struct mt_dev_priv *) ((PNET_DEV) pDev)->driver_private)->priv_flags;
}

VOID	RTMPFreeAdapter(
	IN	VOID		*pAdSrc)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;
	POS_COOKIE os_cookie;

/*1. fix wdev_bcn_buf_init but doesn't wdev_bcn_buf_deinit*/
/*this is linux os bug fix, comment first,when SQC meet memory leakage, will check this point*/

#ifdef MT_MAC
#ifdef CONFIG_AP_SUPPORT
		if ((pAd->chipCap.hif_type == HIF_MT) && (pAd->OpMode == OPMODE_AP))
		{
			BSS_STRUCT *pMbss;
			pMbss = &pAd->ApCfg.MBSSID[MAIN_MBSSID];
			ASSERT(pMbss);
			if (pMbss) {
				wdev_bcn_buf_deinit(pAd, &pMbss->bcn_buf);
			} else {
				DBGPRINT(RT_DEBUG_ERROR, ("%s():func_dev is NULL!\n", __FUNCTION__));
				return;
			}
		}
#endif
#endif


	//todo: check eCos free SpinLock if meet memory leakage.
	
	
	os_cookie=(POS_COOKIE)pAd->OS_Cookie;
	kfree(pAd);
	kfree(os_cookie);
}


#define END_OBJ VOID    
END_OBJ *RtmpVirtualENDDevCreate(
	IN char *initString, 
	IN void *pBSP)
{
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Eddy Notice\n", __FUNCTION__));
    return NULL;
}
#ifdef CONFIG_SNIFFER_SUPPORT

UCHAR get_sniffer_mode(VOID *pDev)
{
	struct mt_dev_priv *priv_info;

	priv_info = (struct mt_dev_priv *) ((PNET_DEV) pDev)->driver_private;
	return priv_info->sniffer_mode;
}

VOID set_sniffer_mode(VOID *net_dev, UCHAR sniffer_mode)
{
	struct mt_dev_priv *priv_info;

	priv_info = (struct mt_dev_priv *) ((PNET_DEV) net_dev)->driver_private;
	priv_info->sniffer_mode = sniffer_mode;
}
#endif





#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
END_OBJ *RtmpApCliENDDevLoad(
	IN char *initString, 
	IN void *pBSP)
{

	ASSERT(initString);
	
	/*  First pass, we just return the device name for function "muxDevLoad()" to check if the device exists */
	if (initString[0] == '\0') {
		bcopy(INF_APCLI_DEV_NAME, initString, strlen(INF_APCLI_DEV_NAME));
		printf("%s():Into phase 1, return initSting=%s!\n", __FUNCTION__, initString);
		return NULL;
	}

	/*   Now we are phase 2, now do END_OBJ related initialization */
	return RtmpVirtualENDDevCreate(initString, pBSP);
}
#endif /* APCLI_SUPPORT */


#ifdef WDS_SUPPORT
END_OBJ *RtmpWDSENDDevLoad(
	IN char *initString, 
	IN void *pBSP)
{
	ASSERT(initString);
	
	/*  First pass, we just return the device name for function "muxDevLoad()" to check if the device exists */
	if (initString[0] == '\0') {
		bcopy(INF_WDS_DEV_NAME, initString, strlen(INF_WDS_DEV_NAME));
		printf("%s():Into phase 1, return initSting=%s!\n", __FUNCTION__, initString);
		return NULL;
	}

	/*   Now we are phase 2, now do END_OBJ related initialization */
	return RtmpVirtualENDDevCreate(initString, pBSP);
}
#endif /* WDS_SUPPORT */


#ifdef MBSS_SUPPORT
END_OBJ *RtmpMBSSENDDevLoad(
	IN char *pInitString, 
	IN void *pBSP)
{	
	ASSERT(pInitString);
	
	/*  First pass, we just return the device name for function "muxDevLoad()" to check if the device exists */
	if (pInitString[0] == '\0') {
		bcopy(INF_MAIN_DEV_NAME, pInitString, strlen(INF_MAIN_DEV_NAME));
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():Into phase 1, return pInitString=%s!\n", __FUNCTION__, pInitString));
		return NULL;
	}

	/*   Now we are phase 2, now do END_OBJ related initialization */
	return RtmpVirtualENDDevCreate(pInitString, pBSP);
}
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */


PNET_DEV RtmpOSNetDevCreate(
	IN INT32					MC_RowID,
	IN UINT32					*pIoctlIF,
	IN INT 						devType,
	IN INT						devNum,
	IN INT						privMemSize,
	IN RTMP_STRING *pNamePrefix)
{
    cyg_netdevtab_entry_t *pNetdevEntry;
    struct eth_drv_sc *sc;
	char				devName[IFNAMSIZ];

    sprintf(devName, "%s%d", pNamePrefix, devNum);
    for (pNetdevEntry = &__NETDEVTAB__[0]; pNetdevEntry != &__NETDEVTAB_END__; pNetdevEntry++)
    {
        sc = (struct eth_drv_sc *)pNetdevEntry->device_instance;
        if (strcmp(sc->dev_name, devName) == 0)
            return sc;
	}
		return NULL;
}



//#ifdef CONFIG_AP_SUPPORT
UCHAR VLAN_8023_Header_Copy(
	IN	USHORT VLAN_VID,
	IN	USHORT VLAN_Priority,
	IN	UCHAR *pHeader802_3,
	IN	UINT HdrLen,
	OUT UCHAR *pData,
	IN	UCHAR *TPID)
{
	UINT16 TCI;
	UCHAR VLAN_Size = 0;


	if (VLAN_VID != 0)
	{
		/* need to insert VLAN tag */
		VLAN_Size = LENGTH_802_1Q;

		/* make up TCI field */
		TCI = (VLAN_VID & 0x0fff) | ((VLAN_Priority & 0x7)<<13);

#ifndef RT_BIG_ENDIAN
		TCI = SWAP16(TCI);
#endif /* RT_BIG_ENDIAN */

		/* copy dst + src MAC (12B) */
		memcpy(pData, pHeader802_3, LENGTH_802_3_NO_TYPE);

		/* copy VLAN tag (4B) */
		/* do NOT use memcpy to speed up */
		*(UINT16 *)(pData+LENGTH_802_3_NO_TYPE) = *(UINT16 *)TPID;
		*(UINT16 *)(pData+LENGTH_802_3_NO_TYPE+2) = TCI;

		/* copy type/len (2B) */
		*(UINT16 *)(pData+LENGTH_802_3_NO_TYPE+LENGTH_802_1Q) = \
				*(UINT16 *)&pHeader802_3[LENGTH_802_3-LENGTH_802_3_TYPE];

		/* copy tail if exist */
		if (HdrLen > LENGTH_802_3)
		{
			memcpy(pData+LENGTH_802_3+LENGTH_802_1Q,
					pHeader802_3+LENGTH_802_3,
					HdrLen - LENGTH_802_3);
		}
	}
	else
	{
		/* no VLAN tag is needed to insert */
		memcpy(pData, pHeader802_3, HdrLen);
	}

	return VLAN_Size;
}
//#endif /* CONFIG_AP_SUPPORT */


#ifdef IAPP_SUPPORT
/* Layer 2 Update frame to switch/bridge */
/* For any Layer2 devices, e.g., bridges, switches and other APs, the frame
   can update their forwarding tables with the correct port to reach the new
   location of the STA */
typedef struct GNU_PACKED _RT_IAPP_L2_UPDATE_FRAME {

    UCHAR   DA[ETH_ALEN]; /* broadcast MAC address */
    UCHAR   SA[ETH_ALEN]; /* the MAC address of the STA that has just associated or reassociated */
    USHORT  Len;          /* 8 octets */
    UCHAR   DSAP;         /* null */
    UCHAR   SSAP;         /* null */
    UCHAR   Control;      /* reference to IEEE Std 802.2 */
    UCHAR   XIDInfo[3];   /* reference to IEEE Std 802.2 */
} RT_IAPP_L2_UPDATE_FRAME, *PRT_IAPP_L2_UPDATE_FRAME;

PNDIS_PACKET RtmpOsPktIappMakeUp(
	IN	PNET_DEV				pNetDev,
	IN	UINT8					*pMac)
{
	RTMP_ADAPTER *pAd = NULL;
	RT_IAPP_L2_UPDATE_FRAME  frame_body;
	INT size = sizeof(RT_IAPP_L2_UPDATE_FRAME);
	PNDIS_PACKET	pNetBuf;

	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(pNetDev);
	pNetBuf = RtmpOSNetPktAlloc(pAd, size);
	if (!pNetBuf)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Error! Can't allocate a skb.\n"));
		return NULL;
	}
	
	/* init the update frame body */
	NdisZeroMemory(&frame_body, size);

	memset(frame_body.DA, 0xFF, ETH_ALEN);
	memcpy(frame_body.SA, pMac, ETH_ALEN);

	frame_body.Len      = OS_HTONS(ETH_ALEN);
	frame_body.DSAP     = 0;
	frame_body.SSAP     = 0x01;
	frame_body.Control  = 0xAF;

	frame_body.XIDInfo[0] = 0x81;
	frame_body.XIDInfo[1] = 1;
	frame_body.XIDInfo[2] = 1 << 1;

	SET_OS_PKT_NETDEV(pNetBuf, pNetDev);
    skb_reserve(pNetBuf, 2);
    memcpy(skb_put(pNetBuf, size), &frame_body, size);
	return pNetBuf;
}
#endif /* IAPP_SUPPORT */


VOID RtmpOsPktInit(
	IN	PNDIS_PACKET		pNetPkt,
	IN	PNET_DEV			pNetDev,
	IN	UCHAR				*pData,
	IN	USHORT				DataSize)
{
	PNDIS_PACKET	pRxPkt;


	pRxPkt = RTPKT_TO_OSPKT(pNetPkt);

	SET_OS_PKT_NETDEV(pRxPkt, pNetDev);
	SET_OS_PKT_DATAPTR(pRxPkt, pData);
	SET_OS_PKT_LEN(pRxPkt, DataSize);
	SET_OS_PKT_DATATAIL(pRxPkt, pData, DataSize);
}

VOID RtmpUtilInit(VOID)
{
}


BOOLEAN RtmpOsStatsAlloc(
	IN	VOID					**ppStats,
	IN	VOID					**ppIwStats)
{
	return TRUE;
}


BOOLEAN RtmpOSTaskWait(
	IN	VOID					*pReserved,
	IN	RTMP_OS_TASK			*pTaskOrg,
	IN	INT32					*pStatus)
{
	RTMP_OS_TASK_SEM_EVENT_WAIT(&(pTaskOrg->taskSema), *pStatus);
	if (*pStatus == 0)
		return TRUE;
	return FALSE;
}


BOOLEAN RTMPL2FrameTxAction(
	IN  VOID				*pCtrlBkPtr,
	IN	PNET_DEV			pNetDev,
	IN	RTMP_CB_8023_PACKET_ANNOUNCE _announce_802_3_packet,
	IN	UCHAR				apidx,
	IN	PUCHAR				pData,
	IN	UINT32				data_len,
	IN	UCHAR			OpMode)
{
	PNDIS_PACKET	pPacket;
	NDIS_STATUS  	Status = NDIS_STATUS_SUCCESS;

	Status = RTMPAllocateNdisPacket(pCtrlBkPtr, &pPacket, NULL, 0, NULL, data_len+2);
	if (Status != NDIS_STATUS_SUCCESS)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s (error:: can't allocate NDIS PACKET)\n", __FUNCTION__));
		return FALSE;
	}

	MEM_DBG_PKT_ALLOC_INC(pAd);
   	SET_OS_PKT_NETDEV(pPacket, pNetDev);

	skb_reserve(GET_OS_PKT_TYPE(pPacket), 2);	/* 16 byte align the IP header */
	NdisMoveMemory(GET_OS_PKT_DATAPTR(pPacket), pData, data_len);

	skb_put(GET_OS_PKT_TYPE(pPacket), data_len);

/*	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Notify 8021.x daemon to trigger EAP-SM for this sta(%02x:%02x:%02x:%02x:%02x:%02x)\n", PRINT_MAC(pEntry->Addr))); */

	_announce_802_3_packet(pCtrlBkPtr, pPacket, OpMode);
	return TRUE;
}

INT RTMP_AP_IoctlPrepare(
	IN	RTMP_ADAPTER			*pAd,
	IN	VOID					*pCB)
{
	return 0;
}

VOID RtmpOsTaskPidInit(
	IN	RTMP_OS_PID				*pPid)
{
	*pPid = THREAD_PID_INIT_VALUE;
}

VOID RtmpOsTaskWakeUp(
	IN	RTMP_OS_TASK			*pTask)
{
#ifdef KTHREAD_SUPPORT
	WAKE_UP(pTask);
#else
	RTMP_OS_TASK_WAKE_UP(&pTask->taskSema);
#endif
}

VOID RtmpOsSendWirelessEvent(
	IN	VOID			*pAd,
	IN	USHORT			Event_flag,
	IN	PUCHAR 			pAddr,
	IN	UCHAR			wdev_idx,
	IN	CHAR			Rssi,
	IN	RTMP_OS_SEND_WLAN_EVENT		pFunc)
{
#ifdef WSC_INCLUDED
	if (IW_WSC_STATUS_SUCCESS == Event_flag)	
		wsc_set_acl_list(pAd,wdev_idx);
#endif	
	pFunc(pAd, Event_flag, pAddr, wdev_idx, Rssi);
}


/*
========================================================================
Routine Description:
	Enable or disable wireless event sent.

Arguments:
	pReserved		- Reserved
	FlgIsWEntSup	- TRUE or FALSE

Return Value:
	None

Note:
========================================================================
*/
VOID RtmpOsWlanEventSet(
	IN	VOID					*pReserved,
	IN	BOOLEAN					*pCfgWEnt,
	IN	BOOLEAN					FlgIsWEntSup)
{
	*pCfgWEnt = FlgIsWEntSup;
}


#ifdef WDS_SUPPORT
VOID AP_WDS_KeyNameMakeUp(
	IN	RTMP_STRING *pKey,
	IN	UINT32						KeyMaxSize,
	IN	INT							KeyId)
{
	sprintf(pKey, "Wds%dKey", KeyId+1);
}


NET_DEV_STATS *RT28xx_get_wds_ether_stats(
    IN PNET_DEV *net_dev)
{
    return NULL;
}
#endif /* WDS_SUPPORT */


#ifdef CONFIG_AP_SUPPORT
VOID AP_E2PROM_IOCTL_PostCtrl(
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq,
	IN	RTMP_STRING *msg)
{
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("%s\n", msg));	
}
#endif /* CONFIG_AP_SUPPORT */
VOID RtmpDrvAllMacPrint(
	IN VOID *pReserved,
	IN UINT32 *pBufMac,
	IN UINT32 AddrStart,
	IN UINT32 AddrEnd,
	IN UINT32 AddrStep)
{
}

VOID RtmpDrvAllE2PPrint(
	IN VOID *pReserved,
	IN USHORT *pMacContent,
	IN UINT32 AddrEnd,
	IN UINT32 AddrStep)
{
}

VOID RtmpDrvAllRFPrint(
	IN VOID *pReserved,
	IN UCHAR *pBuf,
	IN UINT32 BufLen)
{
	int i=0;
	for(i=0; i<BufLen; i++)
		diag_printf("%c",pBuf[i]);
}


/*
========================================================================
Routine Description:
	Wake up the command thread.

Arguments:
	pAd				- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
VOID RtmpOsCmdUp(
	IN RTMP_OS_TASK			*pCmdQTask)
{
#ifdef KTHREAD_SUPPORT
	do{
		OS_TASK	*pTask = RTMP_OS_TASK_GET(pCmdQTask);
		{
			pTask->kthread_running = TRUE;
	        wake_up(&pTask->kthread_q);
		}
	}while(0);
#else
	do{
		OS_TASK	*pTask = RTMP_OS_TASK_GET(pCmdQTask);
		CHECK_PID_LEGALITY(pTask->taskPID)
		{
			RTMP_OS_TASK_WAKE_UP(&(pTask->taskSema));
		}
	}while(0);
#endif /* KTHREAD_SUPPORT */
}

/*
========================================================================
Routine Description:
	Wake up USB Mlme thread.

Arguments:
	pAd				- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
VOID RtmpOsMlmeUp(IN RTMP_OS_TASK *pMlmeQTask)		//todo:check and review mlme up
{
	OS_TASK *pTask = RTMP_OS_TASK_GET(pMlmeQTask);


	if (pTask != NULL) {
			RTMP_OS_TASK_WAKE_UP(&(pTask->taskSema));
		}
	
}


void OS_SPIN_LOCK_IRQSAVE(NDIS_SPIN_LOCK *lock, unsigned long *flags)
{

	//cyg_semaphore_wait(lock);
	HAL_DISABLE_INTERRUPTS(*flags);
	//spin_lock_irqsave((spinlock_t *)(lock), *flags);
}

void OS_SPIN_UNLOCK_IRQRESTORE(NDIS_SPIN_LOCK *lock, unsigned long *flags)
{
	//cyg_semaphore_post(lock);
	HAL_RESTORE_INTERRUPTS(*flags);  
}

void OS_SPIN_LOCK(NDIS_SPIN_LOCK *lock)
{
	if(in_interrupt())				
	{	
		diag_printf("\ncyg_semaphore_wait call in DSR! file:%s line:%d\n",__FILE__,__LINE__);						
		diag_printf("\nERROR! pls check.\n");						
	}	
	
		cyg_semaphore_wait(lock);

}

void OS_SPIN_UNLOCK(NDIS_SPIN_LOCK *lock)
{
		cyg_semaphore_post(lock);

		
}

void OS_SPIN_LOCK_IRQ(NDIS_SPIN_LOCK *lock)
{
	//cyg_semaphore_wait(lock);
	cyg_scheduler_lock();	
}

void OS_SPIN_UNLOCK_IRQ(NDIS_SPIN_LOCK *lock)
{
	//cyg_semaphore_post(lock);
	cyg_scheduler_unlock();	
}

int OS_TEST_BIT(int bit, unsigned long *flags)
{
	
	int	mask;
	
	if (bit >31)
		diag_printf("%s:error !!!!\n",__FUNCTION__);
	mask = 1 << (bit);
	return ((mask & *flags) != 0);
}

void OS_SET_BIT(int bit, unsigned long *flags)
{
	int	mask;

	if (bit >31)
		diag_printf("%s:error !!!!\n",__FUNCTION__);
	mask = 1 << (bit);
	*flags |= mask;
	
}

void OS_CLEAR_BIT(int bit, unsigned long *flags)
{
	int	mask;
	
	if (bit >31)
		diag_printf("%s:error !!!!\n",__FUNCTION__);
	mask = 1 << (bit);
	*flags &= (~mask);

}

void init_completion(RTMP_OS_COMPLETION *comp )
{
	comp->done = 0;
	cyg_semaphore_init(&comp->wait,0);
}

void exit_completion(RTMP_OS_COMPLETION *comp )
{
	comp->done = 1;
	cyg_semaphore_destroy(&comp->wait);
}

void complete(RTMP_OS_COMPLETION *comp )
{
	comp->done =1;
	cyg_semaphore_post(&comp->wait);
	
}
void wait_for_completion_timeout(RTMP_OS_COMPLETION* comp,ULONG abstime )
{
	//todo check: call in DSR.
	cyg_semaphore_timed_wait(&comp->wait, abstime+cyg_current_time());
}



ULONG RTMPMsecsToJiffies(UINT32 m)
{
	if (m<(1000/OS_HZ))
		return 1;
	return (m/(1000/ OS_HZ));
}
void OS_LOAD_CODE_FROM_BIN(unsigned char **image, char *bin_name, void *inf_dev, UINT32 *code_len)
{
//note:eCos not support load FW via bin method,only support header method.
}

void RtmpOSNetDevProtect(BOOLEAN lock_it)
{
	if (lock_it)
		cyg_scheduler_lock();	//rtnl_lock();
	else
		cyg_scheduler_unlock();	//rtnl_unlock();

}

#ifdef WSC_INCLUDED
void wsc_set_acl_list(void *pAd_ptr,UCHAR apidx)
{
	char tmp_buf[512+50];
	char old_cfg[512+50];
	char tmp[50];
	int i =0;
	PRT_802_11_ACL	pACL = NULL;
	int id =0;
	char null_mac[]={0,0,0,0,0,0};
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)pAd_ptr;

	memset(tmp_buf,0,sizeof(tmp_buf));
	memset(old_cfg,0,sizeof(old_cfg));
	
	pACL = &pAd->ApCfg.MBSSID[apidx].AccessControlList;
	
	
	if ((pACL->Policy == 0) ||
		(pACL->Policy == 2))
		return;

	for (i=0; i<pACL->Num; i++)
	{
	if (NdisEqualMemory(pACL->Entry[i].Addr, null_mac, MAC_ADDR_LEN))
		continue;
	    memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%02x:%02x:%02x:%02x:%02x:%02x;",pACL->Entry[i].Addr[0],pACL->Entry[i].Addr[1],
			pACL->Entry[i].Addr[2],pACL->Entry[i].Addr[3],pACL->Entry[i].Addr[4],pACL->Entry[i].Addr[5]);
		if (strlen(tmp) > (sizeof(tmp_buf) - strlen(tmp_buf)))
			{
			diag_printf("%s:%d ERROR tmp_buf over flow\n\n",__FUNCTION__,__LINE__);
			panic("panic down\n");
			return ;
			}		
		strcat(tmp_buf,tmp);
	}

	id = CFG_str2id("WLN_AccessControlList0");
	CFG_get_str(id,old_cfg);
	if (memcmp(old_cfg,tmp_buf,strlen(tmp_buf)))
		{
		CFG_set_str( id,tmp_buf);
		
		}
	
	CFG_get_str(id,old_cfg);
	diag_printf("%s:AccessControlList config:%s\n",__FUNCTION__,old_cfg);

	

}
#endif

UINT RtmpOsWirelessExtVerGet(VOID)
{
	return WIRELESS_EXT;
}

