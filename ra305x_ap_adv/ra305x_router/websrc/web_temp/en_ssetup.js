var week=["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Always"];

var month=["January","February","March","April","May","Jun","July","August","September","October","November","December"];

var date=new Array(" year"," month"," day ")

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

var Alerts=new Array(
"Invalid"
,"!"
,"IP Address"
,"Subnet Mask"
,"Default Gateway"
,"Primary DNS Address"
,"User Name"
,"Are you sure you want to change the password?"
,"Invalid Name"
,"name"
,"the following characters are not supported: ;:~`!?@#$%^&*=+|*,></{}[]\'\"_"
,"Invalid Private IP Address"
,"The input IP address should be LAN IP."
,"The IP address you entered is the IP address of the system."
,"Invalid Public Port"
,"The ending public port number should be larger than the starting public port number."
,"Invalid Private Port"
,"Please enter the name"
,"trigger"
," port range!"
," the starting port should be less than or equal to the ending port."
," starting port!"
," ending port!"
," port must be between 1 and 65535!"
,"Valid public port format: \n\n1.Single figure, ex. 1024\t\n2.A series of numbers, ex. 103,163,55,...\n3.Range of number, ex. 103-157\n4.Mix, ex. 103,106-1333,154"
,"This special application already exists."
,"The public port has been used by the virtual server "
,"."
,"There can be at most "
," information in special applications list. \n\nPlease delete unnecessary data."
,"Are you sure you want to delete this item?"
,"This item has been set."
,"Invalid Scheduling Range"
,"There can be at most "
," information in IP filter list. \n\nPlease delete unnecessary data."
,"MAC Name"
,"Invalid MAC Name"
,"MAC Address"
,"Invalid! Must be unicast address."
,"MAC address must be completed."
,"Repeated MAC address."
,"There can be at most "
," information in MAC filter list. \n\nPlease delete unnecessary data."
,"Keywords must be completed."
,"There can be at most "
," information in URL blocking list. \n\nPlease delete unnecessary data."
,"This keyword already exists."
,"Blocked domain and permitted domain should not be blank at the same time."
,"Blocked Domains"
,"This blocked domain has been established."
,"Permitted Domains"
,"This permitted domain has been established."
,"There can be at most "
," information in blocked domains. \n\nPlease delete unnecessary data."
,"There can be at most "
," information in permitted domains. \n\nPlease delete unnecessary data."
,"There can be at most "
," information in virtual server list. \n\nPlease delete unnecessary data."
," IP address range!"
," starting IP address!"
," the starting IP address of the available IP address pool should be less than or equal to the ending IP address."
," ending IP address!"
,"Source IP "
,"The starting source IP address you entered should be LAN IP."
,"Destination IP"
,"The starting destination IP address you entered should be LAN IP."
,"LAN to LAN is invalid."
,"WAN to WAN is invalid."
,"There can be at most "
," information in firewall list. \n\nPlease delete unnecessary data."
,"Host Name cannot be empty!"
,"User Name cannot be empty!"
,"Password cannot be empty!"
,"Password"
,"DDNS does not enable."
,"User Name"
,"IP Address"
,"Password Length Out of Range!"
,"Load Settings From Local Hard Drive"
,"Please select the file!"
,"Restore To Factory Default Settings"
,"Are you sure you want to upgrade?"
,"The host name or IP address cannot be empty."
,"Are you sure you want to restart?"
,"Invalid Remote Syslog Server!"
,"Remote Syslog Server"
,"Invalid SMTP Server!"
,"Invalid E-mail Address!"
)

var Allow="Deny";
var Deny="Allow";

var Button_txt=['Next','Back','Exit','Help','Reboot','Finish','Apply','Cancel','Continue'];
var topmenu=['home','advanced','tools','status','help','home'];
var leftmenu=['wizard','Virtual Server','Applications','Filters','Firewall','Admin','Time','System','Firmware','Misc'
,'Device info','Log','Stats'];

