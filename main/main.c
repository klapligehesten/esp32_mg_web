/*
 ============================================================================
 Name        : main.c
 Author      : Hans Peter Schultz / hp@hpes.dk
 Version     : 0.9b
 Description : esp32 starter project
 ============================================================================
 */

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <esp_system.h>
#include <nvs_flash.h>

#include "sdkconfig.h"

#include "utils/mdns_a.h"
#include "wifi/wifi.h"
#include "mg/mg_task.h"
#include "fatfs/fatfs.h"
#include "utils/utils.h"
#include "gpio/gpio_task.h"

// Modules included
#define RELAY_GPIO

#ifdef RELAY_GPIO
	#include "relay_gpio/relay_task.h"
#endif

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
#ifdef RELAY_GPIO
	relay_gpio_start_task();
#endif
	wifi_start();

// Just testing for now
//	mdns_advertise("esp32", "rsp32_inst", "esp32_relay");
//	utils_show_chip_info();
//	xTaskCreate( info_task, "info_task", 2048, NULL, 10, NULL);

	return 0;

}

