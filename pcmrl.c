#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

#include <pthread.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include "pcmlib.h"

char *buffer = 0;
static int fd_dev = -1;
unsigned char music[1024*1461];

int initsour(void)
{
	fd_dev = open("/dev/dlaser", O_RDONLY);
	if (fd_dev < 0) {
		perror("open driver error\n");
		return -1;
	}
	return 0;
}

	
int playframe(snd_pcm_t *handle, unsigned char *buffer, snd_pcm_uframes_t frames)
{
	int ret;
	while (1){
		ret = snd_pcm_writei(handle, buffer, frames);
		
		if (ret >= 0)
			break;
		usleep(2000);
		if (ret == -EPIPE) {
			fprintf(stderr, "underrun occurred\n");
			//完成硬件参数设置，使设备准备好 
			snd_pcm_prepare(handle);
		} else if (ret < 0) {
			fprintf(stderr, "error from writei: %s\n", snd_strerror(ret));
		}
	}
	return 0;
}


int main (int argc, char *argv[])
{
	int rc;
	int ret;
	int size;
	snd_pcm_hw_params_t *params;	//硬件信息和PCM流配置
	snd_pcm_t *handle = 0;	//PCI设备句柄
	snd_pcm_uframes_t frames;
	unsigned int val;
	
	
	int dir = 0;
	int channels = 1;
	int frequency = 8000;
	int bit = 8;
	int datablock = 1;
	unsigned char ch[100];	//用来存储wav文件的头信息
	pthread_attr_t attr;	// 线程属性
	struct sched_param sched;	// 调度策略
	int i,j,k,jj1,jj2,t1,t2;
	pthread_t tid1;
	printf("carete getmuisc\n");
	
	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy (&attr, SCHED_RR);

	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		perror("\nopen PCM device failed:");
		exit(1);
	}

	snd_pcm_hw_params_alloca(&params);	//分配params结构体
	if (rc < 0) {
		perror("\nsnd_pcm_hw_params_alloca:");
		exit(1);
	}
	rc = snd_pcm_hw_params_any(handle, params);	//初始化params
	if (rc < 0) {
		perror("\nsnd_pcm_hw_params_any:");
		exit(1);
	}
	rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);	//初始化访问权限
	if (rc < 0) {
		perror("\nsed_pcm_hw_set_access:");
		exit(1);
	}
	//采样位数
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
	
	rc = snd_pcm_hw_params_set_channels(handle, params, channels);	//设置声道,1表示单声>道，2表示立体声
	if (rc < 0) {
		perror("\nsnd_pcm_hw_params_set_channels:");
		exit(1);
	}
	val = frequency;
	rc = snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);	//设置>频率
	if (rc < 0) {
		perror("\nsnd_pcm_hw_params_set_rate_near:");
		exit(1);
	}

	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		perror("\nsnd_pcm_hw_params: ");
		exit(1);
	}

	rc = snd_pcm_hw_params_get_period_size(params, &frames, &dir);	/*获取周期长度 */
	if (rc < 0) {
		perror("\nsnd_pcm_hw_params_get_period_size:");
		exit(1);
	}
	snd_pcm_nonblock(handle, SND_PCM_NONBLOCK);

	size = frames * datablock *100;	/*4 代表数据快长度 */
	buffer = (char *) malloc(size);
	initsour();
	initgpio_read();
	switch(argv[1][0])
	{
		case 't':
		read(fd_dev, buffer, 10);
		for(i = 0; i < 10; i++)
			printf("%x ", buffer[i]);
		printf("\n");
		break;
		
		case 'e':
		readgpio_ls(buffer, 10);
		for(i = 0; i < 10; i++)
			printf("%x ", buffer[i]);
		printf("\n");
		break;
		
		case 'y':
		jj1 = jj2 = 0;
		for(i = 0; i < 100;i++) 
		{
			read(fd_dev, buffer, 1024);
			for(j = 1; j < 1023; j++)
			{
				if(buffer[j] == 0xaa) {
					if((buffer[j-1] != 0xac) || (buffer[j+1] != 0xab))
						jj2++;
				}
				else if(buffer[j] == 0xab) {
					if((buffer[j-1] != 0xaa) || (buffer[j+1] != 0xac))
						jj2++;
				}
				else if(buffer[j] == 0xac) {
					if((buffer[j-1] != 0xab) || (buffer[j+1] != 0xaa))
						jj2++;
				}
				else
					jj1++;
			}
		}
		printf("read /dev/dlaser 102400 error %d %d %d\n", jj1+jj2, jj1,jj2);
		break;
		
		case 'r':
		jj1 = jj2 = 0;
		for(i = 0; i < 10;i++) 
		{
			readgpio_ls(buffer, 1024);
		}
		printf("readgpio_ls 10240 error %d %d %d\n", jj1+jj2, jj1,jj2);
		break;
	
		case 'b':
		for(i = 0; i<200;i++) {
			read(fd_dev, buffer, 1024);
			memcpy(music+i*1024, buffer, 1024);
		}
		for(i = 0; i<200;i++)
			playframe(handle, music+i*1024, frames);
		break;

		case 'w':
		for(i = 0; i<500;i++) {
			readgpio_ls(buffer, 1024);
			playframe(handle, buffer, frames);
		}
		break;
		
		case 'd':
		for(i = 0; i<500;i++) {
			read(fd_dev, buffer, 1024);
			playframe(handle, buffer, frames);
		}
		break;
		
		case 'l':
			read(fd_dev, buffer, 1024);
		break;
		
		case 'i':
		t1=micros();
		while(1)
		{
			if (HIGH == digitalRead(ILASER_PIN)) {
				t2=micros();
				printf("read high %d %d\n",t1,t2);
				break;
			}
			pcmdelay_f(1);
		}
		break;
		
		case 'o':
		t1=micros();
		while(1)
		{
			if (LOW == digitalRead(ILASER_PIN)) {
				t2=micros();
				printf("read LOW %d %d\n",t1,t2);
				break;
			}
//			pcmdelay_f(1);
		}
		break;


		default:
		break;
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
	close(fd_dev);
	return 0;
}
