#include "easy_file.h"
#include "easy_block.h"
#include "easy_defs.h"
#include "easy_disk.h"
#include "print_block.h"
#include <stdio.h>
#include <string.h>

#define FILE_STRUCT_PER_BLOCK (BLOCK_SIZE / sizeof(easy_file_t))

extern easy_disk_system_t *global_disk_system;
easy_dir_t *root_dir;
easy_dir_t *current_dir;
easy_file_t *global_file_pool;
bool *global_file_pool_bitmap;

static easy_file_t *create_file_internal(const char *FileName, file_type Type);

static easy_status easy_dir_add_file(easy_dir_t *dir, uint32_t file_id);

static easy_file_t *easy_dir_get_file(easy_dir_t *dir, const char *file_name);

static easy_status easy_dir_remove_file(easy_dir_t *dir, uint32_t file_id);

/************************************************************
 *        Tool functions
 ************************************************************/

#define for_each_block_id_in_file(block_id, file)                                                                      \
	for (uint32_t __id = 0, block_id; __id < (file)->block_num && (block_id = file->block_ids[__id]); ++__id)

#define is_file_trans(file) ((file)->type == EASY_TYPE_FILE_TRANS || (file)->type == EASY_TYPE_DIR_TRANS)
#define is_dir(file) ((file)->type == EASY_TYPE_DIR || (file)->type == EASY_TYPE_DIR_TRANS)

static uint32_t get_file_name_len(const char *file_name)
{
	return strlen(file_name);
}

/*
 * return 0 - not equal
 *        1 - equal
 */
static bool compare_file_name(const char *name1, const char *name2)
{
	return !strcmp(name1, name2);
}

static easy_status ensure_file_has_block(easy_file_t *file)
{
	easy_status status;

	if (file->block_num == 0) {
		status = alloc_block(&(file->block_ids[0]));
		if (status != EASY_SUCCESS) {
			printf("%s alloc block failed\n", __func__);
			return EASY_BLOCK_ALLOC_ERROR;
		}
		file->block_num++;
	}

	return EASY_SUCCESS;
}

static easy_dir_t *easy_file_to_easy_dir(easy_file_t *file)
{
	easy_status status;
	if (file->type != EASY_TYPE_DIR)
		return NULL;

	status = ensure_file_has_block(file);
	if (status != EASY_SUCCESS) {
		return NULL;
	}

	return (easy_dir_t *)get_block_data(file->block_ids[0]);
}

static easy_file_t *easy_dir_to_easy_file(easy_dir_t *Dir)
{
	return Dir->self_file;
}

static inline bool is_special_dir(easy_file_t *file)
{
	return compare_file_name(file->name, ".") || compare_file_name(file->name, "..");
}

/************************************************************
 *        File Pool Related
 ************************************************************/

#define file_pool_get_file_by_id(file_id) (&global_file_pool[(file_id)])

static easy_file_t *file_pool_get_file(const char *file_name)
{
	uint32_t i;
	easy_file_t *file;

	for (i = 1; i < MAX_FILE_NUM; ++i) {
		if (global_file_pool_bitmap[i]) {
			file = &global_file_pool[i];
			if (compare_file_name(file->name, file_name)) {
				return file;
			}
		}
	}

	return NULL;
}

static easy_file_t *file_pool_get_new_file(const char *file_name, file_type type)
{
	uint32_t i;
	easy_file_t *new_file;
	uint32_t file_name_len;

	/** We remain the first file in gFilePool unused */
	for (i = 1; i < MAX_FILE_NUM; ++i) {
		if (global_file_pool_bitmap[i]) {
			continue;
		}
		global_file_pool_bitmap[i] = 1;
		new_file = &global_file_pool[i];
		file_name_len = get_file_name_len(file_name);
		memcpy(new_file->name, file_name, file_name_len * sizeof(char));
		new_file->type = type;
		new_file->file_size = 0;
		return new_file;
	}

	return NULL;
}

