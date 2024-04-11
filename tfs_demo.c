#include "easy_block.h"
#include "easy_defs.h"
#include "easy_disk.h"
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
	bool is_init;

	ret = init_disk_layer(&global_memory_pool, &is_init);
	if (ret != EASY_SUCCESS) {
		printf("init disk layer failed\n");
		return -EASY_DISK_INIT_FAILED;
	}

	ret = init_block_layer(global_memory_pool, TOTAL_BYTES, is_init);
	if (ret != EASY_SUCCESS) {
		printf("init block layer failed\n");
		return -EASY_BLOCK_INIT_FAILED;
	}

	ret = init_file_layer(is_init);
	if (ret != EASY_SUCCESS) {
		printf("init file layer failed\n");
		return -EASY_FILE_INIT_FAILED;
	}

	/** For now, just flush block & file layer into disk */
	ret = easy_flush_whole();
	if (ret != EASY_SUCCESS) {
		return -EASY_DISK_FLUSH_ERROR;
	}

	return ret;
}

static int check_create_option(const char args[][MAX_LEN])
{
	if (args[1][0] == '-') {
		if (!strcmp(args[1], "-t")) {
			return 1;
		} else {
			return -1;
		}
	}
	return 2;
}

static int create_file(__maybe_unused int argc, const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	int ret;

	if (argc <= 1) {
		printf("usage: create/touch [-t] filename\n");
		return 0;
	}
	ret = check_create_option(args);
	if (ret == 1) {
		return easy_create_trans_file(args[2]);
	} else if (ret == 2) {
#ifdef __TFS_REMOTE
		return easy_create_trans_file(args[1]);
#else
		return easy_create_file(args[1]);
#endif
	}
	printf("unknown option \"%s\"\n", args[1]);
	return 0;
}

static int remove_file(__maybe_unused int argc, const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	if (argc <= 1) {
		printf("usage: remove/rm filename\n");
		return 0;
	}
	if (argc == 2) {
		return easy_remove_file(args[1]);
	}
	if (argc == 3) {
		if (!strcmp(args[1], "-f"))
			return easy_remove_file(args[2]);
		if (!strcmp(args[1], "-r"))
			return easy_remove_dir(args[2]);
		printf("usage: remove/rm [-f/-r] filename\n");
	}

	printf("too many arguments\n");

	return 0;
}

static int create_dir(__maybe_unused int argc, const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	if (argc <= 1) {
		printf("usage: mkdir dirname\n");
		return 0;
	}
	return easy_create_dir(args[1]);
}

static int ls(int argc, __maybe_unused const char args[][MAX_LEN], char *read_buf)
{
	int ret;

	if (argc < 2)
	 	/* only ls */
		ret = easy_ls(read_buf, 0);
	else if (argc == 2) {
		if (strcmp(args[1], "-a")) {
			/* ls dir */
			ret = easy_dir_list_files(args[1], read_buf, 0);
		} else {
			/* ls -a */
			ret = easy_ls(read_buf, 1);
		}
	} else if (argc == 3 && !strcmp(args[1], "-a")) {
		/* ls -a dir */
		ret = easy_dir_list_files(args[2], read_buf, 1);
	} else {
		printf("usage: ls [-a] [dirname]\n");
		return 0;
	}
	if (strlen(read_buf))
		printf("%s\n", read_buf);
	return ret;
}

static int cd(__maybe_unused int argc, const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	if (argc <= 1) {
		printf("usage: cd dirname\n");
		return 0;
	}
	return easy_cd(args[1]);
}

static int pwd(__maybe_unused int argc, __maybe_unused const char args[][MAX_LEN], char *read_buf)
{
	int ret;
	ret = easy_pwd(read_buf);
	printf("%s\n", read_buf);
	return ret;
}

