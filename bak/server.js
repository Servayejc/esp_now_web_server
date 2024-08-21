var webSocket = null;

var Peers = {};
var dataTypes = {};
var HTMLs = {}; 

function start(){
	//alert('start');
  	
	openWSConnection();
	setInterval(check, 5000);
}

$(document).ready(function(){
  $.getJSON("Struct.json", function(struct){
	var Peers  = struct["Struct"][0]["Peers"];
	var DataTypes  = struct["Struct"][1]["DataTypes"];
	for (p = 0; p < Peers.length; p++) {
		var Devices = Peers[p]["Devices"];
		for (d = 0; d < Devices.length; d++){
			typeNdx = Devices[d]["Type"];
			suffix = "_" + Peers[p]["PeerID"] + "_" + Devices[d]["ID"];
			//alert(suffix);
			
			var Title =  Devices[d]["Title"];
			DataType = DataTypes[typeNdx];
			var Title =  Devices[d]["Title"];
			Obj = struct["Struct"][1]["DataTypes"][0];
			HTML = struct["Struct"][2]["HTMLs"][typeNdx];
			s = HTML["HTML"];	
			let Keys = Object.keys(DataType); 
			//let Values = Object.values(DataType);
			for (var key in Keys) {
	  			oldKey = "$"+Keys[key];
				newKey = Keys[key]+suffix;
				s = s.replace(oldKey,newKey);
			}
						
			//alert(s);
			var parent = document.getElementById("cards");
			t = document.createElement("div");
			t.setAttribute("class","card thermostat");
			t.classList.add("S_"+Peers[p]["PeerID"]);
			t.innerHTML = s;
			parent.appendChild(t);
			
			el = document.getElementById("Title"+suffix);
			el.innerHTML = Title;
		}
	}
	//alert("fin");
	
  }).fail(function(){
      console.log("An error has occurred.");
  });
}); 

function findTarget(ev, ClassName){
	var result;
	var el = document.getElementById(ev.id).parentNode;
   	var children = [].slice.call(el.getElementsByTagName('*'),0);
	var idsAndClasses = children.map(function(child) {
		if (child.className == ClassName){ 
			//console.log(child.innerHTML);
			result = child;
			
		}	
	});
	
	//console.log(result.innerHTML);
	return result;
}

function OnBtnUp(ev){
	var el = document.getElementById(ev.id).parentNode;
   	var children = [].slice.call(el.getElementsByTagName('*'),0);
	var idsAndClasses = children.map(function(child) {
		if (child.className == "UpDown"){ 
			value = child.innerHTML; 
			value++;
			child.innerHTML = value;
			sendAsJson(child,value);
		}	
	});
}

function OnBtnDown(ev){
	var el = document.getElementById(ev.id).parentNode;
   	var children = [].slice.call(el.getElementsByTagName('*'),0);
	var idsAndClasses = children.map(function(child) {
		
		if (child.className == "UpDown"){ 
			value = child.innerHTML; 
			value--;
			child.innerHTML = value;
			sendAsJson(child,value);
		}	
	});
}

function OnBtnOn(ev){
	//console.log(ev.id);
    var el = document.getElementById(ev.id).parentNode;
	var children = [].slice.call(el.getElementsByTagName('*'),0);
	var idsAndClasses = children.map(function(child) {
		//console.log(child.id);
		if (child.className == "status"){ 
			
			child.innerHTML = 1;
			//console.log(child.id);
			updateStatuses();
			sendAsJson(child,1);
		}	
	});
}

function OnBtnOff(ev){
	//console.log(ev.id);
	var el = document.getElementById(ev.id).parentNode;
   	var children = [].slice.call(el.getElementsByTagName('*'),0);
	var idsAndClasses = children.map(function(child) {
		//console.log(child.id);
		if (child.className == "status"){ 

			child.innerHTML = 0;
			//console.log(child.id);
			updateStatuses();
			sendAsJson(child,value);
		}	
	});
}


function onKeyPress(ev){
	/*el = document.activeElement; 
	el.removeAttribute("onChange");
	el.setAttribute("onblur","onBlur(this, value)");
	disabled = el.getAttribute("id");
    document.getElementById("disabled").innerHTML = "disabled" ;*/ 	
	
	if (ev.keyCode === 13) {
	//	alert("enter "+ el.value);
     //   el.setAttribute("onchange","onChange(this, value)");	
	//	el.removeAttribute("onblur");
		disabled = "";
		//document.getElementById("disabled").innerHTML = "---";
    }	
}

