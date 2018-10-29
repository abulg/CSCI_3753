#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
//#include "cbuf.h"
/* queue library made by  
* Author: Chris Wailes <chris.wailes@gmail.com>
* Author: Wei-Te Chen <weite.chen@colorado.edu>
* Author: Andy Sayler <andy.sayler@gmail.com> 
*/
#include "queue.h"
#include "multi-lookup.h"

//	function definitions
void* request(void* id);
void* resolve(void* id);

//	request function reads hostnames from input files and saves it to the shared buffer
void* request(void* id){
	struct thread* thread = id;
	char hostname[MAX_NAME_LENGTHS];
	char* domain;
	int count = 0;
	bool done = false;
	char *token;
	FILE* inputfp = thread->threadfile;
	//circular_buf_t* cbuf = thread->buffer;
	queue* buffer = thread->buffer;
	FILE* servicedfile = thread->servicedfile;

	pthread_mutex_t* buffmutex = thread->buffmutex;
	pthread_mutex_t* outmutex = thread->outmutex;

	//	scan input files till EOF
	while(fscanf(inputfp, INPUTFS, hostname) >0){
		//printf("c\n");
		while(!done){
			//	save lines to temp buffer and split by line (\n)
			domain = malloc(SBUFSIZE);
			token = strtok(hostname, "\n");
			strncpy(domain, token, SBUFSIZE);

			//	critical section for adding to buffer
			pthread_mutex_lock(buffmutex);
			//circular_buf_put(cbuf, *domain);
			queue_push(buffer, domain);
			pthread_mutex_unlock(buffmutex);

			done = true;
		}
		done = false;
	}
	//	increment thread counter
	count++;
	
	//	critical section for writing to serviced.txt
	pthread_mutex_lock(outmutex);
	//printf("Thread %lu serviced %d files\n", pthread_self(), count);
	fprintf(servicedfile, "Thread %lu serviced %d files\n", pthread_self(), count);
	pthread_mutex_unlock(outmutex);
	return NULL;
}

//	function to read hostnames from shared buffer and resolve dns and output IP address to output.txt
void* resolve(void* id){
	struct thread* thread = id;
	char* domain;
	FILE* outfile = thread->threadfile;
	pthread_mutex_t* buffmutex = thread->buffmutex;
	pthread_mutex_t* outmutex = thread->outmutex;
	//circular_buf_t* cbuf = thread->buffer;
	queue* buffer = thread->buffer;
	char ip[MAX_IP_LENGTH];

	// loop while shared buffer is not empty or if requester threads still exist
	while(!queue_is_empty(buffer)/*circular_buf_empty(cbuf)*/ || requests_exist){
		// critical section to remove from buffer
		pthread_mutex_lock(buffmutex);
		//circular_buf_get(cbuf, domain);
		domain = queue_pop(buffer);
		pthread_mutex_unlock(buffmutex);

		// dns lookup of hostnames from shared buffer
		if(dnslookup(domain, ip, sizeof(ip)) == UTIL_FAILURE){
			fprintf(stderr, "dnslookup error: %s\n", domain);
			strncpy(ip, "", sizeof(ip));
		}
		printf("%s:%s\n", domain, ip);
		//	critical section to write to output.txt
		pthread_mutex_lock(outmutex);
		fprintf(outfile, "%s,%s\n", domain, ip);
		pthread_mutex_unlock(outmutex);

		free(domain);
	}

	return NULL;
}


