#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include "pcmlib.h"

int writegpio_ls(const char *buffer, int len)
{
	int fd;
	int i,j;
	fd = open("/dev/dlaser", O_WRONLY);
	if (fd < 0)
		return -1;
	for(i = 0; i < len/1024;i++)
		write(fd, buffer+i*1024, 1024);
	return 0;
		
}

int main(int argc, char *argv[])
{
	int fd_sour, i;
	char c;
	char *music;
	pthread_t tid1;

	fd_sour = open("./ljxq_cs.wav", O_RDONLY);
	if (fd_sour < 0) {
		perror("open sour error\n");
		return -1;
	}
	music = (char *)malloc(1461*1024);
	if (0 == music) {
		perror("malloc error\n");
		close(fd_sour);
		return -1;
	}
	lseek(fd_sour, 44, SEEK_SET);
	read(fd_sour, music, 1461*1024);
	close(fd_sour);

	while (1) {
		if (writegpio_fz(music, 1461*1024))
		{
			printf("激光发射失败.\n");
			return -1;
		}
		sleep(1);
	}
	return 0;
}
