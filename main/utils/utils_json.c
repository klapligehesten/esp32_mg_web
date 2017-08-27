#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <esp_log.h>
#include <esp_err.h>
#include <cJSON.h>
#include <cJSON_Utils.h>

#include "utils_json.h"

static char *tag = "json_util";

int json_get_int( cJSON *root, char *key, int def) {

	cJSON *cc = cJSON_GetObjectItem(root, key);
	int i;
	if( cc == NULL) {
		ESP_LOGD( tag, "get_int %s is NULL", key);
		return def;
	}

	i = cc->valueint;
	ESP_LOGD( tag, "Key:%s,Val:%d", key, i);
	return i;

}

char *json_get_str( cJSON *root, char *key, char *def) {

	char *s;
	cJSON *cc = cJSON_GetObjectItem(root, key);
	if( cc == NULL) {
		ESP_LOGD( tag, "get_str %s is NULL", key);
		return def;
	}

	if( cc->type == cJSON_String) {
		s = strdup(cc->valuestring);
		ESP_LOGD( tag, "Key:%s,Val:%s", key, s);
		return s;
	}

	return def;

}

