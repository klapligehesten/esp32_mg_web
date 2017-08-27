#ifndef __FATFS__
#define __FATFS__

// Public prototypes
esp_err_t fatfs_mount();
void 	  fatfs_umount();
void 	  fatfs_list_dir( int remove);

#endif
