var week=["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"];
var month=["January","February","March","April","May","Jun","July","August","September","October","November","December"];

function createSelectTagChild(start,end,selected,special_case)
{
	var str=new String("");

	for(var i=start; i <= end; i++)
	{
		switch (special_case)
		{
		    case "week":
		         if (i==selected)
		            str+="<option value="+i+" selected>"+week[i]+"</option>";
		         else
		            str+="<option value="+i+">"+week[i]+"</option>";
		         break;
		    case "month":
		         if (i==selected)
		            str+="<option value="+i+" selected>"+month[i-1]+"</option>";
		         else
		            str+="<option value="+i+">"+month[i-1]+"</option>";
		         break;
		    default:
		         if (i==selected)
		            str+="<option value="+i+" selected>"+i+"</option>";
		         else
		            str+="<option value="+i+">"+i+"</option>";
		         break;
		}
	}
	document.writeln(str);
}

function ProtToStr(type) {
	if (type=="tcp/udp")
		return "Both";
	else if(type=="tcp")
		return "TCP";
	else if(type=="udp")
		return "UDP";
	else if(type=="icmp")
		return "ICMP";
	else if(type=="")
		return "*";
	else
		return "" ;	
}

function ProtToIdx(prot, type) {
	for(i=0;i<prot.length;i++)
		if(prot[i] == type)
			return i;
	return 0 ;	
}

function IdxToProt(prot, idx) {
	return prot[idx];
}

function showSchedule(en,time,day)
{
    var str=new String("");
    
    switch (en)
    {
     case "1":
     	  var t= time.split("-");
     	  var hr = parseInt(t[0]/3600)%24;
     	  var mi=parseInt((t[0]-hr*3600)/60);
	  str+=(hr>=12?(hr-12):hr);
     	  if (mi<10)
		str+=":0"+mi;
	  else
	  	str+=":"+mi;
     	  str+=(hr>=12?"PM":"AM");
          str+= "~";
          hr = parseInt(t[1]/3600)%24;
     	  mi=parseInt((t[1]-hr*3600)/60);
     	  str+=(hr>=12?(hr-12):hr);
     	  if (mi<10)
		str+=":0"+mi;
	  else
	  	str+=":"+mi;
     	  str+=(hr>=12?"PM":"AM");
          str+= ", ";
          cb=day&1;
          sd=0;
          ed=6;
          for(i=0;i<7;i++)
          {
          	if(((day>>i)&1)  != cb) {
          		if(cb == 0) 
          			sd=i; 
          		else 
          			ed=i-1; 
          		cb=(day>>i)&1;
          	}
          }
          str+=week[sd]+"-"+week[ed];
          break;
     default:
          str+="Always";
          break;   
    }
    return str;
}

function setSchedule(f,en,time,day)
{
    if (en=="1")
        f.schd[1].checked = true;
    else
        f.schd[0].checked = true;
      
    var t= time.split("-");
    var hr = parseInt(t[0]/3600)%24;
    var mi=parseInt((t[0]-hr*3600)/60/5);
    f.hour1.selectedIndex = (hr >=12? hr-12:hr);
    f.min1.selectedIndex = mi;
    f.am1.selectedIndex = (hr>=12? 1:0);
    hr = parseInt(t[1]/3600)%24;
    mi=parseInt((t[1]-hr*3600)/60/5);
    f.hour2.selectedIndex = (hr >=12? hr-12:hr);
    f.min2.selectedIndex = mi;
    f.am2.selectedIndex = (hr>=12? 1:0);
    cb=day&1;
    sd=0;
    ed=6;
    for(i=0;i<7;i++)
    {
      	if(((day>>i)&1)  != cb) {
    		if(cb == 0) 
    			sd=i; 
          	else 
          		ed=i-1; 
          	cb=(day>>i)&1;
          }
    }
    f.day1.selectedIndex=sd;
    f.day2.selectedIndex=ed;
}

function Schedule2str(f)
{
	if(f.schd[1].checked)
    {
    st=(f.hour1.selectedIndex+12*f.am1.selectedIndex)*3600+(f.min1.selectedIndex)*5*60;
    et=(f.hour2.selectedIndex+12*f.am2.selectedIndex)*3600+(f.min2.selectedIndex)*5*60;
    if (et > 86400) et=86400;
    day=0;
    if(f.day2.selectedIndex<f.day1.selectedIndex)
    	cb=1;
    else
    	cb=0;
    for(k=0;k<7;k++)
    {
    	if (f.day2.selectedIndex+1 == k) 
    		cb=0;
        if (f.day1.selectedIndex == k) 
        	cb=1;
        day |= (cb<<k);
    }
    sched=st+"-"+ et+";"+day;
    }
    else
    {
    sched=";";
    }
    return sched;
}

