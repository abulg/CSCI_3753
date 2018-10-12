Contact: dhko9143@colorado.edu


arch/x86/kernel/Makefile: Contains instructions on how to compile and build the system calls and kernel.

arch/x86/kernel/cs3753_add.c: Systemcall program that adds two numbers together and does a KERNALERT of the result.

arch/x86/entry/syscalls/syscall_64.tbl: Added our new system call to the system call table.

include/linux/syscalls.h: Header file that links syscalls to our test programs.

cs3753_add_test.c: Test program to test the new syscalls.
syslog: System log.