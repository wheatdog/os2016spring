#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "printtime.h"
#define MAX_BUF 100

typedef struct fifo {
	pid_t pid;
	int fd[2];
	char name[32];
	int ready_time;
	int execution_time;
	struct timespec time[2];
	int flag;
}FIFO;

int cmp(const void* a, const void* b) {
	FIFO* aa = (FIFO*)a;
	FIFO* bb = (FIFO*)b;
	if (aa->ready_time > bb-> ready_time)
		return 1;
	else if (aa->ready_time < bb-> ready_time)
		return -1;
	else 
		return 0;
}

void readFile(int N, FIFO* fifo_scheduler) {
	int i;
	for (i = 0; i < N; i++) {
		scanf("%s%d%d", fifo_scheduler[i].name, &fifo_scheduler[i].ready_time, &fifo_scheduler[i].execution_time);
		pipe(fifo_scheduler[i].fd);
		fcntl(fifo_scheduler[i].fd[0], F_SETFD, FD_CLOEXEC);
		fcntl(fifo_scheduler[i].fd[1], F_SETFD, FD_CLOEXEC);
		fifo_scheduler[i].flag = 1;
	}
}

void executeFork(FIFO* scheduler, int* nextforfork, int clock, int N) {
	while (*nextforfork < N && clock == scheduler[*nextforfork].ready_time) {
		if ((scheduler[*nextforfork].pid = fork()) < 0) {
			fprintf(stderr, "fork error\n");
			exit(1);
		}

		if (scheduler[*nextforfork].pid == 0) {
			char buffer[MAX_BUF] = {0};
			sprintf(buffer, "%d", scheduler[*nextforfork].execution_time);
			dup2(scheduler[*nextforfork].fd[0], STDIN_FILENO);

			if (execl("./child", "child", scheduler[*nextforfork].name, buffer, (char*)0) < 0) {
				fprintf( stderr, "exec error\n");
				exit(2);
			}
		}

		if (scheduler[*nextforfork].pid > 0) {
			close(scheduler[*nextforfork].fd[0]);
			*nextforfork = *nextforfork + 1;
		}
	}
}

int decideTime(FIFO* scheduler, int N, int nextforfork, int Index, int clock) {
	if (nextforfork == N)
		return scheduler[Index].execution_time;

	int time1 = scheduler[nextforfork].ready_time - clock;
	int time2 = scheduler[Index].execution_time;
	return ((time1 < time2)? time1 : time2);
}

int main() 
{
	struct sched_param param;
	param.sched_priority = 98;
	sched_setscheduler(0, SCHED_FIFO, & param);
	int i;
	int N;
	scanf("%d", &N);
	FIFO* fifo_scheduler = (FIFO*)malloc(N * sizeof(FIFO));
	readFile(N, fifo_scheduler);
	qsort(fifo_scheduler, N, sizeof(FIFO), cmp);
	int clock = 0;
	int Index = 0;
	int nextforfork = 0;
	while (Index < N) {
		while (nextforfork < N && Index == nextforfork && clock < fifo_scheduler[nextforfork].ready_time) {
			volatile unsigned long i; 
			for(i = 0; i<1000000UL ; i++);
			clock++;
		}

		executeFork(fifo_scheduler, &nextforfork, clock, N);
		
		if (fifo_scheduler[Index].flag) {
			gettime(&fifo_scheduler[Index].time[0]);
			fifo_scheduler[Index].flag = 0;
		}

		int time = decideTime(fifo_scheduler, N, nextforfork, Index, clock);
		char buffer[MAX_BUF] = {0};
		sprintf(buffer, "%d", time);
		write(fifo_scheduler[Index].fd[1], buffer, MAX_BUF);
		struct sched_param paramforchild;
		paramforchild.sched_priority = 99;
		sched_setscheduler(fifo_scheduler[Index].pid, SCHED_FIFO, &paramforchild);

		clock += time;
		fifo_scheduler[Index].execution_time -= time;

		if (fifo_scheduler[Index].execution_time == 0) {
			int temp;
			wait(&temp);
			gettime(&fifo_scheduler[Index].time[1]);
			Index++;
		}
		
	}

	for (i = 0; i < N; i++) {
		print_result(fifo_scheduler[i].pid, fifo_scheduler[i].time);
	}
	return 0;
}
