var cPage=window.location.toString().replace(/.*\//,'');
cPage=cPage.replace(/\?.*/,'');

var mNum=0;
var tophref=['wz.htm','adv_virtual.htm','admin.htm','info.htm','help.htm'];

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
new Page(0,'wz','wz.htm',leftmenu[0])
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
,new Page(0,'wlan','status.html','Wireless')

,new Page(1,'nat_vserv','adv_virtual.htm',leftmenu[1])
,new Page(1,'nat_ptrig','adv_appl.htm',leftmenu[2])
,new Page(1,'filters','adv_filters_ip.htm',leftmenu[3])
,new Page(1,'filters','adv_filters_mac.htm',leftmenu[3],1)
,new Page(1,'filters','adv_filters_url.htm',leftmenu[3],1)
,new Page(1,'filters','adv_filters_domain.htm',leftmenu[3],1)
,new Page(1,'firewall','adv_firewall.htm',leftmenu[4])
,new Page(1,'ddns','ddns.htm','DDNS')
,new Page(1,'dmz','adv_dmz.htm','DMZ')
/*REAL<% #ifdef CONFIG_QOS%>REAL*/
,new Page(1,'qos','qos.htm','QOS')
/*REAL<% #endif CONFIG_QOS%>REAL*/
,new Page(1,'wireless_basic','wireless_basic.html','Wi-Fi Basic Setup')
,new Page(1,'wireless_advanced','wireless_advanced.html','Wi-Fi Advanced Setup')
,new Page(1,'wireless_block_ack','wireless_block_ack.html','Wi-Fi Block Ack Setup')
,new Page(1,'wireless_security','wireless_security.html','Wi-Fi Security Setup')
,new Page(1,'wireless_simple_config_setup','wireless_simple_config_setup.html','WPS Configuration Setup')
,new Page(1,'wireless_simple_config_device','wireless_simple_config_device.html','WPS Device Configure')
,new Page(1,'wireless_simple_config_reset','wireless_simple_config_reset.html','WPS Reset Configuration')
,new Page(1,'wireless_apcli_setup','wireless_apcli_setup.html','AP-Client Configuration Setup')

,new Page(2,'admin','admin.htm', leftmenu[5])
,new Page(2,'time','time.htm',leftmenu[6])
,new Page(2,'config','config.htm', leftmenu[7])
,new Page(2,'firmware','firmware.htm',leftmenu[8])
,new Page(2,'misc','misc.htm', leftmenu[9])

,new Page(3,'info','info.htm',leftmenu[10])
,new Page(3,'log','log.htm', leftmenu[11])
,new Page(3,'log_settings','log_settings.htm', leftmenu[11],1)
,new Page(3,'stats','stats.htm', leftmenu[12])

,new Page(4,'','help.htm', '')
,new Page(0,'','do_cmd.htm', '')
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
		
		if ( window.location.toString().substring(window.location.toString().length-15,window.location.toString().length+1) == 'lan.htm')
		{
m+='<tr><td background=images/menu_'+b+'.jpg valign=middle align=center width=89 height=37><a href='+Pages[i+1].p+'>'+
	'<span class=btn_'+b+'>'+Pages[i+1].t+'</span></a></td></tr>';
		}
		else
		{
m+='<tr><td background=images/menu_'+b+'.jpg valign=middle align=center width=89 height=37><a href='+Pages[i].p+'>'+
	'<span class=btn_'+b+'>'+Pages[i].t+'</span></a></td></tr>';
		}
	}

	m+='</table>';
	return m;
}
function TopMenu(p)
{
	var m='<table>';;
	for(var i=0; i<5;i++)
		{
			var b=((p.g==i)? 'd' : 'n');
			m+='<td background=images/menu_'+b+'.jpg valign=middle align=center width=110 height=37><a href='+tophref[i]+'>'+
	'<span class=btn_'+b+'><FONT SIZE=4><b>'+topmenu[i]+'</b></FONT></span></a></td>';
		}
	m+='</table>'
	return m;
}
function setlm(v)
{
	document.all.lmSetup.action=window.location;
	document.all.lmSetup.SET0.value="18350336="+document.all.LM.selectedIndex+""
    document.all.lmSetup.submit();
    
	
			
}

