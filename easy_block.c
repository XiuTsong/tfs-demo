#include "easy_block.h"
#include "easy_defs.h"
// #include <stdio.h>
#include <string.h>

easy_block_system_t *global_block_system;
int block_state_machine[MAX_BLOCK_STATE_NUM][MAX_BLOCK_OP_NUM];

easy_status init_block_system(void *memory_pool, unsigned int nbyte)
{
	uint32_t i;
	uint32_t block_system_size;

	memset(memory_pool, 0, nbyte);

	global_block_system = memory_pool;
	block_system_size = sizeof(*global_block_system);
	global_block_system->block_base = memory_pool + block_system_size;
	global_block_system->block_num = (nbyte - block_system_size) / BLOCK_SIZE;
	for (i = 0; i < global_block_system->block_num; ++i) {
		global_block_system->bitmap[i] = 0;
	}

	return EASY_SUCCESS;
}

easy_status init_block_state_machine(void)
{
	for (int i = 0; i < MAX_BLOCK_STATE_NUM; ++i) {
		for (int j = 0; j < MAX_BLOCK_OP_NUM; ++j) {
			block_state_machine[i][j] = BLOCK_INVALID;
		}
	}
	block_state_machine[BLOCK_FREE][WRITE_NORMAL] = BLOCK_ALLOC;
	block_state_machine[BLOCK_FREE][WRITE_TRANS] = BLOCK_TRANS;
	block_state_machine[BLOCK_ALLOC][DELETE_NORMAL] = BLOCK_FREE;
	block_state_machine[BLOCK_TRANS][DELETE_TRANS] = BLOCK_FREE;
	block_state_machine[BLOCK_TRANS][WRITE_NORMAL] = BLOCK_ALLOC_OVER;
	block_state_machine[BLOCK_FREE_OVER][CLEAN_TRANS] = BLOCK_FREE;
	block_state_machine[BLOCK_FREE_OVER][WRITE_NORMAL] = BLOCK_ALLOC_OVER;
	block_state_machine[BLOCK_ALLOC_OVER][DELETE_NORMAL] = BLOCK_FREE_OVER;
	block_state_machine[BLOCK_ALLOC_OVER][CLEAN_NORMAL] = BLOCK_ALLOC;
	return EASY_SUCCESS;
}

easy_status init_block_layer(void *memory_pool, unsigned int nbyte)
{
	init_block_system(memory_pool, nbyte);

	init_block_state_machine();

	return EASY_SUCCESS;
}

easy_status block_state_transition(const uint32_t block_id, const block_op op)
{
	easy_block_t *block;

	block = get_block(block_id);
	block->state = block_state_machine[block->state][op];
	if (block->state == BLOCK_INVALID) {
		return EASY_BLOCK_STATE_ERROR;
	}
	return EASY_SUCCESS;
}

easy_block_t *get_block(const uint32_t block_id)
{
	return (easy_block_t *)(global_block_system->block_base + BLOCK_SIZE * block_id);
}

void *get_block_data(const uint32_t block_id)
{
	return (void *)(get_block(block_id)->block_data);
}

easy_status read_block(uint32_t block_id, uint32_t nbyte, void *buf)
{
	easy_block_t *block;

	block = get_block(block_id);
	memcpy(buf, block->block_data, nbyte);

	return EASY_SUCCESS;
}

easy_status write_block(uint32_t block_id, uint32_t nbyte, uint32_t write_pos, const void *buf)
{
	easy_block_t *block;
	void *write_buf;

	block = get_block(block_id);
	write_buf = (char *)block->block_data + write_pos;
	memcpy(write_buf, buf, nbyte);

	return EASY_SUCCESS;
}

easy_status alloc_block(uint32_t *block_id)
{
	easy_status status;
	uint32_t i;

	for (i = 0; i < MAX_BLOCK; ++i) {
		if (global_block_system->bitmap[i])
			continue;

		if (block_id)
			*block_id = i;
		status = block_state_transition(i, WRITE_NORMAL);
		// printf("alloc_block: %d\n", i);
		if (status != EASY_SUCCESS) {
			return status;
		}
		global_block_system->bitmap[i] = 1;
		return EASY_SUCCESS;
	}

	return EASY_BLOCK_ALLOC_ERROR;
}

easy_status alloc_block_trans(uint32_t *block_id)
{
	easy_status status;
	uint32_t i;

	for (i = 0; i < MAX_BLOCK; ++i) {
		if (!global_block_system->bitmap[i]) {
			if (block_id)
				*block_id = i;
			status = block_state_transition(i, WRITE_TRANS);
			if (status != EASY_SUCCESS) {
				return status;
			}
			/* Do not set bitmap here */
			// global_block_system->bitmap[i] = 1;
			return EASY_SUCCESS;
		}
	}

	return EASY_BLOCK_ALLOC_ERROR;
}

easy_status free_block(uint32_t block_id)
{
	global_block_system->bitmap[block_id] = 0;

	return EASY_SUCCESS;
}