#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/projdefs.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <malloc.h>
#include <cJSON.h>
#include <driver/gpio.h>
#include <rom/gpio.h>


#include "mg_task.h"
#include "relay_task.h"

xQueueHandle relay_gpio_evt_queue;

static char *tag = "relay_gpio";

#define MAX_RELAYS 5
static int gpios[MAX_RELAYS] ={
		GPIO_NUM_15,
		GPIO_NUM_16,
		GPIO_NUM_17,
		GPIO_NUM_21,
		GPIO_NUM_22};

// Private prototypes
char *gen_response( int relays[]);



// ------------------------------------------
// relay task handle
// ------------------------------------------
void relay_gpio_task(void *pvParameter) {
	const TickType_t xDelay = 2000 / portTICK_PERIOD_MS;
	int relays[MAX_RELAYS] ={0,0,0,0,0};
	MG_WS_MESSAGE m = { .message = NULL};
	int rc;

	for( int i = 0; i < MAX_RELAYS;i++) {
		relays[i] = gpio_get_level(gpios[i]);
	}

	while(1) {

		if(( rc = xQueueReceive(relay_gpio_evt_queue, &m, xDelay))) {
			if( rc == pdTRUE) {
				ESP_LOGI( tag, "Relay event received");
				cJSON *root = cJSON_Parse(m.message);
				cJSON *relay = cJSON_GetObjectItem(root, "gpio_relays");
				cJSON *c0 = cJSON_GetArrayItem(relay, 0);
				cJSON *c1 = cJSON_GetArrayItem(relay, 1);
				int val = c0->valueint;
				int no = c1->valueint;
				free( c0);
				free( c1);
				free( relay);
				free( root);
				if( no >= 0 || no <=MAX_RELAYS) {
					relays[no] = val;
					gpio_set_level(gpios[no], relays[no]);
				}
				free(m.message);
			}
		}
		if( m.message !=  NULL) {
			char *c = gen_response( relays);
			mg_broadcast_poll( c);
			free(c);
		}
	}
}

// ------------------------------------------
// Generate relay json response
// ------------------------------------------
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

	for( int i = 0; i < MAX_RELAYS;i++) {
		gpio_pad_select_gpio(gpios[i]);
		gpio_set_direction(gpios[i], GPIO_MODE_OUTPUT);
	}

	relay_gpio_evt_queue = xQueueCreate(10, sizeof(MG_WS_MESSAGE));
	xTaskCreate(relay_gpio_task, "relay_task", 20000, NULL, 10, NULL);
}
