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
 * CSC369 Assignment 2 - Mutex Validator header file.
 *
 * This is a utility that can be used to test whether something is actually
 * being accessed exclusively by one thread at a time.
 *
 * NOTE: This file will be replaced when we run your code.
 * DO NOT make any changes here.
 */

#pragma once

#include <stdbool.h>

#include "sync.h"


/**
 * Structure used in the mutually-exclusive access validation system.
 *
 * When you want to validate that only one thread at a time can actually access
 * a data structure, you can add a validator. A validator struct should be added
 * inside the data structure and initialized before use. Whenever a thread
 * performs an operation on the data structure that should be done mutually
 * exclusively, the thread should first enter. When done, it should exit. A
 * violation happens if two or more threads enter before one of them exits.
 */
typedef struct mutex_validator {
	/**
	 * The mutex used to synchronize internal validator structures.
	 *
	 * This mutex is held only for a short duration in the processes of
	 * entering and exiting. It is not held while already entered.
	 */
	mutex_t lock;

	/** True if a thread that has entered the validator, but has not yet exited. */
	bool occupied;

} mutex_validator;


#ifndef NDEBUG

/** Initialize a mutex validator. */
void validator_init(mutex_validator *validator);

/** Destroy a mutex validator. */
void validator_destroy(mutex_validator *validator);

/**
 * Enter a mutex validator.
 *
 * @param validator  pointer to the validator.
 * @param delay      how long in nanoseconds to sleep immediately after
 *                   entering. Adding a delay period stretches out the critical
 *                   section, increasing the chance that a violation will occur
 *                   (in a buggy implementation).
 */
void validator_enter(mutex_validator *validator, unsigned long delay);

/** Exit a mutex validator. */
void validator_exit(mutex_validator *validator);

#else// NDEBUG

#define validator_init(x)
#define validator_destroy(x)
#define validator_enter(x, d)
#define validator_exit(x)

#endif// NDEBUG
