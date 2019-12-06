
/*DEMO*/
addCfg("host",0,"Ralink Wireless Access Point");
addCfg("TZ",3,"45");
addCfg("WANT",100,"2");
addCfg("opmode",101,"1");
addCfg("setLoginFlags",25,"1");
/*END_DEMO*/
/*REAL
<%
CGI_MAP(host, CFG_SYS_NAME);
CGI_MAP(TZ, CFG_SYS_TZONE);
CGI_MAP(WANT, CFG_WAN_IP_MODE);
CGI_MAP(opmode, CFG_SYS_OPMODE);
CGI_MAP(setLoginFlags, CFG_SYS_Login_Flags);
%>
REAL*/

//////////// DHCP ( cable )
/*DEMO*/
addCfg("WMAC",31,"00:11:22:33:44:55");
addCfg("clnEn",32,"0");
var cln_MAC = "00:12:34:56:78:90" ;
/*END_DEMO*/
/*REAL
<%
CGI_MAP(WMAC, CFG_WAN_DHCP_MAC);
CGI_MAP(clnEn, CFG_WAN_DHCP_MAC_EN);
%>
var cln_MAC = <%CGI_GET_CLN_MAC();%> ;
REAL*/

var __opmode = 1*getCfg("opmode");
function initDHC(){
	var f=document.DHC;
	cfg2Form(f);
	if (f.clnEn.value=='1')	setCln(f,getCfg("WMAC")); else clrCln(f);
}

function chkDHC() {
	var f=document.DHC;
	if (!verifyMAC(f.WMAC,"MAC Address",1)) return ;
	setCfg("setLoginFlags", 0);
	form2Cfg(f);
	switchPage('C2','D');
}

///////////// FIXED IP 
/*DEMO*/
addCfg("WANIP",31,"0.0.0.0");
addCfg("WANMSK",32,"255.255.255.0");
addCfg("WANGW",33,"0.0.0.0");
addCfg("DSFIX",0x33,"1");
addCfg("DS1",0x34,"");
addCfg("DS2",0x35,'');
/*END_DEMO*/
/*REAL
<%
CGI_MAP(WANIP, CFG_WAN_IP);
CGI_MAP(WANMSK, CFG_WAN_MSK);
CGI_MAP(WANGW, CFG_WAN_GW);

CGI_MAP(DSFIX, CFG_DNS_FIX);
CGI_MAP(DS1, CFG_DNS_SVR+1);
CGI_MAP(DS2, CFG_DNS_SVR+2);
%>
REAL*/

function initSIP(){
	cfg2Form(document.SIP);
}

function chkSIP() {
	var f=document.SIP;
	if (!verifyIP2(f.WANIP,"IP Address")) return ;
	if (!ipMskChk(f.WANMSK,"Subnet Mask")) return ;
	if (!verifyIP2(f.WANGW,"Default Gateway")) return ;

	if (!verifyIP2(f.DS1,"Primary DNS Address")) return ;
	if (!verifyIP0(f.DS2,"Secondary DNS Address")) return ;

	setCfg("DSFIX",1);
	setCfg("setLoginFlags", 0);
	form2Cfg(f);
	switchPage('C1','D');
}

//////////// PPPOE

/*DEMO*/
addCfg("PUN", 50, "");
addCfg("PSN", 51, "");
addCfg("PMTU", 52, "1400");
addCfg("PIDL", 53, "60");
addCfg("PPW", 54, "******" );
/*END_DEMO*/
/*REAL
<%
CGI_MAP(PUN, CFG_POE_USER);
CGI_MAP(PSN, CFG_POE_SVC);
CGI_MAP(PMTU, CFG_POE_MTU);
CGI_MAP(PIDL, CFG_POE_IDLET );
%>
addCfg("PPW",<%CGI_CFG_ID(CFG_POE_PASS);%>,"******");
REAL*/

function initPOE() {
	cfg2Form(document.POE);
}

function chkPOE() {
	var f=document.POE;
	if (!chkStrLen(f.PUN,0,255,"User Name")) return ;
	if (!chkStrLen(f.PPW,0,255,"User Password")) return ;
	if (!chkPwdUpdate(f.PPW,f._ps2,f._changed)) return ;
	if (isBlank(f.PUN.value)) { alert("Invalid User Name"); return ;}
	setCfg("setLoginFlags", 0);
	form2Cfg(f);
	switchPage('C3','D');
}  

function init() {
	var intLoginFlags = 1*getCfg("setLoginFlags");
	if (!intLoginFlags)
	{
		window.location.href='home.htm';
	}
	cfg2Form(document.Mode);
	initDHC();
	initSIP();
	initPOE();
	//showDetail();
}

function setWanMode() {
	var f=document.Mode;
	form2Cfg(f);
	switchPage('B',"C3");
}
function setDHCP() {
	document.getElementById("C2").style.display = ''
	document.getElementsByTagName("li")[1].className = 'active'
	document.getElementById("C3").style.display = 'none'
	document.getElementsByTagName("li")[0].className = ''
}

function setPPPoE() {
	document.getElementById("C2").style.display = 'none'
	document.getElementsByTagName("li")[1].className = ''
	document.getElementById("C3").style.display = ''
	document.getElementsByTagName("li")[0].className = 'active'
}
function setAPClient() {
	window.location = 'wireless_apcli_setup.html'
}
function Apply()
{
	var f=document.frmDone;
	
	subForm(f,'do_cmd.htm','WZ','index.htm');
}

