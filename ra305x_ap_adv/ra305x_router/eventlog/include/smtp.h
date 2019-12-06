
//#define _DEBUG_SMTP_LOGFILE_       	12

#define TRUE 1
#define FALSE 0
#define BUFFER_SIZE 1024

#define OK_SMTP_SOCKET_CLOSE             302

// SMTP error code
#define ERR_SMTP_SOCKET_CREATE          -301
#define ERR_SMTP_SOCKET_CLOSE           -302
#define ERR_SMTP_CONNECT                -303
#define ERR_SMTP_RECV               	-304
#define ERR_SMTP_SEND                   -305
#define ERR_SMTP_COMMAND_HELO           -306
#define ERR_SMTP_COMMAND_MAILFROM       -307
#define ERR_SMTP_COMMAND_RCPTTO         -308
#define ERR_SMTP_COMMAND_DATA           -309
#define ERR_SMTP_COMMAND_FINISH         -310
#define ERR_SMTP_COMMAND_QUIT           -311
#define ERR_SMTP_SEND_TIMEOUT	-312

#define ERR_SMTP_NETWORK_INIT           -401
#define ERR_SMTP_PORT_INIT                      -402
#define ERR_SMTP_FILE_CREATE            -403

/* timeout constant value */
#define	   SET_RECV_TIMEOUT_BEFORE	1000
#define	   SET_RECV_TIMEOUT_AFTER	2000

#define    DOM_LEN         40
#define    IP_LEN          16
#define    MAX_USERS       51 

#define    OK_SOCKET_CLOSE      201
#define    ERR_SOCKET_CLOSE    -202

/* SMTP function declare */
int ConnectSMTPsrv(void);
int SMTP_CMD(char c, int err);
int CMD_HELO(void);
int CMD_MAILFROM(void);
int CMD_RCPTTO(void);
int CMD_DATA(void);
int CMD_QUIT(void);
int SendData(char* szBuffer);
int SendFinish(void);
int SMTPRecv(char* pReply);
int CreateSMTPLogFile(void);
int CloseSMTPLogFile(void);
int CloseSMTPSocket(void);

int EventSendMail(int type);


