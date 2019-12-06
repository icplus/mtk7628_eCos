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
,new Page(1,'wan','wan.htm','WAN')
,new Page(1,'wan','wan_fixed.htm','WAN',1)
,new Page(1,'wan','wan_dhcp.htm','WAN',1)
,new Page(1,'wan','wan_pppoe.htm','WAN',1)
/*REAL<% #ifdef CONFIG_PPTP%>REAL*/
,new Page(1,'wan','wan_pptp.htm','WAN',1)
/*REAL<% #endif //CONFIG_PPTP%>REAL*/
/*REAL<% #ifdef CONFIG_L2TP%>REAL*/
,new Page(1,'wan','wan_l2tp.htm','WAN',1)
/*REAL<% #endif //CONFIG_L2TP%>REAL*/
,new Page(1,'lan','lan.htm','LAN')
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
,new Page(2,'wireless_basic','wireless_basic.html','Wi-Fi Basic Setup')
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

,new Page(3,'admin','admin.htm', 'Admin')
,new Page(4,'info','info.htm', 'Status')
,new Page(2,'time','time.htm','Time')
,new Page(2,'config','config.htm', 'System')
,new Page(4,'firmware','firmware.htm','firmware')
,new Page(2,'Ping','ping.htm','Ping')
,new Page(2,'misc','misc.htm', 'Misc.')

,new Page(3,'info','info.htm','Device info')
,new Page(3,'log','log.htm', 'Log')
,new Page(3,'log_settings','log_settings.htm', 'Log',1)
,new Page(3,'stats','stats.htm', 'Stats')

,new Page(3,'','help.htm', 'help info')
);

function findPage()
{
for (var i=0;i<Pages.length;i++)
	if (Pages[i].p==cPage) return Pages[i];
return Pages[0];
}

function pageHead()
{
	var p=findPage();

	var genernateLi = function (arr) {
		if (!Array.isArray(arr))
			return '';
		var regExp = new RegExp(p.s);
		var liStr = '';
		for(var i = 0; i < arr.length; i++) {
			var item = arr[i];
			if (regExp.test(item.key))
				liStr += '<li class="item active">';
			else
				liStr += '<li class="item">';
			liStr += '<a href="' + item.key + '">' + item.value + '</a></li>';
		}
		return liStr;
	};

	var navArr = [
		{
			'key': 'wireless_basic.html',
			'value': "Wifi"
		}, {
			'key': 'wireless_apcli_setup.html',
			'value': "AP Client"
		}, {
			'key': 'admin.htm',
			'value': "Setting"
		}, {
            'key': 'info.htm',
            'value': "Status"
		}
	];
/*REAL<% CGI_MAP(Selectd_Language, CFG_SYS_Language); %>REAL*/
	m='<div id="header" class="container">\
		<div style="width:1170px;height:120px;">\
		<div style="width:500px;height:120px;float:left;"><a href="home.htm"><img src="images/pandorabox_LOGO.png" alt="logo" class="logo"></a></div>\
		<form name="lang" action="" method="POST" enctype="multipart/form-data" target="upframe">\
		<div style="width:110px;height:40px;margin-top:20px;float:right;">\
		<input type="hidden" name="Selectd_Language" />\
		<a href="javascript:SetChinese();""><img src="images/cn.gif" /></a>&nbsp; <a href="javascript:SetEnglish();""><img src="images/en.gif" /></a>\
		</div>\
		</form>\
		</div>\
		</div>\
		<div id="aside" class="nav">\
			<div class="container">\
				<ul class="navbar">';
	m += genernateLi(navArr);
	m += '</ul></div></div><div class="container"><div class="content">';
	document.writeln(m);
}

