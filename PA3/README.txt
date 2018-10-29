Contact: dhko9143@colorado.edu
Run ./multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]

PA3/Makefile: Contains instructions on how to compile and build the device driver module.

PA3/multi-lookup.c: code for the dns multithreaded lookup program.

PA3/multilookup.h: header for the dns multithreaded lookup program containing variable macros and struct for passing parameters into the thread functions.

PA3/util.c: code containing the dnslookup function

PA3/util.h: header file for util.c

PA3/queue.c: code for a non thread-safe FIFO queue implementation created by Chris Wailes <chris.wailes@gmail.com>, Wei-Te Chen <weite.chen@colorado.edu>, Andy Sayler <andy.sayler@gmail.com>  for use as a shared buffer.

PA3/queue.h: header file for queue.c created by the above authors