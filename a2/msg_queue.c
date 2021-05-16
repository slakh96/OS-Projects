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
 * CSC369 Assignment 2 - Message queue implementation.
 *
 * You may not use the pthread library directly. Instead you must use the
 * functions and types available in sync.h.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "errors.h"
#include "list.h"
#include "msg_queue.h"
#include "ring_buffer.h"

// TODO: check this, it was added by sultan
#include "sync.h"
#include "stdio.h"

// Message queue implementation backend
typedef struct mq_backend {
	// Ring buffer for storing the messages
	ring_buffer buffer;

	// Reference count
	size_t refs;

	// Number of handles open for reads
	size_t readers;
	// Number of handles open for writes
	size_t writers;

	// Set to true when all the reader handles have been closed. Starts false
	// when they haven't been opened yet.
	bool no_readers;
	// Set to true when all the writer handles have been closed. Starts false
	// when they haven't been opened yet.
	bool no_writers;

	//TODO - Add the necessary synchronization variables. - DONE
	mutex_t buffer_read_write_mutex;
	cond_t buffer_full_cond;
	cond_t buffer_empty_cond;
	
	list_head head;

} mq_backend;

typedef struct cv_container{
    cond_t cv;
    list_entry lentry;
    mutex_t mt;
} cv_container;

void broadcast_cvs(mq_backend * mq);
int helper_function(msg_queue_pollfd * fds, size_t nfds);


static int mq_init(mq_backend *mq, size_t capacity)
{
	if (ring_buffer_init(&mq->buffer, capacity) < 0) return -1;

	mq->refs = 0;

	mq->readers = 0;
	mq->writers = 0;

	mq->no_readers = false;
	mq->no_writers = false;

	//TODO
	mutex_init(&(mq->buffer_read_write_mutex));
	cond_init(&(mq->buffer_full_cond));
	cond_init(&(mq->buffer_empty_cond));
	list_init(&(mq->head));
	return 0;
}

static void mq_destroy(mq_backend *mq)
{
	assert(mq->refs == 0);
	assert(mq->readers == 0);
	assert(mq->writers == 0);

	//TODO
	cond_destroy(&(mq->buffer_full_cond));
	cond_destroy(&(mq->buffer_empty_cond));
	mutex_unlock(&(mq->buffer_read_write_mutex));
	mutex_destroy(&(mq->buffer_read_write_mutex));
	list_head * head = &(mq->head);
    list_entry * current = &(head->head);
	list_entry ** parent = &current;
	int counter = 0;
    	//list_entry ** first = &current;//(head->hea
	list_for_each(current, head)
 	{
		if ((*parent == current)&&(counter >0)){
			break;
		}
		counter++;
		cv_container * container;
        container = container_of(current, cv_container, lentry);
		free(container);//TODO CHECK IF THIS CAUSES SEGFAULTS 	
    }	

	ring_buffer_destroy(&mq->buffer);
}


#define ALL_FLAGS (MSG_QUEUE_READER | MSG_QUEUE_WRITER | MSG_QUEUE_NONBLOCK)

// Message queue handle is a combination of the pointer to the queue backend and
// the handle flags. The pointer is always aligned on 8 bytes - its 3 least
// significant bits are always 0. This allows us to store the flags within the
// same word-sized value as the pointer by ORing the pointer with the flag bits.

// Get queue backend pointer from the queue handle
static mq_backend *get_backend(msg_queue_t queue)
{
	mq_backend *mq = (mq_backend*)(queue & ~ALL_FLAGS);
	assert(mq);
	return mq;
}

// Get handle flags from the queue handle
static int get_flags(msg_queue_t queue)
{
	return (int)(queue & ALL_FLAGS);
}

// Create a queue handle for given backend pointer and handle flags
static msg_queue_t make_handle(mq_backend *mq, int flags)
{
	assert(((uintptr_t)mq & ALL_FLAGS) == 0);
	assert((flags & ~ALL_FLAGS) == 0);
	return (uintptr_t)mq | flags;
}


