function commonPageHead(pageIndex){
	var content = '<div class="pageHead">\
	<div class="navbar">\
        <ul class="navbar_nav">\
	 		<li><a href="extender.html">Extender</a></li>\
            <li><a href="ap.html">AP</a></li>\
      			<li><a href="wifisetting.html">Wifi Setting</a></li>\
			<li><a href="tools.html">Tools</a></li>\
        </ul>\
    </div>\
</div>';
	document.writeln(content);
	if(pageIndex >=0){
		document.getElementsByTagName("li").item(pageIndex).childNodes.item(0).style.color = "#FFF";
	}else{
		return;
	}
}
