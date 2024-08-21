var webSocket = null;


AAA = '{"Struct":[    {"Peers":[            {                "PeerID": 10,                "MsgID": 0,                "Devices":[                    {"ID":1, "Type":"1"},                    {"ID":2, "Type":"2"}                ]            },            {                "PeerID": 11,                "MsgID": 0,                "Devices":[                    {"ID":1, "Type":"1"},                    {"ID":2, "Type":"2"},                    {"ID":3, "Type":"3"}                ]            }        ]    },    {"DataTypes":[        {"Title":"Interieur", "SP": 19, "Temp": 21, "Status": 0, "MsgID":0},        {"Title":"Exterieur", "Temp": 21},        {"Title":"Chauffage", "Cmd": 1, "Status":0 }           ]    }]    }';
        XXX = '{"Peers":[{"PeerID": 10, "MsgID": 0, "Devices":[ {"ID":1, "Typ":"1", "Data": [{"Title":"Interieur", "SP": 19, "Temp": 21, "Status": 0,"msgID": 0}]}, {"ID":2, "Type":"2", "Data": [{"Title":"Exterieur", "Temp": 21}]}                ]            },            {                "PeerID": 11,                "MsgID": 0,                "Devices":[                    {"ID":1, "Type":"1", "Data": [{"Title":"Interieur", "SP": 19, "Temp": 21, "Status": 0}]},                    {"ID":2, "Type":"2", "Data": [{"Title":"Exterieur", "Temp": 21 }]},                    {"ID":3, "Type":"3", "Data": [{"Title":"Chauffage", "Cmd": 1, "Status":0 }]}                ]            }        ]}' ;                           const tstat = '<h4><i class="fas fa-tint">	</i> $Title</h4>  <p><span class="reading"><span id="$Temp"></span> &deg;C</span></p>  <p><span class="setpoint"> <span><input id="$SP_" onchange="onChange(this, value)">   </span>  <span id="$SP">    </span> &deg;C</span> </p> <p class="packet">Reading ID: <span id="$msgID"></span></p>'	



function createDevice(PeerID, Device){
	switch (Device.Type) {
		case "1":
			console.log('Create Thermostat');
			t = document.createElement("div");
			t.setAttribute("class", "card thermostat");
			var s = tstat;
			break;
		case "2":
			console.log('Create Thermometer');
			break;
		case "3":
			console.log('Create Relay');
			break;
		default:
			console.log('Unknown Device type');
	}
	Device.Data.forEach(function(Item){
		let Keys = Object.keys(Item); 
		let Values = Object.values(Item);
		
		for (var key in Keys) {
			//alert(Keys[key] + " : " + Values[key]);
			s = s.replace("$"+Keys[key],Keys[key]+"_"+PeerID+"_"+Device.ID);
			//alert(s);
		}
	})
	
	var parent = document.getElementById("cards");
	t.innerHTML = s;
	parent.appendChild(t);
}

function pad(n, width, z) {
	z = z || '0';
	n = n + '';
	return n.length >= width ? n : new Array(width - n.length + 1).join(z) + n;
	}


function start(){
	
	
	
		d = new Date();
	//	alert(d.getTimezoneOffset());
		nd = new Date() ;//- (d.getTimezoneOffset() * 60 * 1000));
	//	alert( nd.getFullYear()+'-'+pad(nd.getMonth()+1,2)+'-'+pad(nd.getDate(),2)+' '+pad(nd.getHours(),2)+':'+pad(nd.getMinutes(),2)+':'+pad(nd.getSeconds(),2));
	
	
	
	
	alert('start');
	
	const collection = document.getElementsByClassName("status");
	alert(collection.length);
	for (let i = 0; i < collection.length; i++) {
		n = collection[i].innerHTML;
		alert(n);
		if (n=="0") {
			collection[i].style.backgroundColor = "red"
			}
		else { 
			collection[i].style.backgroundColor = "green"}
	}
	

	
	//alert(JSON.stringify(myObj));
	
/*	A = "<h4><i class=\"fas fa-thermometer-half\"></i> $title</h4>  <p><span class=\"reading\"><span id=\"$temp\"></span> &deg;C</span></p>  <p class=\"packet\">Reading ID: <span id=\"$td\"></span></p>";
	alert(A);
	A = A.replace('\"','"');
	alert(A);
	
	d = JSON.parse(AAA);
	//alert("1");
	//alert(JSON.stringify(d));
	//alert("2");
	Peers  = d["Struct"][0];
	alert(JSON.stringify(Peers));
	dataTypes = d["Struct"][1];
	alert(JSON.stringify(dataTypes));
	alert(JSON.stringify(dataTypes["DataTypes"][0]));
	
	
	Obj = d["Struct"][1]["DataTypes"][0];
	let Keys = Object.keys(Obj); 
	let Values = Object.values(Obj);
	for (var key in Keys) {
	  alert(Keys[key]+" : " + Values[key]);
	}
	
	alert("3");*/
}	


	/*data = JSON.parse(XXX);
	Peers = data["Peers"];
	data["Peers"].forEach(function (Peer) {
		txt = "Peer: " + Peer.PeerID +"\n";
		Devices = Peer.Devices;
		Devices.forEach(function(Device) {
			createDevice(Peer.PeerID, Device);	
		})
	})
	openWSConnection();
	setInterval(check, 5000);
}*/

