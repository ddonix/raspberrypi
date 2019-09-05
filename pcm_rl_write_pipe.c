#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>

#include <sys/shm.h>
#include "pcmlib.h"

int main (int argc, char *argv[])
{
	int fd_fifo;		//创建有名管道
	initgpio_read();
	char buffer[1024*10];

	if (access(argv[1], F_OK))
    		mkfifo(argv[1],O_CREAT|0666);  
	
	fd_fifo = open (argv[1], O_WRONLY);
	if (fd_fifo < 0) {
		perror ("open fifo error\n");
	  	return -1;
  	}
	
	while(1)
	{
		readgpio_ls(buffer, 1024*10);
		write (fd_fifo, buffer, 1024*10);
		printf("buffer\n");
	}
}
