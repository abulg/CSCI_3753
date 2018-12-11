/*
 * File: pager-lru.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2012/04/03
 * Description:
 * 	This file contains an lru pageit
 *      implmentation.
 */

#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "simulator.h"

static size_t tick = 1;

static size_t timestamps[MAXPROCESSES][MAXPROCPAGES];
// allocate page/s to process
size_t pages_alloc(Pentry q[MAXPROCESSES], int proc){
	int page;
	size_t amount = 0;

	for(page = 0; page < MAXPROCPAGES; page++){
		if(q[proc].pages[page])
			amount++;
	}

	return amount;
}

void lru_page(Pentry q[MAXPROCESSES], int proc, uint32_t tick, int *evictee){
	int page;
	size_t tock;
	// do better than a freshly referenced page
	*evictee = -1;
	tock = tick + 1;

	for(page = 0; page < MAXPROCPAGES; page++){
		if(!q[proc].pages[page]){	// if process is already in memory then cant do anything with it
			continue;
		}
		// if process page timestamp is less than freshly referenced page+1 then evict page
		if(timestamps[proc][page] < tock){
			tock = timestamps[proc][page];
			*evictee = page;

			if(tock <= 1)	// cant do 0 time page referencing so break out
				break;
		}
	}

	if(*evictee < 0){
		printf("page for process %d w/ %u active pages not found with age < %u\n", proc, (unsigned int) pages_alloc(q, proc), tick);
		//exit(0);
	}
}


static void lru_pageit(Pentry q[MAXPROCESSES], uint32_t tick){
	int proc;
	int page;
	int evicted;

	for(proc = 0; proc < MAXPROCESSES; proc++){
		if(!q[proc].active){
			continue;
		}

		page = q[proc].pc/PAGESIZE;
		timestamps[proc][page] = tick;
		// done if process is already in memory
		if(q[proc].pages[page]){
			continue;
		}
		// if page is just about to get placed, then continue since we cant do anything with it
		if(pagein(proc, page)){
			continue;
		}
		// done if no free pages to evict
		if(pages_alloc(q, proc) < 1){
			continue;
		}

		lru_page(q, proc, tick, &evicted);

		if(!pageout(proc, evicted)){
			exit(0);
		}
	}
}

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* This file contains the stub for an LRU pager */
    /* You may need to add/remove/modify any part of this file */

	static int initialized = 0;

    /* Local vars */
    int proctmp;
    int pagetmp;

    /* initialize static vars on first run */
    if(!initialized){
	for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
	    for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
		timestamps[proctmp][pagetmp] = 0; 
	    }
	}
	initialized = 1;
    }
    
    /* TODO: Implement LRU Paging */
    //fprintf(stderr, "pager-lru not yet implemented. Exiting...\n");
    //exit(EXIT_FAILURE);
    /*
    if(atexit(end) != 0){
    	perror(NULL);
    	exit(1);
    }
	*/
    lru_pageit(q, tick);

    /* advance time for next pageit iteration */
    tick++;
} 