var wizard = new Array(
"Welcome to the Network Setup Wizard."
,"The Network Setup will guide the four quick steps to networking. Please click Next."
,"Step 1. Set up an Internet connection"
,"Step 2. Restart"
,"Step 3. Firmware update"
,"Select WAN Type"
,"Select the connection method to connect to your ISP. Click Next to continue."
,"Dynamic IP Address"
,"Obtain an IP address from your ISP."
,"Static IP Address"
,"Set static IP information provided to you by your ISP."
,"PPPoE"
,"Choose this option if your ISP uses PPPoE."
,"Set dynamic IP address"
,"Please enter a particular host name or host-specific MAC addresses.Use the button to copy the MAC address of the Ethernet Card and replace the WAN MAC address with the MAC address of the router. Click Next to continue."
,"Host Name"
,"(Optional)"
,"MAC Address"
,"(Optional)"
,"Finish"
,"Click Back to back to previous steps to change the settings after the completion of the Connection Wizard. Click Restart to save the current settings and restart Ralink Wireless Access Point."
,"Set static IP address"
,"Set static IP information provided to you by your ISP. Click Next to continue."
,"WAN IP Address"
,"WAN Subnet Mask"
,"WAN Gateway"
,"Primary DNS Address"
,"Secondary DNS Address"
,"Finish"
,"Click Back to back to previous steps to change the settings after the completion of the Connection Wizard. Click Restart to save the current settings and restart Ralink Wireless Access Point."
,"PPPoE Settings"
,"The service name is optional, but if your ISP requires this information, please enter the appropriate value. Click Next to continue."
,"User Account"
,"User Password"
,"Confirm Password"
,"Service Name"
,"(Optional)"
,"Finish"
,"Click "
,"Back"
,"to back to previous steps to change the settings after the completion of the Connection Wizard. Click Restart to save the current settings and "
,"restart" 
,"Ralink Wireless Access Point."
,"Clone MAC Address"
,"Restore To Factory MAC Address"
,"(Optional)"
,"Please enter the settings for PPTP. Click"
,"Next"
,"to continue."
,"IP Address"
,"Subnet Mask"
,"Server IP address"
,"User Account"
,"User Password"
,"Confirm Password"
,"Please enter the settings for L2TP. Click"
,"Next"
,"to continue."
,"IP Address"
,"Subnet Mask"
,"Server IP address"
,"User Account"
,"User Password"
,"Confirm Password"
,""
)

var wz_save=new Array(
"Restart!"
,"Only administrators can change settings"
,"Error!"
)

var wz_fw=new Array(
"To enhance the features of Ralink Wireless Access Point, we recommend that you download the newest firmware, the following is the download link."
,"The download URL for the newest firmware"
)

var wanst=new Array(
"WAN Settings"
,"Dynamic IP Address"
,"Obtain an IP address automatically from your ISP."
,"Static IP Address"
,"Set static IP information provided to you by your ISP."
,"PPPoE"
,"Choose this option if your ISP uses PPPoE."
,"Dynamic IP Address"
,"Host Name"
,"(Optional)"
,"MAC Address"
,"(Optional)"
,"Restore To Factory MAC Address"
,"Clone MAC Address"
,"Primary DNS Address"
,"Secondary DNS Address"
,"(Optional)"
,"MTU"
,"Automatic Reconnection"
,"Enable"
,"Disable"
,"Static IP Address"
,"IP Address"
,"Subnet Mask"
,"ISP Gateway Address"
,"Dynamic PPPoE"
,"Static PPPoE "
,"PPPoE User Name"
,"PPPoE User Password"
,"Confirm Password"
,"Service Name"
,"Maximum Idle Time"
,"Min"
,"Connection mode"
,"Auto connection"
,"Manual connection"
,"Connection in use "
,"PPTP Client"
,"Dynamic IP"
,"Static IP "
,"Gateway"
,"Server IP Address/Name"
,"PPTP User Name"
,"PPTP User Password"
,"L2TP User Name"
,"L2TP User Password"
,"Sec"
,"L2TP Client"
)

