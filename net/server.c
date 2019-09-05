#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <netdb.h>  

int port = 6789;
int main(int argc, char **argv)
{
	int socket_descriptor;	//套接口描述字  
	struct sockaddr_in address;	//处理网络通信的地址  
	
	int fd_sour, i;
	char c;
	char *music;
	char buf[1024];

	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");	//这里不一样  
	address.sin_port = htons(port);

	//创建一个 UDP socket  

	socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);	//IPV4  SOCK_DGRAM 数据报套接字（UDP协议）

	fd_sour = open("./ljxq_cs.wav", O_RDONLY);
	if (fd_sour < 0) {
		perror("open sour error\n");
		return -1;
	}
	music = (char *)malloc(1461 * 1024);
	if (0 == music) {
		perror("malloc error\n");
		close(fd_sour);
		return -1;
	}
	lseek(fd_sour, 44, SEEK_SET);
	read(fd_sour, music, 1024*1461);
	close(fd_sour);
	
	i = 0;
	while(1)
	{
		sendto(socket_descriptor, music+i*1024, 1024, 0, (struct sockaddr *)&address, sizeof(address));
		usleep(128000);
		i++;
		if(1461 == i)
			i = 0;
	}
	close(socket_descriptor);
	exit(0);
	return (EXIT_SUCCESS);
}
