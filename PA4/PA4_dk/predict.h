#ifndef PREDICT_H
#define PREDICT_H

#include <string.h>
#include <assert.h>

struct phist_record {
	int page;
	int fault;
};

#define HSIZE (1 << 11)

struct phist {
	struct phist_record records[HSIZE];
	int head;
	int tail;
};


void phist_init(struct phist *ph);
void phist_push(struct phist *ph, const struct phist_record *ph_r);
void phist_len(const struct phist *ph, int *len);
/* phist_len(ph, &hlen);
 * t = 0 -> most recent access
 * t = 1 -> 2nd most recent access
 * ...
 * t = hlen-1 -> oldest access
 */
void phist_at(const struct phist *ph, int t, struct phist_record *ph_r);
void phist_fault_sum(const struct phist *ph, int *sum);
void phist_working_set(const struct phist *ph, int *pset, size_t psize);
void phist_working_set_and_fault_sum(const struct phist *ph, int *pset, size_t psize, int *sum);
void phist_print_records(const struct phist *ph);


#define HPLUS_ONE(_element) ( (_element < (HSIZE-1) )? (_element+1) : 0)
#define HMINUS_ONE(_element) ( (_element == 0 )? (HSIZE-1) : (_element-1) )

static void inc_head(struct phist *ph) {
	int tmp;

	tmp = HPLUS_ONE(ph->head);
	if(tmp == ph->tail){
		ph->tail = HPLUS_ONE(ph->tail);
	}

	ph->head = tmp;
}

void phist_init(struct phist *ph) {
	ph->head = 0;
	ph->tail = 0;
}

void phist_push(struct phist *ph, const struct phist_record *ph_r) {
	ph->records[ph->head] = *ph_r;
	inc_head(ph);
}

void phist_len(const struct phist *ph, int *len) {
	int tmp;

	tmp = HPLUS_ONE(ph->head);
	if(tmp == ph->tail){
		*len = HSIZE;
	}
	else{
		*len = (ph->head - ph->tail) + 1;
	}
}

void phist_at(const struct phist *ph, int t, struct phist_record *ph_r) {
	int len, i, ptr;
	
	phist_len(ph, &len);
	assert(t >= 0 && t < len);

	ptr = ph->head;
	for(i = 0; i < t; i++) {
		ptr = HMINUS_ONE(ptr);
	}

	*ph_r = ph->records[ptr];
}

void phist_fault_sum(const struct phist *ph, int *sum) {
	int ptr;

	*sum = 0;
	for(ptr = ph->tail; ptr != ph->head; ptr = HPLUS_ONE(ptr) ) {
		*sum += (ph->records[ptr].fault == 1);
	}
}

void phist_working_set(const struct phist *ph, int *pset, size_t psize) {
	int ptr;
	unsigned int i;
	
	for(i = 0; i < psize; i++) {
		pset[i] = 0;
	}
	
	for(ptr = ph->tail; ptr != ph->head; ptr = HPLUS_ONE(ptr) ) {
		pset[ph->records[ptr].page] = 1;
	} 
}

void phist_working_set_and_fault_sum(const struct phist *ph, int *pset, size_t psize, int *sum) {
	int ptr;
	unsigned int i;
	
	for(i = 0; i < psize; i++) {
		pset[i] = 0;
	}
	*sum = 0;

	for(ptr = ph->tail; ptr != ph->head; ptr = HPLUS_ONE(ptr) ) {
		pset[ph->records[ptr].page] = 1;
		*sum += (ph->records[ptr].fault == 1);
	} 
}

void phist_print_records(const struct phist *ph) {
	int ptr;

	for(ptr = ph->tail; ptr != ph->head; ptr = HPLUS_ONE(ptr) ) {
		printf("%d ", ph->records[ptr].page);		
	}

}

#endif /* PREDICT_H */