easy_status easy_init_file_pool(bool is_init)
{
	uint32_t i;
	uint32_t file_struct_block_num;
	uint32_t block_id;

	file_struct_block_num = MAX_FILE_NUM / FILE_STRUCT_PER_BLOCK;
	if (is_init) {
		for (i = 0; i < file_struct_block_num; ++i) {
			alloc_block(0);
		}
		global_file_pool = (easy_file_t *)get_block_data(0);
		alloc_block(&block_id);
		global_file_pool_bitmap = (bool *)get_block_data(block_id);
		memset(global_file_pool, 0, sizeof(easy_file_t) * MAX_FILE_NUM);
		memset(global_file_pool_bitmap, 0, MAX_FILE_NUM);

		for (i = 0; i < MAX_FILE_NUM; ++i) {
			global_file_pool[i].id = i;
			global_file_pool_bitmap[i] = 0;
		}
	} else {
		global_file_pool = (easy_file_t *)get_block_data(0);
		global_file_pool_bitmap = (bool *)get_block_data(file_struct_block_num);
	}

	return EASY_SUCCESS;
}

/************************************************************
 *        EasyDir Related
 ************************************************************/
#define get_cur_dir() (current_dir)
#define set_cur_dir(dir) (current_dir = (dir))

easy_dir_t *get_easy_dir_by_name(const char *dir_name, easy_dir_t *cur_dir)
{
	easy_file_t *dir_file;
	easy_dir_t *dir;

	if (compare_file_name(dir_name, "/")) {
		return root_dir;
	}

	dir_file = easy_dir_get_file(cur_dir, dir_name);
	if (!dir_file || dir_file->type != EASY_TYPE_DIR) {
		return NULL;
	}

	dir = easy_file_to_easy_dir(dir_file);

	return dir;
}

static easy_dir_t *get_praent_dir(easy_dir_t *Dir)
{
	easy_dir_t *dot_dot_dir;
	dot_dot_dir = get_easy_dir_by_name("..", Dir);
	return easy_file_to_easy_dir(file_pool_get_file_by_id(dot_dot_dir->file_ids[0]));
}

bool easy_dir_check_file_exist(easy_dir_t *dir, const char *file_name)
{
	uint32_t i;
	easy_file_t *file;
	/** TODO: Check whether @file_name exist in @dir */
	for (i = 0; i < MAX_FILE_NUM; ++i) {
		if (dir->file_ids[i] == 0)
			continue;
		file = &global_file_pool[dir->file_ids[i]];
		if (compare_file_name(file->name, file_name))
			return 1;
	}
	return 0;
}

static easy_dir_t *create_dir_internal(const char *file_name)
{
	easy_file_t *new_dir_file;
	easy_dir_t *new_dir;

	/** Directory is also a type of "EASY_FILE" */
	new_dir_file = create_file_internal(file_name, EASY_TYPE_DIR);
	if (!new_dir_file) {
		return NULL;
	}

	new_dir = easy_file_to_easy_dir(new_dir_file);
	new_dir->self_file = new_dir_file;
	new_dir->file_num = 0;

	return new_dir;
}

easy_dir_t *create_dir(const char *file_name, bool is_root, easy_dir_t *cur_dir)
{
	easy_status status;
	easy_dir_t *new_dir;
	easy_dir_t *dot_dir;	 // ls .
	easy_dir_t *dot_dot_dir; // cd ..

	if (!is_root && easy_dir_check_file_exist(cur_dir, file_name)) {
		// FIXME: Already exist, return NULL
		return NULL;
	}

	new_dir = create_dir_internal(file_name);
	if (!new_dir) {
		return NULL;
	}

	dot_dir = create_dir_internal(".");
	if (!dot_dir) {
		return NULL;
	}

	dot_dot_dir = create_dir_internal("..");
	if (!dot_dot_dir) {
		return NULL;
	}

	status = easy_dir_add_file(dot_dir, easy_dir_to_easy_file(new_dir)->id);
	if (status != EASY_SUCCESS) {
		return NULL;
	}

	if (is_root) {
		/** RootDir: ".." -> RootDir(NewDir) itself */
		easy_dir_add_file(dot_dot_dir, easy_dir_to_easy_file(new_dir)->id);
	} else {
		/** Common Dir . */
		easy_dir_add_file(dot_dot_dir, easy_dir_to_easy_file(cur_dir)->id);
	}

	/** Add . and .. to newly created directory */
	status = easy_dir_add_file(new_dir, easy_dir_to_easy_file(dot_dir)->id);
	if (status != EASY_SUCCESS) {
		return NULL;
	}
	status = easy_dir_add_file(new_dir, easy_dir_to_easy_file(dot_dot_dir)->id);
	if (status != EASY_SUCCESS) {
		return NULL;
	}

	if (!is_root && cur_dir) {
		status = easy_dir_add_file(cur_dir, new_dir->self_file->id);
		if (status != EASY_SUCCESS) {
			return NULL;
		}
	}

	return new_dir;
}

