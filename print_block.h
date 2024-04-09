#ifndef __PRINT_BLOCK_H__
#define __PRINT_BLOCK_H__

#include "easy_block.h"

typedef struct print_system {
	int print_state[MAX_BLOCK];
	uint32_t print_num;
	uint32_t print_size;
	uint32_t print_interval;
} print_system_t;

void print_blocks(uint32_t start_block_id, uint32_t block_num);

#define MAX_STR_LEN 24

#define COLOR_NONE "\033[0m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_DARK_GRAY "\033[1;30m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_PURPLE "\033[0;35m"
#define COLOR_BROWN "\033[0;33m"
#define COLOR_YELLOW "\033[5;33m"
#define COLOR_WHITE "\033[1;37m"
#endif