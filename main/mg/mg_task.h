#ifndef __MG_TASKS__
#define __MG_TASKS__

#include <mongoose.h>


typedef void (*mg_process_http_request_type)(struct mg_connection *, struct http_message *);

#define MG_ACTION_CONFIG_WIFI 1
#define MG_ACTION_CONFIG_DEL_FILES 2
#define MG_ACTION_RELAY_GPIO_TOGGLE 3

typedef struct {
	int  flags;
	char *message;
	int  message_len;
	int  action;
} MG_WS_MESSAGE, P_MG_WS_MESSAGE;


// --- public prototypes ---
void mg_process_http_request( struct mg_connection* nc, struct http_message* message);
void mg_process_http_request_config( struct mg_connection* nc, struct http_message* message);
void mg_start_task(mg_process_http_request_type func);
void mg_broadcast_poll( char* resp);

void mg_stop_task();

#endif
