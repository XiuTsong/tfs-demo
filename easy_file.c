#include "easy_file.h"
#include "easy_block.h"
#include "easy_defs.h"
#include <stdio.h>
#include <string.h>

#define FILE_PER_BLOCK (BLOCK_SIZE / sizeof(easy_file_t))

easy_dir_t *root_dir;
easy_dir_t *current_dir;
easy_file_t *global_file_pool;
bool *global_file_pool_bitmap;

#ifdef __DEMO_USE
int start_block_id;
#endif

static easy_file_t *create_file_internal(const char *FileName, file_type Type);

static easy_status easy_dir_add_file(easy_dir_t *dir, uint32_t file_id);

static easy_file_t *easy_dir_get_file(easy_dir_t *dir, const char *file_name);

static easy_status easy_dir_remove_file(easy_dir_t *dir, uint32_t file_id);

/************************************************************
 *        Tool functions
 ************************************************************/
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

/************************************************************
 *        File Pool Related
 ************************************************************/

#define file_pool_get_file_by_id(file_id) (&global_file_pool[(file_id)])

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

easy_status easy_init_file_pool(void)
{
	uint32_t i;
	uint32_t block_num;
	uint32_t block_id;

	block_num = MAX_FILE_NUM / FILE_PER_BLOCK;
	for (i = 0; i < block_num; ++i) {
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
	/** TODO: Check whether @FileName exist in @Dir */
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
	current_dir = root_dir;

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

easy_status easy_dir_list_files(const char *dir_name, void *buf)
{
	easy_dir_t *dir;
	easy_dir_t *cur_dir = get_cur_dir();
	easy_file_t *file;
	char *buf_ptr;
	uint32_t file_len;
	uint32_t i;

	if (compare_file_name(dir_name, ".")) {
		dir = cur_dir;
	} else if (compare_file_name(dir_name, "..")) {
		dir = get_praent_dir(cur_dir);
	} else {
		dir = get_easy_dir_by_name(dir_name, cur_dir);
	}

	buf_ptr = buf;
	for (i = 0; i < dir->file_num; ++i) {
		file = file_pool_get_file_by_id(dir->file_ids[i]);
		file_len = get_file_name_len(file->name);
		memcpy(buf_ptr, file->name, sizeof(char) * file_len);
		buf_ptr += file_len;
		/** Add space ' ' to separate file names */
		*buf_ptr = ' ';
		buf_ptr++;
	}

	return EASY_SUCCESS;
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
	uint32_t i;

	for (i = 0; i < MAX_FILE_NUM; ++i) {
		if (dir->file_ids[i] == file_id) {
			dir->file_ids[i] = 0;
			dir->file_num--;
			return EASY_SUCCESS;
		}
	}
	return EASY_DIR_NOT_FOUND_ERROR;
}

/************************************************************
 *        EasyFile Related
 ************************************************************/

#define for_each_block_id_in_file(block_id, file) for ((block_id) = 0; (block_id) < (file)->block_num; ++(block_id))

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
		/** File already exist, do nothing. */
		return EASY_SUCCESS;
	}

	new_file = create_file_internal(file_name, EASY_TYPE_FILE);
	if (!new_file) {
		return -EASY_FILE_CREATE_FAILED;
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

easy_status easy_remove_file(const char *file_name)
{
	uint32_t block_id;
	easy_status status;
	easy_file_t *delete_file = NULL;
	easy_dir_t *cur_dir;
	file_type type;

	cur_dir = get_cur_dir();

	if (!easy_dir_check_file_exist(cur_dir, file_name)) {
		/** File does not exist, do nothing. */
		return EASY_SUCCESS;
	}

	delete_file = easy_dir_get_file(cur_dir, file_name);
	if (!delete_file) {
		return -EASY_FILE_NOT_FOUND_ERROR;
	}

	type = delete_file->type;
	for_each_block_id_in_file(block_id, delete_file)
	{
		if (unlikely(type == EASY_TYPE_FILE_TRANS || type == EASY_TYPE_DIR_TRANS)) {
			status = free_block_trans(delete_file->block_ids[block_id]);
		} else {
			status = free_block(delete_file->block_ids[block_id]);
		}
		if (status != EASY_SUCCESS) {
			return status;
		}
	}
	/* Update directory status */
	status = easy_dir_remove_file(cur_dir, delete_file->id);
	if (status != EASY_SUCCESS) {
		return status;
	}

	clean_file_metadata(delete_file);

	return EASY_SUCCESS;
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
		return -EASY_FILE_CREATE_FAILED;
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
	uint32_t block_id;
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
static bool easy_check_file_overwritten(easy_file_t *file)
{
	uint32_t block_id;
	easy_block_t *block;

	for_each_block_id_in_file(block_id, file)
	{
		block = get_block(file->block_ids[block_id]);
		if (block->state == BLOCK_ALLOC_OVER || block->state == BLOCK_FREE_OVER) {
			return true;
		}
	}

	return false;
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

	if (easy_check_file_overwritten(file)) {
		file->state = EASY_FILE_OVER;
		printf("file %s is overwritten\n", file_name);
		/* TODO: delete file, clean file blocks here */
		easy_clean_trans_file(file);
		return EASY_FILE_OVERWRITTEN_ERROR;
	}

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
 */
static easy_status easy_demo_write_file(easy_file_t *file, uint32_t nbyte, const void *buf)
{
	easy_status status;
	uint32_t block_num;
	uint32_t block_id;
	uint32_t i;

	block_num = strlen((char *)buf);
	for (i = file->block_num; i < block_num; ++i) {
		status = alloc_block(&block_id);
		if (status != EASY_SUCCESS) {
			return EASY_BLOCK_ALLOC_ERROR;
		}
		file->block_ids[i] = block_id;
		file->block_num++;
	}

	for (i = 0; i < block_num; ++i) {
		status = write_block(file->block_ids[i], 1, 0, buf + i);
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
easy_status init_file_layer(void)
{
	easy_status status;
	status = easy_init_file_pool();
	if (status != EASY_SUCCESS) {
		return status;
	}

	status = easy_create_root_dir();
	if (status != EASY_SUCCESS) {
		return status;
	}

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

easy_status easy_ls(void *buf)
{
	easy_dir_t *dir;
	easy_dir_t *cur_dir = get_cur_dir();
	easy_file_t *file;
	char *buf_ptr;
	uint32_t file_len;
	uint32_t i;

	dir = cur_dir;

	buf_ptr = buf;
	for (i = 0; i < dir->file_num; ++i) {
		file = file_pool_get_file_by_id(dir->file_ids[i]);
		file_len = get_file_name_len(file->name);
		memcpy(buf_ptr, file->name, sizeof(char) * file_len);
		buf_ptr += file_len;
		/** Add space ' ' to separate file names */
		*buf_ptr = ' ';
		buf_ptr++;
	}

	return EASY_SUCCESS;
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