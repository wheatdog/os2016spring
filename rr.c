#include "msg.h"
#include <stdio.h>
#include <stdlib.h>

#define TIME_QUANTUM 500

struct p_struct {
	char name[32];
	int r_time;
	int e_time;
	struct timespec ts[2];
	int pid;
	int remain_time;
};
struct ready_list {
	struct p_struct* process;
	struct ready_list* next, last;
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
int main(){
	int n;
	scanf("%d", &n);
	struct p_struct *p_arr = malloc(n*sizeof(p_struct));

	int i;
	for(i = 0; i < n; i++){
		scanf("%s%d%d", p_arr[i].name, p_arr[i].r_time, p_arr[i].e_time);
		remain_time = e_time;
	}
	qsort(p_arr, n, sizeof(p_struct), cmp);

	int finish = 0, next = 0, cur_time = 0, remain = 0;
	struct ready_list* cur_job = NULL;
	
	while(finish != n){
		if(next < n && p_arr[next].r_time == cur_time){
			fork_child(p_arr[next]);
			add_list(cur_job, &p_arr[next]);
			next++;
		}
		if(cur_job == NULL){
			wait_for_job(p_arr[name].r_time - cur_time);
			cur_time = p_arr[name].r_time;
			remain = TIME_QUANTUM;
			continue;
		}
		if(next < n &&
		   p_arr[next].r_time - cur_time < remain &&
		   p_arr[next].r_time - cur_time < cur_job->process->remain_time){
			int t = p_arr[next].r_time - cur_time;
			run_job(t);

			remain -= t;
			cur_job->process->remain_time -= t;		
			cur_time = p_arr[next].r_time;
			continue;
		}
		if(remain < cur_job->process->remain_time){
			run_job(remain);

			cur_job->process->remain_time -= remain;
			cur_time += remain;
			remain = TIME_QUANTUM;
			cur_job = cur_job->next;
		else{
			run_job(cur_job->process->remain_time);

			cur_time += cur_job->process->remain_time;
			remain = TIME_QUANTUM;
			remove_list(cur_job);
		}
	}
	free(p_arr);
	return 0;
}