#ifdef __DEMO_USE
extern void set_start_block();
#endif

easy_status easy_create_root_dir(void)
{
	root_dir = create_dir("/", 1, NULL);
	if (!root_dir) {
		return EASY_DIR_CREATE_ROOT_DIR_FAILED;
	}

#ifdef __DEMO_USE
	set_start_block();
#endif

	return EASY_SUCCESS;
}

easy_status easy_create_dir(const char *dir_name)
{
	easy_dir_t *cur_dir = get_cur_dir();
	easy_dir_t *new_dir;
	new_dir = create_dir(dir_name, 0, cur_dir);

	if (!new_dir) {
		return EASY_DIR_CREATE_FAILED;
	}
	return EASY_SUCCESS;
}

const char *color_green = COLOR_GREEN;
const char *color_blue = COLOR_BLUE;
const char *color_white = COLOR_WHITE;
const char *color_none = COLOR_NONE;

#define add_str_to_buf(str, buf_ptr)                                                                                   \
	do {                                                                                                           \
		memcpy(buf_ptr, str, sizeof(char) * strlen(str));                                                      \
		buf_ptr += strlen(str);                                                                                \
	} while (0)

static void add_file_name_to_buf(easy_file_t *file, char **buf_ptr)
{
	char *ptr = *buf_ptr;

	if (is_file_trans(file)) {
		/** The file name of the transparent file is displayed in green */
		add_str_to_buf(color_green, ptr);
	} else if (file->type == EASY_TYPE_FILE) {
		add_str_to_buf(color_blue, ptr);
	} else {
		add_str_to_buf(color_white, ptr);
	}

	add_str_to_buf(file->name, ptr);
	add_str_to_buf(" ", ptr);
	add_str_to_buf(color_none, ptr);

	*buf_ptr = ptr;
}

/** option: 1 -> ls -a */
static easy_status easy_dir_ls_internal(easy_dir_t *dir, void *buf, int option)
{
	easy_file_t *file;
	char *buf_ptr;
	uint32_t i;

	buf_ptr = buf;
	for (i = 0; i < dir->file_num; ++i) {
		file = file_pool_get_file_by_id(dir->file_ids[i]);
		if (is_special_dir(file) && option != 1)
			continue;
#ifdef __TFS_REMOTE
		/** At tfs_remote, do not show local files */
		if (!is_file_trans(file))
			continue;
#else
#endif
		add_file_name_to_buf(file, &buf_ptr);
	}

	return EASY_SUCCESS;
}

easy_status easy_dir_list_files(const char *dir_name, void *buf, int option)
{
	easy_dir_t *dir;
	easy_dir_t *cur_dir = get_cur_dir();

	if (compare_file_name(dir_name, ".")) {
		dir = cur_dir;
	} else if (compare_file_name(dir_name, "..")) {
		dir = get_praent_dir(cur_dir);
	} else {
		dir = get_easy_dir_by_name(dir_name, cur_dir);
	}

	return easy_dir_ls_internal(dir, buf, option);
}

static easy_file_t *easy_dir_get_file(easy_dir_t *dir, const char *file_name)
{
	uint32_t i;
	easy_file_t *file;

	for (i = 0; i < MAX_FILE_NUM; ++i) {
		if (dir->file_ids[i] == 0)
			continue;
		file = &global_file_pool[dir->file_ids[i]];
		if (compare_file_name(file->name, file_name))
			return file;
	}

	return NULL;
}

static easy_status easy_dir_add_file(easy_dir_t *dir, uint32_t file_id)
{
	uint32_t i;

	for (i = 0; i < MAX_FILE_NUM; ++i) {
		if (dir->file_ids[i] == 0) {
			dir->file_ids[i] = file_id;
			dir->file_num++;
			return EASY_SUCCESS;
		}
	}
	return EASY_DIR_TOO_MANY_FILE_ERROR;
}

static easy_status easy_dir_remove_file(easy_dir_t *dir, uint32_t file_id)
{
	uint32_t i, j;

	for (i = 0; i < MAX_FILE_NUM; ++i) {
		if (dir->file_ids[i] == file_id) {
			/* Naive implementation, just move the back part of the array forward */
			for (j = i; j < MAX_FILE_NUM - 1; ++j)
				dir->file_ids[j] = dir->file_ids[j + 1];
			dir->file_num--;
			return EASY_SUCCESS;
		}
	}
	return EASY_DIR_NOT_FOUND_ERROR;
}

