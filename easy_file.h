#ifndef __FILE_H__
#define __FILE_H__

#include "easy_block.h"
#include "easy_defs.h"

typedef unsigned int EASY_FILE_TYPE;

#define EASY_TYPE_FILE 0
#define EASY_TYPE_DIR 0

#define MAX_FILE_NUM 100
#define MAX_FILE_NAME_LEN 20
#define MAX_FILE_BLOCKS 10
#define MAX_DIR_NUM 10

typedef struct {
	char name[MAX_FILE_NAME_LEN];
	uint32_t block_ids[MAX_FILE_BLOCKS];
	uint32_t block_num;
	uint32_t file_size;  // Byte
	EASY_FILE_TYPE type; // EASY_FILE can be file or directory
	uint32_t id;
} easy_file_t;

typedef struct {
	uint32_t file_ids[MAX_FILE_NUM];
	easy_file_t *self_file; // Point to EASY_FILE struct that contains itself
	uint32_t file_num;
} easy_dir_t; // sizeof(EASY_FILE_DIR) = 8Byte * 101 < 1024Byte(1 Block)

easy_status easy_create_dir(const char *dir_name);

easy_status easy_create_file(const char *file_name);

easy_status easy_remove_file(const char *file_name);

easy_status easy_read_file(const char *file_name, uint32_t nbyte, void *Buf);

easy_status easy_write_file(const char *file_name, uint32_t nbyte, const void *Buf);

easy_status easy_dir_list_files(const char *dir_name, void *buf);

easy_status init_file_layer(void);

easy_status easy_pwd(void *buf);

easy_status easy_cd(const char *dir_name);

easy_status easy_cat(const char *file_name, void *buf);

easy_status easy_ls(void *buf);
#endif
