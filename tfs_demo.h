#ifndef __TFS_DEMO_H__
#define __TFS_DEMO_H__

#define TOTAL_BYTES 102400
#define MAX_LEN 256
#define MAX_ARGS 4

struct easy_fs_op {
	const char *cmd_name;
    int (*op)(const char args[][MAX_LEN], char *read_buf);
};

#endif