/************************************************************
 *        EasyFile Related
 ************************************************************/

static easy_file_t *create_file_internal(const char *file_name, file_type type)
{
	easy_file_t *new_file = NULL;

	new_file = file_pool_get_new_file(file_name, type);
	// printf("create_file: %s id: %d\n", file_name, new_file->id);
	if (!new_file) {
		return NULL;
	}
	new_file->block_num = 0;

	return new_file;
}

easy_status easy_create_file(const char *file_name)
{
	easy_file_t *new_file = NULL;
	easy_dir_t *cur_dir;

	cur_dir = get_cur_dir();

	if (easy_dir_check_file_exist(cur_dir, file_name)) {
		printf("file \"%s\" already exists\n", file_name);
		return EASY_SUCCESS;
	}

	new_file = create_file_internal(file_name, EASY_TYPE_FILE);
	if (!new_file) {
		printf("create file \"%s\" failed\n", file_name);
		return EASY_FILE_CREATE_FAILED;
	}

	easy_dir_add_file(cur_dir, new_file->id);

	return EASY_SUCCESS;
}

static void clean_file_metadata(easy_file_t *file)
{
	memset(file->name, 0, MAX_FILE_NAME_LEN);
	file->file_size = 0;
	file->block_num = 0;
}

static easy_status easy_remove_dir_internal(easy_dir_t *delete_dir, easy_dir_t *parent_dir);

static easy_status easy_remove_file_internal(easy_file_t *delete_file, easy_dir_t *parent_dir)
{
	easy_status status;

	if (is_dir(delete_file) && !is_special_dir(delete_file)) {
		return easy_remove_dir_internal(easy_file_to_easy_dir(delete_file), parent_dir);
	}

	/** For "." and "..", treat as nornal file */
	for_each_block_id_in_file(block_id, delete_file)
	{
		if (unlikely(is_file_trans(delete_file))) {
			status = free_block_trans(block_id);
		} else {
			status = free_block(block_id);
		}
		if (status != EASY_SUCCESS) {
			return status;
		}
	}

	status = easy_dir_remove_file(parent_dir, delete_file->id);
	if (status != EASY_SUCCESS) {
		return status;
	}

	clean_file_metadata(delete_file);

	return EASY_SUCCESS;
}

static inline void change_dir_to_file(easy_dir_t *dir)
{
	easy_file_t *dir_file = easy_dir_to_easy_file(dir);
	if (dir_file->type == EASY_TYPE_DIR)
		dir_file->type = EASY_TYPE_FILE;
	else if (dir_file->type == EASY_TYPE_DIR_TRANS)
		dir_file->type = EASY_TYPE_FILE_TRANS;
	else {
		printf("unsupported directory type: %d\n", dir_file->type);
	}
}

static easy_status easy_remove_dir_internal(easy_dir_t *delete_dir, easy_dir_t *parent_dir)
{
	easy_status status;
	easy_file_t *delete_dir_file = NULL;
	easy_file_t *child_file;

	delete_dir_file = easy_dir_to_easy_file(delete_dir);

	while (delete_dir->file_num > 0) {
		child_file = &global_file_pool[delete_dir->file_ids[0]];
		status = easy_remove_file_internal(child_file, delete_dir);
		if (status != EASY_SUCCESS) {
			return status;
		}
	}

	/** Before removing dir inself, change it to file type to avoid nested remove-loop */
	change_dir_to_file(delete_dir);
	easy_remove_file_internal(delete_dir_file, parent_dir);

	return EASY_SUCCESS;
}

easy_status easy_remove_file(const char *file_name)
{
	easy_file_t *delete_file = NULL;
	easy_dir_t *cur_dir;

	cur_dir = get_cur_dir();

	delete_file = easy_dir_get_file(cur_dir, file_name);
	if (!delete_file) {
		printf("file \"%s\" not exists\n", file_name);
		return EASY_FILE_NOT_FOUND_ERROR;
	}
#ifdef __TFS_REMOTE
	if (!is_file_trans(delete_file)) {
		printf("Cannot remove file \"%s\"\n", file_name);
		return EASY_FILE_NOT_SUPPORT;
	}
#endif

	if (is_dir(delete_file) && !is_special_dir(delete_file)) {
		return easy_remove_dir_internal(easy_file_to_easy_dir(delete_file), cur_dir);
	}

	return easy_remove_file_internal(delete_file, cur_dir);
}

