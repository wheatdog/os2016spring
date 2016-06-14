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
size_t get_filesize(const char* filename);//get the size of the input file


int main (int argc, char* argv[])
{
	char buf[PAGE_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size, remain, offset = 0, size, tmp;
	char file_name[50], method[20];
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	char *file_address = NULL, *kernel_address = NULL;



	strcpy(file_name, argv[1]);
	strcpy(method, argv[2]);


	if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
	{
		perror("failed to open /dev/master_device\n");
		return 1;
	}
	if( (file_fd = open (file_name, O_RDWR)) < 0 )
	{
		perror("failed to open input file\n");
		return 1;
	}

	if( (file_size = get_filesize(file_name)) < 0)
	{
		perror("failed to get filesize\n");
		return 1;
	}
	remain = file_size;


	if(ioctl(dev_fd, 0x12345677) == -1) //0x12345677 : create socket and accept the connection from the slave
	{
		perror("ioclt server create socket error\n");
		return 1;
	}

	gettimeofday(&start ,NULL);
	switch(method[0])
	{
		case 'f': //fcntl : read()/write()
			do
			{
				ret = read(file_fd, buf, sizeof(buf)); // read from the input file
				write(dev_fd, buf, ret);//write to the the device
			}while(ret > 0);
			break;
		case 'm':
			kernel_address = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, dev_fd, 0);
			if (kernel_address == MAP_FAILED){
				perror("mmap operation failed");
				return -1;
			}
			do
			{
				size = (remain < PAGE_SIZE)? remain:PAGE_SIZE;
				file_address = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, offset);
				offset += size;
				remain -= size;
				if (file_address == MAP_FAILED){
        				perror("mmap operation failed");
        				return -1;
    				}
				memcpy(kernel_address, file_address, size);
				if(ioctl(dev_fd, 0x12345678, size) == -1) //0x12345678 : mmap send message
				{
					perror("mmap send message error\n");
					return 1;
				}
				munmap(file_address, size);
			}while(remain > 0);
			break;
	}

	if(ioctl(dev_fd, 0x12345679) == -1) // end sending data, close the connection
	{
		perror("ioclt server exits error\n");
		return 1;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.001;
	printf("Transmission time: %lf ms, File size: %d bytes\n", trans_time, file_size);

	close(file_fd);
	close(dev_fd);

	return 0;
}

size_t get_filesize(const char* filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}