static int cat(__maybe_unused int argc, const char args[][MAX_LEN], char *read_buf)
{
	int ret;
	if (argc <= 1) {
		printf("usage: cat filename\n");
		return 0;
	}
	ret = easy_cat(args[1], read_buf);
	if (strlen(read_buf))
		printf("%s\n", read_buf);
	return ret;
}

static int read(__maybe_unused int argc, const char args[][MAX_LEN], char *read_buf)
{
	int ret;
	if (argc <= 1) {
		printf("usage: read filename\n");
		return 0;
	}
	ret = easy_read(args[1], read_buf);
	if (strlen(read_buf))
		printf("%s\n", read_buf);
	return ret;
}

static int open(int argc, const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	int ret;
	if (argc <= 1) {
		printf("usage: open filename\n");
		return 0;
	}
	ret = easy_open(args[1]);
	return ret;
}

static int close(int argc, const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	int ret;
	if (argc <= 1) {
		printf("usage: open filename\n");
		return 0;
	}
	ret = easy_close(args[1]);
	return ret;
}

/* e.g echo "123" a.txt */
static int echo(__maybe_unused int argc, const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	if (argc <= 2) {
		printf("usage: echo \"content\" filename\n");
		return 0;
	}
	return easy_echo(args[2], args[1]);
}

static int write(__maybe_unused int argc, const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	if (argc <= 2) {
		printf("usage: write \"content\" filename\n");
		return 0;
	}
	return easy_write(args[2], args[1]);
}

static int quit(__maybe_unused int argc, __maybe_unused const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	return -1;
}

static int ls_blk(__maybe_unused int argc, __maybe_unused const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
#ifdef __TFS_REMOTE
	printf("\"ls_blk\" is not supported at tfs-remote\n");
	return 0;
#else
	return easy_ls_blocks(read_buf);
#endif
}

static int print_help(__maybe_unused int argc, __maybe_unused const char args[][MAX_LEN], __maybe_unused char *read_buf)
{
	printf("quit/q\n");
	printf("create/touch [-t] filename\n");
	printf("remove/rm [-f/-r] filename/dirname\n");
	printf("mkdir dirname\n");
	printf("ls\n");
	printf("cd dirname\n");
	printf("pwd\n");
	printf("cat filename\n");
	printf("echo \"content\" filename\n");
	printf("lsblk\n");
	printf("open filename\n");
	printf("close filename\n");
	printf("write \"content\" filename\n");
	printf("read filename\n");
	return 0;
}

static int flush()
{
	return easy_flush_whole();
}

static int load()
{
	return easy_load_whole();
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
	{"close", close},
	{"write", write},
	{"read", read},
	{"help", print_help},
};

int start_tfs()
{
	int ret;
	ret = init_file_system();
	if (ret) {
		return ret;
	}
	printf("welcome to tfs!\n");
	return ret;
}

static int end_tfs(__maybe_unused int exit_code)
{
	free(global_memory_pool);
	// printf("[%d] tfs-demo finish!\n", exit_code);
	return 0;
}

static int forward_fs_ops(int argc, const char args[][MAX_LEN], char *buf)
{
	size_t i;
	int ret;
	for (i = 0; i < sizeof(fs_ops) / sizeof(fs_ops[0]); i++) {
		if (!strcmp(args[0], fs_ops[i].cmd_name)) {
			/* For now, load() before, and flush() after every fs operation */
			load();
			ret = fs_ops[i].op(argc, args, buf);
			flush();
			return ret;
		}
	}
	printf("command \"%s\" not found\n", args[0]);
	return 0;
}

/* args needs to be pre-allocated before */
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

#ifdef __TFS_REMOTE
		printf("tfs-remote > ");
#else
		printf("tfs-local > ");
#endif

		if (fgets(global_str, sizeof(global_str), stdin) == NULL) {
			break;
		}
		global_str[strcspn(global_str, "\n")] = '\0';

		argc = split_string_to_args(global_str, global_args);
		if (argc == 0) {
			continue;
		}

		ret = forward_fs_ops(argc, global_args, global_read_buf);
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