easy_status easy_remove_dir(const char *dir_name)
{
	easy_file_t *delete_dir_file = NULL;
	easy_dir_t *cur_dir;
	easy_dir_t *delete_dir;

	cur_dir = get_cur_dir();

	delete_dir_file = easy_dir_get_file(cur_dir, dir_name);
	if (!delete_dir_file) {
		printf("directory \"%s\" not exists\n", dir_name);
		return EASY_FILE_NOT_FOUND_ERROR;
	}

	delete_dir = easy_file_to_easy_dir(delete_dir_file);

	return easy_remove_dir_internal(delete_dir, cur_dir);
}

easy_status easy_create_trans_file(const char *file_name)
{
	easy_file_t *new_file = NULL;
	easy_dir_t *cur_dir;

	cur_dir = get_cur_dir();

	if (easy_dir_check_file_exist(cur_dir, file_name)) {
		/* File already exist, do nothing. */
		return EASY_SUCCESS;
	}

	new_file = create_file_internal(file_name, EASY_TYPE_FILE_TRANS);
	if (!new_file) {
		printf("create file \"%s\" failed\n", file_name);
		return EASY_FILE_CREATE_FAILED;
	}

	easy_dir_add_file(cur_dir, new_file->id);

	return EASY_SUCCESS;
}

easy_status easy_remove_trans_file(const char *file_name)
{
	return easy_remove_file(file_name);
}

/* Ordinary file cannot be overwritten,
 * only try to clean transparet file here.
 */
static easy_status easy_clean_trans_file(easy_file_t *clean_file)
{
	easy_status status;
	easy_dir_t *cur_dir;

	cur_dir = get_cur_dir();

	if (!clean_file) {
		return -EASY_FILE_NOT_FOUND_ERROR;
	}

	for_each_block_id_in_file(block_id, clean_file)
	{
		status = clean_block(block_id);
		if (status != EASY_SUCCESS) {
			return status;
		}
	}

	status = easy_dir_remove_file(cur_dir, clean_file->id);
	if (status != EASY_SUCCESS) {
		return status;
	}

	clean_file_metadata(clean_file);

	return EASY_SUCCESS;
}

/*
 * Check whether the @file is overwritten
 * return 1 - overwritten
 */
static bool is_file_overwritten(easy_file_t *file)
{
	easy_block_t *block;

	for_each_block_id_in_file(block_id, file)
	{
		block = get_block(block_id);
		if (block->state == BLOCK_ALLOC_OVER || block->state == BLOCK_FREE_OVER) {
			return true;
		}
	}

	return false;
}

static inline void mark_file_blocks_open(easy_file_t *file)
{
	easy_block_t *block;

	for_each_block_id_in_file(block_id, file)
	{
		block = get_block(block_id);
		block->is_opened = true;
	}
}

static inline void mark_file_blocks_close(easy_file_t *file)
{
	easy_block_t *block;

	for_each_block_id_in_file(block_id, file)
	{
		block = get_block(block_id);
		block->is_opened = false;
	}
}

/*
 * Open file before read and write.
 * Check overwritten blocks here.
 */
easy_status easy_open_file(const char *file_name, easy_file_t **open_file)
{
	easy_file_t *file;

	file = easy_dir_get_file(get_cur_dir(), file_name);
	if (!file) {
		printf("file %s not found\n", file_name);
		return EASY_FILE_NOT_FOUND_ERROR;
	}
	if (file->state == EASY_FILE_OVER) {
		printf("file %s is overwritten\n", file_name);
		return EASY_FILE_OVERWRITTEN_ERROR;
	}

	if (is_file_trans(file)) {
		if (is_file_overwritten(file)) {
			file->state = EASY_FILE_OVER;
			printf("file %s is overwritten\n", file_name);
			easy_clean_trans_file(file);
			return EASY_FILE_OVERWRITTEN_ERROR;
		}
		/* Concession: mark all blocks "open" to prevent being overwritten */
		mark_file_blocks_open(file);
	}
#ifdef __TFS_REMOTE
	else {
		/* tfs-remote cannot open normal files */
		printf("Cannot access file \"%s\"\n", file_name);
		return EASY_FILE_NOT_SUPPORT;
	}
#endif

	file->state = EASY_FILE_OPEN;
	*open_file = file;

	return EASY_SUCCESS;
}

