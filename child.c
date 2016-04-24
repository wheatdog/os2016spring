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
	while (1) {
		int ret = scanf("%d", &x);
		if(ret == EOF || ret == 0){
			fprintf(stderr, "children%d: %s read EOF or error\n", pid, argv[1]);
			exit(2);
		}
		for (int j = x; j > 0; j--) {
			volatile unsigned long i; 
			for(i = 0; i < 1000000UL; i++);
			execution_time--;
		}

		if (execution_time <= 0) 
			exit(0);

		struct sched_param param;
		param.sched_priority = 97;
		if(sched_setscheduler(0, SCHED_FIFO, & param) < 0){
			fprintf(stderr, "children%d: %s set sched err\n", pid, argv[1]);
			exit(1);
		}
	}
	return 0;
}
