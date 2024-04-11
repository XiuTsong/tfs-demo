CC = gcc
CFLAGS = -g -Wall -Wextra

# Define the source files for each target
SRCS := $(wildcard *.c)

# Define the object files for each target
OBJS := $(SRCS:.c=.o)
LOCAL_OBJS := $(addprefix local_, $(OBJS))
REMOTE_OBJS := $(addprefix remote_, $(OBJS))

.PHONY: all clean tfs_local tfs_remote

all: tfs_local tfs_remote

tfs_local: $(LOCAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

tfs_remote: CFLAGS += -D__TFS_REMOTE
tfs_remote: $(REMOTE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

local_%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

remote_%.o: %.c
	$(CC) $(CFLAGS) -D__TFS_REMOTE -c $< -o $@

clean:
	rm -f tfs_local tfs_remote $(LOCAL_OBJS) $(REMOTE_OBJS)

init:
	rm -f tfs_disk