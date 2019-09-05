#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <sys/shm.h>  

#include <pthread.h>
#include <unistd.h>  
#include <fcntl.h>  
#include <sys/stat.h>  
#include <sys/types.h>  
#include <string.h>  

int readstream(int fd_sour, char *buf)
{
	if (1024 != read(fd_sour, buf, 1024))
		lseek(fd_sour, 0, SEEK_SET);
        //delayMicroseconds(128000);
	return 0;
}

int openstream(char *name)
{
	int fd;
	fd = open(name, O_RDONLY);
	if (fd < 0)
		return fd;
	if (lseek(fd, 44, SEEK_SET) < 0)
	{
		close(fd);
		return -1;
	}
	return fd;
}

int closestream(int fd)
{
	return close(fd);
}

int main(int argc, char *argv[])
{
	int fd_sour;
	int shmid;
	void *shm = NULL;
        char buf[4+1024];
    
	fd_sour=openstream(argv[1]);
	if(fd_sour < 0) 
	{
		perror("open sour error\n");
        	return -1;
	}
	
	//创建共享内存  
	shmid = shmget ((key_t) 1234, 4+1024, 0666 | IPC_CREAT);
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
   	
	*(long*)shm = 0;
	memset(shm+4, 0, 1024);
	while (1)		//向共享内存中写数据  
	{
		readstream(fd_sour, shm+4);
   		(*(long*)shm)++;
		printf("%d\n", *(long*)shm);
	}
	//把共享内存从当前进程中分离  
	if (shmdt (shm) == -1)
	{
		printf ("shmdt failed\n");
		return -1;
	}
	closestream(fd_sour);
   	return 0;
}
