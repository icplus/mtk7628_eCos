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
new Page(0,'wz','wz.htm','連線精靈')
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
,new Page(0,'wlan','status.html','WLAN')
/*REAL<% #endif //CONFIG_WIRELESS %>REAL*/

,new Page(1,'nat_vserv','adv_virtual.htm','虛擬伺服器')
,new Page(1,'nat_ptrig','adv_appl.htm','特殊應用程式')
,new Page(1,'filters','adv_filters_ip.htm','過濾器')
,new Page(1,'filters','adv_filters_mac.htm','過濾器',1)
,new Page(1,'filters','adv_filters_url.htm','過濾器',1)
,new Page(1,'filters','adv_filters_domain.htm','過濾器',1)
,new Page(1,'firewall','adv_firewall.htm','防火牆')
,new Page(1,'ddns','ddns.htm','DDNS')
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

,new Page(2,'admin','admin.htm', '管理者系統')
,new Page(2,'time','time.htm','時間設定')
,new Page(2,'config','config.htm', '系統設定')
,new Page(2,'firmware','firmware.htm','韌體更新')
,new Page(2,'Ping','ping.htm','Ping')
,new Page(2,'misc','misc.htm', '其他項目')

,new Page(3,'info','info.htm','系統訊息')
,new Page(3,'log','log.htm', '系統記錄')
,new Page(3,'log_settings','log_settings.htm', '系統記錄',1)
,new Page(3,'stats','stats.htm', '流量統計')

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
	var m=  '<table cellspacing=10 align=center>';
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
m+='<tr><td background=images/menu_'+b+'.jpg valign=middle align=center width=89 height=37><a href='+Pages[i+1].p+'>'+
	'<span class=btn_'+b+'>'+Pages[i+1].t+'</span></a></td></tr>';
		}
		else
		{
m+='<tr><td background=images/menu_'+b+'.jpg valign=middle align=center width=89 height=37><a href='+Pages[i].p+'>'+
	'<span class=btn_'+b+'>'+Pages[i].t +'</span></a></td></tr>';
		}
	}

	m+='</table>';
	return m;
}

var topMenu=['home','advanced','tools','status','help'];
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
    <TD VALIGN=bottom height=40  colspan=3 background=images/mback.jpg>\
    <IMG BORDER=0 SRC=images/'+topMenu[p.g]+'.jpg USEMAP=#MapMap></td>\
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
<map name=MapMap>\
	<area shape=rect coords="10,5,100,30" href=wz.htm target="_self">';

	if (__opmode == 3) 
	{
		m+='<area shape=rect coords="120,5,210,30" href=wireless_basic.html target="_self">';
	} else {
		m+='<area shape=rect coords="120,5,210,30" href=adv_virtual.htm target="_self">';
	}
m+='<area shape=rect coords="240,5,320,30" href=admin.htm target="_self">\
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
	<a href=javascript:Apply()><img src=images/apply_p.jpg border=0></a>\
	<a href=javascript:Cancel()><img src=images/cancel_p.jpg border=0></a>\
	<a href=javascript:Help()><img src=images/help_p.jpg border=0></a>\
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
<tr><td colspan=2><font face=Arial color=#8bacb1 size=2><b>WAN  設定 </b></font></td></tr>\
<tr><td width=34%><input type=radio name=mode onClick=\"wanChgPage(2, ' + opmode + ');\") value=2>動態 IP 位址</td>\
	<td width=66%>自動地從網際網路服務提供者得到一個 IP 位址。</td>\
</tr>\
<tr><td><input type=radio name=mode onClick=\"wanChgPage(1, ' + opmode + ');\") value=1>固定 IP 位址</td>\
	<td>請輸入網際網路服務提供者所提供之固定 IP 位址設定資訊。</td>\
</tr>';
if (opmode == 1) {
m+='<tr>\
	<td><input type=radio name=mode onClick=\"wanChgPage(3, ' + opmode + ');\") value=3>PPPoE</td>\
	<td>網際網路服務提供者提供 PPPoE 服務，請選擇此項目。</td>\
</tr>';
/*REAL<% #ifdef CONFIG_PPTP%>
m+='<tr>\
	<td><input type=radio name=mode onClick=\"wanChgPage(4, ' + opmode + ');\") value=4>PPTP</td>\
	<td>PPTP</td>\
</tr>';
<% #endif //CONFIG_PPTP%>REAL*/
/*REAL<% #ifdef CONFIG_L2TP%>
m+='<tr>\
	<td><input type=radio name=mode onClick=\"wanChgPage(5, ' + opmode + ');\") value=5>L2TP</td>\
	<td>L2TP</td>\
</tr>';
<% #endif //CONFIG_L2TP%>REAL*/
}
m+='</table>';
	document.write(m);
}

