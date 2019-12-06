/*DEMO*/
addCfg("opmode",101,"1");
addCfg("WANT",100,"3");
addCfg("PUN", 50, "Jack");
addCfg("PSN", 51, "Hinet");
addCfg("PMTU", 52, "1400");
addCfg("PIDL", 53, "60");
addCfg("PCM", 55, 2 );
addCfg("PIPEN", 56, 0 );
addCfg("PIP", 56, "223.128.9.130" );
addCfg("DSFIX",37,"1");
addCfg("DS1",0x34,"168.95.1.1");
addCfg("DS2",0x35,"");
addCfg("WMAC",31,"00:CC:22:33:44:55");
addCfg("clnEn",52,"1");
addCfg("PPW", 54, "******" );
var cln_MAC = "00:12:34:56:78:90" ;
/*END_DEMO*/
/*REAL
<%
CGI_MAP(opmode, CFG_SYS_OPMODE);
CGI_MAP(WANT, CFG_WAN_IP_MODE);
CGI_MAP(PUN, CFG_POE_USER);
CGI_MAP(PSN, CFG_POE_SVC);
CGI_MAP(PMTU, CFG_POE_MTU);
CGI_MAP(PIDL, CFG_POE_IDLET );
CGI_MAP(PCM, CFG_POE_AUTO);
CGI_MAP(PIPEN, CFG_POE_SIPE);
CGI_MAP(PIP, CFG_POE_SIP);
CGI_MAP(DSFIX,CFG_DNS_FIX);
CGI_MAP(DS1, CFG_DNS_SVR+1);
CGI_MAP(DS2, CFG_DNS_SVR+2);
CGI_MAP(WMAC, CFG_WAN_DHCP_MAC);
CGI_MAP(clnEn, CFG_WAN_DHCP_MAC_EN);
%>
addCfg("PPW",<%CGI_CFG_ID(CFG_POE_PASS);%>,"******");
var cln_MAC = <%CGI_GET_CLN_MAC();%> ;
REAL*/
