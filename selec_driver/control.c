#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "cdata_ioctl.h"

int main(int argc, char **argv)
{
	//file descriptor
	int fd;
	int rc=0;
	char *rd_buf[16];
	int ret;
	unsigned int event_id = atoi(argv[1]);
	printf("%s: %d entered\n", argv[0],event_id);

	//open the device
	fd=open("/dev/cdata", O_RDWR);
	if (fd==-1)
	{
		perror("open failed");
		rc=fd;
		exit(-1);
	}
	printf("%s: open: successful\n", argv[0]);

	//send ioctl
	ret=ioctl(fd,IOCTL_ENABLE, event_id);
	if(ret < 0)
	{
		printf("ioctl_write failed:%d\n", ret);
	}
	

	close(fd);
	return 0;

}