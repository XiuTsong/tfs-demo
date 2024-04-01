#ifndef __BLOCK_H__
#define __BLOCK_H__

#include "easy_defs.h"

#define MAX_BLOCK 100
#define BLOCK_SIZE 1024

typedef struct {
	bool bitmap[MAX_BLOCK];
	uint32_t block_num;
	void *block_base;
} easy_block_system_t;

typedef struct {
	char block_byte[BLOCK_SIZE];
} easy_block_t;

easy_status init_block_layer(void *memory_pool, uint32_t nbyte);

easy_status read_block(uint32_t block_id, uint32_t nbyte, void *buf);

easy_status write_block(uint32_t block_id, uint32_t nbyte,
			uint32_t pos, // Write start position
			const void *buf);

easy_status alloc_block(uint32_t *block_id);

easy_status free_block(uint32_t block_id);

easy_block_t *get_block(uint32_t block_id);

#endif