function pageHead()
{
	var p=findPage();

m='<table WIDTH=765 cellpadding=0 cellspacing=0 bgcolor=#D2D6E7>\
  <tr>\<td colspan=2><img border=0 src=images/title.jpg USEMAP=#MapTitle></td>\
  <td><form name=lmSetup method=POST>\
  <INPUT type=hidden name=CMD value=LANG>\
  <INPUT type=hidden name=SET0 value="0">\
  <select name=LM onchange=setlm()>'
  /*REAL<% #ifdef CONFIG_ENGLISH%>REAL*/
  m+='<option '+((0==_LM_)?'selected':'')+'>english</option>'
  /*REAL<%#endif%>REAL*/
  /*REAL<% #ifdef CONFIG_TCHINESE%>REAL*/
  m+='<option '+((1==_LM_)?'selected':'')+'>Tchinese</option>'
  /*REAL<%#endif%>REAL*/
  m+='</select></form></td>\
</tr>\
  <tr>\
	<td valign=top width=230 HEIGHT=600 bgcolor=#FFFFFF>'+
		LeftMenu(p)+'</td>';
	m+='<TD VALIGN=top ></TD>\
	<td valign=top WIDTH=582>\
<TABLE WIDTH=550 CELLPADDING=0 CELLSPACING=0 ALIGN=center>\
<TR ><TD VALIGN=middle height=40 background=images/mback.jpg colspan=3>\
'+TopMenu(p)+'</TD></TR>\
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
<map name=MapMap>\
	<area shape=rect coords="10,5,100,30" href=wz.htm target="_self">\
	<area shape=rect coords="120,5,210,30" href=adv_virtual.htm target="_self">\
	<area shape=rect coords="240,5,320,30" href=admin.htm target="_self">\
	<area shape=rect coords="340,5,430,30" href=info.htm target="_self">\
	<area shape=rect coords="450,5,540,30" href=help.htm target="_self">\
</map>\
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
	<button type=\"button\" onclick=javascript:Apply()>'+Button_txt[6]+'</button>\
	<button type=\"button\" onclick=javascript:Cancel()>'+Button_txt[7]+'</button>\
	<button type=\"button\" onclick=javascript:Help()>'+Button_txt[3]+'</button>\
	</td></tr></table></form>';
	document.write(m);
}

function wanChgPage(m) {
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

function pageWanSel() {
var m='<table width=100%>\
<tr><td colspan=2><font face=Arial color=#8bacb1 size=2><b><span id=_0>WAN Settings</span></b></font></td></tr>\
<tr><td width=34%><input type=radio name=mode onClick=wanChgPage(2) value=2><span id=_1>Dynamic IP Address</span></td>\
	<td width=66%><span id=_2>Obtain an IP address automatically from your ISP.</span></td>\
</tr>\
<tr><td><input type=radio name=mode onClick=wanChgPage(1) value=1><span id=_3>Static IP Address</span></td>\
	<td><span id=_4>Set static IP information provided to you by your ISP.</span></td>\
</tr>';
m+='<tr>\
	<td><input type=radio name=mode onClick=wanChgPage(3) value=3><span id=_5>PPPoE</span></td>\
	<td><span id=_6>Choose this option if your ISP uses PPPoE.</span></td>\
</tr>';
/*REAL<% #ifdef CONFIG_PPTP%>
m+='<tr>\
	<td><input type=radio name=mode onClick=wanChgPage(4) value=4>PPTP</td>\
	<td>PPTP</td>\
</tr>';
<% #endif //CONFIG_PPTP%>REAL*/
/*REAL<% #ifdef CONFIG_L2TP%>
m+='<tr>\
	<td><input type=radio name=mode onClick=wanChgPage(5) value=5>L2TP</td>\
	<td>L2TP</td>\
</tr>';
<% #endif //CONFIG_L2TP%>REAL*/
m+='</table>';
	document.write(m);
}



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
	f.cln.value=wizard[44];
}
function clrCln(f){
	f.clnEn.value=0;
	decomMAC2(f.WMAC, '', 1);
	
	f.cln.value=wizard[43]
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
