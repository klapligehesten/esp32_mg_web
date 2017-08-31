#include "wifi.h"

#include <stdlib.h>
#include <string.h>

#include <esp_err.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <nvs.h>
#include <tcpip_adapter.h>

#include <utils.h>
#include <config_task.h>
#include <flash.h>
#include <mg_task.h>

static char *tag = "wifi_tasks";

// Defines for WiFi
mg_process_http_request_type ap_process_req_type;

// internal prototypes
esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
void wifi_start_client( P_WIFI_CONF wifi_conf);
void wifi_start_ap( P_WIFI_CONF wifi_conf);
void wifi_start_ap_client( P_WIFI_CONF wifi_conf);
void wifi_start_open_ap();
void init_wifi();
int get_ip_part( char *ip_str, int num);
void wifi_set_ap_dhcps(P_WIFI_CONF wifi_conf);
wifi_config_t wifi_set_client_config(P_WIFI_CONF wifi_conf);
wifi_config_t wifi_set_open_ap_config();
wifi_config_t wifi_set_ap_config(P_WIFI_CONF wifi_conf);

// ------------------------------------------
// Start WiFi
// ------------------------------------------
void wifi_start() {
	P_WIFI_CONF wifi_conf;
	init_wifi();
	wifi_conf = config_get_wifi_conf();
	switch( flash_key_exists(CONFIG_OPEN_WIFI_NOT_ENABLED)) {
	case ESP_ERR_NVS_NOT_FOUND:
		ESP_LOGD(tag, "config_get_conf_enabled() = ESP_ERR_NVS_NOT_FOUND");
		wifi_start_open_ap();
		break;
	case ESP_OK:
		ESP_LOGD(tag, "config_get_conf_enabled() = ESP_OK");
		if( wifi_conf != NULL) {
			if( wifi_conf->cli.enabled == 1 && wifi_conf->ap.enabled == 1)
				wifi_start_ap_client(wifi_conf);
			else if( wifi_conf->cli.enabled == 1)
				wifi_start_client( wifi_conf);
			else if( wifi_conf->ap.enabled == 1)
				wifi_start_ap(wifi_conf);
			else
				wifi_start_open_ap();
		}
		else {
			wifi_start_open_ap();
		}
		break;
	}
	free( wifi_conf);
}

// ------------------------------------------
// Intialize WiFi AP
// ------------------------------------------
void init_wifi() {
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
}

