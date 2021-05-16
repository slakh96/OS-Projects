#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;
struct frame* head; 
struct frame* tail; 

void init_pointers(struct frame* current, struct frame* prev, struct frame* next); 

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	int indexToEvict = tail->index; 
	return indexToEvict;  
}

int findPositionInCoremap(pgtbl_entry_t *p); 

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p2) {
	int flaggy = 0;
	
	struct frame* p = &coremap[p2->frame >> PAGE_SHIFT];
	if(p == tail){
		flaggy = 1;
	}
	if(head == p){
		return; 
	}
	
	//Part 1: Patch the "hole"
	p->next->prev = p->prev;
	p->prev->next = p->next;
	
	//Part 2: "Insert" p into the head
	p->next = head;
	struct frame* old_head_prev = head->prev;
	head->prev = p;
	old_head_prev->next = p;
	struct frame *old_p_prev = p->prev;
	p->prev = old_head_prev;
	
	//Part 3: Modify heads and tails
	head = head->prev;
	if(flaggy){//The tail was accessed; so reset the tail pointer;
		tail = old_p_prev;
	}
	
	
	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	// Go through the coremap and set pointers 
	for(int i = 0;i<memsize;i++) {
		if(memsize == 1) {
			init_pointers(&coremap[0], &coremap[0], &coremap[0]);
		}	
		else if(i == 0) {
			init_pointers(&coremap[i], &coremap[memsize - 1], &coremap[i+1]);
		}
		else if(i == memsize - 1) {
			init_pointers(&coremap[i], &coremap[i-1], &coremap[0]);
		}
		else {
			init_pointers(&coremap[i], &coremap[i-1], &coremap[i+1]);
		}
		coremap[i].index = i;
	}
	
	head = &coremap[0]; 
	tail = &coremap[memsize-1]; 
	
}

void init_pointers(struct frame* current, struct frame* prev, struct frame* next) {
	if(current == NULL){
		return; 
	}
	current->prev = prev; 
	current->next = next; 
}
