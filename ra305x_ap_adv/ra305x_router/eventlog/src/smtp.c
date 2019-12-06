/***************************************************************************
 *  Functions in this file :
 *    int ConnectSMTPsrv()
 *    int SMTP_CMD(char c, int err)
 *    int CMD_HELO()
 *    int CMD_MAILFROM()
 *    int CMD_RCPTTO()
 *    int CMD_DATA()
 *    int SendData(char *szBuffer)
 *    int SendFinish()
 *    int CMD_QUIT()
 *    int SMTPRecv(char *pReply)
 *    int CloseSMTPSocket()
 ****************************************************************************/
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <network.h>
#include <cyg/kernel/kapi.h>
#include <cfg_def.h>
#include <smtp.h>
#include <cfg_net.h>
#include <sys_status.h>
#include <eventlog.h>
#include <time.h>


#define TRUE 		1
#define FALSE 		0
  
static char	Buffer[1024];
static char cBuffer[1024];
int		nSMTPSockFd;
  
//int 	unknown_rcpt = 0;

extern	struct eventlog_cfg *evcfg;
extern int log_mail_addr_num;
extern int log_pool_num;


/*
 * ConnectSMTPsrv - "dispatch" type mail sharing, SMTP connecting local mail server
 * parameters : none
 * return : n - socket number
 *          ERR_SMTP_SOCKET_CREATE - error occurrence on socket()
 *          ERR_SMTP_CONNECT - error occurrence on connect()
 *          ERR_SMTP_RECV - local mail server connection failed
 */

int ConnectSMTPsrv()
{
    int  nEcho = 0;
    struct sockaddr_in SMTPSock ;
	
    memset((char *)&SMTPSock, 0, sizeof(SMTPSock));
    SMTPSock.sin_family = AF_INET;
    SMTPSock.sin_addr.s_addr = inet_addr(evcfg->Local_Server_IP);
    SMTPSock.sin_port = htons(25);
    nSMTPSockFd = socket(AF_INET, SOCK_STREAM, 0);
  
    if (nSMTPSockFd < 0)
    {
    	diag_printf("SMTP socket create error.\n");
        return ERR_SMTP_SOCKET_CREATE;
    }
	
    if (connect(nSMTPSockFd, (struct sockaddr *)&SMTPSock, sizeof(SMTPSock)) < 0)
    {
    	diag_printf("SMTP connect error.\n");
        close(nSMTPSockFd);    	//muset close
        return ERR_SMTP_CONNECT;
    }
	
    nEcho = SMTPRecv(Buffer);
    if (!nEcho)
    {
    	diag_printf("SMTP SMTPRecv error.\n");
    	close(nSMTPSockFd);     //muset close
        return ERR_SMTP_RECV;
    }
	
    return nSMTPSockFd;
}

/*
 * SMTP_CMD - "dispatch" type mail sharing, send SMTP message to local mail server
 * parameters : c - anticipant answer
 *              err - given error message
 * return : n - given error message, SMTP message is not acceptable
 *          TRUE - SMTP message is acceptable
 *          ERR_SMTP_SEND - error occurrence on send()
 *          ERR_SMTP_RECV - error occurrence on receiving local mail server SMTP message
 */

int SMTP_CMD(char c, int err)
{
    int nEcho;
	
    nEcho = send(nSMTPSockFd, Buffer, strlen(Buffer), 0);
    if (nEcho < 0)
    {
        return ERR_SMTP_SEND;
    }
	
    nEcho = SMTPRecv(Buffer);
    if (!nEcho)
    {
        return ERR_SMTP_RECV;
    }
	
    if ((err == ERR_SMTP_COMMAND_RCPTTO) && (strncmp(Buffer, "550", 3) == 0))
    {
        //unknown_rcpt = 1;
        return TRUE;
    }
	
    if (Buffer[0] != c)
    {
        return err;
    }
	
    return TRUE;
}

/*
 * CMD_HELO - "dispatch" type mail sharing, send "HELO" message to local mail server
 * parameters : none
 * return : TRUE - "HELO" message is acceptable
 *          ERR_SMTP_COMMAND_HELO - "HELO" message is not acceptable
 *          ERR_SMTP_SEND - error occurrence on send()
 *          ERR_SMTP_RECV - error occurrence on receiving local mail server SMTP message
 */
  
