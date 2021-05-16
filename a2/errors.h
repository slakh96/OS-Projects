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
 * CSC369 Assignment 2 - Error handling utilities header file.
 *
 * NOTE: This file will be replaced when we run your code.
 * DO NOT make any changes here.
 */

#pragma once

#include <stdbool.h>


#define STRINGIZE_(x) #x
#define STRINGIZE(x) STRINGIZE_(x)

/** Get current file and line as a string literal. */
#define CODE_LOCATION " @ " __FILE__ ":" STRINGIZE(__LINE__)


//NOTE: If compiled in debug mode, we will report errors and optionally exit the
// program immediately when an error occurs (see errors.c, EXIT_ON_ERROR).
//
// If compiled in non-debug mode (NDEBUG preprocessor macro is defined), this is
// disabled. In this case, we instead rely on errno to report the error. This
// is what is normally expected from a system library - reporting errors is the
// responsibility of the program using the library.

#ifndef NDEBUG

/**
 * Print debugging information when an error is detected.
 *
 * Assumes that the current value of errno describes the error being reported.
 * This macro is basically an enhanced version of perror() and should be used in
 * a similar fashion (see "man 3 perror"). The message is suffixed with the code
 * location (file name and line number) where the macro is invoked.
 *
 * @param msg  message string. Should include the name of the function that
 *             incurred the error and (optionally) information about the error.
 */
#define report_error(msg) __report_error(msg CODE_LOCATION, false)

#ifdef DEBUG_VERBOSE

/**
 * Print debugging info for an error that could happen during normal operation.
 *
 * Examples include a non-blocking read or write call failing with EAGAIN, a
 * read call failing with EMSGSIZE when the message is larger than the buffer,
 * or a write call failing with EPIPE when there are no readers. Does not
 * terminate the program - the EXIT_ON_ERROR flag is ignored.
 *
 * @param msg  message string. Should include the name of the function that
 *             incurred the error and (optionally) information about the error.
 */
#define report_info(msg) __report_error("[INFO] " msg CODE_LOCATION, true)

#else// DEBUG_VERBOSE

#define report_info(s)

#endif// DEBUG_VERBOSE

#else// NDEBUG

#define report_error(s)
#define report_info(s)

#endif// NDEBUG


/**
 * Underlying implementation of error reporting macros.
 *
 * @param msg   message string.
 * @param info  true if this is just info and EXIT_ON_ERROR should be ignored.
 */
void __report_error(const char *msg, bool info);
