window.onload = function() {

	// Get references to elements on the page.
	var form = document.getElementById('message-form');
	var messagesList = document.getElementById('messages');
	var chkbox = document.querySelectorAll(".relay");
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
    
		for (var i = 0; i < chkbox.length; i++) {
			chkbox[i].checked = obj['relay_' + i];
		}
		messagesList.innerHTML = '<li class="received"><span>Received:</span>' + message + '</li>';
	};

	socket.onclose = function(event) {
	};

  	for (var i = 0; i < chkbox.length; i++) {
  		var c = chkbox[i]; 
  		c.onchange = function(e) {
  			var cbox = {};
  			var rel = {};
			cbox[this.id] = this.checked;
			cbox.no = parseInt(this.dataset.rno);
			cbox.rtype = parseInt(this.dataset.rtype);
  		  	rel.gpio_relays = cbox;
  			console.log('Send: ' + JSON.stringify(rel));
  		  	socket.send( JSON.stringify(rel));
  		};
  	}
};
