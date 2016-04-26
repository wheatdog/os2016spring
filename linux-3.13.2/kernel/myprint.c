#include <linux/linkage.h>
#include <linux/kernel.h>
asmlinkage void sys_myprint(char* arg)
{
	printk("%s", arg);
}
