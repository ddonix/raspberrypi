#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <stdio.h>
#include <alsa/asoundlib.h>

#include <unistd.h>  
#include <fcntl.h>  
#include <sys/stat.h>  
#include <sys/types.h>  

#define BUFFSIZE 1024
int main(int argc, char *argv[])
{
	void *shm = NULL;
	int shmid;
      	int fd_sour;			//
	
	fd_sour=open("./ljxq_cs.wav",O_RDONLY);
	if(fd_sour < 0) 
	{
		perror("open sour error\n");
        	return -1;
	}
	
	//创建共享内存  
	shmid = shmget ((key_t) 1234, BUFFSIZE, 0666 | IPC_CREAT);
	if (shmid == -1)
	{
		printf ("shmget failed\n");
		return -1;
	}
	//将共享内存连接到当前进程的地址空间  
	shm = shmat (shmid, (void *) 0, 0);
	if (shm == (void *) -1)
	{
		printf ("shmat failed\n");
		return -1;
	}
	printf ("Memory attached at %X\n", (int) shm);
	//设置共享内存  
	while (1)		//向共享内存中写数据  
	{
   		if (!read(fd_sour, shm, 1024))
			lseek(fd_sour, 0, SEEK_SET);
                delayMicroseconds(125000);
	}
	//把共享内存从当前进程中分离  
	if (shmdt (shm) == -1)
	{
		fprintf (stderr, "shmdt failed\n");
		exit (EXIT_FAILURE);
	}
	return 0;
}
