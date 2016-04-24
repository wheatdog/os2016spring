#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>

#define gettime(A) syscall(315, A)

static void print_result(int pid, struct timespec ts[2]){
	char str[100];
	sprintf(str, "[Project1] %d %ld.%09ld %ld.%09ld\n", pid,
	                            ts[0].tv_sec, ts[0].tv_nsec,
	                            ts[1].tv_sec, ts[1].tv_nsec);
	syscall(314, str);
}

/* Usage:
	struct timespec t[2];

  // some code

	gettime(&(t[0]));

  // some code

	gettime(&(t[1]));
	print_result(1, t);
*/
