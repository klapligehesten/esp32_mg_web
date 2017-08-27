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
#include "wifi_task.h"

static char *tag = "wifi_tasks";

// Defines for WiFi
mg_process_http_request_type ap_process_req_type;

// internal prototypes
esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
void wifi_start_task_client( P_WIFI_CONF wifi_conf);
void wifi_start_task_ap( P_WIFI_CONF wifi_conf);
void wifi_start_task_open_ap();
void init_wifi();

void wifi_start_task() {
	init_wifi();
	P_WIFI_CONF wifi_conf;
	wifi_conf = config_get_wifi_conf();
	switch( flash_key_exists(CONFIG_OPEN_WIFI_NOT_ENABLED)) {
	case ESP_ERR_NVS_NOT_FOUND:
		ESP_LOGD(tag, "config_get_conf_enabled() = ESP_ERR_NVS_NOT_FOUND");
		wifi_start_task_open_ap();
		break;
	case ESP_OK:
		ESP_LOGD(tag, "config_get_conf_enabled() = ESP_OK");
		if( wifi_conf != NULL) {
			if( wifi_conf->cli.enabled == 1)
				wifi_start_task_client( wifi_conf);
			else if( wifi_conf->ap.enabled == 1)
				wifi_start_task_ap(wifi_conf);
			else
				wifi_start_task_open_ap();
		}
		else {
			wifi_start_task_open_ap();
		}
		break;
	}
}

void init_wifi() {
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
}

void wifi_start_task_client( P_WIFI_CONF wifi_conf) {
	ESP_LOGD(tag, "Starting a wifi client");
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t config;
	config.sta.bssid_set = 0;
	strcpy( (char *)&config.sta.ssid, wifi_conf->cli.ssid);
	strcpy( (char *)&config.sta.password, wifi_conf->cli.passwd);

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_connect());
}

void wifi_start_task_open_ap() {
	ESP_LOGD(tag, "Starting a open wifi access point ...");
	ap_process_req_type = mg_process_http_request_config;
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	char *ap_name = utils_get_ap_name();
	wifi_config_t config;
	strcpy( (char *)&config.ap.ssid, ap_name);
	free(ap_name);
	config.ap.ssid_len=strlen(ap_name);
	config.ap.password[0]=0;
	config.ap.channel=0;
	config.ap.authmode=WIFI_AUTH_OPEN;
	config.ap.ssid_hidden=0;
	config.ap.max_connection=4;
	config.ap.beacon_interval=100;
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_start_task_ap( P_WIFI_CONF wifi_conf) {
	/*
	 * TODO: implement ip assignment
		tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_AP);
		tcpip_adapter_ip_info_t ipInfo;
		IP4_ADDR(&ipInfo.ip, 192,168,1,99);
		IP4_ADDR(&ipInfo.gw, 192,168,1,1);
		IP4_ADDR(&ipInfo.netmask, 255,255,255,0);
		tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ipInfo);
	*/
	ESP_LOGD(tag, "Starting an access wifi point ...");
	ap_process_req_type = mg_process_http_request;
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t config;
	strcpy( (char *)&config.ap.ssid, wifi_conf->ap.ssid);
	strcpy( (char *)&config.ap.password, wifi_conf->ap.passwd);
	config.ap.ssid_len=strlen(wifi_conf->ap.ssid);
	config.ap.channel=0;
	config.ap.authmode=WIFI_AUTH_WPA2_PSK;
	config.ap.ssid_hidden=0;
	config.ap.max_connection=5;
	config.ap.beacon_interval=100;

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

// ESP32 WiFi handler.
esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
	switch(event->event_id) {
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGD(tag, "Got an IP: " IPSTR,IP2STR(&event->event_info.got_ip.ip_info.ip));
		mg_start_task(mg_process_http_request);
		break;
	case SYSTEM_EVENT_STA_START:
		ESP_LOGD(tag, "Received a start request");
		ESP_ERROR_CHECK(esp_wifi_connect());
		mg_stop_task();
		break;
	case SYSTEM_EVENT_AP_START:
		ESP_LOGD(tag, "Wifi access point started");
		mg_start_task(ap_process_req_type);
		break;
	default:
		break;
	}

	return ESP_OK;
}
