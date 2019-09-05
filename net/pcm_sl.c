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
	for (i = 0; i < 1461; i++)
		read(fd_sour, music + i * 1024, 1024);
	close(fd_sour);


	initgpio_write();
	
	while (1) {
		writegpio_ls(music, 1461*1024);
		sleep(1);
	}
	return 0;
}
