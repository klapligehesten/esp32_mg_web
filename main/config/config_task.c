#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/projdefs.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "fatfs/fatfs.h"
#include "flash/flash.h"
#include "mg/mg_task.h"
#include "utils/utils_json.h"

#include "config/config_task.h"

xQueueHandle config_evt_queue;

static char *tag = "config";

// Local prototypes
void config_task(void *pvParameter);
int config_list_del_files(MG_WS_MESSAGE *m, char *);
int config_save_wifi_conf( MG_WS_MESSAGE *m, char *);
int config_get();

// void (*functions[256])();
// ------------------------------------------
// config get configuration
// ------------------------------------------
P_WIFI_CONF config_get_wifi_conf() {
	esp_err_t rc = 0;
	char *buffer;
	WIFI_CONF *wifi_conf = NULL;

	if( ( rc = flash_get_key(CONFIG_WIFI_PARAMETERS, &buffer)) == ESP_OK) {
		cJSON *root = cJSON_Parse(buffer);
		if( root != NULL) {
			cJSON *config = cJSON_GetObjectItem(root, "config");;
			cJSON *cli = cJSON_GetObjectItem(config, "wifi_cli");
			cJSON *ap = cJSON_GetObjectItem(config, "wifi_ap");
			if( config != NULL && cli != NULL && ap != NULL) {

				wifi_conf = (P_WIFI_CONF) calloc( 1, sizeof(WIFI_CONF));

				wifi_conf->cli.enabled = json_get_int(cli, "enabled", false);
				wifi_conf->cli.ssid = json_get_str(cli, "ssid", "Foo");
				wifi_conf->cli.passwd = json_get_str(cli, "passwd","foo");
				free(cli);

				wifi_conf->ap.enabled = json_get_int(ap, "enabled", false);
				wifi_conf->ap.hostname = json_get_str(ap, "hostname", "foo");
				wifi_conf->ap.ssid = json_get_str(ap, "ssid", "foo");
				wifi_conf->ap.passwd = json_get_str(ap, "passwd", "foo");
				wifi_conf->ap.ipadr = json_get_str(ap, "ipadr", "foo");
				wifi_conf->ap.gateway = json_get_str(ap, "gateway", "foo");
				wifi_conf->ap.netmask = json_get_str(ap, "netmask", "foo");
				free(ap);

				free(config);
				free(root);
			}
			else {
				ESP_LOGD( tag, "config | cli | ap item is NULL");
				wifi_conf = NULL;
			}
		}
		else {
			ESP_LOGD( tag, "root item is NULL");
			wifi_conf = NULL;
		}
	}
	else {
		ESP_LOGD( tag, "flash_get_key returned: %x", rc);
	}
	free(buffer);

	return wifi_conf;
}

// ------------------------------------------
// Start the gpio task
// ------------------------------------------
void config_start_task() {
	config_evt_queue = xQueueCreate(10, sizeof(MG_WS_MESSAGE));
	xTaskCreate(config_task, "config_task", 8192, NULL, 10, NULL);
}

// ============================================================================
// Local functions
// ============================================================================
// ------------------------------------------
// config task handle
// ------------------------------------------
void config_task(void *pvParameter) {
	MG_WS_MESSAGE m;
	char resp[100];
	int rc = ESP_OK;
	while(1) {
		// TODO: Some security to deny not all can call config.
		if(( rc = xQueueReceive(config_evt_queue, &m, portMAX_DELAY)) == pdPASS) {
			switch( m.action) {
			case MG_ACTION_CONFIG_WIFI:
				// Save config
				ESP_LOGI( tag, "CONFIG save wifi event received");
				rc = config_save_wifi_conf( &m, resp);
				break;
			case MG_ACTION_CONFIG_GET_WIFI:
				// Get config
				ESP_LOGI( tag, "CONFIG get wifi event received");
				rc = config_get( );
				break;
			case MG_ACTION_CONFIG_DEL_FILES:
				ESP_LOGI( tag, "CONFIG list/del files event received");
				rc = config_list_del_files( &m, resp);
				break;
			}
			free(m.message);
		}
		if( rc != ESP_OK ) {
			sprintf( resp, "{\"status\":\"Error '%x' in save of configaration!!!!\"}", rc);
			mg_broadcast_poll(resp);
		}
	}
}

// ------------------------------------------
// Get wifi config
// ------------------------------------------
int config_get() {
	int rc;
	char *buffer;

	if ((rc = flash_get_key(CONFIG_WIFI_PARAMETERS, &buffer)) == ESP_OK) {
		mg_broadcast_poll(buffer);
	}

	return rc;
}

// ------------------------------------------
// Save wifi config file and reboot
// ------------------------------------------
int config_save_wifi_conf( MG_WS_MESSAGE *m, char *resp) {
	const TickType_t xDelay = 2000 / portTICK_PERIOD_MS;
	int rc;

	if ((rc = flash_set_key(CONFIG_WIFI_PARAMETERS, m->message)) == ESP_OK) {
		if ((rc = flash_set_key(CONFIG_OPEN_WIFI_NOT_ENABLED, "{\"status\":\"no open config enabled\"}")) == ESP_OK) {
			sprintf(resp, "{\"status\":\"Configuration saved...Rebooting\"}");

			mg_broadcast_poll(resp);
			// Wait a few seconds for the ws frame to be send
			vTaskDelay(xDelay);
			esp_restart();
		}
	}
	return rc;
}

// ------------------------------------------
// Callback from dir list
// ------------------------------------------
void config_list( char *c, int del) {
	char resp[255];
	sprintf( resp, "{\"status\":\"File:%s&emsp;removed:%s\"}", c, del ? "Yes":"No");
	mg_broadcast_poll(resp);
}

// ------------------------------------------
// Check parms and maybe delete existing files
// ------------------------------------------
int config_list_del_files(MG_WS_MESSAGE *m, char *resp) {
	cJSON *root = cJSON_Parse(m->message);
	cJSON *config = cJSON_GetObjectItem(root, "config_del_files");;
	cJSON *misc = cJSON_GetObjectItem(config, "misc");
	int del = json_get_int(misc, "del_fat_files", false);
	free(misc);
	free(config);
	free(root);
	fatfs_list_dir(del, config_list);

	return ESP_OK;
}

