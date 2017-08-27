/*
 ============================================================================
 Name        : test.c
 Author      : HP
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <freertos/semphr.h>

#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_system.h>
#include <esp_spi_flash.h>
#include <esp_event_loop.h>

#include "sdkconfig.h"

#include "wifi_task.h"
#include "mongoose.h"
#include "mg_task.h"
#include "fatfs.h"
#include "utils.h"
#include "gpio_task.h"
#include "relay_task.h"

SemaphoreHandle_t flashSemaphore = NULL;

void init_shared_resource_sem() {
	flashSemaphore = xSemaphoreCreateMutex();
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
	wifi_start_task();
//	utils_show_chip_info();
	return 0;

}

