#include "easy_block.h"
#include "easy_defs.h"
#include <string.h>

easy_block_system_t global_block_system;

easy_status init_block_layer(void *memory_pool, unsigned int nbyte)
{
	uint32_t i;

	global_block_system.block_base = memory_pool;
	global_block_system.block_num = nbyte / BLOCK_SIZE;
	for (i = 0; i < global_block_system.block_num; ++i) {
		global_block_system.bitmap[i] = 0;
	}

	memset(memory_pool, 0, nbyte);

	return EASY_SUCCESS;
}

easy_block_t *get_block(uint32_t block_id)
{
	return (easy_block_t *)(global_block_system.block_base + BLOCK_SIZE * block_id);
}

easy_status read_block(uint32_t block_id, uint32_t nbyte, void *buf)
{
	easy_block_t *Block;

	Block = get_block(block_id);
	memcpy(buf, Block->block_byte, nbyte);

	return EASY_SUCCESS;
}

easy_status write_block(uint32_t block_id, uint32_t nbyte, uint32_t write_pos, const void *buf)
{
	easy_block_t *Block;
	void *WriteBuf;

	Block = get_block(block_id);
	WriteBuf = (char *)Block->block_byte + write_pos;
	memcpy(WriteBuf, buf, nbyte);

	return EASY_SUCCESS;
}

easy_status alloc_block(uint32_t *block_id)
{
	uint32_t i;

	for (i = 0; i < MAX_BLOCK; ++i) {
		if (!global_block_system.bitmap[i]) {
			*block_id = i;
			global_block_system.bitmap[i] = 1;
			return EASY_SUCCESS;
		}
	}

	return EASY_BLOCK_ALLOC_ERROR;
}

easy_status free_block(uint32_t block_id)
{
	global_block_system.bitmap[block_id] = 0;

	return EASY_SUCCESS;
}