static msg_queue_t mq_open(mq_backend *mq, int flags)
{
	++mq->refs;

	if (flags & MSG_QUEUE_READER) {
		++mq->readers;
		mq->no_readers = false;
	}
	if (flags & MSG_QUEUE_WRITER) {
		++mq->writers;
		mq->no_writers = false;
	}

	return make_handle(mq, flags);
}

// Returns true if this was the last handle
static bool mq_close(mq_backend *mq, int flags)
{
	assert(mq->refs != 0);
	assert(mq->refs >= mq->readers);
	assert(mq->refs >= mq->writers);
	
	int flaggy = 0;

	if (flags & MSG_QUEUE_READER) {
		if (--mq->readers == 0){
		    mq->no_readers = true;
		    if (flaggy == 0){
		        broadcast_cvs(mq);
		        flaggy = 1;
		    }
		} 
	}
	if (flags & MSG_QUEUE_WRITER) {
		if (--mq->writers == 0){
		    mq->no_writers = true;
		    if (flaggy == 0){
		        broadcast_cvs(mq);
		        flaggy = 1;
		    }
		}
	}

	if (--mq->refs == 0) {
		assert(mq->readers == 0);
		assert(mq->writers == 0);
		return true;
	}
	return false;
}


msg_queue_t msg_queue_create(size_t capacity, int flags)
{
	if (flags & ~ALL_FLAGS) {
		errno = EINVAL;
		report_error("msg_queue_create");
		return MSG_QUEUE_NULL;
	}

	mq_backend *mq = (mq_backend*)malloc(sizeof(mq_backend));
	if (mq == NULL) {
		report_error("malloc");
		return MSG_QUEUE_NULL;
	}
	// Result of malloc() is always aligned on 8 bytes, allowing us to use the
	// 3 least significant bits of the handle to store the 3 bits of flags
	assert(((uintptr_t)mq & ALL_FLAGS) == 0);
	
	if (mq_init(mq, capacity) < 0) {
		// Preserve errno value that can be changed by free()
		int e = errno;
		free(mq);
		errno = e;
		return MSG_QUEUE_NULL;
	}

	mutex_lock(&(mq->buffer_read_write_mutex));
	msg_queue_t  mqt = mq_open(mq, flags);
	mutex_unlock(&(mq->buffer_read_write_mutex));
	return mqt;
}

msg_queue_t msg_queue_open(msg_queue_t queue, int flags)
{
	if (!queue) {
		errno = EBADF;
		report_error("msg_queue_open");
		return MSG_QUEUE_NULL;
	}

	if (flags & ~ALL_FLAGS) {
		errno = EINVAL;
		report_error("msg_queue_open");
		return MSG_QUEUE_NULL;
	}

	mq_backend *mq = get_backend(queue);

	//TODO
	mutex_lock(&(mq->buffer_read_write_mutex));

	msg_queue_t new_handle = mq_open(mq, flags);
	mutex_unlock(&(mq->buffer_read_write_mutex));

	return new_handle;
}

int msg_queue_close(msg_queue_t *queue)
{
	if (!(*queue)) {
		errno = EBADF;
		report_error("msg_queue_close");
		return -1;
	}

	mq_backend *mq = get_backend(*queue);

	//TODO
	mutex_lock(&(mq->buffer_read_write_mutex));
	
	if (mq_close(mq, get_flags(*queue))) {
		mq_destroy(mq);
		free(mq);
		*queue = MSG_QUEUE_NULL;
		
		return 0;
	}
	mutex_unlock(&(mq->buffer_read_write_mutex));

	*queue = MSG_QUEUE_NULL;
	return 0;
}


