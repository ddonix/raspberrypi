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
	int t1,t2;
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

	size = frames * datablock;	/*4 代表数据快长度 */
	buffer = (char *) malloc(size);
	initgpio_read();
	while (1) {
		readgpio_ls(buffer, 1024);
		
		// 写音频数据到PCM设备

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
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
	return 0;
}
