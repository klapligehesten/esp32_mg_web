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
  // var socket  = new WebSocket('ws://192.168.4.1');

  socket.onopen  = function(event) {
	  var config_get = {};
	  config_get.config_get = true;
	  socket.send(JSON.stringify(config_get));
  }
  
  socket.onerror = function(error) {
    console.log('WebSocket Error: ' + error);
  };

  socket.onmessage = function(event) {
    var message = JSON.parse(event.data);
    if( message.hasOwnProperty('config')) {
    	var cli = message.config.wifi_cli;
    	enabled_cli.checked = cli.enabled;
    	ssid_cli.value = cli.ssid;
    	passwd_cli.value = cli.passwd;

    	var ap =  message.config.wifi_ap;
    	enabled_ap.checked = ap.enabled;
    	hostname.value = ap.hostname;
    	ssid_ap.value = ap.ssid;
    	passwd_ap.value = ap.passwd;
    	ipadr_ap.value = ap.ipadr;
    	gateway_ap.value = ap.gateway;
    	netmask_ap.value = ap.netmask;
    }
    if( message.hasOwnProperty('status')) {
        messagesList.innerHTML += '<li class="received">' + message.status + '</li>';
    }
    // messagesList.innerHTML += '<li class="received">' + event.data + '</li>';
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
	parm_ap.hostname = hostname.value;
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

