#ifndef __CONFIG_TASK__
#define __CONFIG_TASK__

#define CONFIG_OPEN_WIFI_NOT_ENABLED "wifi_no_conf"
#define CONFIG_WIFI_PARAMETERS "wifi_config"

typedef struct {
	int  enabled;
	char *ssid;
	char *passwd;
} CLI_W;

typedef struct {
	int  enabled;
	char *ssid;
	char *passwd;
	char *ipadr;
	char *gateway;
	char *netmask;
} AP_W;

typedef struct {
	int  del_fat_files;
} MISC;


typedef struct {
	CLI_W cli;
	AP_W ap;
	MISC misc;
} WIFI_CONF, *P_WIFI_CONF;

// --- public prototypes ---
void config_start_task();
P_WIFI_CONF config_get_wifi_conf();

#endif
