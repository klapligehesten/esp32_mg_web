/*
 ============================================================================
 Name        : main.c
 Author      : Hans Peter Schultz / hp@hpes.dk
 Version     : 0.9b
 Description : esp32 starter project
 ============================================================================
 */

#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <freertos/semphr.h>
#include <esp_system.h>
#include <mdns_a.h>

#include "sdkconfig.h"

#include <nvs_flash.h>
#include <wifi.h>
#include "mg_task.h"
#include "fatfs.h"
#include "utils.h"
#include "gpio_task.h"
#include "relay_task.h"

static char *tag = "main";

SemaphoreHandle_t flashSemaphore = NULL;

void init_shared_resource_sem() {
	flashSemaphore = xSemaphoreCreateMutex();
}

void info_task(void *pvParameter) {
	while(1) {
		ESP_LOGD(tag, "main free heap size: %d, min. %d", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}
}

// ------------------------------------------
// Main app entry point
// ------------------------------------------
int app_main(void) {

	nvs_flash_init();
	init_shared_resource_sem();
	fatfs_mount();
	gpio_start_task();
	relay_gpio_start_task();
	wifi_start();
//	mdns_advertise("esp32", "rsp32_inst", "esp32_relay");
//	utils_show_chip_info();
//	xTaskCreate( info_task, "info_task", 2048, NULL, 10, NULL);

	return 0;

}