function SelectRow(tn,row)
{
    if (SelRow != row)
    {
        var bar = document.getElementById(tn);
        if (SelRow >= 0)
            bar.rows[SelRow].style.backgroundColor = SelRowBC;
        if (row >= 0)
        {
            SelRowBC = bar.rows[row].style.backgroundColor;
            bar.rows[row].style.backgroundColor = "#FFFF00";
        }
        SelRow = row;
    }
}

// this function is used to check if the value "n" is a number or not.
function isNumber(n)
{
    if (n.length==0) return false;
    for (var i=0;i < n.length;i++)
    {
        if (n.charAt(i) < '0' || n.charAt(i) > '9') return false;
    }
    return true;
}

// this function is used to check if the inputted string is blank or not.
function isBlank(s)
{
    var i=0;
    for (i=0; i < s.length;i++)
    {
        if (s.charCodeAt(i)!=32) break;
    }
    if (i==s.length) return true; 
   
    return false;
}

function checkMAC(m)
{
    var allowChar="01234567890ABCEDF";
    
    m=m.toUpperCase();
    for (var i=0; i < m.length; i++)
    {
        if (allowChar.indexOf(m.charAt(i)) == -1) return false;
    }
    if (m.length==0) return false;
    return true;
}

function isIP(ip)
{
   var sIP=ip.split(".");
   if (sIP.length!=4) return false;
   for(var i=0; i< sIP.length; i++)
   {
      if (!isNumber(sIP[i])) return false;
      if (parseInt(sIP[i]) < 0 || parseInt(sIP[i]) > 255) return false;
   }
   
   return true;
}

function validIP(ip)
{
   var sIP=ip.split(".");
   if (sIP.length!=4) return false;
   for(var i=0; i< sIP.length; i++)
   {
      if (!isNumber(sIP[i])) return false;
      if (parseInt(sIP[i]) < 0 || parseInt(sIP[i]) > 255) return false;
   }

   if (sIP[0]==0 || sIP[0]==255) return false;
   if (sIP[3]==0 || sIP[3]==255) return false;
   
   return true;	
}

function validMaskIP(ip)
{
   var sIP=ip.split(".");
   if (sIP.length!=4) return false;
   for(var i=0; i< sIP.length; i++)
   {
      if (!isNumber(sIP[i])) return false;
      if (parseInt(sIP[i]) < 0 || parseInt(sIP[i]) > 255) return false;
   }

   if (sIP[0]==0 || sIP[0]==255) return false;
   if (sIP[3]==255) return false;
   
   return true;	
}

function evalIP(ip)
{
   var sIP=ip.split(".");

   for(var i=0; i< sIP.length; i++)
      sIP[i]=parseInt(sIP[i],10);
  
   dIP = sIP[0]+"."+sIP[1]+"."+sIP[2]+"."+sIP[3];
   return dIP;	
}

function validPort(port)
{
    var tmp_port=parseInt(port);
	if (tmp_port<=65535 && tmp_port >0) return true;
	
	alert("Port must be between 1 and 65535!");
		
	return false;	
}

function isSameClass(ip1,ip2,mask)
{
   var test1=ip1.split(".");
   var test2=ip2.split(".");
   var myMask=mask.split(".");
   
   if (test1.length!=4 || test2.length!=4 || myMask.length!=4)
   {
     //alert("無效的IP或子網路遮罩!");
     return false;
   }
   
   for(var i=0; i<=3; i++)   
   {
      if ( (test1[i]&myMask[i])!=(test2[i]&myMask[i]) )
        return false;
   }
   return true;
}

function ipCmp(ip1,ip2)
{
	var i1=ip1.split(".");
	var i2=ip2.split(".");
	var a,b;
	if (i1.length!=4 || i1.length!=4) return NaN;
	for(var k=0; k<4; k++)
	{
		a=Number(i1[k]);
		b=Number(i2[k]);
      	if ( a!=b ) { return (a-b); }
	}
    return 0; //eq
}

