#include <pthread.h>
#include <stdbool.h>

#define USAGE "<# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]"
#define MINARGS 6
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_INPUT_FILES 10
#define MAX_RESOLVE_THREADS 10
#define MAX_REQUEST_THREADS 5
#define MAX_NAME_LENGTHS 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

bool requests_exist = true;

struct thread{
	FILE* threadfile;
	FILE* servicedfile;
	//char* buffer;
	//circular_buf_t* buffer;
	queue* buffer;
	//int count;
	pthread_mutex_t* buffmutex;
	pthread_mutex_t* outmutex;
};