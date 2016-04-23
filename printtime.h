#include <stdio.h>
#include <sys/time.h>
#include <sys/syscall.h>
#define gettime(A) syscall(315, A)
void print_result(int pid, struct timespec ts[2]){
	char str[100];
	sprintf(str, "[Project1] %d %ld.%09ld %ld.%09ld\n", pid,
	                            ts[0].tv_sec, ts[0].tv_nsec,
	                            ts[1].tv_sec, ts[1].tv_nsec);
	syscall(314, str);
}
/*int main(){
	struct timespec t[2];
	gettime(&(t[0]));
	gettime(&(t[1]));
	//syscall(315, &(t[0]));
	//syscall(315, &(t[1]));
	print_result(1, t);
	return 0;
}*/
