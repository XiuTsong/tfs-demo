#include "easy_block.h"
#include "easy_defs.h"
#include "easy_file.h"
#include "tfs_demo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *global_memory_pool;
char global_str[MAX_LEN];
char global_args[MAX_ARGS][MAX_LEN];
char global_read_buf[MAX_LEN];

int init_file_system(void)
{
	int ret;
	ret = init_block_layer(global_memory_pool, TOTAL_BYTES);
	if (ret != EASY_SUCCESS) {
		printf("init block layer failed\n");
		return -EASY_BLOCK_INIT_FAILED;
	}

	ret = init_file_layer();
	if (ret != EASY_SUCCESS) {
		printf("init file layer failed\n");
		return -EASY_FILE_INIT_FAILED;
	}

	return ret;
}
static int create_file(const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	if (!strcmp(args[1], "-t")) {
		return easy_create_trans_file(args[2]);
	}
	return easy_create_file(args[1]);
}

static int remove_file(const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	return easy_remove_file(args[1]);
}

static int create_dir(const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	return easy_create_dir(args[1]);
}

static int ls(__maybe_unused const char args[][MAX_LEN], char *read_buf)
{
	int ret;
	ret = easy_ls(read_buf);
	printf("%s\n", read_buf);
	return ret;
}

static int cd(const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	return easy_cd(args[1]);
}

static int pwd(__maybe_unused const char args[][MAX_LEN], char *read_buf)
{
	int ret;
	ret = easy_pwd(read_buf);
	printf("%s\n", read_buf);
	return ret;
}

static int cat(const char args[][MAX_LEN], char *read_buf)
{
	int ret;
	ret = easy_cat(args[1], read_buf);
	if (strlen(read_buf))
		printf("%s\n", read_buf);
	return ret;
}

static int read(const char args[][MAX_LEN], char *read_buf)
{
	int ret;
	ret = easy_read(args[1], read_buf);
	if (strlen(read_buf))
		printf("%s\n", read_buf);
	return ret;
}

static int open(const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	int ret;
	ret = easy_open(args[1]);
	return ret;
}

/* e.g echo "123" a.txt */
static int echo(const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	return easy_echo(args[2], args[1]);
}

static int write(const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	return easy_write(args[2], args[1]);
}

static int quit(__maybe_unused const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	return -1;
}

static int ls_blk(__maybe_unused const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	return easy_ls_blocks(read_buf);
}

const struct easy_fs_op fs_ops[] = {
	{"quit", quit},
	{"q", quit},
	{"create", create_file},
	{"touch", create_file},
	{"remove", remove_file},
	{"rm", remove_file},
	{"mkdir", create_dir},
	{"ls", ls},
	{"cd", cd},
	{"pwd", pwd},
	{"cat", cat},
	{"echo", echo},
	{"lsblk", ls_blk},
	{"open", open},
	{"write", write},
	{"read", read},
};

static int start_tfs()
{
	int ret;
	global_memory_pool = malloc(sizeof(char) * TOTAL_BYTES);
	ret = init_file_system();
	if (ret) {
		return ret;
	}
	printf("welcome to tfs-demo!\n");
	return ret;
}

static int end_tfs(int exit_code)
{
	free(global_memory_pool);
	printf("[%d] tfs-demo finish!\n", exit_code);
	return 0;
}

static int forward_fs_ops(const char args[][MAX_LEN], char *buf)
{
	size_t i;
	for (i = 0; i < sizeof(fs_ops) / sizeof(fs_ops[0]); i++) {
		if (!strcmp(args[0], fs_ops[i].cmd_name)) {
			return fs_ops[i].op(args, buf);
		}
	}
	printf("command \"%s\" not found\n", args[0]);
	return 0;
}

/* args need to be pre-allocated before */
static int split_string_to_args(char *str, char args[][MAX_LEN])
{
	int num = 0;
	char *token;

	for (int i = 0; i < MAX_ARGS; i++) {
		memset(args[i], 0, MAX_ARGS * MAX_LEN);
	}

	token = strtok(str, " ");
	if (token == NULL) {
		return 0;
	}
	memcpy(args[num++], token, strlen(token));
	while (token != NULL) {
		/* The second time, use "NULL" as the first parameter */
		token = strtok(NULL, " ");
		if (token != NULL) {
			memcpy(args[num++], token, strlen(token));
		}
	}
	return num;
}

int tfs_main_work()
{
	int ret = 0;

	while (ret >= 0) {
		int argc;

		printf("tfs-demo > ");

		if (fgets(global_str, sizeof(global_str), stdin) == NULL) {
			break;
		}
		global_str[strcspn(global_str, "\n")] = '\0';

		argc = split_string_to_args(global_str, global_args);
		if (argc == 0) {
			continue;
		}

		ret = forward_fs_ops(global_args, global_read_buf);
	}

	ret = end_tfs(ret);

	return ret;
}

int main()
{
	int ret;

	ret = start_tfs();
	if (ret) {
		printf("start tfs failed, ret %d\n", ret);
	}

	ret = tfs_main_work();

	return ret;
}