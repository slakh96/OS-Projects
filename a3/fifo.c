#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include <stdio.h>


extern int memsize;

extern int debug;

extern struct frame *coremap;
extern int hit_count;


// The index representing the next slot to insert into, in the queue 
int indexPointer; 
int fifo_hits_count;

/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
	return indexPointer; 
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
	
	if (fifo_hits_count != hit_count){
		return; 
	}
	else {
		indexPointer++;
		if (indexPointer == memsize){
			indexPointer = 0;
		}
	}
	fifo_hits_count = hit_count; //Update fifo hits count to reflect the 
}

/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void fifo_init() {
	// Create a new queue of size memsize. This queue will store pointer  
	// to page table entry 
	
	// initialize nextIndex to be 0 
	indexPointer = 0;
	fifo_hits_count = hit_count;
}
