#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <sys/shm.h>

int main (int argc, char *argv[])
{
	int fd_fifo;		//创建有名管道
	char buf[1024];
	int tag, tmp;
	int running = 1;	//程序是否继续运行的标志  
	void *shm = NULL;	//分配的共享内存的原始首地址  
	int shmid;		//共享内存标识符  
	//创建共享内存  
	shmid = shmget ((key_t) 1234, 4+1024, 0666 | IPC_CREAT);
	if (shmid == -1)
	{
		printf ("shmget failed\n");
		return -1;
	}
	//将共享内存连接到当前进程的地址空间  
	shm = shmat (shmid, 0, 0);
	if (shm == (void *) -1)
	{
		printf ("shmat failed\n");
		return -1;
	}
	printf ("\nMemory attached at %X\n", (int) shm);
	//设置共享内存

	if (access(argv[1], F_OK))
    		mkfifo(argv[1],O_CREAT|0666);  
	
	fd_fifo = open (argv[1], O_WRONLY);
	if (fd_fifo < 0) {
		perror ("open fifo error\n");
	  	return -1;
  	}
	tag = *((long *)shm);
	printf("tag begin %d\n", tag); 
	while (1)
	{
		if (*((long *)shm) == tag)
                	delayMicroseconds(10);
		else {

			tag = *((long *)shm);
			printf("%d\n", tag);
	  		memcpy(buf, shm+4, 1024);
			write (fd_fifo, buf, 1024);
		}
	}

	close (fd_fifo);

	//把共享内存从当前进程中分离  
	if (shmdt (shm) == -1)
	{
		printf ("shmdt failed\n");
		return -1;
	}
	//删除共享内存  
	if (shmctl (shmid, IPC_RMID, 0) == -1)
	{
		printf ("shmctl(IPC_RMID) failed\n");
		return -1;
	}
	return 0;
}