function openWSConnection() {
	var webSocketURL = "";
    try {
        var hostname = window.location.hostname;
		if (hostname != "") { 
			webSocketURL = "ws://"+window.location.hostname+"/ws";
		} else {
			webSocketURL = "ws://server.local/ws";
		}	
		//alert(webSocketURL);
		webSocket = new WebSocket(webSocketURL);
        
        webSocket.onopen = function(openEvent) {
            console.log("WebSocket OPEN: " + JSON.stringify(openEvent, null, 4));
			//el = document.getElementById("wsStatus");
			//el.innerHTML = "Connected";
		};
        
        webSocket.onclose = function (closeEvent) {
            console.log("WebSocket CLOSE: " + JSON.stringify(closeEvent, null, 4));
			//el = document.getElementById("wsStatus").innerHTML = "Closed";
			check();
        };
        
        webSocket.onerror = function (errorEvent) {
			console.error('Socket encountered error: ', err.message, 'Closing socket');
			webSocket.close();
        };
        
        webSocket.onmessage = function (messageEvent) {
            var wsMsg = messageEvent.data;
			//console.log(wsMsg);
            console.log("WebSocket MESSAGE: " + wsMsg);
			//console.log(wsMsg[0]);
            if (wsMsg.indexOf("error") > 0) {
                //document.getElementById("incomingMsgOutput").value += "error: " + wsMsg.error + "\r\n";
            } else {
				if (wsMsg[0]=='{') {
				//	alert(wsMsg);
					processMessage(wsMsg);
				}
            }
        };
		
    } catch (exception) {
		console.error(exception);
	}
}

function check(){
    if (!webSocket || webSocket.readyState == 3) {
		//el = document.getElementById("wsStatus");
		//el.innerHTML = "Closed";
		openWSConnection();
	  }
  }
  
function sendAsJson(el, value){
		//console.log("----sendAsJson----");
	  	id =  el.getAttribute("id");
		//console.log(id);
		x = id.split("_");
		peerID = x[1];
		DevID = x[2];
		DevType = x[3];
		var js = {"peerID": peerID , "DevID":DevID , "DevType":DevType,  "value" : value};
		console.log(JSON.stringify(js));
		webSocket.send(JSON.stringify(js));
}
	
function onChange(el, value){
	 sendAsJson(el,value);
}
	
function onChange(el, value){
  	 sendAsJson(el,value);
}		

if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 function pad(n, width, z) {
	z = z || '0';
	n = n + '';
	return n.length >= width ? n : new Array(width - n.length + 1).join(z) + n;
}

function updateStatuses(){
	const statuses = document.getElementsByClassName("status");	
	for (let i = 0; i < statuses.length; i++) {
		value = statuses[i].innerHTML;
		if (value == "0") {
			statuses[i].style.backgroundColor = "red"
		}
		else
		{ 
			statuses[i].style.backgroundColor = "green"
		}
	}
}	

 //{"Title":"Bonjour","PeerID":10,"DeviceID":1,"D":[{"Temp":22.54999924,"SP":9,"Status":3,"msgId":5}]}
 source.addEventListener('new_readings', function(e) {
  //	console.log("new_readings", e.data);
  	var obj = JSON.parse(e.data);
  	//console.log(JSON.stringify(obj));
	suffix = "_"+obj.PeerID+"_"+obj.DeviceID;
  	let Keys = Object.keys(obj.D[0]); 
	let Values = Object.values(obj.D[0]);
	for (var key in Keys) {
	  	el = document.getElementById(Keys[key]+suffix);
		if (el != null){
		switch (el.tagName){
			case "SPAN" : 
				el.innerHTML = Values[key];
				break;
			case "H4" :
				el.innerHTML = Values[key];
				break;
			default : el.innerHTML = Values[key];	
		}		
	}

	updateStatuses();

	nd = new Date();
	el = document.getElementById("MsgID"+suffix);	
	el.innerHTML = nd.getFullYear()+'-'+pad(nd.getMonth()+1,2)+'-'+pad(nd.getDate(),2)+' '+pad(nd.getHours(),2)+':'+pad(nd.getMinutes(),2)+':'+pad(nd.getSeconds(),2);
	}
 }, false);

source.addEventListener('peers_status', function(e) {
	var obj = JSON.parse(e.data);
	console.log(JSON.stringify(obj));
	console.log(obj.D[0]);
	let Keys = Object.keys(obj.D[0]); 
	let Values = Object.values(obj.D[0]);
	console.log(Keys[1]);
	console.log(Values[1]);
	for (var key in Keys) {
		grayed = document.getElementsByClassName(Keys[key]);
		console.log(grayed.length);
		for (let i = 0; i < grayed.length; i++) {
			if (Values[key] == 0){
				grayed[i].classList.add("grayed");
			} else{
				grayed[i].classList.remove("grayed");
			}
		}	
	}
},false)};
