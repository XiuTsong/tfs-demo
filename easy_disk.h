#ifndef __EASY_DISK_H__
#define __EASY_DISK_H__

#include "easy_defs.h"

typedef struct easy_disk_system {
	void *memory_pool;
	uint32_t total_bytes;
	struct block_metadata {
		uint32_t start;
		uint32_t num;
	} block;
	struct file_metadata {
		uint32_t file_pool_start;
		uint32_t bitmap_start;
	} file;
} easy_disk_system_t;

easy_status init_disk_layer(void **memory_pool, bool *is_init);
easy_status easy_flush_whole();
easy_status easy_load_whole();

#endif