var webSocket = null;
var webSocketURL;

var TitlesTable = {};
var URL;
var dict = new Object();

function pad(n, width, z) {
	z = z || '0';
	n = n + '';
	return n.length >= width ? n : new Array(width - n.length + 1).join(z) + n;
	}
  
function localTime(x) {
		d = new Date(x);
		nd = new Date(d.getTime() - (d.getTimezoneOffset() * 60 * 1000));
		return( nd.getFullYear()+'-'+pad(nd.getMonth()+1,2)+'-'+pad(nd.getDate(),2)+' '+pad(nd.getHours(),2)+':'+pad(nd.getMinutes(),2));
	}

var arr; //??


function getURL(){
	var x = window.location.href;
	URL = x.substring(0, x.lastIndexOf('/'));
	console.log(URL);
	return URL;
	
}

function processTemplate(DataType, suffix){
	let Keys = Object.keys(DataType); 
	let Values = Object.values(DataType);
	for (var key in Keys) {
		  // replace $xxx for the element.id
		oldKey = "$"+Keys[key];
		newKey = Keys[key]+suffix;
		s = s.replace(oldKey,newKey);
	}		//console.log(URL);
}

function processLog(DataType, suffix){
	let Keys = Object.keys(DataType); 
	let Values = Object.values(DataType);
	for (var key in Keys) {
		if (Values[key] == "L"){
		   	el = document.getElementById(Keys[key]+suffix);	
			el.classList.add("log");
			cols.push(Keys[key]+suffix);  // used by graph
		}
	}		
}

function setStatus(status){
	el = document.getElementById("wsStatus");
	el.innerHTML = "Loading Structure";
}

function convertDate(date) {						// used by setCalendar
	var yyyy = date.getFullYear().toString();
	var mm = (date.getMonth()+1).toString();
	var dd  = date.getDate().toString();
  
	var mmChars = mm.split('');
	var ddChars = dd.split('');
  
	return yyyy + '-' + (mmChars[1]?mm:"0"+mmChars[0]) + '-' + (ddChars[1]?dd:"0"+ddChars[0]);
}

function setCalender(){
	nd = new Date();
	el = document.getElementById("logDate").value = convertDate(nd);
}


$.get

$(document).ready(function(){
	setStatus("Loading Structure");	
	setInterval(check, 5000);
	setCalender();
	//console.log("create page");
	$.getJSON("Struct.json", function(struct){
		var Peers  = struct.Peers;
		var DataTypes  = struct.DataTypes;
		// loop on Peers	
		TitlesTable = struct.Titles;
		for (p = 0; p < Peers.length; p++) {
			//console.log(p);
			var Devices = Peers[p]["Devices"];
			// loop on devices
			for (d = 0; d < Devices.length; d++){
				// get device type
				typeNdx = Devices[d]["Type"];
				DataType = DataTypes[typeNdx];

				// get elements id suffix 
				suffix = "_" + Peers[p]["PeerID"] + "_" + Devices[d]["ID"];
				//alert(suffix);
			
				// get html template
					var HTML = struct.HTMLs[typeNdx];
				s = HTML["HTML"];
				processTemplate(DataType, suffix);	
				
				var parent = document.getElementById("cards");
				t = document.createElement("div");
				t.setAttribute("class","card thermostat");
				t.classList.add("S_"+Peers[p]["PeerID"]);
				t.setAttribute("id","S_"+Peers[p]["PeerID"]+"_"+Devices[d]["ID"]);
				t.innerHTML = s;
				parent.appendChild(t);

				processLog(DataType, suffix);

				var Title =  Devices[d]["Title"];
				dict["F1"+suffix] = Title;  // used by graph
				el = document.getElementById("Title"+suffix);
				el.classList.add("title");
				el.innerHTML = Title;

				shadow = document.getElementById("S"+suffix);
				shadow.addEventListener("animationend", 
					function(event) {event.srcElement.classList.remove("play")},
					{ passive: true }
				);	
			}
		}
		openWSConnection();
 	}).fail(function(){
      	console.log("An error has occurred.");
  	});
}); 

/*
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
	return result;
}
*/