var timeTable=[
[0,"-12:00","安尼威吐克,瓜加林島"],
[1,"-11:00","美國的中西部,薩摩亞"],
[2,"-10:00","夏威夷"],
[3,"-09:00","阿拉斯加"],
[4,"-08:00","太平洋標準時間(美國,加拿大)"],
[5,"-07:00","亞利桑那州,山脈標準時間(美國,加拿大)"],
[7,"-06:00","中元標準時間(美國,加拿大),墨西哥,德古斯加巴(宏都拉斯首都)"],
[10,"-05:00","波哥大,利瑪(秘魯首都),基多(厄瓜多爾首都)"],
[11,"-05:00","東方標準時間(美國,加拿大),印第安納州 (東邊)"],
[13,"-04:00","大西洋標準時間(美國,加拿大),卡拉卡斯(委內瑞拉首都),拉巴斯"],
[54,"-03:30","紐芬蘭"],
[16,"-03:00","布宜諾斯艾利斯(阿根廷首都),喬治城,巴西利亞"],
[19,"-02:00","中部-阿根廷"],
[20,"-01:00","亞速爾群島,德總角"],
[22,"+00:00","卡薩布蘭卡市,蒙羅維亞,都柏林,愛丁堡,里斯本(葡萄牙首都),倫敦"],
[24,"+01:00","阿姆斯特丹,柏林,伯恩,羅馬,斯德哥爾摩,維也納"],
[25,"+01:00","貝爾格來德,布拉迪斯拉發,布達佩斯,爐布爾雅那,布拉格"],
[26,"+01:00","布魯塞爾,哥本哈根,馬德里,巴黎,維爾紐斯"],
[27,"+01:00","薩拉熱窩,斯科普里,索非亞,華沙,札格拉布"],
[30,"+02:00","雅典,布加勒斯特,開羅,伊斯坦布爾,明斯克"],
[31,"+02:00","哈拉雷,赫爾辛基,耶路撒冷,普利托里亞,里加,塔林"],
[36,"+03:00","莫斯科,聖彼得堡,窩瓦河,巴格達,科威特,利雅德"],
[55,"+03:30","德黑蘭"],
[39,"+04:00","阿布達比,巴庫,馬斯喀特,第比利斯"],
[56,"+04:30","喀布爾"],
[41,"+05:00","凱撒琳鎮,伊斯蘭馬巴德,克拉啻港市,塔什干"],
[57,"+05:30","新德里"],
[42,"+06:00","亞斯塔蔕,阿曼蒂,可倫波,狄哈卡"],
[43,"+07:00","曼谷,河內,雅加達"],
[45,"+08:00","北京,香港,新加坡,台北"],
[46,"+09:00","漢城,東京,亞庫次客"],
[58,"+09:30","愛得萊德,達爾文"],
[47,"+10:00","坎培拉,關島,摩爾斯比港,海蔘威"],
[51,"+11:00","麥哲倫,所羅門島"],
[52,"+12:00","斐濟,勘查加半島,馬紹爾群島,威爾尼頓"]];

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
	f.cln.value='恢復出廠 MAC 位址';
}
function clrCln(f){
	f.clnEn.value=0;
	decomMAC2(f.WMAC, '', 1);
	f.cln.value='從用戶端複製 MAC 位址'
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
