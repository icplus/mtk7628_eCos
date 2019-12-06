#include <ppp_api.h>
#include <pppd.h>


extern PPP_IF_VAR_T *ppp_if_var;


#if 0
#define ppp_task_priority    50
/**********************************/
/* Data sturcture definition      */
/**********************************/

/*--------------------------------------------------------------
* ROUTINE NAME - ppp_ParseIfname
*---------------------------------------------------------------
* FUNCTION: parse interface name  
*
* INPUT:    None 
* RETURN: unit number or ERROR
---------------------------------------------------------------*/
int ppp_ParseIfname(char *ifname, char *pDevname, short *pUnit)
{
    char *cp;
    short  unit;
	char *ep, c;
    char *np;

	for (cp = ifname, np = pDevname; cp < ifname + IFNAMSIZ && *cp; cp++, np++)
    {
		if (*cp >= '0' && *cp <= '9')
        {
            *np = 0;
			break;
        }
        else
            *np = *cp;
    }
	if (*cp == '\0' || cp == ifname + IFNAMSIZ)
		return -1;
	/*
	 * Save first char of unit, and pointer to it,
	 * so we can put a null there to avoid matching
	 * initial substrings of interface names.
	 */
	c = *cp;
	ep = cp;
	for (unit = 0; *cp >= '0' && *cp <= '9'; )
		unit = unit * 10 + *cp++ - '0';
    *pUnit = unit;
    return 0;
}
#endif

/*--------------------------------------------------------------
* ROUTINE NAME - ppp_IfAdd
*---------------------------------------------------------------
* FUNCTION: Adds an PPP interface to ppp_if_var.  
*
* INPUT:    None 
* RETURN: unit number or ERROR
---------------------------------------------------------------*/
int ppp_IfAdd(PPP_IF_VAR_T *pPppIf)
{
    PPP_IF_VAR_T **ppIf = &ppp_if_var;
	
    cyg_scheduler_lock();
	
    while (*ppIf)
    {
		// Simon, for multiple pppoe
        if (strcmp(pPppIf->pppname, (*ppIf)->pppname)== 0)
        {
            diag_printf("\n\rppp_IfAdd: Re-Add ppp interface %s", pPppIf->ifname);
            cyg_scheduler_unlock();
            return ERROR;
        }
        ppIf = &((*ppIf)->ppp_next);
    }
    
    *ppIf = pPppIf;

    cyg_scheduler_unlock();
   
    return OK;
}


/*--------------------------------------------------------------
* ROUTINE NAME - ppp_IfDelete
*---------------------------------------------------------------
* FUNCTION: Deletes an PPP interface from ppp_if_var.  
*
* INPUT:    None 
---------------------------------------------------------------*/
int ppp_IfDelete(PPP_IF_VAR_T *pPppIf)
{
    PPP_IF_VAR_T **ppIf = &ppp_if_var;
	
    if(!(*ppIf))
    {
		diag_printf("ppp_IfDelete failure!\n");
        return;
    }	
    cyg_scheduler_lock();
    
    while(*ppIf != pPppIf)
    {
        ppIf = &((*ppIf)->ppp_next);
    }
	
    *ppIf = (*ppIf)->ppp_next;
            
    cyg_scheduler_unlock();
    
    return OK;
}

#if 0
/*--------------------------------------------------------------
* ROUTINE NAME - ppp_Stop
*---------------------------------------------------------------
* FUNCTION: This function is called by device interfce to 
*           delete ppp task. Schedule a HANGUP evnet to PPP task 
*           () to free resource 
*
* INPUT:    None 
---------------------------------------------------------------*/
void ppp_Stop(PPP_IF_VAR_T *pPppIf)
{
    pppSendHungupEvent(pPppIf);
}
#endif