/*
 * Close file after operations finished.
 * A block of a transparent file cannot be overwritten
 * until the file is closed.
 */
easy_status easy_close_file(easy_file_t *file)
{
	file->state = EASY_FILE_CLOSE;
	if (is_file_trans(file))
		mark_file_blocks_close(file);

	return EASY_SUCCESS;
}

#ifndef __DEMO_USE
static easy_status easy_read_file(easy_file_t *file, uint32_t nbyte, void *buf)
{
	easy_status status;

	/* Currently we assume one file contains only one blocks! */
	if (nbyte > BLOCK_SIZE) {
		return EASY_FILE_NOT_SUPPORT;
	}
	status = ensure_file_has_block(file);
	if (status != EASY_SUCCESS) {
		return status;
	}

	read_block(file->block_ids[0], nbyte, buf);

	return EASY_SUCCESS;
}

static easy_status easy_write_file(easy_file_t *file, uint32_t nbyte, const void *buf)
{
	easy_status status;
	uint32_t write_pos;

	/* Currently we assume one file contains only one blocks! */
	if (nbyte > BLOCK_SIZE) {
		printf("File size too large\n");
		return EASY_FILE_SIZE_EXCEED;
	}

	/* Currently we only support write-append.
	 * Thus write postion == file size.
	 */
	write_pos = file->file_size;
	status = ensure_file_has_block(file);
	if (status != EASY_SUCCESS) {
		return status;
	}
	status = write_block(file->block_ids[0], nbyte, write_pos, buf);
	if (status != EASY_SUCCESS) {
		return status;
	}
	file->file_size += nbyte;

	return EASY_SUCCESS;
}
#else
/**
 * For demo use, each character use one block.
 * Only support write append now
 */
static easy_status easy_demo_write_file(easy_file_t *file, uint32_t nbyte, const void *buf)
{
	easy_status status;
	uint32_t new_block_num;
	uint32_t block_id;
	uint32_t write_pos;
	uint32_t i;

	new_block_num = strlen((char *)buf);
	write_pos = file->file_size;
	for (i = 0; i < new_block_num; ++i) {
		if (unlikely(is_file_trans(file))) {
			status = alloc_block_trans(&block_id);
		} else {
			status = alloc_block(&block_id);
		}
		if (status != EASY_SUCCESS) {
			return EASY_BLOCK_ALLOC_ERROR;
		}
		file->block_ids[i + write_pos] = block_id;
		file->block_num++;
	}

	for (i = 0; i < new_block_num; ++i) {
		status = write_block(file->block_ids[i + write_pos], 1, 0, buf + i);
		if (status != EASY_SUCCESS) {
			return status;
		}
	}
	file->file_size += nbyte;

	return EASY_SUCCESS;
}

static easy_status easy_demo_read_file(easy_file_t *file, uint32_t nbyte, void *buf)
{
	uint32_t i;
	if (nbyte > file->block_num) {
		nbyte = file->block_num;
	}

	for (i = 0; i < nbyte; ++i)
		read_block(file->block_ids[i], 1, (char *)buf + i);

	return EASY_SUCCESS;
}
#endif

/************************************************************
 *        File Layer
 ************************************************************/
easy_status init_file_layer(bool is_init)
{
	easy_status status;
	easy_file_t *root_file;

	status = easy_init_file_pool(is_init);
	if (status != EASY_SUCCESS) {
		return status;
	}

	if (is_init) {
		status = easy_create_root_dir();
		if (status != EASY_SUCCESS) {
			return status;
		}
	} else {
		root_file = file_pool_get_file("/");
		if (!root_file) {
			printf("Can not file root\n");
			return EASY_FILE_NOT_FOUND_ERROR;
		}
		root_dir = easy_file_to_easy_dir(root_file);
	}

	current_dir = root_dir;

	return status;
}

easy_status easy_pwd(void *buf)
{
	easy_dir_t *cur_dir = get_cur_dir();
	easy_file_t *cur_dir_file = easy_dir_to_easy_file(cur_dir);
	uint32_t file_name_len;

	file_name_len = get_file_name_len(cur_dir_file->name);

	memcpy(buf, cur_dir_file->name, sizeof(char) * file_name_len);

	return EASY_SUCCESS;
}

