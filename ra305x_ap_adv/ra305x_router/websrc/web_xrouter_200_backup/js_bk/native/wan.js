// JavaScript Document
//Lan Address
/*DEMO*/
addCfg("LANIP",0,"192.168.0.1");
addCfg("LANMSK",0,"255.255.255.0");
addCfg("dns",6,1);
addCfg("domain",7,'');
addCfg("lanspeed",8,1);
addCfg("opmode",100,"1");
addCfg("iptv_en",11,0);
addCfg("iptv_port",12,9);
/*END_DEMO*/
/*REAL
<%
CGI_MAP(LANIP, CFG_LAN_IP);
CGI_MAP(LANMSK, CFG_LAN_MSK);
CGI_MAP(dns, CFG_DNS_EN);
CGI_MAP(domain, CFG_SYS_DOMAIN);
CGI_MAP(lanspeed, CFG_LAN_PHY);
CGI_MAP(wanspeed, CFG_WAN_PHY);
CGI_MAP(opmode, CFG_SYS_OPMODE);
 #if  defined (CONFIG_IPTV)
CGI_MAP(iptv_en, CFG_IPTV_EN);
 #ifndef CONFIG_IPTV_FIX_PORT
CGI_MAP(iptv_port, CFG_IPTV_PORT);
 #endif // (CONFIG_IPTV_FIX_PORT)
 #endif // (CONFIG_IPTV)
%>
REAL*/
function CheckIPClass_MASK(ipa,mn)
{	
	var ip_class='A';
	ip = new Array();
	if (ipa.length==4)
	{
		for (var i=0;i<4;i++)
			ip[i]=ipa[i].value;
	}
	else
		ip=ipa.value.split(".");
	if (ip.length!=4)
	{
		alert("Invalid "+"IP Address"+"!");
		return false;
	}
	var m=new Array();
	if (mn.length==4)
		for (i=0;i<4;i++) m[i]=mn[i].value;
	else
	{
		m=mn.value.split('.');
		if (m.length!=4) { alert("Invalid "+"netmask"+"!"); ; return 0; }
	}

	if (ip[0]<=127 && ip[0]>=0) ip_class = 'A';
	else if (ip[0]<=191 && ip[0]>=128) ip_class = 'B';
	else if (ip[0]<=223 && ip[0]>=192) ip_class = 'C';

	if (ip_class == 'A' && m[0]!=255)
	{
		alert("Invalid "+"netmask"+"!"+",netmask of A-class IP address should be no less  than 255.0.0.0 ");
		return false;
	}
	if (ip_class == 'B' &&( m[0]!=255 ||m[1]!=255))
	{
		alert("Invalid "+"netmask"+"!"+",netmask of B-class IP address should be no less  than 255.255.0.0 ");
		return false;
	}
			
	if (ip_class == 'C' &&( m[0]!=255 ||m[1]!=255||m[2]!=255))
	{
		alert("Invalid "+"netmask"+"!"+",netmask of C-class IP address should be no less  than 255.255.255.0 ");
		return false;
	}
	return true;	

}

function setLan() {
	var j=document.LAN;
	var LIP = j.LANIP.value;
	var WIP = "<%CGI_WAN_IP() ;%>";
	var WanMSK = "<% CGI_WAN_MSK(); %>";
	var LanMsk="<% CGI_CFG_GET(CFG_LAN_MSK); %>";

	if (!verifyIP2(j.LANIP,"IP address")) return ;
	if (!ipMskChk(j.LANMSK,"subnet mask")) return ;
	if (!CheckIPClass_MASK(j.LANIP,j.LANMSK)) return ;

	if(ByMask_IPrange(LIP, LanMsk, 0)==ByMask_IPrange(WIP, WanMSK, 0))
	{
			alert("Attention: LAN IP and WAN IP should not in the same subnet!!\nWAN IP:"+WIP+"\nLAN IP:"+LIP);
			return;
	}		
	form2Cfg(j);

	subForm(j,'do_cmd.htm','LAN',cPage);
}