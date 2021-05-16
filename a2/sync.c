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
 * CSC369 Assignment 2 - Synchronization primitives implementation.
 *
 * NOTE: This file will be replaced when we run your code.
 *       DO NOT make any changes here.
 */

// Errors in all functions in this file are considered fatal. report_error()
// will always print to stderr, and the EXIT_ON_ERROR flag is in effect.
#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef EXIT_ON_ERROR
#define EXIT_ON_ERROR
#endif

#include <errno.h>

#include "errors.h"
#include "sync.h"


//NOTE: pthread functions don't set errno, they return the error code instead

void mutex_init(mutex_t *mutex)
{
	int ret = pthread_mutex_init(&mutex->mutex, NULL);
	if (ret != 0) {
		errno = ret;
		report_error("pthread_mutex_init");
	}
}

void mutex_destroy(mutex_t *mutex)
{
	int ret = pthread_mutex_destroy(&mutex->mutex);
	if (ret != 0) {
		errno = ret;
		report_error("pthread_mutex_destroy");
	}
}

void mutex_lock(mutex_t *mutex)
{
	int ret = pthread_mutex_lock(&mutex->mutex);
	if (ret != 0) {
		errno = ret;
		report_error("pthread_mutex_lock");
	}
}

void mutex_unlock(mutex_t *mutex)
{
	int ret = pthread_mutex_unlock(&mutex->mutex);
	if (ret != 0) {
		errno = ret;
		report_error("pthread_mutex_unlock");
	}
}


void cond_init(cond_t *cond)
{
	int ret = pthread_cond_init(&cond->cond, NULL);
	if (ret != 0) {
		errno = ret;
		report_error("pthread_cond_init");
	}
}

void cond_destroy(cond_t *cond)
{
	int ret = pthread_cond_destroy(&cond->cond);
	if (ret != 0) {
		errno = ret;
		report_error("pthread_cond_destroy");
	}
}

void cond_wait(cond_t *cond, mutex_t *mutex)
{
	// COND_AUTO_WAKEUP can be enabled to cause threads waiting on condition
	// variables to sporadically and automatically wake up. This is a debugging
	// feature and can be used to detect thread death conditions.
#ifndef COND_AUTO_WAKEUP
	int ret = pthread_cond_wait(&cond->cond, &mutex->mutex);
#else
	struct timepsec t;
	t.secs = 0;
	t.nsecs = 1000;
	int ret = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &t)
#endif

	if (ret != 0) {
		errno = ret;
		report_error("pthread_cond_wait");
	}
}

void cond_signal(cond_t *cond)
{
	int ret = pthread_cond_signal(&cond->cond);
	if (ret != 0) {
		errno = ret;
		report_error("pthread_cond_signal");
	}
}

void cond_broadcast(cond_t *cond)
{
	int ret = pthread_cond_broadcast(&cond->cond);
	if (ret != 0) {
		errno = ret;
		report_error("pthread_cond_broadcast");
	}
}

void nap(unsigned long nanoseconds)
{
	if (nanoseconds == 0) return;

	// How long we want to sleep.
	struct timespec required;
	required.tv_sec = 0;
	required.tv_nsec = nanoseconds;
	// Set to the time remaining if sleep was interrupted.
	struct timespec remainder;

	// Keep sleeping for remaining time if interrupted.
	int result;
	do {
		result = nanosleep(&required, &remainder);
		required = remainder;
	} while (result == -1 && errno == EINTR);

	if (result != 0) report_error("nanosleep");
}
