#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define PAGE_SIZE 4096
#define BUF_SIZE 512


int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size, offset = 0, tmp;
	char file_name[50], method[20];
	char *kernel_address = NULL, *file_address = NULL;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed

	if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
	{
		perror("failed to open /dev/master_device\n");
		return 1;
	}

	char* address = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, dev_fd, 0);
	printf("addr: %p\n",address);
	if (address == MAP_FAILED)
	{
		perror("mmap operation failed");
		return -1;
	}
	sprintf(address, "mm success!!!\n");
	/*if(ioctl(dev_fd, 0x12345678) == -1){
		perror("mm error\n");
		return 1;
	}*/


	close(dev_fd);

	return 0;
}

