#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <soc/soc.h>
#include <mdns.h>

#include "mdns_a.h"

static char *TAG = "mdns_task";

mdns_server_t *mdns = NULL;

void mdns_advertise( char *hostname, char *instanse, char *http_name) {

	esp_err_t err = mdns_init(TCPIP_ADAPTER_IF_STA, &mdns);
	if (err) {
		ESP_LOGE(TAG, "Failed starting MDNS: %u", err);
		return;
	}
	ESP_ERROR_CHECK( mdns_set_hostname( mdns, hostname));
	ESP_ERROR_CHECK( mdns_set_instance( mdns, instanse));
	ESP_ERROR_CHECK( mdns_service_add(mdns, "_http", "_tcp", 80));
	ESP_ERROR_CHECK( mdns_service_instance_set(mdns, "_http", "_tcp", http_name));

}
