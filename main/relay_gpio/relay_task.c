#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/projdefs.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <malloc.h>
#include <cJSON.h>

#include "mg_task.h"
#include "relay_task.h"

#define MAX_RELAYS 8
xQueueHandle relay_gpio_evt_queue;


static char *tag = "relay_gpio";

extern xQueueHandle broardcast_evt_queue;

// Private prototypes
char *gen_response( int relays[]);


// ---------------------------------
// Mongoose event handler. Dont work
// ---------------------------------
/*
#define MG_CTL_MSG_MESSAGE_SIZE 8192
struct ctl_msg {
  mg_event_handler_t callback;
  char message[MG_CTL_MSG_MESSAGE_SIZE];
};

void relay_event_handler( struct mg_connection *nc, int ev, void *evData) {
	struct ctl_msg *x;
	switch (ev) {
	case MG_EV_POLL:
 		x = (struct ctl_msg *) evData;
		ESP_LOGD( tag, "MG_EV_POLL %s", x->message);
		break;
	default:
		ESP_LOGD( tag, "default %d", ev);
		break;
	}
}
*/
// ------------------------------------------
// relay task handle
// ------------------------------------------
void relay_gpio_task(void *pvParameter) {
	MG_WS_MESSAGE m = {.nc = NULL, .message = NULL, .message_len = 0, .action = -1};
	const TickType_t xDelay = 2000 / portTICK_PERIOD_MS;
	int relays[MAX_RELAYS] ={0,0,0,0,0,0,0,0};

	while(1) {
		if(xQueueReceive(relay_gpio_evt_queue, &m, xDelay) == pdPASS) {
			ESP_LOGI( tag, "Relay event received");
			cJSON *root = cJSON_Parse(m.message);
			cJSON *relay = cJSON_GetObjectItem(root, "gpio_relays");
			int val = (cJSON_GetArrayItem(relay, 0))->valueint;
			int no = (cJSON_GetArrayItem(relay, 1))->valueint;
			free( relay);
			free( root);
			if( no >= 0 || no <=8) {
				relays[no] = val;
				// TODO: send mess to gpio task
			}
			free(m.message);

		}
		if( m.nc != NULL) {
			m.message = gen_response( relays);
			m.message_len = strlen(m.message);

			// Send to mg_task for prcessing in MG_EV_POLL event
			xQueueSendToBack( broardcast_evt_queue, &m, 10 );

			// calling ouer own broardcast partially works,
			// but 'panics' frequently because mongoose is'nt reintrant
			// mg_broadcast_message( m.nc, m.message, m.message_len);

			// mongoose broardcast don't work
			// mg_broadcast(m.nc->mgr, relay_event_handler, m.message, m.message_len);


		}
	}
}

char *gen_response( int relays[]) {
	int i;
	char tmp_buf[255];
	char *c = tmp_buf;
	c += sprintf(c, "{\"gpio_relays\": {");
	for( i = 0; i < MAX_RELAYS; i++) {
		c += sprintf(c, "\"relay_%d\":%s,", i, (relays[i] ? "true" : "false"));
	}
	sprintf(c-1, "}}");

	return strdup( tmp_buf);
}

// ------------------------------------------
// Start the relay gpio task
// ------------------------------------------
void relay_gpio_start_task() {
	relay_gpio_evt_queue = xQueueCreate(10, sizeof(MG_WS_MESSAGE));
	xTaskCreate(relay_gpio_task, "relay_task", 20000, NULL, 10, NULL);
}
