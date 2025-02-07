    


$(document).ready(function() 
   {    
    console.log("create page");
	$.getJSON("Struct.json", function(struct){
        var parent = document.getElementById("peers");
        var Peers  = struct["Struct"][0]["Peers"];
		alert(JSON.stringify(Peers));
        for (p = 0; p < Peers.length; p++) {
            Peer = document.createElement("div");
	        Peer.setAttribute("class","card");
	        Peer.innerHTML =  Peers[p].PeerID;
            var Devices = Peers[p]["Devices"];
            for  (d = 0; d < Devices.length; d++){
               Dev = document.createElement("div");
               Dev.setAttribute("class","reading"); 
               Dev.innerHTML = Devices[d]["ID"]; 
               Peer.appendChild(Dev);  
            }   
            parent.appendChild(Peer);
	    }	
    }).fail(function(){
        console.log("An error has occurred.");
    });
});






