#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>

int main(int argc, char** argv)
{
	
	pid_t pid = getpid(); 
	int execution_time = atoi(argv[2]);
	int x;
	printf("%s %d\n", argv[1], pid);
	while (execution_time > 0) {
		scanf("%d", &x);
		for (int j = x; j > 0; j--) {
			volatile unsigned long i; 
			for(i = 0; i < 1000000UL; i++);
			execution_time--;
		}

		if (execution_time > 0) {
			struct sched_param param;
			param.sched_priority = 97;
			sched_setscheduler(0, SCHED_FIFO, & param);
		}
	}
	return 0;
}