int CMD_HELO()
{
	if (evcfg->sysDomain[0]==0)
		sprintf(Buffer,"HELO %s\r\n", evcfg->sysName);
	else
		sprintf(Buffer,"HELO %s.%s\r\n", evcfg->sysName, evcfg->sysDomain);
	
	return SMTP_CMD('2', ERR_SMTP_COMMAND_HELO);
}
  
/*
 * CMD_MAILFROM - "dispatch" type mail sharing, send "MAIL FROM" message to local mail server
 * parameters : none
 * return : TRUE - "MAIL FROM" message is acceptable
 *          ERR_SMTP_COMMAND_MAILFROM - "MAIL FROM" message is not acceptable
 *          ERR_SMTP_SEND - error occurrence on send()
 *          ERR_SMTP_RECV - error occurrence on receiving local mail server SMTP message
 */

int CMD_MAILFROM()
{
    //sprintf(Buffer, "MAIL FROM: <%s>\r\n", FromTo.szFrom);
    sprintf(Buffer, "MAIL FROM: <%s>\r\n", evcfg->MailTo[0]);
    return SMTP_CMD('2', ERR_SMTP_COMMAND_MAILFROM);
}

/*
 * CMD_RCPTTO - "dispatch" type mail sharing, send "RCPT TO" message to local mail server
 * parameters : none
 * return : TRUE - "RCPT TO" message is acceptable
 *          ERR_SMTP_COMMAND_RCPTTO - "RCPT TO" message is not acceptable
 *          ERR_SMTP_SEND - error occurrence on send()
 *          ERR_SMTP_RECV - error occurrence on receiving local mail server SMTP message
 */
  
int CMD_RCPTTO()
{
	int i;
	int nEcho;
	
	//from vincent's mail list to get the mail addr.
	for (i=0; i<log_mail_addr_num; i++)
	{
		/* Send "RCPT TO" */
		sprintf(Buffer, "RCPT TO: <%s>\r\n", evcfg->MailTo[i]);
		
		if ((nEcho = SMTP_CMD('2', ERR_SMTP_COMMAND_RCPTTO)) != TRUE)
			return nEcho;
	}
	
	return TRUE;
}

/*
 * CMD_DATA - "dispatch" type mail sharing, send "DATA" message to local mail server
 * parameters : none
 * return : TRUE - "DATA" message is acceptable
 *          ERR_SMTP_COMMAND_DATA - "DATA" message is not acceptable
 *          ERR_SMTP_SEND - error occurrence on send()
 *          ERR_SMTP_RECV - error occurrence on receiving local mail server SMTP message
 */
  
int CMD_DATA()
{
	/* Send the "DATA" */
	sprintf(Buffer, "DATA\r\n");
	return SMTP_CMD('3', ERR_SMTP_COMMAND_DATA);
}

/*
 * SendData - "dispatch" type mail sharing, send SMTP mail body to local mail server
 * parameters : szBuffer - data area, containing mail body
 * return : TRUE - complete sending mail body to local mail server
 *          ERR_SMTP_SEND - error occurrence on send()
 */
  
int SendData(char *szBuffer)
{
    int nlen, nleft;
    char *p = szBuffer;
    
    nleft = strlen(szBuffer);
    while (nleft > 0)
    {
        if ((nlen = send(nSMTPSockFd, p, nleft, 0)) < 0)
        {
            SendFinish();
            return ERR_SMTP_SEND;
        }
        p += nlen;
        nleft -= nlen;
    }
  
    return TRUE;
}

/*
 * SendFinish - "dispatch" type mail sharing, send "." message (mail finish) to local mail server
 * parameters : none
 * return : TRUE - "." message is acceptable
 *          ERR_SMTP_COMMAND_FINISH - "." message is not acceptable
 *          ERR_SMTP_SEND - error occurrence on send()
 *          ERR_SMTP_RECV - error occurrence on receiving local mail server SMTP message
 */
  
int SendFinish()
{
    /* Send the dot punctuation*/
    sprintf(Buffer, ".\r\n");
    return SMTP_CMD('2', ERR_SMTP_COMMAND_FINISH);
}

