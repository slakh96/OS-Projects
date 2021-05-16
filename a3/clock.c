#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;
extern int hit_count;
int clkIndex;
int evicted; 
int clockHitCount; 

extern struct frame *coremap;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	//Iterate through the coremap; at each index, if the ref bit not set, then
	//immediately return clkIndex; else set the ref bit to zero and clkIndex++.
	//If clkIndex == memsize, then clkIndex = 0
	int i = 0;
	evicted = 1; 
	while(i < memsize + 1){
		pgtbl_entry_t* p = coremap[clkIndex].pte;
		
		if (!(p->frame & PG_REF)){
			int old = clkIndex; 
			clkIndex++; 
			if (clkIndex == memsize){ //Loop back to the beginning
				clkIndex = 0;
			}
			return old; 
		}
		else {
			p->frame = p->frame & ~PG_REF;  //Unset the pg ref bit
			clkIndex++;
			if (clkIndex == memsize){ //Loop back to the beginning
				clkIndex = 0;
			}
		}
		i++;
	}
	return 0;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	return; 
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	clkIndex = 0; //This index tells us which index of coremap the clock is pointing to.
	evicted = 0; 
	clockHitCount = hit_count; 
}
