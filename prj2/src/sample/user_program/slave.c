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
int main (int argc, char* argv[])
{
	char buf[PAGE_SIZE];
	int offset = 0;
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size = 0, data_size = -1;
	char file_name[50];
	char method[20];
	char ip[20];
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;

	strcpy(file_name, argv[1]);
	strcpy(method, argv[2]);
	strcpy(ip, argv[3]);

	if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}
	if( (file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
	{
		perror("failed to open input file\n");
		return 1;
	}

	if(ioctl(dev_fd, 0x12345677, ip) == -1)	//0x12345677 : connect to master in the device
	{
		perror("ioclt create slave socket error\n");
		return 1;
	}



	gettimeofday(&start ,NULL);
	switch(method[0])
	{
		case 'f'://fcntl : read()/write()
			do
			{
				ret = read(dev_fd, buf, sizeof(buf)); // read from the the device
				write(file_fd, buf, ret); //write to the input file
				file_size += ret;
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
				int len = ioctl(dev_fd, 0x12345678);
				if(len == -1) { //0x12345678 : sent data 
					perror("ioclt server krecive\n");
					return 1;
				}

				if (len == 0) break;

                                lseek(file_fd, len - 1, SEEK_END);
				write(file_fd, "0", 1);
				file_address = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, offset);
				if (file_address == MAP_FAILED){
   					perror("mmap operation failed or finish!");
  					break;
   				}

				memcpy(file_address, kernel_address, len);

				munmap(file_address, len);
				offset += len;
				file_size += len;
			}while(1);
			break;
	}



	if(ioctl(dev_fd, 0x12345679) == -1)// end receiving data, close the connection
	{
		perror("ioclt client exits error\n");
		return 1;
	}

	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.001;
	printf("Transmission time: %lf ms, File size: %d bytes\n", trans_time, file_size);


	close(file_fd);
	close(dev_fd);
	return 0;
}