function pageTail(msg)
{
	var p=findPage();

	var m='</div></div>\
    <div class="footer"><div class="span">Powered by PandoraBox Team. Engine by PandoraBox#. Theme by Lafite#0.1.4</div></div>';
	document.writeln(m);
	translate();
}
function printCopyRight()
{
	var m=CopyRightStr();
	document.writeln(m);
}
function pageButton(fnName)
{
	var m='<input type=\"button\" value=\"Apply\" onclick=\"';
	if (fnName)
		m += fnName;
	else
		m += 'Apply';
	m += '()\" class=\"btn\" />';
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
	var gWanLi = function (opmode) {
		var wanLiStr = '';
		var wanArr = [
			{
				'key' : 'PPPoE',
				'value' : 3
			}, {
				'key' : 'Fixed',
				'value' : 1
			},{
				'key' : 'DHCP',
				'value' : 2
			}
		];
		for (var i = 0; i < wanArr.length; i++) {
			var item = wanArr[i];
			var val = item.value;
			var regExp = new RegExp(item.key, 'i');
			if (regExp.test(findPage().p)) {
				wanLiStr += '<li class="active">';
			}
			else
				wanLiStr += '<li>';
			wanLiStr += '<a href="javascript:;"';
			wanLiStr += ' onClick="wanChgPage(' + val + ',' + opmode + ');"' + 'value=' + val + ' >';
			wanLiStr += item.key +'</a></li>';
		}
		return wanLiStr;
	};

	var m = '<div class="title">\
	<label>Internet</label>\
	<ul>';
	m += gWanLi(opmode);
	m += '</ul></div>';

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
function multiLang (tagName) {
	var zh_cn = {
		// button
		"Apply" : "应用",
		"Restore" : "恢复",
        "Continue" : "继续",
		// Switch
		"Disable" : "禁用",
		"Enable" : "启用",
		// Nav
		"Firmware" : "固件",
		"Home" : "首页",
		"About" : "关于",
		"Internet" : "网络",
		"Repeater" : "中继",
		"Reboot" : "重启",
		"Reset" : "恢复出厂设置",
		"Next" : "下一步",
		"Back" : "上一步",
		"Exit" : "退出",
		"scan" : "扫描",
		"connect" : "连接",
		"Setting" : "设置",
		"Wifi" : "无线",
		"AP Client" : "无线中继",
		"Status" : "网络状态",
		// Wan
		"DHCP" : "动态获取",
		"Confirm Password" : "确认密码",
		"Fixed" : "静态地址",
		"IP Address" : "IP地址",
		"ISP Gateway Address" : "网关",
		"PPPoE" : "拨号上网",
		"Primary DNS Address" : "DNS",
		"PPPoE User Name" : "上网账号",
		"PPPoE User Password" : "上网密码",
		"Subnet Mask" : "子网掩码",
		// Wifi
		"AES": "AES",
		"AutoSelect" : "自动",
		"Bridge Mode" : "桥接模式",
		"BSSID" : "BSSID",
		"Key 1" : "密码1",
		"Key 2" : "密码2",
		"Key 3" : "密码3",
		"Key 4" : "密码4",
		"Lazy Mode" : "懒人模式",
		"NONE" : "不加密",
		"Network Name(SSID)" : "网络名称",
		"Power" : "开关",
		"Password" : "密码",
		"Repeater Mode" : "中继模式",
		"Security" : "安全选项",
		"TKIP" : "TKIP",
		"WDS Configuration" : "分布式无线配置",
		"WEP" : "WEP",
		"WPA-PSK" : "WPA-PSK",
		"WPA2-PSK" : "WPA2-PSK",
		// Wifi Repeter
		"AP Client mode" : "接入点客户端",
		"AP-Client Configuration Setup" : "接入点客户端配置",
		"APCli Function" : "接入点客户端功能",
		"Ascii Text" : "Ascii Text",
		"Authentication Mode" : "认证方式",
		"Security Mode" : "加密方式",
		"Wireless Network" : "无线网络",
		"Wire Equivalence Protection (WEP)" : "WEP",
		"Authentication Type" : "认证类型",
		"WEP Key Length" : "WEP密码长度",
		"WEP Key Entry Method" : "WEP接入方式",
		"WEP Key 1 :" : "WEP密码1",
		"WEP Key 2 :" : "WEP密码2",
		"WEP Key 3 :" : "WEP密码3",
		"WEP Key 4 :" : "WEP密码4",
		"Default Key" : "默认密码",
		"Frequency (Channel)" : "信道",
		"Hexadecimal" : "",
		"Open System" : "开放式",
		"Pass Phrase" : "密码",
		"Shared Key" : "共享密码",
		"WPA" : "WPA",
		"WPA Algorithms" : "WPA算法",
		"WPA-Personal" : "WPA-Personal",
		"WPA2-Personal" : "WPA2-Personal",
		"64 bit (10 hex digits/ 5 ascii keys)" : "64位",
		"128 bit (26 hex digits/13 ascii keys)" : "128位",
		// Setting
		"Administrator Settings":"管理员设置",
		"Firmware Upgrade" : "固件升级",
		"LAN Settings" : "局域网设置",
		"New Password" : "新密码",
		"Restore To Factory Default Settings" : "恢复出厂设置",
		"Reboot Router":"重启路由",
		"reboot" : "重启",
		"Configuring parameters, please wait.." : "正在配置参数，请稍候..",
		//wz
		"Welcome to the PandoraBox router" : "欢迎使用PandoraBox路由器",
		"Please read the 'User License Agreement' and select whether to continue!" : "请阅读《用户许可协议》并选择是否继续！",
		"Agree, continue" : "同意，继续",
		"Join the User Experience Improvement Program" : "加入用户体验改善计划",
		"You do not seem to have a network cable" : "您似乎没有连接网线",
		"Please select the operating mode of the router to continue" : "请选择路由器的工作模式继续",
		"Router operating mode(Create a new WiFi network)" : "路由器工作模式（创建新的WiFi网络）",
		"AP Client mode(Extend existing wireless network)" : "中继工作模式(扩展现有的无线网络)",
		"Enter the user name and password provided by your network operator (ISP)" : "请输入网络运营商(ISP)提供的用户名和密码",
		"The next step" : "下一步",
		"Finish" : "恭喜您，设置已完成，请点击重启，保存设置！",
		"Your choice of access to DHCP, do not need to modify the parameters, click Next!" : "您的选择的上网方式为DHCP，不需要修改参数，请点击下一步！",
		"Set static IP address" : "设置静态IP",
		"Previous" : "返回",
		"WAN IP Address" : "IP地址",
		"WAN Subnet Mask" : "子网掩码",
		"WAN Gateway" :"网关",
		"PPPoE Settings" : "PPPoE拨号设置",
		"User Account" : "用户名",
		"User Password" : "密码",
		"Service Name" : "服务器名",
		"Ralink Wireless Access Point" : "Pandorabox智能路由",
		//Network Status
		"Network Status" : "网络状态",
		"LAN" : "局域网",
		"WAN" : "广域网",
		"MAC Address" : "MAC地址",
		"IP Address" : "IP地址",
		"Subnet Mask" : "子网掩码",
		"DHCP Server" : "DHCP服务",
		"Connection" : "连接",
		"Default Gateway" : "默认网关"

	};

	var en_us = {
		// button
		"Apply" : "Apply",
		"Restore" : "Restore",
        "Continue" : "Continue",
		// Switch
		"Disable" : "Disable",
		"Enable" : "Enable",
		// Nav
		"Firmware" : "Firmware",
		"Home" : "Home",
		"About" : "About",
		"Internet" : "Internet",
		"Repeater" : "Repeater",
		"Reboot" : "Reboot",
		"Reset" : "Reset",
		"Next" : "Next",
		"Back" : "Back",
		"Exit" : "Exit",
		"Setting" : "Setting",
		"Wifi" : "Wifi",
		// Wan
		"DHCP" : "DHCP",
		"Confirm Password" : "Confirm Password",
		"Fixed" : "Fixed",
		"IP Address" : "IP Address",
		"ISP Gateway Address" : "ISP Gateway Address",
		"PPPoE" : "PPPoE",
		"Primary DNS Address" : "Primary DNS Address",
		"PPPoE User Name" : "PPPoE User Name",
		"PPPoE User Password" : "PPPoE User Password",
		"Subnet Mask" : "Subnet Mask",
		// Wifi
		"AutoSelect" : "AutoSelect",
		"AES" : "AES",
		"Bridge Mode" : "Bridge Mode",
		"BSSID" : "BSSID",
		"Key 1" : "Key 1",
		"Key 2" : "Key 2",
		"Key 3" : "Key 3",
		"Key 4" : "Key 4",
		"Lazy Mode" : "Lazy Mode",
		"NONE" : "NONE",
		"Network Name(SSID)" : "Network Name(SSID)",
		"Power" : "Power",
		"Password" : "Password",
		"Repeater Mode" : "Repeater Mode",
		"Security" : "Security",
		"TKIP" : "TKIP",
		"WDS Configuration" : "WDS Configuration",
		"WEP" : "WEP",
		"WPA-PSK" : "WPA-PSK",
		"WPA2-PSK" : "WPA2-PSK",
		// Wifi Repeter
		"AP Client mode" : "AP Client mode",
		"AP-Client Configuration Setup" : "AP-Client Configuration Setup",
		"APCli Function" : "APCli Function",
		"Ascii Text" : "Ascii Text",
		"Authentication Mode" : "Authentication Mode",
		"Security Mode" : "Security Mode",
		"Wireless Network" : "Wireless Network",
		"Wire Equivalence Protection (WEP)" : "Wire Equivalence Protection (WEP)",
		"Authentication Type" : "Authentication Type",
		"WEP Key Length" : "WEP Key Length",
		"WEP Key Entry Method" : "WEP Key Entry Method",
		"WEP Key 1 :" : "WEP Key 1 :",
		"WEP Key 2 :" : "WEP Key 2 :",
		"WEP Key 3 :" : "WEP Key 3 :",
		"WEP Key 4 :" : "WEP Key 4 :",
		"Default Key" : "Default Key",
		"Frequency (Channel)" : "Frequency (Channel)",
		"Hexadecimal" : "Hexadecimal",
		"Open System" : "Open System",
		"Pass Phrase" : "Pass Phrase",
		"Shared Key" : "Shared Key",
		"WPA" : "WPA",
		"WPA Algorithms" : "WPA Algorithms",
		"WPA-Personal" : "WPA-Personal",
		"WPA2-Personal" : "WPA2-Personal",
		"64 bit (10 hex digits/ 5 ascii keys)" : "64 bit (10 hex digits/ 5 ascii keys)",
		"128 bit (26 hex digits/13 ascii keys)" : "128 bit (26 hex digits/13 ascii keys)",
		// Setting
		"Administrator Settings":"Administrator Settings",
		"Firmware Upgrade" : "Firmware Upgrade",
		"LAN Settings" : "LAN Settings",
		"New Password" : "New Password",
		"Restore To Factory Default Settings": "Restore To Factory Default Settings",
		"Reboot Router" : "Reboot Router",
		//wz
		"Welcome to the Network Setup Wizard." : "Welcome to the Network Setup Wizard.",
		"The Network Setup will guide the three quick steps to networking." : "The Network Setup will guide the three quick steps to networking.",
		"Step 1. Set up router Internet access!" : "Step 1. Set up router Internet access!",
		"Step 2. Set Internet access parameters!" : "Step 2. Set Internet access parameters!",
		"Step 3. Restart router!" : "Step 3. Restart router!",
		"Select WAN Type" :　"Select Internet Type",
		"Dynamic IP Address" : "DHCP",
		"Static IP Address" : "Static IP Address",
		"PPPoE" : "PPPoE",
		"Your choice of access to DHCP, do not need to modify the parameters, click Next!" : "Your choice of access to DHCP, do not need to modify the parameters, click Next!",
		"Finish" : "Congratulations, the setup has been completed, please click restart, save the settings!",
		"Set static IP address" : "Set static IP address",
		"WAN IP Address" : "WAN IP Address",
		"WAN Subnet Mask" : "WAN Subnet Mask",
		"WAN Gateway" :"WAN Gateway",
		"PPPoE Settings" : "PPPoE Settings",
		"User Account" : "User Account",
		"User Password" : "User Password",
		"Service Name" : "Service Name",
		"Ralink Wireless Access Point" : "PandoraBox W1 Router",
		"FirGra" : "PandoraBox router system is based on the development of eCos secondary router system, which aims to provide the perfect solution with high reliability and stability of the smart device manufacturers and developers.<br>New web framework and rich feature of this system is a plug-in support, and easy to use guidance system enables your device to easily enjoy the experience of the past, not the same as control, especially for intelligent routing.<br>",
		"Team" : "PandoraBox team is created by the open source project team members DreamBox Lintel, consumer focus on providing network equipment system solutions.<br>The current team size of about 15, mainly in the OpenWrt related research and development, such as BSP software and hardware support, Linux network enhanced, DPI engines.<br>Business Cooperation：<br>ray@xcloud.cc<br>D-Team Technology Co,Ltd. ShenZhen(PandoraBox Team)<br>Address: Southern District Science and Technology Park, Nanshan District, Shenzhen High-tech four W1-A layer 612, Building 6",
		"fir" : "Firmware",
		"team" : "Team"
	};
	// Load Language File Which Choosen, @TODO
	var dict;	
	var Current_Sys_Language= "<% CGI_CFG_GET(CFG_SYS_Language); %>";
	if(Current_Sys_Language == 1)
			dict = zh_cn;

	else if(Current_Sys_Language == 0)
			dict = en_us;

	else if(Current_Sys_Language != 1 && Current_Sys_Language != 0)
			dict = en_us;
	// Get Tags Which Should Be Translated
	var translateEle = document.getElementsByTagName(tagName);
	for(var i = 0; i < translateEle.length; i++) {
		var item = translateEle[i];
		var EleKey = "innerHTML";
		if (tagName == "input")
			EleKey = "value";
		if (typeof item[EleKey] == 'string')
			if(item[EleKey].length > 0) {
				var valueObj = dict[item[EleKey]];
				if (typeof valueObj == 'string' && valueObj.length > 0)
					item[EleKey] = valueObj
			}
	}
	return;
}
function translate() {
	multiLang("a");
	multiLang("p");
	multiLang("h2");
	multiLang("h3");
	multiLang("h4");
	multiLang("td");
	multiLang("b");
	multiLang("span");
	multiLang("input");
	multiLang("label");
	multiLang("option");
	multiLang("title");
}

function SetChinese()
{	
	var f=document.lang;
	f.Selectd_Language.value = 1;
	form2Cfg(f);
	subForm(f,'do_cmd.htm','lang',cPage);
}
function SetEnglish()
{	
	var f=document.lang;
	f.Selectd_Language.value = 0;
	form2Cfg(f);
	subForm(f,'do_cmd.htm','lang',cPage);
}