/*$(document).ready(function(){
  $.getJSON("/J.JSON", function(data){
    //console.log(data.Peers[0].PeerID); 
  }).fail(function(){
      alert("An error has occurred.");
  });
}); */


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
			webSocketURL = "ws://server_2.local/ws";
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
	  id =  el.getAttribute("id");
		x = id.split("_");
		name = x[0];
		peerID = x[1];
		
		var js = {};
		var entries = []
		
		js.entries = entries;

		var entry = {
		  "peerID": peerID ,
		  "name": name ,
		  "value" : value
		}
		
		js.entries.push(entry);
		//alert(JSON.stringify(js));
		webSocket.send(JSON.stringify(js));

    //  {"entries":[{"peerID":"1","name":"sp","value":"14"}]}
	}
	
	function onChange(el, value){
    
	 sendAsJson(el,value);
  			
	}
	
	function onChange(el, value){
  	 sendAsJson(el,value);
  }		


/*const th = 	'<h4><i class="fas fa-thermometer-half"></i> $title</h4>  <p><span class="reading"><span id="$temp"></span> &deg;C</span></p>  <p class="packet">Reading ID: <span id="$td"></span></p>';
  const hu = 	'<h4><i class="fas fa-tint">            </i> $title</h4>  <p><span class="reading"><span id="$temp"></span> &deg;C</span></p>  <p><span class="reading"><span id="$hum"></span> &percnt;</span></p> <p class="packet">Reading ID: <span id="$td"></span></p>'		
*/ 

/*function createThermometer(parent, title, temp, td){
  t = document.createElement("div");
  t.setAttribute("class", "card temperature");
  var s = th.replace('$title',title);
  s = s.replace("$temp", temp);
  s = s.replace("$td", td);
  t.innerHTML = s;
  parent.appendChild(t);
}*/

function createThermostat(parent, title, temp, setpoint, td){
  t = document.createElement("div");
  t.setAttribute("class", "card thermostat");
  var s = tstat.replace('$title',title);
  s = s.replace("$temp", temp);
  s = s.replace("$sp", setpoint);
  s = s.replace("$td", td);
  t.innerHTML = s;
  parent.appendChild(t);
  
}

/*function createHumidity(parent, title, temp, hum, td){
  t = document.createElement("div");
  t.setAttribute("class", "card humidity");
  var s = hu.replace('$title',title);
  s = s.replace("$temp", temp);
  s = s.replace("$hum", hum);
  s = s.replace("$td", td);
  t.innerHTML = s;
  parent.appendChild(t);
}
*/

var parent = document.getElementById("cards");

//createThermostat(parent, "Poules", "t_1", "sp_1", "rt_1");

/*document.getElementById("t_1").innerHTML = "...";
document.getElementById("sp_1").innerHTML = "...";
document.getElementById("rt_1").innerHTML = "...";*/


/*if (!!window.EventSource) {
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
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("t_"+obj.id).innerHTML = obj.temperature.toFixed(2);
  document.getElementById("aaa").innerHTML = obj.temp_sp;
  document.getElementById("bbb").innerHTML = obj.readingId;
  }, false);
}*/

var SP = 9;
 
function OnBtnUp(ev){
	var el = document.getElementById(ev.id).parentNode;
   	var children = [].slice.call(el.getElementsByTagName('*'),0);
	var idsAndClasses = children.map(function(child) {
		if (child.className == "UpDown"){ 
			SP = child.innerHTML; 
			SP++;
			child.innerHTML = SP;
		}	
	});

}

function OnBtnDown(ev){
	var el = document.getElementById(ev.id).parentNode;
   	var children = [].slice.call(el.getElementsByTagName('*'),0);
	var idsAndClasses = children.map(function(child) {
		if (child.className == "UpDown"){ 
			SP = child.innerHTML; 
			SP--;
			child.innerHTML = SP;
		}	
	});
}


