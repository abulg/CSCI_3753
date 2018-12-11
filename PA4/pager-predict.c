/*
 * File: pager-predict.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2012/04/03
 * Description:
 * 	This file contains a predictive pageit
 *      implmentation.
 */

#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include "simulator.h"
#include "predict.h"

#define MAX_PAGE_ALLOC 40
//#define REALLOC_BASE 50
//#define REALLOC_INTERVAL (REALLOC_BASE*100) /* ticks */

static struct phist phist_arr[MAXPROCESSES];
static int pg_alloc[MAXPROCESSES];
static int proc_faults[MAXPROCESSES];
static uint32_t proc_susp[MAXPROCESSES];
static int proc_pset[MAXPROCESSES][MAXPROCPAGES];
static int proc_last_evict[MAXPROCESSES];
static int proc_last_unsat[MAXPROCESSES];
static int proc_last_pagein[MAXPROCESSES];


static size_t tick = 1;

static size_t timestamps[MAXPROCESSES][MAXPROCPAGES];

struct proc_fault_pair
{
    int proc;
    int faults;
};

size_t pages_alloc(Pentry q[MAXPROCESSES], int proc){
    int page;
    size_t amount = 0;

    for(page = 0; page < MAXPROCPAGES; page++){
        if(q[proc].pages[page]){
            amount++;
        }
    }

    return amount;
}

void lru_page(Pentry q[MAXPROCESSES], int proc, uint32_t tick, int *evictee){
    int page;
    size_t tock;

    *evictee = -1;
    tock = tick + 1;

    for(page = 0; page < MAXPROCPAGES; page++){
        if(!q[proc].pages[page]){
            continue;
        }

        if(timestamps[proc][page] < tock){
            tock = timestamps[proc][page];
            *evictee = page;

            if(tock <= 1){
                break;
            }
        }
    }

    if(*evictee < 0){
        printf("page for process %d w/ %u active pages not found with age < %u\n", proc, (unsigned int) pages_alloc(q, proc), tick);
        //exit(0);
    }
}

static int cmp_pfp(const void * pfp1, const void * pfp2){
    struct proc_fault_pair *p1;
    struct proc_fault_pair *p2;

    p1 = (struct proc_fault_pair *) pfp1;
    p2 = (struct proc_fault_pair *) pfp2;

    if(p1 -> faults < p2 -> faults){
        return -1;
    }else if (p1 -> faults == p2 -> faults){
        return 0;
    }else{
        return 1;
    }
}

