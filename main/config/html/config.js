window.onload = function() {
  var form = document.getElementById('config-form');
  var messagesList = document.getElementById('messages');
  var chkbox = document.querySelectorAll(".cbox");
  var enabled_cli = document.getElementById('enabled_cli');
  var ssid_cli = document.getElementById('ssid_cli');
  var passwd_cli = document.getElementById('passwd_cli');
  var enabled_ap = document.getElementById('enabled_ap');
  var ssid_ap = document.getElementById('ssid_ap');
  var passwd_ap = document.getElementById('passwd_ap');
  var ipadr_ap = document.getElementById('ipadr_ap');
  var gateway_ap = document.getElementById('gateway_ap');
  var netmask_ap = document.getElementById('netmask_ap');
  var list_rem = document.getElementById('list_rem');
  var del_fat_files = document.getElementById('del_fat_files');
  
  var socket  = new WebSocket('ws://' + location.host);

  socket.onerror = function(error) {
    console.log('WebSocket Error: ' + error);
  };

  socket.onmessage = function(event) {
    var message = JSON.parse(event.data);
    
    messagesList.innerHTML += '<li class="received">' + message.status + '</li>';
    // socket.close();
  };
  
  form.onsubmit = function(e) {
	e.preventDefault();
	var parm_cli = {};
	var parm_ap = {};
	var sec = {};
	var config = {};
	
	parm_cli.enabled = enabled_cli.checked;
	parm_cli.ssid = ssid_cli.value;
	parm_cli.passwd = passwd_cli.value;
	sec.wifi_cli = parm_cli;
	
	parm_ap.enabled = enabled_ap.checked;
	parm_ap.ssid = ssid_ap.value;
	parm_ap.passwd = passwd_ap.value;
	parm_ap.ipadr = ipadr_ap.value;
	parm_ap.gateway = gateway_ap.value;
	parm_ap.netmask = netmask_ap.value;
	sec.wifi_ap = parm_ap;
	
	config.config = sec;
	
	socket.send(JSON.stringify(config));

	return false;
  };
  
  for (var i = 0; i < chkbox.length; i++) {
	var c = chkbox[i];
	c.onclick = function(e) {
	  for (var i = 0; i < chkbox.length; i++) {
		chkbox[i].checked = false;
	  }
	  this.checked = true; 
	};
  }
  
  list_rem.onclick = function(e) {
	e.preventDefault();
	var parm_misc = {};
	var sec = {};
	var config = {};
		
	parm_misc.del_fat_files = del_fat_files.checked;
	sec.misc = parm_misc;
	config.config_del_files = sec;
		
	socket.send(JSON.stringify(config));
  };
  
};

