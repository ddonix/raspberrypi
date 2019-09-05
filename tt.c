#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#define ILASER_PIN 3

int main (int argc, char *argv[])
{
	int fd_fifo;		//创建有名管道
	char buf[4+1024];
	int tag, tmp, i,j;
	int running = 1;	//程序是否继续运行的标志  

	wiringPiSetup();
	pinMode(ILASER_PIN, INPUT);
	pullUpDnControl(ILASER_PIN, PUD_DOWN);
	
	i = j = 1;
	while(1)
	{
		if (LOW == digitalRead(ILASER_PIN))
		{
			i++;
			printf("L %d\n",i);
		} else {
			j++;
			printf("H %d\n",j);
		}
		printf("H%d / L%d = %f\n", j, i, (double )j/(double)i);
	}
	return 0;
}
