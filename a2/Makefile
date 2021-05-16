# This code is provided solely for the personal and private use of students
# taking the CSC369H course at the University of Toronto. Copying for purposes
# other than this use is expressly prohibited. All forms of distribution of
# this code, including but not limited to public repositories on GitHub,
# GitLab, Bitbucket, or any other online platform, whether as given or with
# any changes, are expressly prohibited.
#
# Authors: Alexey Khrabrov, Andrew Pelegris, Karen Reid
#
# All of the files in this directory and all subdirectories are:
# Copyright (c) 2019 Karen Reid

EXTRA_CFLAGS = -DEXIT_ON_ERROR #-DNDEBUG -DDEBUG_VERBOSE

CC = gcc
CFLAGS := -g3 -Wall -Wextra -Werror -pthread $(EXTRA_CFLAGS) $(CFLAGS)
LDFLAGS := -pthread $(LDFLAGS)

.PHONY: all clean

MQ_OBJ_FILES = errors.o msg_queue.o mutex_validator.o ring_buffer.o sync.o

ALL_TESTS = dummy_test prodcon

all: $(ALL_TESTS)

dummy_test: $(MQ_OBJ_FILES) dummy_test.o

prodcon: $(MQ_OBJ_FILES) prodcon.o

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:.c=.o)

-include $(OBJ_FILES:.o=.d)

%.o: %.c
	$(CC) $< -o $@ -c -MMD $(CFLAGS)

clean:
	rm -f $(OBJ_FILES) $(OBJ_FILES:.o=.d) $(ALL_TESTS)