var lanst=new Array(
"LAN Settings"
,"IP Address"
,"Subnet Mask"
,"Domain Name"
,"DNS Relay"
,"Enable"
,"Disable"
,"(Optional)"
)

var dhcpst= new Array(
"DHCP Server"
,"DHCP Server"
,"Enable"
,"Disable"
,"Starting IP Address"
,"Ending IP Address"
,"Lease Time"
,"1 Hour"
,"2 Hours"
,"3 Hours"
,"1 Day"
,"2 Days"
,"3 Days"
,"1 Week"
,"Static DHCP"
,"Enable"
,"Disable"
,"Host Name"
,"IP Address"
,"MAC Address"
,"DHCP Client"
,"Static DHCP Client Table"
,"(Lots/Total Lots)"
,"Host Name"
,"IP Address"
,"MAC Address"
,"Dynamic DHCP Client Table"
,"Host Name"
,"IP Address"
,"MAC Address"
,"Expired Time"
,"(Lots/Total Lots)"
,"Clone"
)

var wlanst=new Array(
"WLAN Status"
,"System Info"
,"Wireless"
,"Other Information"
)

var adv_virtual=new Array(
"Virtual Server"
,"Enable"
,"Disable"
,"Name"
,"Private IP Address"
,"Protocol Type"
,"Public Port"
,"Private Port"
,"Schedule"
,"Always"
,"From"
,"time "
,"to "
,"day "
,"to "
,"Virtual Server List"
,"Name"
,"Private IP Address"
,"Protocol"
,"Schedule"
,"(Lots/Total Lots)"
)


var adv_appl=new Array(
"Special Applications"
,"Enable"
,"Disable"
,"Name"
,"Trigger Port"
,"Trigger Type"
,"Public Port"
,"Public Type"
,"Special Applications List"
,"Name"
,"Trigger"
,"Public Port"
,"(Lots/Total Lots)"
)

var adv_filters_ip=new Array(
"Filters"
,"IP Filters"
,"MAC Filters"
,"URL Blocking"
,"Domain Blocking"
,"IP Filters"
,"Enable"
,"Disable"
,"IP Address"
,"Port"
,"Protocol Type"
,"Schedule"
,"Always"
,"From"
,"time "
,"to "
,"day "
,"to "
,"IP Filter List"
,"IP Range"
,"Protocol"
,"Schedule"
,"(Lots/Total Lots)"
)

var adv_filters_mac=new Array(
"Filters"
,"Filters are used to allow or deny LAN users from accessing the Internet."
,"IP Filters"
,"MAC Filters"
,"URL Blocking"
,"Domain Blocking"
,"MAC Filters"
,"Use MAC address to allow or deny computers access to the network."
,"Disable MAC Filters"
,"Only"
,"allow"
,"computers with MAC address listed below to access the network"
,"Only"
,"deny"
,"computers with MAC address listed below to access the network"
,"MAC Name"
,"MAC Address"
,"DHCP Client"
,"MAC Filter List"
,"MAC Name"
,"MAC Address"
,"(Lots/Total Lots)"
)

var adv_filters_url=new Array(
"Filters"
,"Filters are used to allow or deny LAN users from accessing the Internet."
,"IP Filters"
,"MAC Filters"
,"URL Blocking"
,"Domain Blocking"
,"URL Blocking"
,"Block those URLs which contain keywords listed below."
,"Enable"
,"Disable"
,"(Lots/Total Lots)"
)

var adv_filters_domain=new Array(
"Filters"
,"Filters are used to allow or deny LAN users from accessing the Internet."
,"IP Filters"
,"MAC Filters"
,"URL Blocking"
,"Domain Blocking"
,"Domain Blocking"
,"Disable"
,"Allow "
,"users to access all domains except \"Blocked Domains\""
,"Deny "
,"sers to access all domains except \"Permitted Domains\""
,"Blocked Domains"
,"Permitted Domains"
,"(Lots/Total Lots)"
)

