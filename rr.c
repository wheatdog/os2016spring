#include "printtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#define TIME_QUANTUM 500
#define STR_SIZE 50

struct p_struct {
	char name[STR_SIZE];
	int r_time;
	int e_time;
	struct timespec ts[2];
	int pid;
	int remain_time;
	int fd;
};
struct ready_list {
	struct p_struct* process;
	struct ready_list *next, *last;
}; 
	

static int cmp(const void* a,const void* b){
	struct p_struct* pa = (struct p_struct*)a;
	struct p_struct* pb = (struct p_struct*)b;
	if(pa->r_time < pb->r_time){
		return -1;
	}else if(pa->r_time > pb->r_time){
		return 1;
	}else{
		return 0;
	}
}
static int min(int a, int b){
	return (a < b)? a:b;
}
static void fork_child(struct p_struct *p){
	int fd[2];
	pipe(fd);
	fcntl(fd[0], F_SETFD, FD_CLOEXEC);
	fcntl(fd[1], F_SETFD, FD_CLOEXEC);
	if ((p->pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	if (p->pid == 0) {
		dup2(fd[0], STDIN_FILENO);

		char arg[STR_SIZE] = {0};
		sprintf(arg, "%d", p->e_time);
		execl("./child", "child", p->name, arg, (char*)0);
		fprintf(stderr, "exec error\n");
		exit(2);
	}else{
		close(fd[0]);
		p->fd = fd[1];
	}
}
static void wait_for_job(int time){
	while(time > 0){
		{ volatile unsigned long i; for(i=0;i<1000000UL;i++); }
		time--;
	}
}
static void run_job(struct p_struct *process, int time){
	if(time <= 0){
		return;
	}
	if(process->remain_time == process->e_time){
		gettime(&process->ts[0]);
	}
	char str[STR_SIZE] = {0};
	sprintf(str, "%12d ", time);
	write(process->fd, str, 13);
	struct sched_param paramforchild;
	paramforchild.sched_priority = 99;
	if(sched_setscheduler(process->pid, SCHED_FIFO, &paramforchild) < 0 ){
		fprintf(stderr, "set sched err\n");
		exit(3);
	}
}
static struct ready_list* add_list(struct ready_list *job, struct p_struct *p){
	struct ready_list *new_job = malloc(sizeof(struct ready_list));
	new_job->process = p;
	if(job == NULL){
		job = new_job;
		job->last = job->next = new_job;
	}else{
		new_job->next = job;
		new_job->last = job->last;
		job->last->next = new_job;
		job->last = new_job;
	}
	return job;
}
static struct ready_list* remove_list(struct ready_list *job){
	fprintf(stderr, "die pid:%d n:%s\n", job->process->pid, job->process->name);
	if(job->next == job){
		free(job);
		return NULL;
	}
	job->next->last = job->last;
	job->last->next = job->next;
	struct ready_list* next_job = job->next;
	free(job);
	return next_job;
}

int RR(){
	struct sched_param param;
	param.sched_priority = 98;
	if(sched_setscheduler(0, SCHED_FIFO, &param) < 0 ){
		fprintf(stderr, "set sched err\n");
		exit(3);
	}

	int n;
	scanf("%d", &n);
	struct p_struct *p_arr = malloc(n*sizeof(struct p_struct));

	int i;
	for(i = 0; i < n; i++){
		scanf("%s%d%d", p_arr[i].name, &p_arr[i].r_time, &p_arr[i].e_time);
		p_arr[i].remain_time = p_arr[i].e_time;
	}
	qsort(p_arr, n, sizeof(struct p_struct), cmp);

	int finish = 0, next = 0, cur_time = 0, remain = TIME_QUANTUM;
	struct ready_list* cur_job = NULL;
	
	while(finish < n){
		fprintf(stderr, "f = %d %d\n", finish, n);
		if(next < n && p_arr[next].r_time == cur_time){
			fork_child(&(p_arr[next]));
			fprintf(stderr, "\tt:%d fork %d %s\n", cur_time, p_arr[next].pid, p_arr[next].name);
			cur_job = add_list(cur_job, &p_arr[next]);
			next++;
		}
		if(cur_job == NULL){
			fprintf(stderr, "t:%d idle %d\n", cur_time, p_arr[next].r_time - cur_time);
			wait_for_job(p_arr[next].r_time - cur_time);
			cur_time = p_arr[next].r_time;
			remain = TIME_QUANTUM;
			continue;
		}
		if(next < n &&
		   p_arr[next].r_time - cur_time < remain &&
		   p_arr[next].r_time - cur_time < cur_job->process->remain_time){
			int t = p_arr[next].r_time - cur_time;
			fprintf(stderr, "to fork ");
			fprintf(stderr, "t:%d pid:%d n:%s run:%d r:%d\n", cur_time, cur_job->process->pid, cur_job->process->name, t, cur_job->process->remain_time);
			run_job(cur_job->process, t);

			remain -= t;
			cur_job->process->remain_time -= t;		
			cur_time = p_arr[next].r_time;
			continue;
		}
		if(remain < cur_job->process->remain_time){
			fprintf(stderr, "to next ");
			fprintf(stderr, "t:%d pid:%d n:%s run:%d r:%d\n", cur_time, cur_job->process->pid, cur_job->process->name, remain, cur_job->process->remain_time);
			run_job(cur_job->process, remain);

			cur_job->process->remain_time -= remain;
			cur_time += remain;
			remain = TIME_QUANTUM;
			cur_job = cur_job->next;
		}else{
			fprintf(stderr, "to die ");
			fprintf(stderr, "t:%d pid:%d n:%s run=r:%d\n", cur_time, cur_job->process->pid, cur_job->process->name, cur_job->process->remain_time);
			run_job(cur_job->process, cur_job->process->remain_time);
			wait(NULL);
			gettime(&(cur_job->process->ts[1]));
			print_result(cur_job->process->pid, cur_job->process->ts);

			cur_time += cur_job->process->remain_time;
			remain = TIME_QUANTUM;
			cur_job = remove_list(cur_job);
			finish++;
		}
	}
	free(p_arr);
	return 0;
}
