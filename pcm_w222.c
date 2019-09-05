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

int main(int argc, char *argv[])
{
	unlink(argv[1]);                 //如果明明管道存在，则先删除  
	mkfifo(argv[1],O_CREAT|0777);  
   	return 0;
}
