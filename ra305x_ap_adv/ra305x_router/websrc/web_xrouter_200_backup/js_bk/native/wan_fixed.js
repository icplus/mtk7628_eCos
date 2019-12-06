
/*DEMO*/
addCfg("opmode",101,"1");
addCfg("WANT",100,"1");
addCfg("WANIP",31,"10.10.10.221");
addCfg("WANMSK",32,"255.255.255.0");
addCfg("WANGW",33,"10.10.10.253");
addCfg("MTU",50,"1500");
addCfg("DSFIX",0x33,"1");
addCfg("DS1",0x34,"168.95.1.1");
addCfg("DS2",0x35,"");
addCfg("WMAC",51,"00:CC:11:22:33:44");
addCfg("clnEn",52,"1");
addCfg("WANIP6",103,"3ffe:501:ffff:101:20c:43FF:FE56:5224");
addCfg("WANMSK6",104,"ffff:ffff:ffff:ffff:0000:0000:0000:0000");
addCfg("WANGW6",105,"fe80:2::0200:00ff:fe00:0100");
var cln_MAC="00:CC:AA:00:12:45";
/*END_DEMO*/
/*REAL
<%
CGI_MAP(opmode, CFG_SYS_OPMODE);
CGI_MAP(WANIP, CFG_WAN_IP);
CGI_MAP(WANMSK, CFG_WAN_MSK);
CGI_MAP(WANGW, CFG_WAN_GW);
CGI_MAP(MTU, CFG_WAN_MTU);
CGI_MAP(DSFIX, CFG_DNS_FIX);
CGI_MAP(DS1, CFG_DNS_SVR+1);
CGI_MAP(DS2, CFG_DNS_SVR+2);
CGI_MAP(WANT, CFG_WAN_IP_MODE);
CGI_MAP(WMAC, CFG_WAN_DHCP_MAC);
CGI_MAP(clnEn, CFG_WAN_DHCP_MAC_EN);
CGI_MAP(WANIP6, CFG_WAN_IP_v6);
CGI_MAP(WANMSK6, CFG_WAN_MSK_v6);
CGI_MAP(WANGW6, CFG_WAN_GW_v6);
%>
var cln_MAC = <%CGI_GET_CLN_MAC();%> ;
REAL*/
