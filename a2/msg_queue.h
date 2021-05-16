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
 * CSC369 Assignment 2 - Message queue header file.
 *
 * You may not use the pthread library directly. Instead you must use the
 * functions and types available in sync.h.
 */

#pragma once

#include <stdint.h>
#include <sys/types.h>


/** Opaque handle to a message queue. */
typedef uintptr_t msg_queue_t;

/** Symbol for a null message queue handle. */
#define MSG_QUEUE_NULL ((msg_queue_t)0)

/**
 * Create a message queue.
 *
 * The returned message queue handle can also optionally be open for
 * reading and/or writing depending on the given flags.
 *
 * Errors:
 *   EINVAL  flags are invalid.
 *   ENOMEM  Not enough memory (NOTE: malloc automatically sets this).
 *
 * @param capacity  number of bytes of capacity that the queue will have.
 * @param flags     a bitwise OR of MSG_QUEUE_* constants that describe the
 *                  attributes of the opened read and/or write handle. Set to 0
 *                  if not opening a read or write handle.
 * @return          handle to the new message queue on success;
 *                  MSG_QUEUE_NULL on failure (with errno set).
 */
msg_queue_t msg_queue_create(size_t capacity, int flags);

/** Queue handle flag indicating that the handle is for reading messages. */
#define MSG_QUEUE_READER 0x01

/** Queue handle flag indicating that the handle is for writing messages. */
#define MSG_QUEUE_WRITER 0x02

/**
 * Queue handle flag indicating non-blocking mode.
 *
 * If set, reading or writing from the queue handle will never block.
 */
#define MSG_QUEUE_NONBLOCK 0x04

/**
 * Open an additional handle to a message queue.
 *
 * Errors:
 *   EBADF   queue is not a valid message queue handle.
 *   EINVAL  flags are invalid.
 *
 * @param queue  handle to an existing message queue.
 * @param flags  MSG_QUEUE_* constants that indicate attributes of the
 * 	             opened read and/or write handle.
 * @return       new handle to the message queue on success;
 *               MSG_QUEUE_NULL on failure (with errno set).
 */
msg_queue_t msg_queue_open(msg_queue_t queue, int flags);

/**
 * Close and invalidate a message queue handle.
 *
 * The queue is destroyed when the last handle is closed.
 *
 * Errors:
 *   EBADF   queue is not a pointer to a valid message queue handle.
 *
 * @param queue  pointer to the message queue handle.
 * @return       0 on success, -1 on failure (with errno set).
 */
int msg_queue_close(msg_queue_t *queue);


/**
 * Read a message from a message queue.
 *
 * Blocks until the queue contains at least one message, unless the handle is
 * open in non-blocking mode. If the buffer is too small, the message stays in
 * the queue, and negated message size is returned. If the queue is empty and
 * all the writer handles to the queue have been closed, 0 is returned.
 *
 * Note that 0 is only returned when all writer handles have closed. It is not
 * returned if no writer handles have been opened yet.
 *
 * Errors:
 *   EAGAIN    The queue handle is non-blocking and the read would block.
 *   EBADF     queue is not a valid message queue handle open for reads.
 *   EMSGSIZE  The buffer is not large enough to hold the message.
 *
 * @param queue   handle to the queue to read from.
 * @param buffer  pointer to the buffer to write into.
 * @param count   maximum number of bytes to read.
 * @return        message size on success;
 *                negated message size if the buffer is too small;
 *                0 if all the writers have closed the queue;
 *                -1 in case of other failures (with errno set).
 */
ssize_t msg_queue_read(msg_queue_t queue, void *buffer, size_t count);

/**
 * Write a message into a message queue.
 *
 * Blocks until the queue has enough free space for the message, unless the
 * handle is open in non-blocking mode.
 *
 * Errors:
 *   EAGAIN    The queue handle is non-blocking and the write would block.
 *   EBADF     queue is not a valid message queue handle open for writes.
 *   EINVAL    Zero length message.
 *   EMSGSIZE  The queue buffer is not large enough to hold the message.
 *   EPIPE     All reader handles to the queue have been closed.
 *
 * @param queue   message queue handle.
 * @param buffer  pointer to the buffer to read from.
 * @param count   number of bytes to write.
 * @return        0 on success, -1 on failure (with errno set).
 */
int msg_queue_write(msg_queue_t queue, const void *buffer, size_t count);


/** Describes a message queue to be monitored in a msg_queue_poll() call. */
typedef struct msg_queue_pollfd {

	/**
	 * Message queue handle to be monitored.
	 *
	 * If set to MSG_QUEUE_NULL, this entry is ignored by msg_queue_poll().
	 */
	msg_queue_t queue;

	/**
	 * Requested events.
	 *
	 * A bitwise OR of MQPOLL_* constants. Set by the user before calling
	 * msg_queue_poll() and describe what events are subscribed to.
	 */
	int events;

	/**
	 * Returned events.
	 *
	 * A bitwise OR of MQPOLL_* constants. Set by the msg_queue_poll() call and
	 * indicate what events have actually occurred. If the queue handle is
	 * readable (writable), MQPOLL_NOWRITERS (MQPOLL_NOREADERS) may be reported,
	 * even if not explicitly subscribed to.
	 */
	int revents;

} msg_queue_pollfd;

/**
 * Event flag indicating that the *very next* read call will not block.
 *
 * This means that either there is a message to be read, or all the writer
 * handles to the queue have been closed. *Very next* means that no other read
 * or write calls happen between the poll call and the read call.
 */
#define MQPOLL_READABLE 0x01

/**
 * Event flag indicating that the *very next* write call will not block.
 *
 * This means that either there is room for a message to be written, or all the
 * reader handles to the queue have been closed. *Very next* means that no other
 * read or write calls happen between the poll call and the write call.
 */
#define MQPOLL_WRITABLE 0x02

/**
 * Event flag indicating that all reader handles to the queue have been closed.
 * Not triggered if no reader handles to the queue have been opened yet.
 */
#define MQPOLL_NOREADERS 0x04

/**
 * Event flag indicating that all writer handles to the queue have been closed.
 * Not triggered if no writer handles to the queue have been opened yet.
 */
#define MQPOLL_NOWRITERS 0x08

/**
 * Wait for an event to occur in any message queue from a set of queues.
 *
 * Errors:
 *   EINVAL  nfds is 0.
 *   EINVAL  No events specified in a pollfd entry (after implicitly included
 *           ones are considered).
 *   EINVAL  Event flags are invalid.
 *   EINVAL  MQPOLL_READABLE requested for a non-reader queue handle or
 *           MQPOLL_WRITABLE requested for a non-writer queue handle.
 *   ENOMEM  Not enough memory (NOTE: malloc automatically sets this).
 *
 * @param fds   message queues to be monitored.
 * @param nfds  number of items in the fds array.
 * @return      number of queues ready for I/O (with nonzero revents) on success;
 *              -1 on failure (with errno set).
 */
int msg_queue_poll(msg_queue_pollfd *fds, size_t nfds);
