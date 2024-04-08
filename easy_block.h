#ifndef __BLOCK_H__
#define __BLOCK_H__

#include "easy_defs.h"

#define MAX_BLOCK 100
#define BLOCK_META_SIZE 24
#define BLOCK_SIZE 1024
#define BLOCK_DATA_SIZE (BLOCK_SIZE - BLOCK_META_SIZE)

typedef enum easy_block_operation {
	WRITE_NORMAL = 0,
	WRITE_TRANS,
	DELETE_NORMAL,
	DELETE_TRANS,
	CLEAN,
	MAX_BLOCK_OP_NUM
} block_op;

typedef enum easy_block_state {
	BLOCK_INVALID = -1,
	BLOCK_FREE = 0,
	BLOCK_ALLOC,
	BLOCK_TRANS,
	BLOCK_ALLOC_OVER,
	BLOCK_FREE_OVER,
	MAX_BLOCK_STATE_NUM
} block_state;

typedef struct {
	bool bitmap[MAX_BLOCK];
	uint32_t block_num;
	void *block_base;
} easy_block_system_t;

typedef struct {
	/* metadata: at most 24 Bytes */
	block_state state;

	/* data: 1000 Bytes */
	char block_data[BLOCK_DATA_SIZE];
} easy_block_t;

easy_status init_block_layer(void *memory_pool, uint32_t nbyte);

easy_status read_block(uint32_t block_id, uint32_t nbyte, void *buf);

easy_status write_block(uint32_t block_id, uint32_t nbyte,
			uint32_t pos, // Write start position
			const void *buf);

easy_status alloc_block(uint32_t *block_id);

easy_status alloc_block_trans(uint32_t *block_id);

easy_status free_block(uint32_t block_id);

easy_status free_block_trans(uint32_t block_id);

easy_status clean_block(uint32_t block_id);

easy_block_t *get_block(const uint32_t block_id);

easy_status list_blocks(void *buf);

void *get_block_data(const uint32_t block_id);

#endif