ssize_t msg_queue_read(msg_queue_t queue, void *buffer, size_t count)
{
	//TODO
	if (!queue) {
		errno = EBADF;
		report_error("msg_queue_read");
		return MSG_QUEUE_NULL;
	}
	mq_backend * mq = get_backend(queue);
	mutex_lock(&(mq->buffer_read_write_mutex));
	int used = ring_buffer_used(&(mq->buffer));
	if (mq->no_writers && used == 0){
		mutex_unlock(&(mq->buffer_read_write_mutex));
		return 0;
	}

	if ((queue & MSG_QUEUE_NONBLOCK) && (used == 0)){ 
        errno = EAGAIN;
        report_error("msg_queue_read");
		mutex_unlock(&(mq->buffer_read_write_mutex));
        return -1;
    }
	 while(ring_buffer_used(&(mq->buffer)) == 0){
	    cond_wait(&(mq->buffer_empty_cond), &(mq->buffer_read_write_mutex));
	 }
	 size_t intBuffer;
	 bool peekIntSuccess = ring_buffer_peek(&(mq->buffer), &intBuffer, sizeof(size_t));
	 if (!peekIntSuccess){
		mutex_unlock(&(mq->buffer_read_write_mutex));
		return -1;
	 }
	 if (intBuffer > count){ //buffer too small to hold msg
	    errno = EMSGSIZE;
        report_error("msg_queue_read");
        mutex_unlock(&(mq->buffer_read_write_mutex));
        return -1*intBuffer;
	 }
	 bool readIntSuccess = ring_buffer_read(&(mq->buffer), &intBuffer, sizeof(size_t));
	 if (!readIntSuccess){
		mutex_unlock(&(mq->buffer_read_write_mutex));
		return -1;
	 }
	 bool readMsgSuccess = ring_buffer_read(&(mq->buffer), buffer, intBuffer);
	 if (!readMsgSuccess){
		mutex_unlock(&(mq->buffer_read_write_mutex));
		return -1;
	 }
	 cond_signal(&(mq->buffer_full_cond));
	 broadcast_cvs(mq);
	 mutex_unlock(&(mq->buffer_read_write_mutex));
	 
	 return count;
}

int msg_queue_write(msg_queue_t queue, const void *buffer, size_t count)
{
    if (!queue) {
		errno = EBADF;
		report_error("msg_queue_writee");
		return MSG_QUEUE_NULL;
	}
	mq_backend * mq = get_backend(queue);
	mutex_lock(&(mq->buffer_read_write_mutex));
	int used = ring_buffer_used(&(mq->buffer));
	int free = ring_buffer_free(&(mq->buffer));
	bool no_readers = mq->no_readers;
    //Check if the queue is nonblocking mode and the write would block (msg_queue already full)    
    if ((queue & MSG_QUEUE_NONBLOCK) && (free < (int)count)){ //Is it zero or count to put here? TODO
        errno = EAGAIN;
        report_error("msg_queue_write");
        return -1;
    }
    if (count == 0){
        errno = EINVAL;
		report_error("msg_queue_wtire");
		return MSG_QUEUE_NULL;
    }
    if (free + used < (int)count){
        errno = EMSGSIZE;
        report_error("msg_queue_write");
        return -1;
    }
    if (no_readers){
        errno = EPIPE;
        report_error("msg_queue_write");
        return -1;
    }
   
    while (ring_buffer_free(&(mq->buffer)) == 0){
		cond_wait(&(mq->buffer_full_cond), &(mq->buffer_read_write_mutex)); 
	}
	
	bool intSuccess = ring_buffer_write(&(mq->buffer), &count, sizeof(count)); //Write the size_t representing the size of msg about to be written
	if (!intSuccess){
	    mutex_unlock(&(mq->buffer_read_write_mutex));
	    return -1;
	}
	
	
	bool msgSuccess = ring_buffer_write(&(mq->buffer),buffer, count); //Write the size_t representing the size of msg about to be written
	if (!msgSuccess){
	    mutex_unlock(&(mq->buffer_read_write_mutex));
	    return -1;
	}
	broadcast_cvs(mq);
	cond_signal(&(mq->buffer_empty_cond));
	mutex_unlock(&(mq->buffer_read_write_mutex));
	
	return 0;
}

void broadcast_cvs(mq_backend * mq){
    //mutex_lock(&(mq->buffer_read_write_mutex));
    list_head * head = &(mq->head);
    list_entry * current = &(head->head);
    list_entry ** first = &current;//(head->head);
    int counter = 0;
    list_for_each(current, head)
    {
        if ((current == *first) && (counter > 0)){
            break;
    	}
		cv_container * container = container_of(current, cv_container, lentry);
		mutex_lock(&(container->mt));
		cond_signal(&(container->cv));
		mutex_unlock(&(container->mt));
        counter++;
    }
}

