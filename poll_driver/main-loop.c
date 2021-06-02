#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include "cdata_ioctl.h"

struct cmd_struct *cmd_buf;
int main(int argc, char **argv)
{
	//file descriptor
	int fd;
	int rc=0;
	char *rd_buf[16];
	int ret;
	struct pollfd fds;
	printf("%s: entered\n", argv[0]);
	int event_id = 0;
	cmd_buf = malloc(sizeof(struct cmd_struct));
	//open the device
	fd=open("/dev/cdata", O_RDWR);
	if (fd==-1)
	{
		perror("open failed");
		rc=fd;
		exit(-1);
	}
	printf("%s: open: successful\n", argv[0]);

	fds.fd=fd;
	fds.events = POLLIN;

	while(1)
	{
	
		ret = poll(&fds,sizeof(fds),-1);

		if(ret==-1)
		{
			perror("poll failed\n");
			return -1;
		}
		printf("%s: event: %X\n", argv[0], fds.revents);
		if(fds.revents & POLLIN)
		{
			//Issue a read
			rc=read(fd, rd_buf,0);
			if(rc==-1)
			{
				perror("read failed");
				close(fd);
				exit(-1);
			}
			
			int ret_val = ioctl(fd, IOCTL_TX_CMD, cmd_buf);

  			if (ret_val < 0) {
    			printf ("ioctl_get_msg failed:%d\n", ret_val);
    			exit(-1);
  			}
			printf("%s: cmd event: %X\n", argv[0], cmd_buf->cmd);
			break;
		} 
		
	}//end of while

		close(fd);
		return 0;

}