#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_system.h>

#include "config/config_html.h"
#include "config/config_task.h"

#include "mg/mg_task.h"

// --- internal prototypes ---
void mongoose_event_handler( struct mg_connection *nc, int ev, void *evData);
struct mg_str upload_fname(struct mg_connection *nc, struct mg_str fname);
void process_websocket_frame( struct mg_connection *nc, struct websocket_message *evData);
void mg_broadcast_message( struct mg_connection *nc, char *message, int len);

// This pointer either points to config html page
// processing or to process_http_request for 'normal'
// website processing from flash fat file system
mg_process_http_request_type mg_process_http_request_ptr;

SemaphoreHandle_t uploadSemaphore = NULL;

static char *tag = "mg_task";


#define MG_TASK_STATE_STOP 1
#define MG_TASK_STATE_RUNNING 2
static int mg_task_state = MG_TASK_STATE_STOP;

static struct mg_serve_http_opts s_http_server_opts;

extern const char *base_path;
extern xQueueHandle config_evt_queue;
extern xQueueHandle relay_gpio_evt_queue;
xQueueHandle broardcast_evt_queue;

typedef struct {
	char *cmd;
	int action;
	xQueueHandle *evt_queue;
} QUE_EVENTS_T, *P_QUE_EVENTS_T;

#define MAX_QUE_EVENTS 4
QUE_EVENTS_T que_events_t[MAX_QUE_EVENTS] = {
	{"config", MG_ACTION_CONFIG_WIFI, &config_evt_queue},
	{"config_get", MG_ACTION_CONFIG_GET_WIFI, &config_evt_queue},
	{"config_del_files", MG_ACTION_CONFIG_DEL_FILES, &config_evt_queue},
	{"gpio_relays", MG_ACTION_RELAY_GPIO_TOGGLE, &relay_gpio_evt_queue}
};

// ------------------------------------
// Process MG_EV_HTTP_REQUEST event
// ------------------------------------
void mg_process_http_request( struct mg_connection* nc, struct http_message* message) {
	// ESP_LOGI(tag, "Enter process_http_request:%.*s", message->uri.len, message->uri.p);
	mg_serve_http(nc, message, s_http_server_opts);
}

// ------------------------------------
// Process config MG_EV_HTTP_REQUEST
// ------------------------------------
void mg_process_http_request_config( struct mg_connection* nc, struct http_message* message) {
	// ESP_LOGI(tag, "Enter process_http_request_config");
	if ((mg_vcmp(&message->uri, "/" ) == 0 || mg_vcmp(&message->uri, "/config.html" ) == 0)) {
		mg_send_head(nc, 200, config_html_len, "Content-Type: text/html");
		mg_send(nc, config_html, config_html_len);
	}
	else if (mg_vcmp(&message->uri, "/config.css") == 0) {
		mg_send_head(nc, 200, config_css_len, "Content-Type: text/css");
		mg_send(nc, config_css, config_css_len);
	}
	else if (mg_vcmp(&message->uri, "/config.js") == 0) {
		mg_send_head(nc, 200, config_js_len, "Content-Type: text/javascript");
		mg_send(nc, config_js, config_js_len);
	}
	nc->flags |= MG_F_SEND_AND_CLOSE;
}

// ------------------------------------
// Dispatch MG_EV_WEBSOCKET_FRAME event
// ------------------------------------
void process_websocket_frame( struct mg_connection *nc, struct websocket_message *evData) {
	ESP_LOGD(tag, "WS_EV_WEBSOCKET_FRAME");
	MG_WS_MESSAGE m;
	int found = 0;

	m.message = strndup( (const char *)evData->data, evData->size);
	m.flags = evData->flags;
	m.message_len = evData->size;
	cJSON *request_json = cJSON_Parse(m.message);
	cJSON* item = cJSON_GetArrayItem(request_json, 0);
	if( item != NULL) {
		for( int i = 0; i < MAX_QUE_EVENTS; i++) {
			if( strcmp(item->string, que_events_t[i].cmd) == 0) {
				m.action = que_events_t[i].action;
				xQueueSendToBack( *que_events_t[i].evt_queue, &m, 10 );
				found = 1;
				break;
			}
		}
		if( !found) {
			ESP_LOGI(tag, "processing %s service not implemented yet :-)", item->string);
		}
	}
	else {
		ESP_LOGE(tag, "JSON object field type NULL");
	}
	free(item);
	free(request_json);
}

