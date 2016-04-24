#include "printtime.h"
#include "sjf.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#define MAX_BUF 100

typedef struct {
	pid_t pid;
	int fd[2];
	char name[32];
	int ready_time;
	int execution_time;
	struct timespec time[2];
	int flag;
}DATA;

static int cmp(const void* a, const void* b) {
	DATA* aa = (DATA*)a;
	DATA* bb = (DATA*)b;
	if (aa->ready_time > bb-> ready_time)
		return 1;
	else if (aa->ready_time < bb-> ready_time)
		return -1;
	else 
		return 0;
}

static int SJFcmp(const void* a, const void* b) {
	DATA* aa = (DATA*)a;
	DATA* bb = (DATA*)b;
	if (aa->execution_time > bb->execution_time)
		return 1;
	else if (aa->execution_time < bb->execution_time)
		return -1;
	else 
		return 0;
}

static void readFile(int N, DATA* scheduler) {
	int i;
	for (i = 0; i < N; i++) {
		scanf("%s%d%d", scheduler[i].name, &scheduler[i].ready_time, &scheduler[i].execution_time);
		pipe(scheduler[i].fd);
		fcntl(scheduler[i].fd[0], F_SETFD, FD_CLOEXEC);
		fcntl(scheduler[i].fd[1], F_SETFD, FD_CLOEXEC);
		scheduler[i].flag = 1;
	}
}

static void executeFork(DATA* scheduler, int* nextforfork, int clock, int N) {
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

static int decideTime(DATA* scheduler, int N, int nextforfork, int Index, int clock) {
	if (nextforfork == N)
		return scheduler[Index].execution_time;

	int time1 = scheduler[nextforfork].ready_time - clock;
	int time2 = scheduler[Index].execution_time;
	return ((time1 < time2)? time1 : time2);
}

static void waitLoop(DATA* scheduler, int N, int nextforfork, int Index, int* clock) {
	while (nextforfork < N && Index == nextforfork && *clock < scheduler[nextforfork].ready_time) {
		volatile unsigned long i; 
		for(i = 0; i<1000000UL ; i++);
		*clock = (*clock) + 1;
	}
}

static void assign(DATA* scheduler, int N, int nextforfork, int *Index, int* clock) {
	int time = decideTime(scheduler, N, nextforfork, *Index, *clock);
	char buffer[MAX_BUF] = {0};	
	sprintf(buffer, "%d ", time);
	write(scheduler[*Index].fd[1], buffer, strlen(buffer));
	struct sched_param paramforchild;
	paramforchild.sched_priority = 99;
	if (sched_setscheduler(scheduler[*Index].pid, SCHED_FIFO, &paramforchild)) {
	fprintf(stderr, "sched_setscheduler error\n");
	exit(3);
}

	*clock = (*clock) + time;
	scheduler[*Index].execution_time -= time;

	if (scheduler[*Index].execution_time == 0) {
		int temp;
		wait(&temp);
		gettime(&scheduler[*Index].time[1]);
		*Index = (*Index) + 1;
	}
}

int SJF() 
{
	struct sched_param param;
	param.sched_priority = 98;
	sched_setscheduler(0, SCHED_FIFO, & param);
	int N;
	scanf("%d", &N);
	DATA* scheduler = (DATA*)malloc(N * sizeof(DATA));
	readFile(N, scheduler);
	qsort(scheduler, N, sizeof(DATA), cmp);

	int i;
	int clock = 0, Index = 0, nextforfork = 0;
	while (Index < N) {
		waitLoop(scheduler, N, nextforfork, Index, &clock);
		executeFork(scheduler, &nextforfork, clock, N);
		
		if (scheduler[Index].flag) {
			qsort(&scheduler[Index], nextforfork - Index, sizeof(DATA), SJFcmp);
			gettime(&scheduler[Index].time[0]);
			scheduler[Index].flag = 0;
		}
		else 
			qsort(&scheduler[Index + 1], nextforfork - Index - 1, sizeof(DATA), SJFcmp);
		
		assign(scheduler, N, nextforfork, &Index, &clock);
	}

	for (i = 0; i < N; i++) {
		print_result(scheduler[i].pid, scheduler[i].time);
	}
	free(scheduler);
	return 0;
}
