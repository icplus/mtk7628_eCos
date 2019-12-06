function selectItem(id,status){
	var elem = document.getElementById(id);
	var classVal = elem.getAttribute("class");
	if(status == 0){
		if(classVal.indexOf("item-selected") != -1)
			classVal = classVal.replace("item-selected","");
		else
			return;
	}else{
		if(classVal.indexOf("item-selected") != -1)
			return;
		else
			classVal = classVal.concat(" item-selected");
	}
	elem.setAttribute("class",classVal);
}
function displayDiv(id,status){
	var elem = document.getElementById(id);
	var classVal = elem.getAttribute("class");
	if(status == 1){
		if(classVal.indexOf("off") != -1)
			classVal = classVal.replace("off","");
		else
			return;
	}else{
		if(classVal.indexOf("off") != -1)
			return;
		else
			classVal = classVal.concat(" off");
	}
	elem.setAttribute("class",classVal);
}
function unDoAllSelect(){
	var items = document.getElementsByClassName("item");
	for(var i=1; i<items.length+1; i++){
		selectItem("item"+i,0);
		displayDiv("item"+i+"-page",0);
	}
}


function itemClickAction(elem){
	unDoAllSelect();
	var classVal = elem.getAttribute("class");
	classVal = classVal.concat(" item-selected");
	elem.setAttribute("class",classVal);
	var idVal = elem.getAttribute("id");
	displayDiv(idVal+"-page",1);
}

function initItemClickAction(){
	var items = document.getElementsByClassName("item");
	for(var i=0; i< items.length; i++){
		items.item(i).onclick = function(){
			itemClickAction(this);	
		};
	}
}


