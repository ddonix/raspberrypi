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

	if (argc != 2) {
		printf("Usage:wav-player+wav file name\n");
		exit(1);
	}

	int nread;
	int fd_fifo;	//打开有名管道，用于读取PCM数据
        fd_fifo=open(argv[1],O_RDONLY);  
        if(fd_fifo < 0) {
		printf("open pipe %s failed.\n", argv[1]);
		exit(1);
	}

	printf("声道数：%d\n", 1);
	printf("采样频率：%d\n", 8000);
	printf("采样的位数 :%d\n", 8);
	printf("采样字节数 :%d\n",8000);


	set_pcm_play(fd_fifo);
	return 0;
}

int set_pcm_play(int fd_fifo)
{
	int rc;
	int ret;
	int size;
	snd_pcm_t *handle;	//PCI设备句柄
	snd_pcm_hw_params_t *params;	//硬件信息和PCM流配置
	unsigned int val;
	int dir = 0;
	snd_pcm_uframes_t frames;
	char *buffer;
	int channels = 1;
	int frequency = 8000;
	int bit = 8;
	int datablock = 1;
	unsigned char ch[100];	//用来存储wav文件的头信息



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
	switch (bit / 8) {
	case 1:
		snd_pcm_hw_params_set_format(handle, params,
					     SND_PCM_FORMAT_U8);
		break;
	case 2:
		snd_pcm_hw_params_set_format(handle, params,
					     SND_PCM_FORMAT_S16_LE);
		break;
	case 3:
		snd_pcm_hw_params_set_format(handle, params,
					     SND_PCM_FORMAT_S24_LE);
		break;

	}
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

	rc = snd_pcm_hw_params_get_period_size(params, &frames, &dir);	/*获取周期
									   长度 */
	if (rc < 0) {
		perror("\nsnd_pcm_hw_params_get_period_size:");
		exit(1);
	}

	size = frames * datablock;	/*4 代表数据快长度 */

	buffer = (char *) malloc(size);

	while (1) {
		memset(buffer, 0, sizeof(buffer));
		ret = read(fd_fifo, buffer, size);
		if (ret == 0) {
			printf("歌曲播放结束\n");
			break;
		} else if (ret != size) {
		}
		// 写音频数据到PCM设备 
		while (ret = snd_pcm_writei(handle, buffer, frames) < 0) {
			usleep(2000);
			if (ret == -EPIPE) {
				/* EPIPE means underrun */
				fprintf(stderr, "underrun occurred\n");
				//完成硬件参数设置，使设备准备好 
				snd_pcm_prepare(handle);
			} else if (ret < 0) {
				fprintf(stderr,
					"error from writei: %s\n",
					snd_strerror(ret));
			}
		}

	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
	return 0;
}
