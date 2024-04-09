#include "print_block.h"
#include "easy_block.h"
#include <stdio.h>

print_system_t print_core;

#define DEFAULT_PRINT_SIZE 4
#define DEFAULT_PRINT_INTERVAL 4
#define DEFAULT_BLOCK_PER_LAYER 8
#define DEFAULT_LAYER_INTERVAL 2

const char *default_symbol = "■";

static void init_print_system(uint32_t start_block_id, uint32_t block_num)
{
	uint32_t i;
	easy_block_t *block;

	for (i = 0; i < block_num; i++) {
		block = get_block(start_block_id + i);
		print_core.print_state[i] = block->state;
	}
	print_core.print_num = block_num;
	print_core.print_size = DEFAULT_PRINT_SIZE;
	print_core.print_interval = DEFAULT_PRINT_INTERVAL;
}

#define color_print(color, str) do { printf(color "%s" COLOR_NONE, str); } while(0)

static bool is_block_alloc(uint32_t state)
{
    return state == BLOCK_ALLOC || state == BLOCK_ALLOC_OVER || state == BLOCK_TRANS;
}

static void __print_colored_string(uint32_t state, const char* str)
{
    switch (state) {
        case BLOCK_FREE:
            color_print(COLOR_WHITE, str);
            break;
        case BLOCK_ALLOC:
            color_print(COLOR_BLUE, str);
            break;
        case BLOCK_TRANS:
            color_print(COLOR_GREEN, str);
            break;
        case BLOCK_ALLOC_OVER:
            color_print(COLOR_PURPLE, str);
            break;
        case BLOCK_FREE_OVER:
            color_print(COLOR_RED, str);
            break;
    }
}

static void __print_top_or_bottom(uint32_t state)
{
    uint32_t j;
    for (j = 0; j < print_core.print_size; j++) {
        if (j == print_core.print_size - 1) {
            __print_colored_string(state, default_symbol);
        }
        else {
            __print_colored_string(state, default_symbol);
            printf(" ");
        }
    }
}

static void __print_middle(uint32_t state)
{
    uint32_t j;
    for (j = 0; j < print_core.print_size; j++) {
        if (j == 0) {
            __print_colored_string(state, default_symbol);
            printf(" ");
        }
        else if (j == print_core.print_size - 1)
            __print_colored_string(state, default_symbol);
        else if (is_block_alloc(state)) {
            __print_colored_string(state, default_symbol);
            printf(" ");
        }
        else
            printf("  ");
    }
}

static void __print_interval()
{
    uint32_t j;
    for (j = 0; j < print_core.print_interval; j++) {
        printf(" ");
    }
}

static void print_block_top_or_bottom(uint32_t start, uint32_t num)
{
	uint32_t i;
	for (i = start; i < start + num; i++) {
        __print_top_or_bottom(print_core.print_state[i]);
        if (i != print_core.print_num - 1)
            __print_interval();
	}
	printf("\n");
}

static void print_block_middle(uint32_t start, uint32_t num)
{
	uint32_t i;
	for (i = start; i < start + num; i++) {
        __print_middle(print_core.print_state[i]);
        if (i != print_core.print_num - 1)
            __print_interval();
	}
	printf("\n");
}

static void __print_one_layer(uint32_t start, uint32_t num)
{
    uint32_t i;
    for (i = 0; i < print_core.print_size; i++) {
        if (i == 0 || i == print_core.print_size - 1) {
            print_block_top_or_bottom(start, num);
        } else {
            print_block_middle(start, num);
        }
    }
}

static void __print_blocks()
{
    uint32_t i;
    uint32_t block_per_layer = DEFAULT_BLOCK_PER_LAYER;
    uint32_t layer_interval = DEFAULT_LAYER_INTERVAL;
    uint32_t layer_num = print_core.print_num / block_per_layer;
    uint32_t last_line_block_num = print_core.print_num % block_per_layer;

    printf("\n");
    for (i = 0; i < layer_num; i++) {
        __print_one_layer(i * block_per_layer, block_per_layer);
        if (i != layer_num - 1) {
            for (uint32_t j = 0; j < layer_interval; j++) {
                printf("\n");
            }
        }
    }
    printf("\n");

    if (last_line_block_num != 0) {
        __print_one_layer(layer_num * block_per_layer, last_line_block_num);
    }
}

void print_blocks(uint32_t start_block_id, uint32_t block_num)
{
    init_print_system(start_block_id, block_num);
    __print_blocks();
}