easy_status easy_cd(const char *dir_name)
{
	easy_dir_t *dir;
	easy_dir_t *cur_dir;
	easy_dir_t *parent_dir;

	cur_dir = get_cur_dir();
	if (compare_file_name(dir_name, ".")) {
		return EASY_SUCCESS;
	}
	if (compare_file_name(dir_name, "..")) {
		parent_dir = get_praent_dir(cur_dir);
		set_cur_dir(parent_dir);
		return EASY_SUCCESS;
	}

	dir = get_easy_dir_by_name(dir_name, cur_dir);
	if (!dir) {
		printf("directory \"%s\" not found\n", dir_name);
		return EASY_DIR_NOT_FOUND_ERROR;
	}

	set_cur_dir(dir);

	return EASY_SUCCESS;
}

easy_status easy_cat(const char *file_name, void *buf)
{
	easy_status status;
	easy_file_t *file;

	status = easy_open_file(file_name, &file);
	if (status != EASY_SUCCESS) {
		return status;
	}

#ifndef __DEMO_USE
	status = easy_read_file(file, file->file_size, buf);
#else
	status = easy_demo_read_file(file, file->file_size, buf);
#endif
	if (status != EASY_SUCCESS) {
		return status;
	}

	status = easy_close_file(file);
	if (status != EASY_SUCCESS) {
		return status;
	}

	return EASY_SUCCESS;
}

easy_status easy_ls(void *buf, int option)
{
	easy_dir_t *cur_dir = get_cur_dir();

	return easy_dir_ls_internal(cur_dir, buf, option);
}

easy_status easy_echo(const char *file_name, const void *write_buf)
{
	easy_status status;
	easy_file_t *file;

	status = easy_open_file(file_name, &file);
	if (status != EASY_SUCCESS) {
		return status;
	}

#ifndef __DEMO_USE
	status = easy_write_file(file, strlen(write_buf), write_buf);
#else
	status = easy_demo_write_file(file, strlen(write_buf), write_buf);
#endif
	if (status != EASY_SUCCESS) {
		return status;
	}

	status = easy_close_file(file);
	if (status != EASY_SUCCESS) {
		return status;
	}

	return status;
}

easy_status easy_ls_blocks(void *buf)
{
	return list_blocks(buf);
}

easy_status easy_open(const char *file_name)
{
	easy_status status;
	easy_file_t *file;

	status = easy_open_file(file_name, &file);
	if (status != EASY_SUCCESS) {
		return status;
	}

	return EASY_SUCCESS;
}

easy_status easy_close(const char *file_name)
{
	easy_status status;
	easy_file_t *file;

	file = easy_dir_get_file(get_cur_dir(), file_name);
	if (!file) {
		printf("file %s not found\n", file_name);
		return EASY_FILE_NOT_FOUND_ERROR;
	}
	if (file->state == EASY_FILE_OVER) {
		printf("file %s is overwritten\n", file_name);
		return EASY_FILE_OVERWRITTEN_ERROR;
	}

	status = easy_close_file(file);
	if (status != EASY_SUCCESS) {
		return status;
	}

	return EASY_SUCCESS;
}

/*
 * Raw file write without open() and close().
 * Be aware that we don't check overwritten blocks here.
 */
easy_status easy_write(const char *file_name, const void *write_buf)
{
	easy_status status;
	easy_file_t *file;

	file = easy_dir_get_file(get_cur_dir(), file_name);
	if (!file) {
		printf("file %s not found\n", file_name);
		return EASY_FILE_NOT_FOUND_ERROR;
	}

#ifndef __DEMO_USE
	status = easy_write_file(file, strlen(write_buf), write_buf);
#else
	status = easy_demo_write_file(file, strlen(write_buf), write_buf);
#endif
	if (status != EASY_SUCCESS) {
		return status;
	}

	return status;
}

/*
 * Raw file read without open() and close().
 * Be aware that we don't check overwritten blocks here.
 */
easy_status easy_read(const char *file_name, void *read_buf)
{
	easy_status status;
	easy_file_t *file;

	file = easy_dir_get_file(get_cur_dir(), file_name);
	if (!file) {
		printf("file %s not found\n", file_name);
		return EASY_FILE_NOT_FOUND_ERROR;
	}

#ifndef __DEMO_USE
	status = easy_read_file(file, file->file_size, read_buf);
#else
	status = easy_demo_read_file(file, file->file_size, read_buf);
#endif
	if (status != EASY_SUCCESS) {
		return status;
	}

	return EASY_SUCCESS;
}