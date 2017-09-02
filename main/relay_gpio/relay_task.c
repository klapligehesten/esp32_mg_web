#include <malloc.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/projdefs.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <cJSON.h>
#include <driver/gpio.h>
#include <rom/gpio.h>

#include "mg/mg_task.h"
#include "relay_gpio/relay_task.h"

xQueueHandle relay_gpio_evt_queue;

static char *tag = "relay_gpio";

#define MAX_RELAYS 5

#define RELAY_TYPE_SWITCH          0
#define RELAY_TYPE_LEARN           1
#define RELAY_TYPE_PUSH   		   2
#define RELAY_TYPE_LEARN_RUNNING   9


#define TIMER_STATE_ON  	      0
#define TIMER_STATE_OFF           1
#define TIMER_STATE_SECOND_ON     2
#define TIMER_STATE_LEARN_RUNNING 3

typedef struct {
	TimerHandle_t xtimer;
	int state;
	TickType_t on;
	TickType_t off;
	TickType_t second_on;
} RTIMER;

typedef struct {
	int no;
	int state;
	int rtype;
	int gpio;
	char *name;
	RTIMER timer;
} RELAYS, *P_RELAYS;



// Private prototypes
char *gen_response(P_RELAYS r);
void handle_learning( P_RELAYS r);

// ------------------------------------------
// relay task handle
// ------------------------------------------
void relay_gpio_task(void *pvParameter) {
	const TickType_t xDelay = 2000 / portTICK_PERIOD_MS;
	MG_WS_MESSAGE m = { .message = NULL};
	int rc;

	RELAYS relays[MAX_RELAYS] = {
		{0, 0, -1, GPIO_NUM_15, "G_15", { NULL, TIMER_STATE_ON, 0, 0, 0}},
		{0, 0, -1, GPIO_NUM_16, "G_16", { NULL, TIMER_STATE_ON, 0, 0, 0}},
		{0, 0, -1, GPIO_NUM_17, "G_17", { NULL, TIMER_STATE_ON, 0, 0, 0}},
		{0, 0, -1, GPIO_NUM_21, "G_21", { NULL, TIMER_STATE_ON, 0, 0, 0}},
		{0, 0, -1, GPIO_NUM_22, "G_22", { NULL, TIMER_STATE_ON, 0, 0, 0}}
	};

	for( int i = 0; i < MAX_RELAYS;i++) {
		gpio_pad_select_gpio(relays[i].gpio);
		gpio_set_direction(relays[i].gpio, GPIO_MODE_OUTPUT);
		relays[i].state = gpio_get_level(relays[i].gpio);
		relays[i].no = i;
	}

	while(1) {

		if(( rc = xQueueReceive(relay_gpio_evt_queue, &m, xDelay))) {
			if( rc == pdTRUE) {
				ESP_LOGI( tag, "Relay event received %s", m.message);
				cJSON *root = cJSON_Parse(m.message);
				cJSON *relay = cJSON_GetObjectItem(root, "gpio_relays");
				cJSON *c0 = cJSON_GetArrayItem(relay, 0);
				cJSON *c1 = cJSON_GetArrayItem(relay, 1);
				cJSON *c2 = cJSON_GetArrayItem(relay, 2);
				int val = c0->valueint;
				int no = c1->valueint;
				int tp = c2->valueint;
				free( c0);
				free( c1);
				free( c2);
				free( relay);
				free( root);
				if( no >= 0 || no <=MAX_RELAYS) {
					relays[no].state = val;
					relays[no].rtype = tp;

					switch( relays[no].rtype) {
					case RELAY_TYPE_LEARN_RUNNING:
					case RELAY_TYPE_SWITCH:
						gpio_set_level(relays[no].gpio, relays[no].state);
						break;
					case RELAY_TYPE_LEARN:
						handle_learning( &relays[no]);
						gpio_set_level(relays[no].gpio, relays[no].state);
						break;
					case RELAY_TYPE_PUSH:
						//TODO: Implement.
						break;
					}
				}
				free(m.message);
			}
		}
		if( m.message !=  NULL) {
			char *c = gen_response(relays);
			mg_broadcast_poll( c);
			free(c);
		}
	}
}

// ------------------------------------------
// Start the relay gpio task
// ------------------------------------------
void relay_gpio_start_task() {

	relay_gpio_evt_queue = xQueueCreate(10, sizeof(MG_WS_MESSAGE));
	xTaskCreate(relay_gpio_task, "relay_task", 20000, NULL, 10, NULL);
}

// ============================================================================
// Local functions
// ============================================================================
// ------------------------------------------
// Learning switch timer callback
// ------------------------------------------
void timerCallback( TimerHandle_t xTimer ) {
	P_RELAYS r = (P_RELAYS) pvTimerGetTimerID( xTimer );
	char tmp_buf[255];
	MG_WS_MESSAGE m;
	int len;

	ESP_LOGD( tag, "timer name %s", r->name);
	if( r->timer.state == TIMER_STATE_LEARN_RUNNING) {
		if( r->state) {
			r->state = 0;
			int x = (r->timer.second_on - r->timer.off);
			xTimerChangePeriod(r->timer.xtimer,x, x);
		}
		else {
			r->state = 1;
			int x = (r->timer.off - r->timer.on);
			xTimerChangePeriod(r->timer.xtimer,x, x);
		}

		len = sprintf(tmp_buf, "{\"gpio_relays\":{\"relay_%d\":%s,\"no\":%d,\"rtype\":9}}",
				 	 r->no, (r->state ? "true" : "false"), r->no);

		m.message = strdup(tmp_buf);
		m.flags = 0;
		m.message_len = len;
		xQueueSendToBack( relay_gpio_evt_queue, &m, 10);
	}
}

// ------------------------------------------
// Handle learning switch processing
// ------------------------------------------
void handle_learning( P_RELAYS r) {

	if( r->timer.xtimer == NULL) {
		// Init. timer
		r->timer.xtimer = xTimerCreate( r->name, 100, pdTRUE, (void *) r , timerCallback);
	}

	switch( r->timer.state) {
	case TIMER_STATE_ON:
		// save start time
		r->timer.on = xTaskGetTickCount();
		r->timer.state = TIMER_STATE_OFF;
		break;
	case TIMER_STATE_OFF:
		r->timer.off = xTaskGetTickCount();
		r->timer.state = TIMER_STATE_SECOND_ON;
		break;
	case TIMER_STATE_SECOND_ON:
		r->timer.second_on = xTaskGetTickCount();
		r->timer.state = TIMER_STATE_LEARN_RUNNING;

		int x = (r->timer.off - r->timer.on);
		xTimerChangePeriod(r->timer.xtimer,x, x);
		xTimerStart(r->timer.xtimer, x);
		break;
	case TIMER_STATE_LEARN_RUNNING:
		xTimerStop(r->timer.xtimer, 0);
		r->timer.state = TIMER_STATE_ON;
		r->state = 0;
		break;
	}
}

// ------------------------------------------
// Generate relay JSON response
// ------------------------------------------
char *gen_response(P_RELAYS relays) {
	int i;
	char tmp_buf[255];
	char *c = tmp_buf;
	c += sprintf(c, "{\"gpio_relays\": {");
	for( i = 0; i < MAX_RELAYS; i++) {
		c += sprintf(c, "\"relay_%d\":%s,", i, (relays[i].state ? "true" : "false"));
	}
	sprintf(c-1, "}}");

	return strdup( tmp_buf);
}

