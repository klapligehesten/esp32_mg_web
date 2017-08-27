#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <esp_spi_flash.h>
#include <esp_system.h>

#include "utils.h"

static char *tag = "web_main";

// ------------------------------------------
// Show some info in the system
// ------------------------------------------
void utils_show_chip_info() {
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	ESP_LOGD(tag, "This is ESP32 chip model %d with %d CPU cores, WiFi%s%s, ",
			chip_info.model,
			chip_info.cores,
			(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
			(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

	ESP_LOGD(tag, "silicon revision %d, ", chip_info.revision);
	ESP_LOGD(tag, "%dMB %s flash", spi_flash_get_chip_size() / (1024 * 1024),
			(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
	ESP_LOGD(tag, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
	ESP_LOGD(tag, "esp_get_minimum_free_heap_size: %d", esp_get_minimum_free_heap_size());
	ESP_LOGD(tag, "esp_get_idf_version: %s", esp_get_idf_version());
	ESP_LOGD(tag, "AP name: %s", utils_get_ap_name());

}
char *utils_get_ap_name() {
	char tmp[255];
	uint8_t mac;
	uint8_t *p_mac = &mac;
	esp_efuse_mac_get_default(&mac);
	sprintf( tmp, "ESP32_%02X%02X%02X%02X%02X%02X",
				 (unsigned int)*(p_mac), (unsigned int)*(p_mac+1),
				 (unsigned int)*(p_mac+2), (unsigned int)*(p_mac+3),
				 (unsigned int)*(p_mac+4), (unsigned int)*(p_mac+5));
	return strdup(tmp);
}