// ------------------------------------------
// Connect to WiFi as client
// ------------------------------------------
void wifi_start_ap_client( P_WIFI_CONF wifi_conf) {
	ESP_LOGD(tag, "Starting a wifi AP and client");
	ap_process_req_type = mg_process_http_request;
	wifi_set_ap_dhcps(wifi_conf);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
	wifi_config_t config = wifi_set_client_config(wifi_conf);
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
	config = wifi_set_ap_config(wifi_conf);
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

// ------------------------------------------
// Connect to WiFi as client
// ------------------------------------------
void wifi_start_client( P_WIFI_CONF wifi_conf) {
	ESP_LOGD(tag, "Starting a wifi client");
	ap_process_req_type = mg_process_http_request;
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t config = wifi_set_client_config(wifi_conf);
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

// ------------------------------------------
// Start WiFi AP
// ------------------------------------------
void wifi_start_ap( P_WIFI_CONF wifi_conf) {
	ESP_LOGD(tag, "Starting an access wifi point ...");
	ap_process_req_type = mg_process_http_request;
	wifi_set_ap_dhcps(wifi_conf);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t config = wifi_set_ap_config(wifi_conf);
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

// ------------------------------------------
// Start open WiFi AP
// ------------------------------------------
void wifi_start_open_ap() {
	ESP_LOGD(tag, "Starting a open wifi access point ...");
	ap_process_req_type = mg_process_http_request_config;
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t config = wifi_set_open_ap_config();
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

// ------------------------------------------
// Set WiFi AP config
// ------------------------------------------
wifi_config_t wifi_set_ap_config(P_WIFI_CONF wifi_conf) {
	wifi_config_t config;
	strcpy((char*) &config.ap.ssid, wifi_conf->ap.ssid);
	strcpy((char*) &config.ap.password, wifi_conf->ap.passwd);
	config.ap.ssid_len = 0;
	config.ap.channel = 0;
	config.ap.authmode = WIFI_AUTH_WPA2_PSK;
	config.ap.ssid_hidden = 0;
	config.ap.max_connection = 10;
	config.ap.beacon_interval = 100;
	return config;
}

// ------------------------------------------
// Set open WiFi AP config
// ------------------------------------------
wifi_config_t wifi_set_open_ap_config() {
	char* ap_name = utils_get_ap_name();
	wifi_config_t config;
	strcpy((char*) &config.ap.ssid, ap_name);
	free(ap_name);
	config.ap.ssid_len = 0;
	config.ap.password[0] = 0;
	config.ap.channel = 0;
	config.ap.authmode = WIFI_AUTH_OPEN;
	config.ap.ssid_hidden = 0;
	config.ap.max_connection = 1;
	config.ap.beacon_interval = 100;
	return config;
}

// ------------------------------------------
// Set DHCP on WiFi AP
// ------------------------------------------
void wifi_set_ap_dhcps(P_WIFI_CONF wifi_conf) {
	tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
	tcpip_adapter_ip_info_t ipInfo;
	IP4_ADDR(&ipInfo.ip, get_ip_part(wifi_conf->ap.ipadr, 1),
			get_ip_part(wifi_conf->ap.ipadr, 2),
			get_ip_part(wifi_conf->ap.ipadr, 3),
			get_ip_part(wifi_conf->ap.ipadr, 4));
	IP4_ADDR(&ipInfo.gw, get_ip_part(wifi_conf->ap.gateway, 1),
			get_ip_part(wifi_conf->ap.gateway, 2),
			get_ip_part(wifi_conf->ap.gateway, 3),
			get_ip_part(wifi_conf->ap.gateway, 4));
	IP4_ADDR(&ipInfo.netmask, get_ip_part(wifi_conf->ap.netmask, 1),
			get_ip_part(wifi_conf->ap.netmask, 2),
			get_ip_part(wifi_conf->ap.netmask, 3),
			get_ip_part(wifi_conf->ap.netmask, 4));
	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP, wifi_conf->ap.hostname);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ipInfo);
	tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
}

// ------------------------------------------
// Set WiFi client
// ------------------------------------------
wifi_config_t wifi_set_client_config(P_WIFI_CONF wifi_conf) {
	wifi_config_t config;
	config.sta.bssid_set = 0;
	strcpy((char*) &config.sta.ssid, wifi_conf->cli.ssid);
	strcpy((char*) &config.sta.password, wifi_conf->cli.passwd);
	return config;
}

// ------------------------------------------
// ESP32 WiFi handler.
// ------------------------------------------
esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
	switch(event->event_id) {
	case SYSTEM_EVENT_STA_GOT_IP:
		// ESP_LOGD(tag, "Got an IP: " IPSTR,IP2STR(&event->event_info.got_ip.ip_info.ip));
		mg_start_task(ap_process_req_type);
		break;
	case SYSTEM_EVENT_STA_START:
		ESP_LOGD(tag, "Received a start request");
		ESP_ERROR_CHECK(esp_wifi_connect());
		break;
	case SYSTEM_EVENT_AP_START:
		ESP_LOGD(tag, "Wifi access point started");
		mg_start_task(ap_process_req_type);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		ESP_LOGD(tag, "Wifi client disconnect");
		break;
	default:
		break;
	}

	return ESP_OK;
}

// ------------------------------------------
// get n part of ip adr.
// ------------------------------------------
int get_ip_part( char *ip_str, int num) {
	// TODO: some validations
	char tmp_buf[20] = {'0',0};
	strcat(tmp_buf, ip_str);
	char * d = tmp_buf;
	char *c = d;
	for( int i = 0; i < num; i++) {
		d = c+1;
		c = strchr(d, '.');
	}
	if( c != NULL)
		*(c) = '\0';

	return atoi(d);
}

