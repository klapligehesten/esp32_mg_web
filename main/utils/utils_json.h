#ifndef __UTILS_JSON__
#define __UTILS_JSON__

// --- public prototypes ---
int json_get_int( cJSON *root, char *key, int def);
char *json_get_str( cJSON *root, char *key, char *def);

#endif