int helper_function(msg_queue_pollfd * fds, size_t nfds){
    int count = 0;
    
    for (int i = 0; i < (int)nfds; i++){
		// acquire individual mutex
		if (!fds[i].queue){
			errno = EBADF;
			report_error("helper_function");
			return MSG_QUEUE_NULL;
		}
		mq_backend * mq = get_backend(fds[i].queue);
		bool writer = ((get_flags((fds[i].queue))) & MSG_QUEUE_WRITER);
	    bool reader = ((get_flags((fds[i].queue))) & MSG_QUEUE_READER);
		mutex_lock(&(mq->buffer_read_write_mutex));
		int flags = fds[i].events;
		if (flags & MQPOLL_NOWRITERS){
			if (mq->no_writers) {
				fds[i].revents = fds[i].revents|MQPOLL_NOWRITERS;
			}
		}
		if (flags & MQPOLL_NOREADERS){
			if (mq->no_readers){
				fds[i].revents = fds[i].revents|MQPOLL_NOREADERS;
			}
		}
		if (mq->no_writers && reader){
			fds[i].revents = fds[i].revents|MQPOLL_NOWRITERS;
		}
		if (mq->no_readers && writer){
			fds[i].revents = fds[i].revents|MQPOLL_NOREADERS;
		}
		if (flags & MQPOLL_WRITABLE){
			if ((mq->no_readers) || (ring_buffer_free(&(mq->buffer)) > 0)){
				fds[i].revents = fds[i].revents|MQPOLL_WRITABLE;
			}
		}
		if (flags & MQPOLL_READABLE){
			if ((mq->no_writers) || (ring_buffer_used(&(mq->buffer)) > 0)){
				fds[i].revents = fds[i].revents|MQPOLL_READABLE;
			}
		}
		if (fds[i].revents > 0){
			count ++;
		}
		mutex_unlock(&(mq->buffer_read_write_mutex));

	}
	return count;
}


int msg_queue_poll(msg_queue_pollfd *fds, size_t nfds)
{
	if (nfds == 0){
			errno = EINVAL;
        	report_error("msg_queue_poll");
        	return -1;
		}
	
	cv_container * container = (cv_container *)(malloc(sizeof(cv_container))); //TODO figure out where to free this memory
	cond_init(&(container->cv));
	list_entry_init(&(container->lentry));
	mutex_init(&(container->mt));
	mutex_lock(&(container->mt));

	// cv_wait calls 
	int helperAns = helper_function(fds, nfds);
	if (helperAns > 0){
	    mutex_unlock(&(container->mt));
	    return helperAns;
	} 
	for (int i = 0;i < (int)nfds; i++){
		if ((fds[i].events & MQPOLL_WRITABLE) && (~(get_flags((fds[i].queue))) & MSG_QUEUE_WRITER)){
			errno = EINVAL;
        	report_error("msg_queue_poll");
        	return -1;
		}
		if ((fds[i].events & MQPOLL_READABLE) && (~(get_flags((fds[i].queue))) & MSG_QUEUE_READER)){
			errno = EINVAL;
        	report_error("msg_queue_poll");
        	return -1;
		}
		if (!fds[i].queue){
			errno = EBADF;
			report_error("msg_queue_poll");
			return MSG_QUEUE_NULL;
		}
	    mq_backend * mq = get_backend(fds[i].queue);
	    
	    mutex_lock(&(mq->buffer_read_write_mutex));
	    list_head * head = &(mq->head);
	    list_add_tail(head, &(container->lentry));
	    mutex_unlock(&(mq->buffer_read_write_mutex));
	}
	
	while((helper_function(fds, nfds)) == 0){
	    cond_wait(&(container->cv), &(container->mt));
	}
	int x = helper_function(fds, nfds);
	mutex_unlock(&(container->mt));
	return x;
}
