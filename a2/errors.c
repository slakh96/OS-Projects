/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC369H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Alexey Khrabrov, Andrew Pelegris, Karen Reid
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 Karen Reid
 */

/**
 * CSC369 Assignment 2 - Errors handling utilities implementation.
 *
 * NOTE: This file will be replaced when we run your code.
 * DO NOT make any changes here.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "errors.h"


void __report_error(const char *msg, bool info)
{
	// Preserve errno value that can be changed by perror()
	int e = errno;
	fprintf(stderr, "[ERROR %d]: ", e);
	perror(msg);
	errno = e;

	if (!info) {
#ifdef EXIT_ON_ERROR
		// Immediately exit when an error is detected
		exit(e);
#endif
	}
}