//requester log: serviced.txt
//resolver log: results.txt
//data file: input.txt's
//multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]
int main(int argc, char* argv[]){

	//clock_t t;
	//t = clock();
	// start runtime timer
	struct timeval start, end;
	gettimeofday(&start, NULL);

	// accept program arguments
	if(argc < MINARGS){
		fprintf(stderr, "You need at least %d arguments to run this program\n", MINARGS);
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}

	// initialize local variables
	queue buffer;
	//char buffer;
	//char* buffer = malloc(SBUFSIZE * sizeof(char));
	//cbuf_handle_t cbuf = circular_buf_init(buffer, SBUFSIZE);

	//printf("Buffer initialized. Full: %d, empty: %d, capacity: %zu\n", circular_buf_full(cbuf), circular_buf_empty(cbuf), circular_buf_capacity(cbuf));
	int input_files = argc - 5;
	FILE* outfile = NULL;
	FILE* servicedfile = NULL;
	FILE* inputfps[input_files];

	// # of threads from input
	int numRequests = atoi(argv[1]);//strtol(argv[1], NULL, 10);
	int numResolves = atoi(argv[2]);//strtol(argv[2], NULL, 10);

	//	error catching of requester/resolver thread number arguments
	if(numRequests > MAX_REQUEST_THREADS){
		fprintf(stderr, "Max number of requester threads allowed is %d\n", MAX_REQUEST_THREADS);
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}else{
		printf("# of requester threads is %d\n", numRequests);
	}

	if(numResolves > MAX_RESOLVE_THREADS){
		fprintf(stderr, "Max number of resolver threads allowed is %d\n", MAX_RESOLVE_THREADS);
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}else{
		printf("# of resolver threads is %d\n", numResolves);
	}

	pthread_t requests[numRequests];
	pthread_t resolves[numResolves];

	//Mutexes for requester log, resolver log, output results and structs to pass parameters to threads
	pthread_mutex_t buffmutex;
	pthread_mutex_t outmutex;
	struct thread request_info[numRequests];
	struct thread resolve_info[numResolves];

	//	error catching max input file argument
	if(input_files > MAX_INPUT_FILES){
		fprintf(stderr, "Max number of input files allowed is %d\n", MAX_INPUT_FILES);
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}

	//	Open results output file
	outfile = fopen(argv[4], "w");
	if(!outfile){
		perror("Error opening output results file");
		return EXIT_FAILURE;
	}

	//	Open serviced.txt file
	servicedfile = fopen(argv[3], "w");
	if(!servicedfile){
		perror("Error opening serviced file");
		return EXIT_FAILURE;
	}

	//Open input files
	int i;
	for(i = 1; i <= input_files; i++){
		inputfps[i-1] = fopen(argv[4+i], "r");
	}

	//	initialize buffer and mutexes
	queue_init(&buffer, MAX_NAME_LENGTHS);

	pthread_mutex_init(&outmutex, NULL);
	pthread_mutex_init(&buffmutex, NULL);

	//	create requester and resolver threads
	for(i = 0; i < numRequests; i++){
		request_info[i].threadfile = inputfps[i];
		request_info[i].buffer = &buffer;
		request_info[i].buffmutex = &buffmutex;
		request_info[i].outmutex = &outmutex;	//NULL
		request_info[i].servicedfile = servicedfile;

		pthread_create(&(requests[i]), NULL, request, &(request_info[i]));
		printf("Create request %d\n", i);
	}

	for(i = 0; i < numResolves; i++){
		resolve_info[i].threadfile = outfile;
		resolve_info[i].buffer = &buffer;
		resolve_info[i].buffmutex = &buffmutex;
		resolve_info[i].outmutex = &outmutex;

		pthread_create(&(resolves[i]), NULL, resolve, &(resolve_info[i]));
		printf("Create resolver %d\n", i);
	}

	//	join created threads
	for(i = 0; i < numRequests; i++){
		
		pthread_join(requests[i], NULL);
		printf("Requested %d\n", i);
	}
	requests_exist = false;
	for(i = 0; i < numResolves; i++){
		pthread_join(resolves[i], NULL);
		printf("Resolved %d\n", i);
	}

	//clean up/close files and buffer
	//circular_buf_free(cbuf);
	queue_cleanup(&buffer);
	fclose(outfile);
	fclose(servicedfile);
	
	for(i = 0; i < input_files; i++){
		fclose(inputfps[i]);
	}

	pthread_mutex_destroy(&buffmutex);
	pthread_mutex_destroy(&outmutex);

	//	end runtime timer
	//t = clock() - t;
	//double time_taken = ((double)t)/CLOCKS_PER_SEC;
	gettimeofday(&end, NULL);

	// print total program runtime in seconds
	printf("Runtime: %lds\n", ((end.tv_sec) - (start.tv_sec)));
	//printf("Computation time: %f seconds\n", time_taken);

	return 0;
}
