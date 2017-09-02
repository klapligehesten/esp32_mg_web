#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <wear_levelling.h>

#include "fatfs/fatfs.h"

static const char *TAG = "fatfs";

// Handle to wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

// Mount path for the partition
const char *base_path = "/spiflash";

// Private prototypes
void unlink_file( char * filename);


// ------------------------------------------
// Mount FATFS on flash
// ------------------------------------------
esp_err_t fatfs_mount() {

	esp_err_t rc;
    ESP_LOGD(TAG, "Mounting FAT filesystem");
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 20,
            .format_if_mount_failed = true
    };

    rc = esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle);
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount fatfs: 0x%x", rc);
    }

    return rc;
}

// ------------------------------------------
// Unmount FATFS from flash
// ------------------------------------------
void fatfs_umount() {
    ESP_LOGD(TAG, "Unmounting FAT filesystem");
    ESP_ERROR_CHECK( esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle));
}


// ------------------------------------------
// List all files
// ------------------------------------------
void fatfs_list_dir( int remove, fatfs_list_type func) {
	DIR *d;
	struct dirent *dir;
	char *c;

	d = opendir(base_path);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if( remove)
				unlink_file( dir->d_name);
			c = strdup( dir->d_name);
			func( c, remove);
			free(c);
		}
	}

    closedir(d);
}

// ------------------------------------------
// Remove all files
// ------------------------------------------
void unlink_file( char * filename) {
	char tmp_filename[255];
	sprintf(tmp_filename, "%s/%s", base_path, filename);

	unlink(tmp_filename);
}


