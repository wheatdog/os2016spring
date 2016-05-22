#include <linux/time.h>

asmlinkage void sys_gettime(struct timespec *ts){
	getnstimeofday(ts);
}