//--------------------- User events -------------------- 
function sendAsJson(el, value){
	//console.log("----sendAsJson----");
  	id =  el.getAttribute("id");
	//console.log(id);
	x = id.split("_");
	peerID = x[1];
	DevID = x[2];
	DevType = x[3];
	var js = {"peerID": peerID , "DevID":DevID , "DevType":DevType ,  "value" : value};
	//console.log(JSON.stringify(js));
	webSocket.send(JSON.stringify(js));
}

function OnBtnUp(ev){												// used by thermostat set point 
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

function OnBtnDown(ev){												// used by thermostat set point 
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


function abc(el){								// NOT USED 
	//console.log("-----"+ el.checked);

	sendAsJson(el,+el.checked);
}


function onchange(el){
	//alert("-----"+el.id);
	//console.log("-----"+el.checked);

	//sendAsJson(el,el.checked);
}	

/*function OnBtnOn(ev){
	//console.log(ev.id);
    var el = document.getElementById(ev.id).parentNode;
	var children = [].slice.call(el.getElementsByTagName('*'),0);
	var idsAndClasses = children.map(function(child) {
		//console.log(child.id);
		if (child.className == "status"){ 
			
			child.innerHTML = 1;
			//console.log(child.id);
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
			sendAsJson(child,value);
		}	
	});
}*/


function onKeyPress(ev){
	if (ev.keyCode === 13) {
		disabled = "";
    }	
}

// ----------------------- Web socket ------------------------- //
function openWSConnection() {
    try {
		console.log("openWSConnection");
		var hostname = window.location.hostname;
		var port = window.location.port;
		console.log("hostname : " + hostname);
		console.log("port : " + port);
		if (port.length < 1) { 
		    port=80;
		}
		console.log("port : " + port);
		webSocketURL = "ws://"+hostname+":"+port+"/ws";

		console.log("websocketURL : "+ webSocketURL);
		webSocket = new WebSocket(webSocketURL);
        
        webSocket.onopen = function(openEvent) {
            console.log("WebSocket OPEN: " + JSON.stringify(openEvent, null, 4));
			el = document.getElementById("wsStatus");
			el.innerHTML = "Connected, waiting for data";
		};
        
        webSocket.onclose = function (closeEvent) {
            console.log("WebSocket CLOSE: " + JSON.stringify(closeEvent, null, 4));
			el = document.getElementById("wsStatus").innerHTML = "Closed";
			check();
        };
        
        webSocket.onerror = function (errorEvent) {
			console.error('Socket encountered error: ', err.message, 'Closing socket');
			webSocket.close();
        };
        
        webSocket.onmessage = function (messageEvent) {
            var wsMsg = messageEvent.data;
			//console.log(wsMsg);
            //console.log("WebSocket MESSAGE: " + wsMsg);
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
		el = document.getElementById("wsStatus");
			el.innerHTML = "Waiting Dada";
    } catch (exception) {
		console.error(exception);
	}
}

function check(){
    if (!webSocket || webSocket.readyState == 3) {
		el = document.getElementById("wsStatus");
		el.innerHTML = "Closed";
		openWSConnection();
	}
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
  		//console.log("message", e.data);
 	}, false);
 
 	function pad(n, width, z) {
		z = z || '0';
		n = n + '';
		return n.length >= width ? n : new Array(width - n.length + 1).join(z) + n;
	}

	//{"PeerID":4,"DeviceID":2,"D":[{"F1":"24.56","F2":0,"U1":0,"MsgID":"28:fe:8c:75:d0:01:3c:7c"}]}
	//TitleTable = {"Titles":[{"28:fe:8c:75:d0:01:3c:7c":"Salon"},{"28:fe:8c:75:d0:01:3c:7c":"Sous-Sol"}]} ;
 
 	source.addEventListener('new_readings', function(e) {

		var obj = JSON.parse(e.data);
		console.log(JSON.stringify(obj));
		suffix = "_"+obj.PeerID+"_"+obj.DeviceID;
		
		rssi = obj.D[0].rssi;
		let Keys = Object.keys(obj.D[0]); 
		let Values = Object.values(obj.D[0]);

		shadow = document.getElementById("S"+suffix);
		//console.log("S"+suffix);

		shadow.classList.add("play");
		//console.log("Listener "+TitlesTable);
		isDS = false;
		for (var key in Keys) {
			if (Keys[key] == "MsgID") {
				isDS = true;		 
				K = Values[key];
				//console.log("K : "+K);
				T = TitlesTable[0][K];
				dict["F1"+suffix] = T;  // dict is used by graph
				//console.log("Title : "+TitlesTable[K]);
				//console.log("Title : "+TitlesTable[0][K]);
				el = document.getElementById("Title"+suffix);
				el.innerHTML = T + "  (" +rssi+"dbm)"; 
			}

			// update html elements by tag names 
			el = document.getElementById(Keys[key]+suffix);
			if (el != null){
				switch (el.tagName){
				case "HEAT" : 
					if (Values[key] == 0){
						el.classList.add("heating");
					} else{
						el.classList.remove("heating");
					}
					break;
				case "SPAN" : 
					el.innerHTML = Values[key];
					break;
				case "H4" :
					el.innerHTML = Values[key];
					break;
				default : el.innerHTML = Values[key];	
				}	
			}
			// update rssi in Title element 
			el = document.getElementById("Title"+suffix);
			if (el.innerHTML.indexOf("dbm)") == -1){
				el.innerHTML = el.innerHTML + "  (" +rssi+"dbm)"
			}
		}
	}, false);
};

// ------------------ user actions from HTML --------------
function OnPairRQ(){
	var js = {"peerID": 99 , "DevID":99 , "DevType":99 ,  "value" : 99};
	console.log(JSON.stringify(js));
	webSocket.send(JSON.stringify(js));
}

function OnTest(){
	var js = {"peerID": 98 , "DevID":98 , "DevType":98 ,  "value" : 98};
	console.log(JSON.stringify(js));
	webSocket.send(JSON.stringify(js));
	
}


// --------------------- Graph ----------------------------------------

var cols = [];
var names = ["YYYYY","ZZZ"];

async function fetchData(url) {
	const res = await fetch(url, {mode: 'no-cors'} );
	if (!res.ok) {
	  console.error("There was an error:", res.statusText);
	  noData();
	  return;
	}
	const data = await res.json();
	console.log(data);
	generateChart(data, Graph, cols, names);
  }

function getGraphData(){
	e = document.getElementById("logDate");
	const m = e.value.split("-");
    //url = URL + "/readFile?FN=/" + (pad(m[1],2) + "_"+pad(m[2],2)) + ".JSN";		// TO REMOVE
	
	url = getURL() + "/readFile?FN=/" + (pad(m[1],2) + "_"+pad(m[2],2)) + ".JSN";			
	
	console.log(url);
	fetchData(url);
}	

function noData(){
	el = document.getElementById("Graph");
	el.innerHTML = "File not Found";
}

function generateChart(json, id, cols, names) {	
	
//	console.log(dict);
	
	//console.log("---in generateChart ----");
	//console.log(JSON.stringify(json));
	var chart = c3.generate({
		size: {
			width: 690,
			height: 360
			},
		layout:"fitData",
		bindto: id,
		data: {
			json: json.Data,
			xFormat: '%Y-%m-%dT%H:%M:%S',
			keys: {
				x: 'UID',
				value: cols
				},
			names:  dict,
			type: 'spline'
		},
		grid:{y: {show: true}},
		axis : {
			y : {
				tick: {
					max: 30,
					min: 0
					}
				},						  
			x:{
				type : "timeseries",
				tick: {
				format: function (x) {return(localTime(x).substring(11, 18))},
				rotate: 60
				}			
			}
		}
	});
	return chart;	
	}	

	function handleVisibilityChange(){
		if (document.hidden) {
			console.log("Inactive");
			webSocket.close;
		} else {
			console.log("Active");
			console.log(webSocketURL);
			webSocket = new WebSocket(webSocketURL);
			webSocket.onmessage = function (messageEvent) {
			   console.log(messageEvent.data);
			} 
			webSocket.onopen = function(openEvent) {
				console.log("WebSocket OPEN: " + JSON.stringify(openEvent, null, 4));
				el = document.getElementById("wsStatus");
				el.innerHTML = "Connected";
			};
			
			webSocket.onclose = function (closeEvent) {
				console.log("WebSocket CLOSE: " + JSON.stringify(closeEvent, null, 4));
				el = document.getElementById("wsStatus").innerHTML = "Closed";
			};		
		}
	}    