/*
 * CMD_QUIT - "dispatch" type mail sharing, send "DATA" message to local mail server
 * parameters : none
 * return : TRUE - "QUIT" message is acceptable
 *          ERR_SMTP_COMMAND_QUIT - "QUIT" message is not acceptable
 *          ERR_SMTP_SEND - error occurrence on send()
 *          ERR_SMTP_RECV - error occurrence on receiving local mail server SMTP message
 */
  
int CMD_QUIT()
{
    sprintf(Buffer, "QUIT\r\n");
    return SMTP_CMD('2', ERR_SMTP_COMMAND_QUIT);
}

/*
 * SMTPRecv - "dispatch" type mail sharing, receive local mail server SMTP message
 * parameters : pReply - data buffer, free space for storing SMTP message
 * return : TRUE - successfully receive message
 *          FALSE - receive message fail
 */
int SMTPRecv(char *pReply)
{
	int nRecvCount;
	time_t current;
	int nlen, ret;
	char *p;
	
	memset(pReply,'\0',BUFFER_SIZE);
	nRecvCount = 0;
	nlen = 0;
	
	current = time(0);
	
	for (;;)
	{
		fd_set fdset ;
		struct timeval timeout ;
		
		timeout.tv_sec = 1 ;
		timeout.tv_usec = 0 ;
		
		FD_ZERO(&fdset) ;
		FD_SET(nSMTPSockFd, &fdset) ;
		if ((ret=select(64, &fdset, NULL, NULL, &timeout)) > 0)
		{
			nRecvCount = recv(nSMTPSockFd, (pReply+nlen), (BUFFER_SIZE-nlen),0);
		}
		else if (ret==0)
		{
			/* select time out */
			if ((time(0) - current) > 5)
				return FALSE;
			else
				continue;
		}
		else    /* select error */
		{
			diag_printf("Recv failed\n");
			return FALSE;
		}
	
		if (nRecvCount <= 0)
		{
			if (nRecvCount == 0)
				return FALSE;
			//else if ((time(NULL) - current) > 30000)
			else if ((time(0) - current) > 10)
				return FALSE;
			else
			{
				continue;
			}
		}
		
		nlen += nRecvCount;
		current = time(0);
		if ((p = strstr(pReply, "\r\n")) != NULL)
		{
			*p = '\0';
			return TRUE;
		}
	}
}

/*
 * CloseSMTPSocket - "dispatch" type mail sharing, close SMTP connection with local mail server
 * parameters : none
 * return : OK_SOCKET_CLOSE - connection close successfully
 *          ERR_SOCKET_CLOSE - error occurrence on closesocket()
 */
