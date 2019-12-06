/*DEMO*/
addCfg("opmode",101,"1");
addCfg("WANT",100,"2");
addCfg("WMAC",31,"00:CC:11:22:33:44");
addCfg("clnEn",32,"1");
//addCfg("WMAC",31,"");
//addCfg("clnEn",32,"0");
addCfg("DMTU", 33,"1496");
addCfg("HN",36,"router");
addCfg("DSFIX",37,"1");
addCfg("DS1",0x34,'168.95.1.1');
addCfg("DS2",0x35,'');
addCfg("mancon",0x35,'1');
var	cln_MAC	= "00:12:34:56:78:90" ;
/*END_DEMO*/
/*REAL
<%
CGI_MAP(opmode, CFG_SYS_OPMODE);
CGI_MAP(WANT, CFG_WAN_IP_MODE);
CGI_MAP(WMAC, CFG_WAN_DHCP_MAC);
CGI_MAP(clnEn, CFG_WAN_DHCP_MAC_EN);
CGI_MAP(DMTU, CFG_WAN_DHCP_MTU);
CGI_MAP(HN,	CFG_SYS_NAME);
CGI_MAP(DSFIX,CFG_DNS_FIX);
CGI_MAP(DS1, CFG_DNS_SVR+1);
CGI_MAP(DS2, CFG_DNS_SVR+2);
CGI_MAP(mancon, CFG_WAN_DHCP_MAN);
%>
var	cln_MAC	= <%CGI_GET_CLN_MAC();%> ;
REAL*/