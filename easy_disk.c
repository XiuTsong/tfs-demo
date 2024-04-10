#include "easy_disk.h"
#include "easy_defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

easy_disk_system_t global_disk_system;
const char easy_disk_name[] = "tfs_disk";

/*
 * If disk file has been created, read it to memory_pool.
 * Otherwise, create a new disk file.
 */
easy_status init_disk_layer(void **memory_pool, bool *is_init)
{
	FILE *disk;
    size_t num;

	*memory_pool = malloc(sizeof(char) * TOTAL_BYTES);
	memset(*memory_pool, 0, TOTAL_BYTES);

	global_disk_system.memory_pool = *memory_pool;
    global_disk_system.total_bytes = TOTAL_BYTES;

	disk = fopen(easy_disk_name, "rb+");
	if (disk == NULL) {
		*is_init = true;
		disk = fopen(easy_disk_name, "wb+");
		fclose(disk);
		return EASY_SUCCESS;
	}

	*is_init = false;

	num = fread(*memory_pool, 1, TOTAL_BYTES, disk);
    if (num != TOTAL_BYTES) {
        printf("init disk error\n");
        return EASY_DISK_INIT_FAILED;
    }

	fclose(disk);

	return EASY_SUCCESS;
}

/* Naive implementation: just flush the whole memory into disk */
easy_status easy_flush_whole()
{
	FILE *disk;
    size_t num;
	disk = fopen(easy_disk_name, "wb+");
	if (disk == NULL) {
        printf("disk file not found\n");
		return -EASY_DISK_NOT_FOUND_ERROR;
	}
    fseek(disk, 0, SEEK_SET);
    num = fwrite(global_disk_system.memory_pool, 1, global_disk_system.total_bytes, disk);
    if (num != TOTAL_BYTES) {
        printf("flush error\n");
        return EASY_DISK_FLUSH_ERROR;
    }

    // printf("write %lu bytes to disk\n", num);
    fclose(disk);

    return EASY_SUCCESS;
}

/* Naive implementation: just load the whole memory from disk */
easy_status easy_load_whole()
{
	FILE *disk;
    size_t num;

	disk = fopen(easy_disk_name, "rb+");
	if (disk == NULL) {
		return EASY_DISK_LOAD_ERROR;
	}

    fseek(disk, 0, SEEK_SET);
	num = fread(global_disk_system.memory_pool, 1, global_disk_system.total_bytes, disk);
    if (num != TOTAL_BYTES) {
        printf("init disk error\n");
        return EASY_DISK_INIT_FAILED;
    }

	fclose(disk);

	return EASY_SUCCESS;
}