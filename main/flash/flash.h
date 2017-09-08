#ifndef __FLASH__
#define __FLASH__

// --- public prototypes ---
esp_err_t flash_get_key( char *key, char **value);
esp_err_t flash_set_key( char *key, char *value);
esp_err_t flash_del_key( char *key);
esp_err_t flash_key_exists( char *key);
esp_err_t flash_init_user_space( char *namespace);

#endif
