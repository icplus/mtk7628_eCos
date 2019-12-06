function setSecurity(SecuritySel){
	/*alert(SecuritySel);*/
	if(SecuritySel != null)
	{
		if(SecuritySel.indexOf("NONE") != -1){
			setCfg("apcliauth2","OPEN");
			setCfg("apcliencryptype2","NONE");
			return;
		}else {
			var authStr = new Array();
			authStr = SecuritySel.split('/');
			if(authStr.length == 2){
				if(authStr[0].indexOf("1") != -1){
					if(authStr[0].indexOf("WPA2PSK") != -1){
						setCfg("AuthMode","WPA2PSK");
						setCfg("apcliauth2","WPA2PSK");	
					}else{
						setCfg("AuthMode","WPAPSK");
						setCfg("apcliauth2","WPAPSK");
					}
				}else{
					setCfg("AuthMode",authStr[0]);
					setCfg("apcliauth2",authStr[0]);
				}
				//alert(authStr[0]);
				//alert(authStr[1]);
				setCfg("apcliencryptype2","AES");
				setCfg("EncrypType","AES");
			}
		}
	} else {
		setCfg("apcliauth2","");
		setCfg("apcliencryptype2","");
		alert("wifi信息获取错误！请刷新重试！");
		/*一些别的操作...重新获取wifi扫描信息等*/
	}
}

function trim(str){   
    str = str.replace(/^(\s|\u00A0)+/,'');   
    for(var i=str.length-1; i>=0; i--){   
        if(/\S/.test(str.charAt(i))){   
            str = str.substring(0, i+1);   
            break;   
        }   
    }   
    return str;   
}

function displayElem(id,status){
	var elem = document.getElementById(id);
	var classVal = elem.getAttribute("class");
	if(status == 1){
		if(classVal.indexOf("off") != -1)
		{
			classVal = classVal.replace("off","");
			elem.style.visibility = "visible";
			elem.style.display = "";
		}
		else
			return;
	}else{
		if(classVal.indexOf("off") != -1)
			return;
		else
		{
			classVal = classVal.concat(" off");
			elem.style.visibility = "hidden";
			elem.style.display = "none";
		}
	}
	elem.setAttribute("class",classVal);
}

function displayList(){
	var wifiList = document.getElementById("div-wifiList");
	if(wifiList.getAttribute("class").indexOf("off") != -1){
		/*显示wifi列表*/
		displayElem("div-wifiList",1);
		/*隐藏密码框*/
		displayElem("ex-passwd",0);
		/*隐藏提交按钮*/
		//displayElem("btn_submit",0);
		//alert("no off");
		
	}else{
		/*隐藏wifi列表*/
		displayElem("div-wifiList",0);
		/*显示密码框*/
		displayElem("ex-passwd",1);
		/*显示和密码框*/
		//displayElem("btn_submit",1);
	}
}
function reLoad(){
	window.location.reload();
}

/*点击处理函数*/
function onSel(li){
	var ssid = li.getAttribute("data-ssid");/*获取ssid*/
	var ch = li.getAttribute("data-ch");/*获取信道*/
	var security = li.getAttribute("data-security");/*获取加密模式*/
	/*修改cfg中的值*/
	setCfg("apcli_ssid",ssid);//上级路由ssid
	setCfg("ssid",ssid+"-Plus");//设置ssid
	/*设置信道值*/
	setCfg("sz11gChannel",ch);
	/*设置安全模式*/
	setSecurity(security);
	
	var txt_ssid = document.getElementById("txt-ssid");
	txt_ssid.value = ssid;
	var wifiList = document.getElementById("div-wifiList");
	/*隐藏div-wifiList*/
	displayElem("div-wifiList",0);
	/*显示 item-line*/
	//displayElem("item-line",1);
	/*隐藏刷新按钮 显示提交按钮和密码框*/
	//displayElem("btn_refresh",0);
	/*显示密码框*/
	displayElem("ex-passwd",1);
	/*显示和提交按钮*/
	displayElem("btn_submit",1);
}
function arrSort(array){
	var num = array.length/5;
	var i=0,j=0,k=0;
	for(i=0; i<num-1; i++){
		for(j=0;j<num-1-i;j++){
			if(parseInt(array[j*5+4]) < parseInt(array[(j+1)*5+4])){
				for(k=0;k<5;k++){
					var temp = array[j*5+k];
					array[j*5+k] = array[(j+1)*5+k];
					array[(j+1)*5+k] = temp;
				}
			}
		}
	}
	return array;
}
