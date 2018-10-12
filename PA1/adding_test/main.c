#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define sys_cs3753_add 334

int main()
{
    printf("Testing cs3753_add syscall\n");
    int result;
    int number1 = 1;
    int number2 = 2;
    syscall(sys_cs3753_add, number1, number2, &result);
    printf("Sum of %d and %d is %d\n", number1, number2, result);
    return 0;
}

