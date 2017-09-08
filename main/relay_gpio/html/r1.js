window.onload = function() {

	// Get references to elements on the page.
	var form = document.getElementById('message-form');
	var messagesList = document.getElementById('messages');
	var radiobox = document.querySelectorAll(".relay");
	var eye = document.getElementById("eye");

	// Create a new WebSocket.
	var socket  = new WebSocket('ws://' + location.host);
	//var socket  = new WebSocket('ws://172.16.1.78');

	socket.onerror = function(error) {
		console.log('WebSocket Error: ' + error);
	};
	
	eye.addEventListener('animationiteration', function(){
	    this.style.webkitAnimationPlayState="paused";
	}, false);

	socket.onmessage = function(event) {
		eye.style.webkitAnimationPlayState="running";
		var message = event.data;
		var parsedJSON = JSON.parse(message);
		var obj = parsedJSON.gpio_relays;
    
		for (var i = 0, x = 0; i < radiobox.length-1; i+=2, x++) {
			if( obj['relay_' + x]) {
				radiobox[i].checked = false;
				radiobox[i+1].checked = true;
			}
			else {
				radiobox[i].checked = true;
				radiobox[i+1].checked = false;
			}
		}
		// messagesList.innerHTML = '<li class="received"><span>Received:</span>' + message + '</li>';
	};

	socket.onclose = function(event) {
	};

  	for (var i = 0; i < radiobox.length-1; i+=2) {
  		radiobox[i].onchange = function(e) {
  			sendMessage(this, false)
  		};
  		radiobox[i+1].onchange = function(e) {
  			sendMessage(this, true)
  		};
  	}
  	
  	function sendMessage( obj, b) {
		var rbox = {};
		var rel = {};
		rbox[obj.name] = b;
		rbox.no = parseInt(obj.dataset.rno);
		rbox.rtype = parseInt(obj.dataset.rtype);
	  	rel.gpio_relays = rbox;
		// console.log('Send: ' + JSON.stringify(rel));
	  	socket.send( JSON.stringify(rel));
  	}
};