static void pred_pageit(Pentry q[MAXPROCESSES], uint32_t tick){
    int proc;
    int page;
    int evicted;
    int i;

    struct phist_record ph_r;
    int unsat[MAXPROCESSES];
    int amount_unsat = 0;
    int allocated[MAXPROCESSES];
    int faults[MAXPROCESSES];
    int pset_sizes[MAXPROCESSES];
    int pages_freed = 0;
    int free_phys = 100;

    for(i = 0; i < MAXPROCESSES; i++){
        unsat[i] = -1;
    }

    for(proc = 0; proc < MAXPROCESSES; proc++){
        if(!q[proc].active){
            continue;
        }

        allocated[proc] = pages_alloc(q, proc);
        free_phys -= allocated[proc];

        if(proc_susp[proc] > 0){
            continue;
        }

        page = q[proc].pc/PAGESIZE;
        timestamps[proc][page] = tick;

        ph_r.page = page;
        if(q[proc].pages[page]){
            ph_r.fault = 0;
            phist_push(&phist_arr[proc], &ph_r);
            continue;
        }

        ph_r.fault = 1;
        phist_push(&phist_arr[proc], &ph_r);

        phist_working_set_and_fault_sum(&phist_arr[proc], proc_pset[proc], MAXPROCESSES, &faults[proc]);

        pset_sizes[proc] = 0;
        for(i = 0; i < MAXPROCESSES; i++){
            if(proc_pset[proc][i] == 1){
                pset_sizes[proc]++;
            }
        }

        if(pset_sizes[proc] < allocated[proc]){
            for(i = 0; i < MAXPROCPAGES; i++){
                if(q[proc].pages[i] && proc_pset[proc][i] == 0){
                    pageout(proc, i);
                    pages_freed++;
                }
            }
        }

        if(pagein(proc, page)){
            proc_last_pagein[proc] = tick;
            free_phys--;
            continue;
        }

        if(allocated[proc] < 1 && (tick - proc_last_unsat[proc]) < 100){
            continue;
        }

        unsat[proc] = page;
        amount_unsat++;
        proc_last_unsat[proc] = tick;
    }

    assert(free_phys >= 0);

    if(amount_unsat > 0) {     
        
        if(pages_freed >= amount_unsat)
            return;

        struct proc_fault_pair thrash_list[MAXPROCESSES];
        for(i = 0; i < MAXPROCESSES; i++) {
            thrash_list[i].proc = i;
            thrash_list[i].faults = ((q[i].active && (proc_susp[i] == 0) )? faults[i] : -1);
        } /* ordered by proc */

        /* now reorder by faults */
        qsort(thrash_list, MAXPROCESSES, sizeof(struct proc_fault_pair), cmp_pfp);
        
        int k;
        for(k = 0; k < MAXPROCESSES; k++) {
            if((tick - proc_last_pagein[thrash_list[MAXPROCESSES-1-k].proc]) > 100 && thrash_list[MAXPROCESSES-1-k].faults >= 2048 && allocated[thrash_list[MAXPROCESSES-1-k].proc] > 0) {
                for(i = 0; i < MAXPROCPAGES; i++) 
                    if(q[thrash_list[MAXPROCESSES-1-k].proc].pages[i]) {
                        pageout(thrash_list[MAXPROCESSES-1-k].proc, i);
                        pages_freed++;
                    }
    
                if(unsat[thrash_list[MAXPROCESSES-1-k].proc] == 1){
                    pages_freed++;
                }
                
                fflush(stdout);
                proc_susp[thrash_list[MAXPROCESSES-1-k].proc] = tick;   
            }

            if(pages_freed >= amount_unsat){
                return;
            }
        }
            

        /* free a page from each unsatisfied process until
         * we have free at least amt_unsat pages */
        for(proc = 0; proc < MAXPROCESSES; proc++) {
            if(unsat[proc] < 0){
                continue;
            }

            if(allocated[proc] < 1){
                continue;
            }
                
            lru_page(q, proc, tick, &evicted);
            if(!pageout(proc, evicted) ) {
                exit(0);
            }
            pages_freed++;
            
            if(pages_freed >= amount_unsat){
                break;
            }
        }
    } /* amt_unsat > 0 */
    else if(free_phys > 0) {

        /* unsuspend a process which has less or equal to 
         * the number of free pages in its working set  */
        for(proc = 0; proc < MAXPROCESSES; proc++) {
            int wset_size = 0;
            if(proc_susp[proc] == 0){
                continue;
            }

            if( tick - proc_susp[proc] < 500 ){
                continue;
            }
                
            if(free_phys < 1){
                break;
            }

            for(i = 0; i < MAXPROCPAGES; i++) {
                wset_size += (proc_pset[proc][i] != 0);
            }

            if(wset_size > free_phys){
                continue;
            }

            //printf("%d: unsusp %d with %d pages (%d free)\n", tick, proc, wset_size, free_phys); 
            proc_susp[proc] = 0;
            for(i = 0; i < MAXPROCPAGES; i++) {
                if(proc_pset[proc][i]) {
                    if( !pagein(proc, i) ){
                        free_phys = 0;
                    }
                        
                    free_phys--;
                }
            }

        } 
    } 
}

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* This file contains the stub for a predictive pager */
    /* You may need to add/remove/modify any part of this file */

    /* Static vars */
    static int initialized = 0;
    //static int tick = 1; // artificial time
    
    /* Local vars */
    

    /* initialize static vars on first run */
    if(!initialized){
	/* Init complex static vars here */
        int i;
        for(i = 0; i < MAXPROCESSES; i++) {
            phist_init(&phist_arr[i]);
            pg_alloc[i] = 20;
            proc_faults[i] = 0;
            proc_susp[i] = 0;
            proc_last_evict[i] = 0;
            proc_last_unsat[i] = 0;
            proc_last_pagein[i] = 0;
        }
	initialized = 1;
    }
    
    /* TODO: Implement Predictive Paging */
    //fprintf(stderr, "pager-predict not yet implemented. Exiting...\n");
    //exit(EXIT_FAILURE);
    pred_pageit(q, tick);
    /* advance time for next pageit iteration */
    tick++;
} 
