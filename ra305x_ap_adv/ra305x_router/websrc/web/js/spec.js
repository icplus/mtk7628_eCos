var cPage=window.location.toString().replace(/.*\//,'');
cPage=cPage.replace(/\?.*/,'');

var mNum=0;

function Page(g,s,p,t,f)
{
    this.g=g;
	this.s=s;
	this.p=p;
	this.t=t;
	this.idx=mNum++;
	if (f) this.f=f; else this.f=0;
}

var Pages=new Array(
/*REAL<% #ifdef APCLI_SUPPORT %>REAL*/
new Page(0,'opmode','opmode.htm','OpMode'),
/*REAL<% #endif //APCLI_SUPPORT %>REAL*/
new Page(0,'wz','wz.htm','Wizard')
,new Page(0,'wan','wan.htm','WAN')
,new Page(0,'wan','wan_fixed.htm','WAN',1)
,new Page(0,'wan','wan_dhcp.htm','WAN',1)
,new Page(0,'wan','wan_pppoe.htm','WAN',1)
/*REAL<% #ifdef CONFIG_PPTP%>REAL*/
,new Page(0,'wan','wan_pptp.htm','WAN',1)
/*REAL<% #endif //CONFIG_PPTP%>REAL*/
/*REAL<% #ifdef CONFIG_L2TP%>REAL*/
,new Page(0,'wan','wan_l2tp.htm','WAN',1)
/*REAL<% #endif //CONFIG_L2TP%>REAL*/
,new Page(0,'lan','lan.htm','LAN')
,new Page(0,'dhcp','dhcp.htm','DHCP')
/*REAL<% #ifdef CONFIG_WIRELESS%>REAL*/
,new Page(0,'wlan','status.html','Wireless')
/*REAL<% #endif //CONFIG_WIRELESS %>REAL*/

,new Page(1,'nat_vserv','adv_virtual.htm','Virtual Server')
,new Page(1,'nat_ptrig','adv_appl.htm','Applications')
,new Page(1,'filters','adv_filters_ip.htm','Filters')
,new Page(1,'filters','adv_filters_mac.htm','Filters',1)
,new Page(1,'filters','adv_filters_url.htm','Filters',1)
,new Page(1,'filters','adv_filters_domain.htm','Filters',1)
,new Page(1,'firewall','adv_firewall.htm','Firewall')
/*REAL<% #ifdef CONFIG_DDNS%>REAL*/
,new Page(1,'ddns','ddns.htm','DDNS')
/*REAL<% #endif CONFIG_DDNS%>REAL*/
,new Page(1,'dmz','adv_dmz.htm','DMZ')
/*REAL<% #ifdef CONFIG_QOS%>REAL*/
,new Page(1,'qos','qos.htm','QOS')
/*REAL<% #endif CONFIG_QOS%>REAL*/
/*REAL<% #ifdef CONFIG_WIRELESS%>REAL*/
,new Page(1,'wireless_basic','wireless_basic.html','Wi-Fi Basic Setup')
,new Page(1,'wireless_advanced','wireless_advanced.html','Wi-Fi Advanced Setup')
,new Page(1,'wireless_block_ack','wireless_block_ack.html','Wi-Fi Block Ack Setup')
,new Page(1,'wireless_security','wireless_security.html','Wi-Fi Security Setup')
/*REAL<% #ifdef WSC_AP_SUPPORT %>REAL*/
,new Page(1,'wireless_simple_config_setup','wireless_simple_config_setup.html','WPS Configuration Setup')
,new Page(1,'wireless_simple_config_device','wireless_simple_config_device.html','WPS Device Configure')
,new Page(1,'wireless_simple_config_reset','wireless_simple_config_reset.html','WPS Reset Configuration')
/*REAL<% #endif //WSC_AP_SUPPORT %>REAL*/
/*REAL<% #ifdef APCLI_SUPPORT %>REAL*/
,new Page(1,'wireless_apcli_setup','wireless_apcli_setup.html','AP-Client Configuration Setup')
/*REAL<% #endif //APCLI_SUPPORT %>REAL*/
/*REAL<% #endif //CONFIG_WIRELESS %>REAL*/

,new Page(2,'admin','admin.htm', 'Admin')
,new Page(2,'time','time.htm','Time')
,new Page(2,'config','config.htm', 'System')
,new Page(2,'firmware','firmware.htm','Firmware')
,new Page(2,'Ping','ping.htm','Ping')
,new Page(2,'misc','misc.htm', 'Misc.')

,new Page(3,'info','info.htm','Device info')
,new Page(3,'log','log.htm', 'Log')
,new Page(3,'log_settings','log_settings.htm', 'Log',1)
,new Page(3,'stats','stats.htm', 'Stats')

,new Page(4,'','help.htm', '')
);

function findPage()
{
for (var i=0;i<Pages.length;i++)
	if (Pages[i].p==cPage) return Pages[i];
return Pages[0];
}

function LeftMenu(p)
{
	var m='<table cellspacing=10 align=center>';
	for (var i=0; i<Pages.length;i++)
	{
		if (Pages[i].g!=p.g)  continue;
		if (Pages[i].t=='') continue;
		if (Pages[i].f&1) continue;
		var b=((p.t!=Pages[i].t)? 'n' : 'd');

		if (__opmode == 3) 
		{
			if ((Pages[i].s == "wz") || (Pages[i].s == "wan") || (Pages[i].s == "dhcp"))
				continue;

			if ((Pages[i].s == "nat_vserv") || (Pages[i].s == "nat_ptrig") || (Pages[i].s == "filters") || (Pages[i].s == "firewall") || (Pages[i].s == "ddns") || (Pages[i].s == "dmz") || (Pages[i].s == "qos"))
				continue;
		}			
		
		if ( window.location.toString().substring(window.location.toString().length-15,window.location.toString().length+1) == 'lan.htm')
		{
			m+='<tr><td><a href='+Pages[i+1].p+'><div class=nav_left_'+b+'>'+
				Pages[i+1].t+'</div></a></td></tr>';
		}
		else
		{
			m+='<tr><td><a href='+Pages[i].p+'><div class=nav_left_'+b+'>'+
				Pages[i].t+'</div></a></td></tr>';
		}
	}

	m+='</table>';
	return m;
}


function get_style(c, p)
{
	if(c == p.g)
		return "_hl";
	else
		return "";
}

function pageHead()
{
	var p=findPage();

	m='<table WIDTH=765 cellpadding=0 cellspacing=0 bgcolor=#D2D6E7>\
	  <tr><td colspan=4><img border=0 src=images/title.jpg USEMAP=#MapTitle></td></tr>\
	  <tr>\
		<td valign=top width=230 HEIGHT=600 bgcolor=#FFFFFF>'+
			LeftMenu(p)+'</td>';
		m+='<TD VALIGN=top></TD>\
		<td valign=top WIDTH=582>\
	<TABLE WIDTH=550 CELLPADDING=0 CELLSPACING=0 ALIGN=center>\
	<TR >\
	    <TD VALIGN=bottom height=40  colspan=3 style=\"background-color: rgb(74,97,140);\">\
	    <span onclick=\"window.open(\'wz.htm\', \'_self\')\" class=\"nav_top'+get_style(0,p)+'\">Home</span>';

		if (__opmode == 3) 
		{
			m+='<span onclick=\"window.open(\'wireless_basic.html\', \'_self\')\" class=\"nav_top'+get_style(1,p)+'\">Advanced</span>';
		} else {
			m+='<span onclick=\"window.open(\'adv_virtual.htm\', \'_self\')\" class=\"nav_top'+get_style(1,p)+'\">Advanced</span>';
		}

	m += '<span onclick=\"window.open(\'admin.htm\', \'_self\')\" class=\"nav_top'+get_style(2,p)+'\">Tools</span>\
	    <span onclick=\"window.open(\'info.htm\', \'_self\')\" class=\"nav_top'+get_style(3,p)+'\" >Status</span>\
	    <span onclick=\"window.open(\'help.htm\', \'_self\')\" class=\"nav_top'+get_style(4,p)+'\">Help</span>\
	</TR>\
	<TR></TR><TR></TR><TR></TR><TR></TR><TR></TR>\
	<TR BGCOLOR=white>\
	    <td width=11 HEIGHT=530>\
	    <td width=550 valign=top align=left>';
	document.writeln(m);
}

function pageTail(msg)
{
	var p=findPage();

	var m='</td>\
	    <TD WIDTH=19></TD>\
	</tr>\
	<tr>\
		<td align=right></td>\
	</TR>\
	</table>\
	  </TD>\
	  <TD VALIGN=top WIDTH=25 BACKGROUND=images/br.gif></TD>\
	</TR>\
	\
	</TR>\
	<map name=MapTitle>\
		<area shape=rect coords="18,8,152,58" href=http://www.ralink.com.tw target=_blank>\
	</map>\
	</table>';
	document.writeln(m);
}
function showBanner()
{
	var m=printBanner();
	document.writeln(m);
}
function printCopyRight()
{
	var m=CopyRightStr();
	document.writeln(m);
}
function pageButton()
{
var m='<form><table width=100%><tr><td align=right>\
	<input type=\"button\" value=\"Apply\" onclick=\"Apply()\"    class=\"btn_nav\" /> \
	<input type=\"button\" value=\"Cancel\" onclick=\"Cancel()\"  class=\"btn_nav\" /> \
	<input type=\"button\" value=\"Help\" onclick=\"Help()\"      class=\"btn_nav\" /> \
	</td></tr></table></form>';
	document.write(m);
}

function wanChgPage(m, opmode) {
	if ((opmode == 2) && (m > 2))
		m = 2;

	switch (m)
	{
		case 1: window.location='wan_fixed.htm'; break;
		case 2: window.location='wan_dhcp.htm'; break;
		case 3: window.location='wan_pppoe.htm'; break;
/*REAL<% #ifdef CONFIG_PPTP%>REAL*/
		case 4: window.location='wan_pptp.htm'; break;
/*REAL<% #endif //CONFIG_PPTP%>REAL*/
/*REAL<% #ifdef CONFIG_L2TP%>REAL*/
		case 5: window.location='wan_l2tp.htm'; break;
/*REAL<% #endif //CONFIG_L2TP%>REAL*/
		default: return;
	}
}

function pageWanSel(opmode) {

var m='<table width=100%>\
<tr><td colspan=2><font face=Arial color=#8bacb1 size=2><b>WAN Settings</b></font></td></tr>\
<tr><td width=34%><input type=radio name=mode onClick=\"wanChgPage(2, ' + opmode + ');\" value=2>Dynamic IP Address</td>\
	<td width=66%>Obtain an IP address automatically from your ISP.</td>\
</tr>\
<tr><td><input type=radio name=mode onClick=\"wanChgPage(1, ' + opmode + ');\" value=1>Static IP Address</td>\
	<td>Set static IP information provided to you by your ISP.</td>\
</tr>';
if (opmode == 1) {
m+='<tr>\
	<td><input type=radio name=mode onClick=\"wanChgPage(3, ' + opmode + ');\" value=3>PPPoE</td>\
	<td>Choose this option if your ISP uses PPPoE.</td>\
</tr>';
/*REAL<% #ifdef CONFIG_PPTP%>
m+='<tr>\
	<td><input type=radio name=mode onClick=\"wanChgPage(4, ' + opmode + ');\" value=4>PPTP</td>\
	<td>PPTP</td>\
</tr>';
<% #endif //CONFIG_PPTP%>REAL*/
/*REAL<% #ifdef CONFIG_L2TP%>
m+='<tr>\
	<td><input type=radio name=mode onClick=\"wanChgPage(5, ' + opmode + ');\" value=5>L2TP</td>\
	<td>L2TP</td>\
</tr>';
<% #endif //CONFIG_L2TP%>REAL*/
}
m+='</table>';
	document.write(m);
}

var timeTable=[
[0,"-12:00","Enewetak, Kwajalein"],
[1,"-11:00","Midway Island, Samoa"],
[2,"-10:00","Hawaii"],
[3,"-09:00","Alaska"],
[4,"-08:00","Pacific Time (US & Canada), Tijuana"],
[5,"-07:00","Arizona, Mountain Time (US & Canada)"],
[7,"-06:00","Central Time (US & Canada), Mexico City, Tegucigalpa"],
[10,"-05:00","Bogota, Lima, Quito"],
[11,"-05:00","Eastern Time (US & Canada), Indiana (East)"],
[13,"-04:00","Atlantic Time (Canada), Caracas, La Paz"],
[54,"-03:30","Newfoundland"],
[16,"-03:00","Buenos Aires, Georgetown"],
[19,"-02:00","Mid-Atlantic"],
[20,"-01:00","Azores, Cape Verde Is."],
[22,"+00:00","Casablanca, Monrovia, Dublin, Edinburgh, Lisbon, London"],
[24,"+01:00","Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna"],
[25,"+01:00","Belgrade, Bratislava, Budapest, Ljubljana, Prague"],
[26,"+01:00","Brussels, Copenhagen, Madrid, Paris, Vilnius"],
[27,"+01:00","Sarajevo, Skopje, Sofija, Warsaw, Zagreb"],
[30,"+02:00","Athens, Bucharest, Cairo, Istanbul, Minsk"],
[31,"+02:00","Harare, Helsinki, Israel, Pretoria, Riga, Tallinn"],
[36,"+03:00","Moscow, St. Petersburg, Nairobi, Baghdad, Kuwait, Riyadh"],
[55,"+03:30","Tehran"],
[39,"+04:00","Abu Dhabi, Kazan, Muscat, Tbilisi"],
[56,"+04:30","Kabul"],
[41,"+05:00","Islamabad, Karachi, Ekaterinburg, Tashkent"],
[42,"+06:00","Almaty, Dhaka"],
[43,"+07:00","Bangkok, Jakarta, Hanoi"],
[45,"+08:00","Beijing, Chongqing, Urumqi, Hong Kong, Perth, Singapore, Taipei"],
[46,"+09:00","Toyko, Osaka, Sapporo, Yakutsk"],
[47,"+10:00","Brisbane, Canberra, Melbourne, Sydney, Guam, Port Moresby, Vladivostok, Hobart"],
[51,"+11:00","Magadan, Solamon, New Caledonia"],
[52,"+12:00","Fiji, Kamchatka, Marshall Is., Wellington, Auckland"]];

function genTimeOpt()
{
	var s='';
	for (i=0; i<timeTable.length; i++)
	{
		var t=timeTable[i];
		s+='<option value='+t[0]+'> (GMT'+t[1]+') '+t[2]+'</option>\n';
	}
	document.write(s);
}

function setCln(f,m){
	f.clnEn.value=1;
	decomMAC2(f.WMAC, m, 1);
	f.cln.value='Restore to Factory MAC Address';
}
function clrCln(f){
	f.clnEn.value=0;
	decomMAC2(f.WMAC, '', 1);
	f.cln.value='Clone MAC Address'
}
function clnMac(f)
{
	if  (f.clnEn.value=='1') clrCln(f); else setCln(f,cln_MAC);
}
function evalDnsFix(ds1,ds2)
{
	if ((ds1==''||ds1=='0.0.0.0')&&(ds2==''||ds2=='0.0.0.0')) return 0;
	return 1;
}
