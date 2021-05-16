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
 * CSC369 Assignment 2 - Mutex Validator implementation.
 *
 * NOTE: This file will be replaced when we run your code.
 * DO NOT make any changes here.
 */

#include <assert.h>
#include <errno.h>
#include <time.h>

#include "errors.h"
#include "mutex_validator.h"


#ifndef NDEBUG

void validator_init(mutex_validator *validator)
{
	mutex_init(&validator->lock);
	validator->occupied = false;
}

void validator_destroy(mutex_validator *validator)
{
	mutex_destroy(&validator->lock);
}

void validator_enter(mutex_validator *validator, unsigned long delay)
{
	// Record that the validator has been entered. The lock ensures that
	// we can accurately test that no other thread has already entered.
	mutex_lock(&validator->lock);
	if (validator->occupied) {
		errno = EPERM;
		report_error("validator_enter: already occupied");
	}
	validator->occupied = true;
	mutex_unlock(&validator->lock);

	nap(delay);
}

void validator_exit(mutex_validator *validator)
{
	mutex_lock(&validator->lock);
	// Validator should be occupied, otherwise this indicates incorrect usage.
	assert(validator->occupied);
	validator->occupied = false;
	mutex_unlock(&validator->lock);
}

#endif// NDEBUG
