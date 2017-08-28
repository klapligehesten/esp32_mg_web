#ifndef __FATFS__
#define __FATFS__

typedef void (*fatfs_list_type)(char *c, int del);

// Public prototypes
esp_err_t fatfs_mount();
void 	  fatfs_umount();
void 	  fatfs_list_dir( int remove, fatfs_list_type);

#endif
