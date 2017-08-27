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

#include <fatfs.h>
#include <flash.h>
#include <config_task.h>
#include <mg_task.h>
#include <utils_json.h>

xQueueHandle config_evt_queue;

extern xQueueHandle broardcast_evt_queue;

static char *tag = "config";

// Private prototypes
int config_list_del_files(MG_WS_MESSAGE *m, char *);
int config_save_wifi_conf( MG_WS_MESSAGE *m, char *);

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
			}
		}
		else {
			ESP_LOGD( tag, "root item is NULL");
		}
	}
	else {
		ESP_LOGD( tag, "flash_get_key returned: %d", rc);
	}
	free(buffer);

	return wifi_conf;
}

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
			case MG_ACTION_CONFIG_DEL_FILES:
				ESP_LOGI( tag, "CONFIG list/del files event received");
				rc = config_list_del_files( &m, resp);
				break;
			}
		}
		if( rc != ESP_OK ) {
			sprintf( resp, "{\"status\":\"Error '%s' in save of configaration!!!!\"}", flash_error_str(rc));

			m.message = resp;
			m.message_len = strlen(resp);
			// Send to mg_task for processing in MG_EV_POLL event
			xQueueSendToBack( broardcast_evt_queue, &m, 10 );

		}
	}
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

			m->message = resp;
			m->message_len = strlen(resp);
			// Send to mg_task for processing in MG_EV_POLL event
			xQueueSendToBack( broardcast_evt_queue, m, 10 );

			// Wait a few seconds for the ws frame to be send
			vTaskDelay(xDelay);
			esp_restart();
		}
	}
	return rc;
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
	fatfs_list_dir(del);
	if( del)
		sprintf(resp, "{\"status\":\"All files removed!!!!\"}");
	else
		sprintf(resp, "{\"status\":\"Files listed on console\"}");

	m->message = resp;
	m->message_len = strlen(resp);
	// Send to mg_task for processing in MG_EV_POLL event
	xQueueSendToBack( broardcast_evt_queue, m, 10 );

	return ESP_OK;
}

// ------------------------------------------
// Start the gpio task
// ------------------------------------------
void config_start_task() {
	config_evt_queue = xQueueCreate(10, sizeof(MG_WS_MESSAGE));
	xTaskCreate(config_task, "config_task", 4096, NULL, 10, NULL);
}
