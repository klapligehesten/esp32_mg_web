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

#define DEFAULT_NAMESPACE "defspace"
#define MAX_VAR_LEN 4*1024

extern SemaphoreHandle_t flashSemaphore;

// --------------------------------------
// Get key value
// --------------------------------------
esp_err_t flash_get_key( char *key, char **value) {
	esp_err_t  rc;
	char tmp_val[MAX_VAR_LEN+1];
	size_t len = MAX_VAR_LEN;
	nvs_handle my_handle;

    if( xSemaphoreTake( flashSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
    	if( (rc = nvs_open( DEFAULT_NAMESPACE, NVS_READWRITE, &my_handle)) == ESP_OK) {

			if( (rc = nvs_get_str( my_handle, key, tmp_val, &len)) == ESP_OK) {
				*value = strdup( tmp_val);
			}
	    	nvs_commit(my_handle);
	    	nvs_close(my_handle);
			xSemaphoreGive( flashSemaphore );
    	}
    }
    else {
    	rc = ESP_ERR_INVALID_STATE;
	}

	return rc;
}

// --------------------------------------
// key exists
// --------------------------------------
esp_err_t flash_key_exists( char *key) {
	esp_err_t  rc;
	char tmp_val[MAX_VAR_LEN+1];
	size_t len = MAX_VAR_LEN;
	nvs_handle my_handle;

    if( xSemaphoreTake( flashSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
    	if( (rc = nvs_open( DEFAULT_NAMESPACE, NVS_READWRITE, &my_handle)) == ESP_OK) {
    		rc = nvs_get_str( my_handle, key, tmp_val, &len);
        	nvs_commit(my_handle);
	    	nvs_close(my_handle);
			xSemaphoreGive( flashSemaphore );
    	}
    }
    else {
    	rc = ESP_ERR_INVALID_STATE;
	}

	return rc;
}

// --------------------------------------
// remove key
// --------------------------------------
esp_err_t flash_del_key( char *key) {
	esp_err_t  rc;
	nvs_handle my_handle;

    if( xSemaphoreTake( flashSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
    	if( (rc = nvs_open( DEFAULT_NAMESPACE, NVS_READWRITE, &my_handle)) == ESP_OK) {
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
    	if( (rc = nvs_open( DEFAULT_NAMESPACE, NVS_READWRITE, &my_handle)) == ESP_OK) {
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

// --------------------------------------
// Display error
// --------------------------------------
char *flash_error_str( esp_err_t rc) {
	switch(rc) {
	case ESP_ERR_NVS_NOT_FOUND:
		return "Requested key does not exist";
	case ESP_ERR_NVS_NOT_INITIALIZED :
		return "The storage driver is not initialized";
	case ESP_ERR_NVS_TYPE_MISMATCH   :
		return "The type of set or get operation doesn't match the type of value stored in NVS";
	case ESP_ERR_NVS_READ_ONLY       :
		return "Storage handle was opened as read only";
	case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
		return "There is not enough space in the underlying storage to save the value";
	case ESP_ERR_NVS_INVALID_NAME    :
		return "Namespace name does not satisfy constraints";
	case ESP_ERR_NVS_INVALID_HANDLE  :
		return "Handle has been closed or is NULL";
	case ESP_ERR_NVS_REMOVE_FAILED   :
		return "The value was not updated because flash write operation has failed.";
	case ESP_ERR_NVS_KEY_TOO_LONG    :
		return "Key name is too long";
	case ESP_ERR_NVS_PAGE_FULL       :
		return "Internal error; never returned by nvs_ API functions";
	case ESP_ERR_NVS_INVALID_STATE   :
		return "NVS is in an inconsistent state";
	case ESP_ERR_NVS_INVALID_LENGTH  :
		return "String or blob length is not sufficient to store data";
	case ESP_ERR_NVS_NO_FREE_PAGES   :
		return "NVS partition does not contain any empty pages.";
	case ESP_ERR_NVS_VALUE_TOO_LONG  :
		return "String or blob length is longer than supported by the implementation";
	}
	return "Unknown error";
}
