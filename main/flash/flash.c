#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/projdefs.h>
#include <freertos/semphr.h>
#include <nvs.h>
#include <nvs_flash.h>

char *user_namespace = NULL;
#define MAX_VAR_LEN 4*1024


static char *tag = "flash";

extern SemaphoreHandle_t flashSemaphore;


// --------------------------------------
// create flash user namespace
// --------------------------------------
esp_err_t flash_init_user_space( char *namespace) {
	esp_err_t rc;
	user_namespace = strdup( namespace);
	if(( rc = nvs_flash_init_partition(namespace)) != ESP_OK) {
		ESP_LOGE( tag, "Error %x in nvs_flash_init_partition", rc);
	}

	return rc;
}

// --------------------------------------
// Get key value
// --------------------------------------
esp_err_t flash_get_key( char *key, char **value) {
	esp_err_t  rc;
	char *tmp_val = malloc(MAX_VAR_LEN);
	size_t len = MAX_VAR_LEN;
	nvs_handle my_handle;

    if( xSemaphoreTake( flashSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
    	if( (rc = nvs_open( user_namespace, NVS_READWRITE, &my_handle)) == ESP_OK) {

			if( (rc = nvs_get_str( my_handle, key, tmp_val, &len)) == ESP_OK) {
				*value = strdup( tmp_val);
			}
	    	nvs_commit(my_handle);
	    	nvs_close(my_handle);
			xSemaphoreGive( flashSemaphore );
    	}
    	else {
			xSemaphoreGive( flashSemaphore );
			ESP_LOGE( tag, "Error %x on nvs_open", rc);
    	}
    }
    else {
    	rc = ESP_ERR_INVALID_STATE;
	}

    free( tmp_val);
	return rc;
}

// --------------------------------------
// key exists
// --------------------------------------
esp_err_t flash_key_exists( char *key) {
	esp_err_t  rc;
	char *tmp_val = malloc(MAX_VAR_LEN);
	size_t len = MAX_VAR_LEN;
	nvs_handle my_handle;

    if( xSemaphoreTake( flashSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
    	if( (rc = nvs_open( user_namespace, NVS_READWRITE, &my_handle)) == ESP_OK) {
    		rc = nvs_get_str( my_handle, key, tmp_val, &len);
        	nvs_commit(my_handle);
	    	nvs_close(my_handle);
			xSemaphoreGive( flashSemaphore );
    	}
    }
    else {
    	rc = ESP_ERR_INVALID_STATE;
	}
    free(tmp_val);
	return rc;
}

// --------------------------------------
// remove key
// --------------------------------------
esp_err_t flash_del_key( char *key) {
	esp_err_t  rc;
	nvs_handle my_handle;

    if( xSemaphoreTake( flashSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
    	if( (rc = nvs_open( user_namespace, NVS_READWRITE, &my_handle)) == ESP_OK) {
    		rc = nvs_erase_key( my_handle, key);
    	}
    	nvs_commit(my_handle);
		nvs_close(my_handle);
		xSemaphoreGive( flashSemaphore );
    }
    else {
    	rc = ESP_ERR_INVALID_STATE;
	}

	return rc;
}

// --------------------------------------
// Set key value
// --------------------------------------
esp_err_t flash_set_key( char *key, char *value) {
	esp_err_t  rc;
	nvs_handle my_handle;

    if( xSemaphoreTake( flashSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
    	if( (rc = nvs_open( user_namespace, NVS_READWRITE, &my_handle)) == ESP_OK) {
    		rc = nvs_set_str( my_handle, key, value);
    	}
    	nvs_commit(my_handle);
    	nvs_close(my_handle);
    	xSemaphoreGive( flashSemaphore );
	}
    else {
    	rc = ESP_ERR_INVALID_STATE;
    }

	return rc;
}

