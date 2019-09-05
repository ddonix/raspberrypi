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

int p(int a)
{/*
    char buf[100];
    while(1)  
    {  
        printf("please input you cmd:");  
        fflush(stdout);  
        fgets(buf,sizeof(buf),stdin);       //从标准输入获取数据  
        buf[strlen(buf)]='\0';  
        printf("*%s*\n",buf);             
        if(write(fd_fifo,buf,strlen(buf))!=strlen(buf))  
            perror("write");                //将命令写入命名管道
    }
*/	return 0;
}  
int main(int argc, char *argv[])
{
	int fd_fifo;                     //创建有名管道，用于写PCM数据到管道
      	int fd_sour;
        char buf[1024];
	int i;
	fd_fifo=open("/tmp/pcm_fifo",O_WRONLY);  
	if(fd_fifo < 0) 
	{ 
		perror("open fifo error\n");  
        	return -1;
	}
        fd_sour=open("./ljxq_cs.wav",O_RDONLY);  
	if(fd_sour < 0) 
	{ 
		perror("open sour error\n");  
        	return -1;
	}
	i=0;
	while(1)
	{
   		if (!read(fd_sour, buf, 1024))
			break;
		write(fd_fifo, buf, 1024);
		printf("%d \n", i++);
	}
	close(fd_fifo);
	close(fd_sour);
	return 0;

	     
   	return 0;
}
