#include <stdint.h>

#include <driver/gpio.h>
#include <esp_attr.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <rom/gpio.h>

#include "flash/flash.h"
#include "config/config_task.h"

#include "gpio/gpio_task.h"

static char *tag = "gpio_task";
xQueueHandle gpio_evt_queue;

static void IRAM_ATTR gpio_isr_handler(void* arg);
void gpio_task(void *pvParameter);
void button_setup( gpio_config_t *io_conf, int pin, int mode);

// ------------------------------------------
// gpio interrupt handler
// ------------------------------------------
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// ------------------------------------------
// gpio task handle
// ------------------------------------------
void gpio_task(void *pvParameter) {
    gpio_config_t io_conf;
   	uint32_t io_num;
	button_setup(&io_conf, GPIO_NUM_0, GPIO_MODE_INPUT);
	while(1) {
		if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			ESP_LOGI(tag, "GPIO[%d] intr, val: %d", io_num, gpio_get_level(io_num));
			if( io_num == GPIO_NUM_0 && gpio_get_level(io_num)) {
				// Set wifi configuration enabled and restart system
				flash_del_key(CONFIG_OPEN_WIFI_NOT_ENABLED);
				esp_restart();
			}
		}
	}
}

// ------------------------------------------
// Set gpio pin/mode
// ------------------------------------------
void gpio_set_pin( gpio_config_t *io_conf, int pin, int mode) {

	//interrupt of rising edge
	io_conf->intr_type = GPIO_PIN_INTR_POSEDGE;

	//bit mask of the pins, use GPIO0 here
	io_conf->pin_bit_mask |= (1 << pin);

	//set as->input mode
	io_conf->mode = mode;
	//GPIO_MODE_INPUT;

	//enable pull-up mode
	io_conf->pull_up_en = 1;
	gpio_config(io_conf);

	//change gpio intrrupt type for one pin
	gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE);

	//install gpio isr service
	gpio_install_isr_service(0);

	//hook isr handler for specific gpio pin
	gpio_isr_handler_add( pin, gpio_isr_handler,(void*) pin);

	//remove isr handler for gpio number.
	gpio_isr_handler_remove(pin);

	//hook isr handler for specific gpio pin again
	gpio_isr_handler_add(pin, gpio_isr_handler,(void*) pin);
}

// ------------------------------------------
// initialize the button
// ------------------------------------------
void button_setup( gpio_config_t *io_conf, int pin, int mode) {

	//create a queue to handle gpio event from isr
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

	gpio_set_pin( io_conf, pin, GPIO_MODE_INPUT);

}

// ------------------------------------------
// Start the gpio task
// ------------------------------------------
void gpio_start_task() {
	xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
}