// ---------------------------------
// callback from mg_file_upload_handler
// ---------------------------------
struct mg_str upload_fname(struct mg_connection *nc, struct mg_str fname) {
	struct mg_str c;
	c.p = calloc(1, 255);
	c.len = sprintf((char*)c.p, "%s/%.*s", base_path, fname.len, fname.p);
	return c;
}

// ---------------------------------
// Mongoose event handler.
// ---------------------------------
void mongoose_event_handler(struct mg_connection *nc, int ev, void *evData) {
//	ESP_LOGD(tag, "enter free heap size: %d, min. %d", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());

	switch (ev) {
	case MG_EV_HTTP_REQUEST:
		mg_process_http_request_ptr( nc, (struct http_message*) evData);
		break;
	case MG_EV_HTTP_PART_BEGIN:
		xSemaphoreTake( uploadSemaphore, ( TickType_t ) 10 );
		mg_file_upload_handler(nc, ev, evData, upload_fname);
		break;
	case MG_EV_HTTP_PART_DATA:
		mg_file_upload_handler(nc, ev, evData, upload_fname);
		break;
	case MG_EV_HTTP_PART_END:
		mg_file_upload_handler(nc, ev, evData, upload_fname);
		xSemaphoreGive( uploadSemaphore );
		break;
	case MG_EV_HTTP_MULTIPART_REQUEST_END:
		nc->flags |= MG_F_SEND_AND_CLOSE;
		break;
	case MG_EV_WEBSOCKET_FRAME:
    	process_websocket_frame( nc, (struct websocket_message *)evData);
		break;
	case MG_EV_POLL: {
		MG_WS_MESSAGE p;

		if(xQueueReceive(broardcast_evt_queue, &p, 0) == pdPASS) {
			// Don't broadcast when uploading
		    if( xSemaphoreTake( uploadSemaphore, ( TickType_t ) 100 ) == pdTRUE ) {
		    	mg_broadcast_message(nc, p.message, p.message_len);
		    	free(p.message);
		    }
			xSemaphoreGive( uploadSemaphore );
		}
		break;
		}
	}
//	ESP_LOGD(tag, "exit free heap size: %d, min. %d", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
}

// ------------------------------------------
// Send to mg_task for processing in MG_EV_POLL event
// ------------------------------------------
void mg_broadcast_poll( char* resp) {
	MG_WS_MESSAGE m;
	m.message = strdup(resp);
	m.message_len = strlen(resp);
	xQueueSendToBack(broardcast_evt_queue, &m, 10);
}

// ------------------------------------------
// Send a message to all connected ws clients
// ------------------------------------------
void mg_broadcast_message( struct mg_connection *nc, char *message, int len) {
	struct mg_connection *c;
	// Instead of mg_next(.. i've rewritten this to show
	// witch line 'panics' if something goes wrong
	c = nc->mgr->active_connections;
	while(1){
		mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, message, len);
		if( c->next	== NULL)
			break;
		c = c->next;
	}
}

// ---------------------------------
// Mongoose Task
// ---------------------------------
void mongoose_task(void *data) {
	struct mg_mgr mgr;
	mg_mgr_init(&mgr, NULL);
	struct mg_connection *c = mg_bind(&mgr, ":80", mongoose_event_handler);
	ESP_LOGD(tag, "MG task starting");
	if (c == NULL) {
		ESP_LOGE(tag, "No connection to wifi");
		vTaskDelete(NULL);
		return;
	}
	mg_task_state = MG_TASK_STATE_RUNNING;
	mg_set_protocol_http_websocket(c);

	while (mg_task_state == MG_TASK_STATE_RUNNING) {
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr);
}

// -------------------------------------
// Start mongoose task pinned to one cpu
// -------------------------------------
void mg_start_task( mg_process_http_request_type func) {
	if( mg_task_state == MG_TASK_STATE_RUNNING) {
		ESP_LOGD(tag, "MG task running");
		return;
	}
	uploadSemaphore = xSemaphoreCreateMutex();
	mg_process_http_request_ptr = func;
	memset(&s_http_server_opts, 0, sizeof(s_http_server_opts));
	s_http_server_opts.document_root = base_path;
	s_http_server_opts.index_files = NULL; // Don't work

	config_start_task();
	broardcast_evt_queue = xQueueCreate(10, sizeof(MG_WS_MESSAGE));
	xTaskCreate(mongoose_task, "mgTask", 20000, NULL, 5, NULL);
}

// ---------------------------------
// Stop mongoose task
// ---------------------------------
void mg_stop_task() {
	if( mg_task_state == MG_TASK_STATE_RUNNING)
		mg_task_state = MG_TASK_STATE_STOP;
}