int EventSendMail(int type)
{
	//int i, j;
	int j;
	int nServerResponse = 0;
	struct hostent *hostinfo;
	int	diff = ntp_time() - time(0);
	//char **len_table = 0;
	int rc = -1;
	
	//init Param
	if (Check_IP(evcfg->Local_Server_IP) < 0)  /* not a IP */
	{
		hostinfo = gethostbyname(evcfg->Local_Server_IP);
		if (hostinfo && (hostinfo->h_length == 4))
		{
			sprintf(evcfg->Local_Server_IP, inet_ntoa(*(struct in_addr *)hostinfo->h_addr));
		}
		else
		{
			diag_printf("EventSendMail: Can not get DNS\n");
			return -1;
		}
	}
	 
	if ((nSMTPSockFd = ConnectSMTPsrv()) < 0)
	{
		diag_printf("Connect to local mail server failed\n");
		return -1;  // The socket have been close
	}
	
	if ((nServerResponse = CMD_HELO()) < 0)
	{
		diag_printf("SMTP: CMD_HELO fail\n");
		goto bad_sock;
	}
	
	if ((nServerResponse = CMD_MAILFROM()) < 0)
	{
		diag_printf("SMTP: CMD_MAILFROM fail\n");
		goto bad_sock;
	}	
		
	if ((nServerResponse = CMD_RCPTTO()) < 0)
	{
		diag_printf("SMTP: CMD_RCPTTO fail\n");
		goto bad_sock;
	}
	
	if ((nServerResponse = CMD_DATA()) < 0)
	{
		diag_printf("SMTP: CMD_DATA fail\n");
		goto bad_sock;
	}
	
	/* To */
	sprintf(cBuffer,"To: <%s>\r\n", evcfg->MailTo[0]);
	if (SendData(cBuffer) < 0)
	{
		diag_printf("SMTP: SendData fail\n");
		goto bad_sock;
	}
	
	/* send subject */
	if (evcfg->sysDomain[0] == 0)
		sprintf(cBuffer,"Subject: %s System Log\r\n\r\n", evcfg->sysName);
	else
		sprintf(cBuffer,"Subject: %s.%s System Log\r\n\r\n", evcfg->sysName, evcfg->sysDomain);
	
	if (SendData(cBuffer)<0)
	{
		diag_printf("SMTP: SendData fail\n");
		goto bad_sock;
	}
	
	//*********************************
	// Send log out here...
	//*********************************
	for(j=0; j<log_pool_num; j++)
	{   
		//char *start, char *end, *p;
		char *start;
		char *end;
		char *curr;
		//int  total_entry;
		
		// Find start pointer
		if (j != type && type != -1)
			continue;
		
		if (type == -1)
			start = FindEvent(j);
		else
			start = FindMailEvent(j);
		
		curr = end = EndEvent(j);
		
		if (start == NULL || end == NULL)
			continue;
		
		// Allocate entry table to count
		//for (i=0, p=start; p; i++, p=NextEvent(p, j))
		//	;
		//
		//total_entry = i;
		//if (total_entry == 0)	// Should not have this case
		//	continue;
		
		/* send the event log file */	
		//len_table = (char **)malloc(sizeof(char *)*total_entry);	
		//if (len_table == NULL)
		//	continue;
		
		// Send title
		if (j == 0)
		{
			sprintf (cBuffer,"/*---------------SYSTEM LOG---------------*/\r\n");
		}	
		else
		{
			sprintf (cBuffer,"/*---------------SECURITY LOG---------------*/\r\n");
		}	
		
		if (SendData(cBuffer) < 0)
		{	
			diag_printf("SMTP: SendData fail\n");
			break;
		}
		
		// Save entry to table, and send out
		//for (i=0, p=start; p; i++, p=NextEvent(p, j))
		//	len_table[i] = p;
		
		//for (i=total_entry-1; i>=0; i--)
		do
		{
			EventLogEntry *ev;
			UINT32	ev_dateTime;
			char time_info[32]={0};
			
			curr = PrevEvent(curr, j);
			if (curr == 0)
			{
				diag_printf("Reaching start\n");
				break;
			}
			
			//ev = (EventLogEntry *)(len_table[i]);
			ev = (EventLogEntry *)curr;
			ev_dateTime = gmt_translate2localtime(diff + ev->unix_dateTime);
			ctime_r(&ev_dateTime, time_info);
			if (strlen(time_info)==0)
				{
				diag_printf("[%s:%d]:strlen(time_info)==0",__FUNCTION__,__LINE__);
				goto bad_sock;
				}
			time_info[strlen(time_info)-1] = 0;  // Strip \n
			sprintf (cBuffer,"[%s]: <%s-%s> %s %s\r\n", 
					time_info, 
					EventClass(ev),
					EventLevel(ev),
					EventModule(ev),
					ev->text);
			
			if (SendData(cBuffer)<0)
			{	
				diag_printf("SMTP: SendData fail\n");
				break;
			}
		}
		while (curr != start);
		
		//free(len_table);
		//len_table = 0;
	}
	
	// Check if send with no problem
	if (j != log_pool_num)
		goto bad_sock;
	
	/* Send Finish */
	if (SendFinish() < 0)
	{
		diag_printf("SMTP: SendFinish fail\n");
		goto bad_sock;
	}
	
	if ((nServerResponse = CMD_QUIT()) < 0)
	{
		diag_printf("SMTP: CMD_QUIT fail\n");
		goto bad_sock;
	}
	
	rc = 0;
	
bad_sock:
	close(nSMTPSockFd);
	
	//if (len_table)
	//	free(len_table);
	
	return rc;
}

