#ifndef __TYPE_DEFS_H__
#define __TYPE_DEFS_H__

#include <stdbool.h>

typedef unsigned int easy_status;
typedef unsigned int easy_status;
#define uint32_t unsigned int

#define TOTAL_BYTES 102400

#define EASY_SUCCESS 0

#define EASY_BLOCK_INIT_FAILED 1
#define EASY_BLOCK_ALLOC_ERROR 2
#define EASY_BLOCK_FREE_ERROR 3
#define EASY_BLOCK_WRITE_ERROR 4
#define EASY_BLOCK_READ_ERROR 5
#define EASY_BLOCK_STATE_ERROR 6

#define EASY_FILE_INIT_FAILED 10
#define EASY_FILE_NOT_SUPPORT 20
#define EASY_FILE_NO_MORE_FILE_ERROR 30
#define EASY_FILE_NOT_FOUND_ERROR 40
#define EASY_FILE_CREATE_FAILED 50
#define EASY_FILE_OVERWRITTEN_ERROR 60
#define EASY_FILE_SIZE_EXCEED 70

#define EASY_DIR_CREATE_ROOT_DIR_FAILED 100
#define EASY_DIR_TOO_MANY_FILE_ERROR 200
#define EASY_DIR_NOT_FOUND_ERROR 300
#define EASY_DIR_CREATE_FAILED 400

#define EASY_DISK_INIT_FAILED 1000
#define EASY_DISK_NOT_FOUND_ERROR 2000
#define EASY_DISK_FLUSH_ERROR 3000
#define EASY_DISK_LOAD_ERROR 4000

#define __align(n) __attribute__((aligned(n)))
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define __maybe_unused __attribute__((unused))

#define __DEMO_USE

#endif