var adv_firewall=new Array(
"Firewall"
,"Enable"
,"Disable"
,"Name"
,"Action"
,"Allow "
,"Deny"
,"Interface"
,"IP Range"
,"Protocol"
,"Port Range"
,"Source"
,"Destination"
,"Schedule"
,"Always"
,"From"
,"time "
,"to "
,"day "
,"to "
,"Firewall List"
,"Action"
,"Name"
,"Source"
,"Destination"
,"Protocol"
,"(Lots/Total Lots)"
,"clear"
)

var adv_ddns=new Array(
"Dynamic DNS"
,"DDNS"
,"Enable"
,"Disable"
,"Server Provider"
,"Host Name"
,"User Name / E-Mail Address"
,"Password"
,"Connection Test"
,"Refresh"
)

var adv_dmz=new Array(
"DMZ"
,"Enable"
,"Disable"
,"IP Address"
)

var admin=new Array(
"Administrator Settings"
,"Administrator (The Login Name is \"admin\") "
,"New Password"
,"Comfirm Password" 
,"Remote Management"
,"Enable"
,"Disable"
,"IP Address"
,"Port"
)

var time=new Array(
"Time"
,"Local Time :"
,"Set the system time :"
,"Enable NTP"
,"Your Computer"
,"Manual Setting"
,"Time Zone"
,"Daylight Saving"
,"Enable"
,"Disable"
,"Set the Time automatically from NTP Server"
,"Default NTP Server"
,"(Optional)"
,"Set the Time"
,"Year"
,"Month"
,"Day"
,"Hour"
,"Minute"
,"Second"
)

var config=new Array(
"System Settings"
,"Save Settings To Local Hard Drive"
,"Load Settings From Local Hard Drive"
,"Load"
,"Restore To Factory Default Settings"
,"Restore"
)

var firmware=new Array(
"Firmware Upgrade"
,"Attention!!! During firmware updates, the power cannot be turned off. The system will restart automatically after completing the upgrade."
,"Current Firmware Version: "
,"Firmware Date:"
)

var misc=new Array(
"Ping test"
,"Host Name or IP Address"
,"Ping"
,"Reboot Device"
,"Reboot"
,"Block WAN Ping"
,"Discard PING from WAN side"
,"Enable","Disable"
,"UPnP Settings"
,"Enable","Disable"
,"Enable","Disable"
,"Enable","Disable"
,"Enable","Disable"
,"Non-Standard FTP Port"
,"Access Non-Standard FTP Port"
,"Port:"
)

var info=new Array(
"Device Information"
,"Day"
,"Local Time: "
,"Firmware Version: "
,"LAN"
,"MAC Address"
,"IP Address"
,"Subnet Mask"
,"DHCP Server"
,"Disable"
,"Enable"
,"WAN"
,"MAC Address"
,"Connection"
,"Subnet Mask"
,"DNS"
,"DHCP Release"
,"DHCP Renew"
,"Connect"
,"Disconnect"
)
var typeStr=["" ,"Static IP","Dynamic IP","PPPoE","PPTP","L2TP"]
var statStr=["Disconnect","connected","connecting"];

var log=new Array(
"View Log"
,"View Log displays the activities. Click on Log Settings for advance features"
,"First Page"
,"Last Page"
,"Previous"
,"Next"
,"Clear"
,"Log Settings"
,"Refresh"
,"Time"
,"Message"
)

var log_settings=new Array(
"Log Settings"
,"Logs can be saved by sending it to an admin email address or a storage server."
,"Remote Syslog Server"
,"Remote Syslog Server / IP Address"
,"Enable"
,"Disable"
,"E-Mail Notification"
,"SMTP Server / IP Address"
,"E-mail Address"
,"Send Mail Now"
,"Save Logs To Local Hard Drive"
,"Save"
,"Log Type"
,"System Activity"
,"Debug Information"
,"Attacks"
,"Dropped Packets"
,"Notice"
)

var stats=new Array(
"Traffic Statistics"
,"Refresh"
,"Reset"
,"Receive"
,"Transmit"
,"Packets"
)