function chkIpRange(s,e,m,any)
{
	if ((s+e)=="") { alert("Invalid "+m+" IP address range!"); return false;}
if (any=='*')
{
	if(((s=="*")&&(e!="")&&(e!="*"))||
		((e=="*")&&(s!="")&&(s!="*")))
	{
		alert("Invalid "+m+" IP address range!");
		return false;
	}
	else if((s+e)=="*"||(s+e)=="**")
		return true;
}
	//if((s!="") && (s!="*"))
	if(s!="")
	{
		if (!validMaskIP(s))
		{
			alert("Invalid "+m+" starting IP address!");
			return false;
		}
	}
    if(e && ipCmp(s,e)>0)
	{
		alert(m+" the starting IP address of the available IP address pool should be less than or equal to the ending IP address.");
		return false;
	} 
	if ((e!="") && (e!="*"))
	{
		if (!validMaskIP(e))
		{
			alert("Invalid "+m+" ending IP address!");
			return false;
		}
	}
	return true;
}

function chkPortRange(s,e,m,any)
{
	var t=(s+e);
if (any=='*')
{
	if (t=='*'||t=='**') return 1;
}
	if (t=='') {  alert("Invalid "+m+" port range!"); return 0; }
	var sn=Number(s);
	var en=Number(e);
	var sv=(!isNaN(sn) && (sn>0 && sn<=65535));
	var ev=(!isNaN(en) && (en>0 && en<=65535));

	if (sv && e=='') return 1;
	if (ev && s=='') return 1;
	if (sv && ev )
	{
		if (sn>en) { alert(m+" the starting port should be less than or equal to the ending port."); return 0; }
		else return 1;
	}

	if (isNaN(sn)) { alert("Invalid "+m+" starting port!"); return 0; }
	if (isNaN(en)) { alert("Invalid "+m+" ending port!"); return 0; }
	if ((sn <=0 || sn>65535) || (en <=0 || en>65535))
	{
		alert(m+" port must be between 1 and 65535!");
		return 0;
	}
	alert("Invalid "+m+" port range!");
	return 0;
}
function ipRange(s,e)
{
	if (s=="*"||e=="*") return "";
	if (e) return (evalIP(s)+"-"+evalIP(e));
	return evalIP(s);
}
function portRange(s,e)
{
    if (s=="*"||e=="*") return "";
	if (s&&e) return (Number(s)+"-"+Number(e));
	if (s=='') return Number(e);
	return Number(s);
}

function dateStrC(sec)
{
	var d=new Date(1000*sec);
	d.setTime(d.getTime()+d.getTimezoneOffset()*60000);
	var str=d.getFullYear()+' Year '+(1+d.getMonth())+' Month '+d.getDate()+' Day '+ week[d.getDay()];
	return str;
}

function chkPortRangeFmt(str,m)
{
	var s=str.split(",");
	var flag=true;
	for (i=0; i < s.length; i++)
	{
		if ((s[i].indexOf("-"))!=-1)
		{
			var p=s[i].split("-");
			if (p.length!=2) { flag=false; break;}
			for (var j=0; j <2; j++)
				if (!validPort(p[j])) return false;
			if(Number(p[0])>Number(p[1])) { flag=false; break;}
		}
		else
			if (!validPort(s[i])) return false;
	}
	if (!flag) alert(m);
	return flag;
}

function chkPortRange2(port,range)
{
	var r=range.split(",");
	var pn=Number(port);

	for (i=0; i < r.length; i++)
	{
		if ((r[i].indexOf("-"))!=-1)
		{ // "start-end" pair
			var p=r[i].split("-");
			if (!(pn<Number(p[0]) || pn>Number(p[1]))) return false;
		}
		else
		{ // "single"
			if (pn==Number(r[i])) return false;
		}
	}
	return true;
}
function chksc(n, msg)
{
	var ck=/[\;]/;
	if (ck.test(n))
	{
		alert(msg+" contains an invalid symbol: \;");
		return false;
	}
	return true;
}

function chksc(n, msg)
{
	var ck=/[\;:~`!?@#$%^&*=+|*,><\\\/{}\[\]\'"_]/;
	if (ck.test(n))
	{
		alert(msg+" the following characters are not supported: \;:~`!?@#$%^&*=+|*,></{}[]\'\"_");
		return false;
	}
	return true;
}


function chken(s, msg)
{
    var i=0;
    var val=0;
    for (i=0; i < s.length;i++)
    {
        val = s.charCodeAt(i);
        if ((val<47) || ((val>57)&&(val<65)) || ((val>90)&&(val<97)) || (val>122))
        {
        	if((val==45) || (val==95))
        	{
        		//return true;
        	}
        	else
        	{
			alert(msg+" please do not enter characters, only accept [A~Z][a~z][0~9][-][_]");
			return false;        	
		}
        }
        
    }
    return true;	
}
