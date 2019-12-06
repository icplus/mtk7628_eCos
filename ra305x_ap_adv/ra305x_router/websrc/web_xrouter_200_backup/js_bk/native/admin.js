// JavaScript Document
/*DEMO*/
addCfg("opmode",100,"1");
addCfg("RMIP",101,"0.0.0.0");
addCfg("RMPORT",102,"1080");
addCfg("RMEN",103,1);
addCfg("SYSPS",106,"******" );
/*END_DEMO*/

/*REAL
<%
CGI_MAP(RMIP, CFG_SYS_RM_IP);
CGI_MAP(RMPORT,CFG_SYS_RM_PORT);
CGI_MAP(RMEN,CFG_SYS_RM_EN);
CGI_MAP(opmode, CFG_SYS_OPMODE);
%>
addCfg("SYSPS",<%CGI_CFG_ID(CFG_SYS_ADMPASS);%>,"******");
REAL*/
