#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define sys_helloworld 333

int main(void)
{
    long res = syscall(sys_helloworld);
    //printf("System call returned %ld.\n", res);
    return res;
}