var help=new Array(
"Home"
,"Wizard"
,"WAN"
,"LAN"
,"DHCP"
,"Wireless"
,"Advanced"
,"Virtual Server"
,"Applications"
,"Filters"
,"Firewall"
,"Tools"
,"Admin"
,"Time"
,"System"
,"Firmware"
,"Misc."
,"Status"
,"Device info"
,"Log"
,"Stats"
)

var help_home=new Array(
"Home"
,"Setup Wizard"
,"The setup wizard will guide you to configure the Ralink Wireless Access Point to connect to your ISP."
,"WAN Settings"
,"Please select the appropriate option to connect to your ISP."
,"LAN Settings"
,"These are the IP settings of the LAN interface for the Ralink Wireless Access Point. "
,"DHCP Server"
,"The information of the DHCP clients connecting to this device will be displayed in the DHCP client list. "
,"Wireless Status"
,"Show the status of wireless."
)

var help_adv=new Array(
"Advanced"
,"Virtual Server"
,"The Ralink Wireless Access Point can be configured as a virtual server so that remote users accessing Web or FTP services via the public IP address can be automatically redirected to local servers in LAN."
,"Special Applications"
,"Special applications are used to run applications that require multiple connections, such as online games, video conferencing and Internet telephony ..."
,"Filters"
,"Filters are used to deny or allow LAN computers from accessing the Internet."
,"Firewall"
,"Firewall is used to deny or allow traffic from passing through the system. "
,"DDNS"
,"Dynamic Domain Name System is a method of keeping a domain name linked to a changing IP Address. "
,"DMZ"
,"DMZ is used to allow a single computer that sits between a trusted internal network, such as a corporate private LAN, and an untrusted external network, such as the public Internet. "
,"Wi-Fi Basic Setup"
,"Opens a page where you can configure the minimum number of Wireless settings for communication, such as the Network Name (SSID) and Channel. The Access Point can be set simply with only the minimum setting items. "
,"Wi-Fi Advanced Setup"
,"Use the Advanced Setup page to make detailed settings for the Wireless. Advanced Setup includes items that are not available from the Basic Setup page, such as Beacon Interval, Control Tx Rates and Basic Data Rates. "
,"Wi-Fi Block Ack Setup"
,"Enable the Block Ack scheme to improve the MAC efficiency."
,"Wi-Fi Security Setup"
,"The AP supports four different types of security settings for your network. Wi-Fi Protected Access (WPA) Pre-Shared key, WPA Remote Access Dial In User Service (RADIUS), 802.1X, and Wire Equivalence Protection (WEP). "
,"WPS Configuration Setup"
,"The AP supports Simple Config settings for your wireless network to establish a secure wireless home network easily. "
,"WPS Device Configure"
,"Select PIN(Personal Identification Number) or PBC(Push Button Configuration) to configure WPS. "
,"WPS Reset Configuration"
,"Restore to factory default settings."
,"AP-Client Configuration Setup"
,"Opens a page where you can configure the Wireless AP-Client settings for communication, such as the Network Name (SSID). The Access Point can be set simply with only the minimum setting items."
)

var help_tools=new Array(
"Tools"
,"Admin"
,"The management interface for accessing the web browser."
,"Time"
,"Set the Time Zone appropriate for your area."
,"System"
,"The current system settings can be saved as a file into the local hard drive. "
,"Firmware"
,"Click on \"Browse\" to browse the local hard drive and locate the firmware to be used for the update. "
,"Misc"
,"Additional features and settings."
)

var help_status=new Array(
"Status"
,"Device info"
,"Display the current information of LAN and WAN."
,"Log"
,"The Ralink Wireless Access Point keeps a running log of events and activities occurring on the router."
,"Stats"
,"Display the traffic statistics. "
)

var err_msg = ["Error","Saving Error","Out of Range", "Ping Result: Timeout","Only administrators can change settings","Invalid File"];
var ok_msg=["Success", "Save Settings",	 "Ping Results: Alive", "Reboot","E-mail has been sent","Upgrade is complete","Upgrade is complete","Connecting" ];
var do_cmd=new Array(
"Success"
,"Error"
,"Update.."
)
