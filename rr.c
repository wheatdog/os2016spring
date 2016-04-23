#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include "printtime.h"

#define TIME_QUANTUM 500
#define STR_SIZE 30

struct p_struct {
	char name[32];
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
	

int cmp(const void* a,const void* b){
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
int min(int a, int b){
	return (a < b)? a:b;
}
void fork_child(struct p_struct *p){
	printf("\tfork %d %s\n", p->pid, p->name);
	if ((p->pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	int fd[2];
	pipe(fd);
	fcntl(fd[0], F_SETFD, FD_CLOEXEC);
	fcntl(fd[1], F_SETFD, FD_CLOEXEC);

	if (p->pid == 0) {
		dup2(fd[0], STDIN_FILENO);

		char arg[STR_SIZE] = {0};
		sprintf(arg, "%d", p->e_time);
		execl("./child", "child", p->name, arg, (char*)0);
		fprintf(stderr, "exec error\n");
		exit(2);
	}

	if (p->pid > 0) {
		close(fd[0]);
		p->fd = fd[1];
	}
}
void wait_for_job(int time){
	printf("idle %d\n", time);
	while(time > 0){
		{ volatile unsigned long i; for(i=0;i<1000000UL;i++); }
		time--;
	}
}
void run_job(struct p_struct *process, int time){
	printf("pid:%d n:%s run:%d\n", process->pid, process->name, time);
	char str[STR_SIZE] = {0};
	sprintf(str, "%d", time);
	write(process->fd, str, STR_SIZE);
	struct sched_param paramforchild;
	paramforchild.sched_priority = 99;
	sched_setscheduler(process->pid, SCHED_FIFO, &paramforchild);
}
struct ready_list* add_list(struct ready_list *job, struct p_struct *p){
	struct ready_list *new_job = malloc(sizeof(struct ready_list));
	new_job->process = p;
	if(job == NULL){
		job = job->last = job->next = new_job;
	}else{
		new_job->next = job;
		new_job->last = job->last;
		job->last->next = new_job;
		job->last = new_job;
	}
	return job;
}
struct ready_list* remove_list(struct ready_list *job){
	printf("die pid:%d n:%s\n", job->process->pid, job->process->name);
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
int main(){
	char type[20];
	scanf("%s", type);
	struct sched_param param;
	param.sched_priority = 98;
	sched_setscheduler(0, SCHED_FIFO, &param);

	int n;
	scanf("%d", &n);
	struct p_struct *p_arr = malloc(n*sizeof(struct p_struct));

	int i;
	for(i = 0; i < n; i++){
		scanf("%s%d%d", p_arr[i].name, &p_arr[i].r_time, &p_arr[i].e_time);
		p_arr[i].remain_time = p_arr[i].e_time;
	}
	qsort(p_arr, n, sizeof(struct p_struct), cmp);

	int finish = 0, next = 0, cur_time = 0, remain = 0;
	struct ready_list* cur_job = NULL;
	
	while(finish < n){
		if(next < n && p_arr[next].r_time == cur_time){
			fork_child(&p_arr[next]);
			cur_job = add_list(cur_job, &p_arr[next]);
			next++;
		}
		if(cur_job == NULL){
			wait_for_job(p_arr[next].r_time - cur_time);
			cur_time = p_arr[next].r_time;
			remain = TIME_QUANTUM;
			continue;
		}
		if(cur_job->process->remain_time == cur_job->process->e_time){
			gettime(&cur_job->process->ts[0]);
		}
		if(next < n &&
		   p_arr[next].r_time - cur_time < remain &&
		   p_arr[next].r_time - cur_time < cur_job->process->remain_time){
			int t = p_arr[next].r_time - cur_time;
			run_job(cur_job->process, t);

			remain -= t;
			cur_job->process->remain_time -= t;		
			cur_time = p_arr[next].r_time;
			continue;
		}
		if(remain < cur_job->process->remain_time){
			run_job(cur_job->process, remain);

			cur_job->process->remain_time -= remain;
			cur_time += remain;
			remain = TIME_QUANTUM;
			cur_job = cur_job->next;
		}else{
			run_job(cur_job->process, cur_job->process->remain_time);
			wait(NULL);
			gettime(&cur_job->process->ts[1]);
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
