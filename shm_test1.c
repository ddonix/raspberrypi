#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/socket.h>

#define SEM_KEY     100
#define SHM_KEY     101
#define TOTAL_SEM   10
#define UNIX_DOMAIN "/tmp/unix_sock"

typedef struct shared_use {
	int pktlen;
	unsigned char pkt[1528];
} shared_use_t;

union semun {
	int val;		/*   value   for   SETVAL   */
	struct semid_ds *buf;	/*   buffer   for   IPC_STAT,   IPC_SET   */
	unsigned short *array;	/*   array   for   GETALL,   SETALL   */
	/*   Linux   specific   part:   */
	struct seminfo *__buf;	/*   buffer   for   IPC_INFO   */
};

static unsigned char ucast_pkt[] = {
	0x00, 0xd0, 0xf8, 0x00, 0x99, 0x22, 0x00, 0x00,
	0x00, 0x01, 0x01, 0x07, 0x08, 0x06, 0x00, 0x01,
	0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xbc, 0x30,
	0x00, 0x00, 0x00, 0x01, 0x01, 0x07, 0xc0, 0xa8,
	0xcc, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xc0, 0xa8, 0xcc, 0x64, 0x01, 0x02, 0x03, 0x04,
	0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
	0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14
};

static int sem_id = 0;
static int com_fd = 0;
static void *shm = NULL;

			/* 获取信号量资源 */
static int semaphore_v(void)
{
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = 1;
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1) {
		return -1;
	}

	return 0;
}

							    /* 释放信号量资源 */
static int semaphore_p(void)
{
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = -1;
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1) {
		return -1;
	}

	return 0;
}

												/* 初始化信号量 */
static int semaphore_init(void)
{
	union semun sem_union;

	sem_id = semget((key_t) SEM_KEY, 1, 0666 | IPC_CREAT);
	if (sem_id == -1) {
		return -1;
	}

	sem_union.val = 1;
	if (semctl(sem_id, 0, SETVAL, sem_union) == -1) {
		return -1;
	}

	return 0;
}

																		/* 初始化共享内存 */
static int sharemmy_init(void)
{
	int shmid;

	/* 创建共享内存区域块 */
	shmid = shmget((key_t) SHM_KEY, sizeof(shared_use_t), 0666 | IPC_CREAT);
	if (shmid == -1) {
		return shmid;
	}

	/* 将本进程的虚拟内存映射到共享内存 */
	shm = shmat(shmid, (void *)0, 0);
	if (shm == (void *)-1) {
		return -1;
	}

	return shmid;
}

static int sharemmy_destroy(int shmid, void *shm)
{
	int rv;

	/* 删除本进程与共享内存的映射 */
	rv = shmdt(shm);
	if (rv == -1) {
		return rv;
	}

	/* 删除创建的共享内存块 */
	rv = shmctl(shmid, IPC_RMID, 0);
	if (rv == -1) {
		return rv;
	}

	return 0;
}

static int domain_server(void)
{
	int rv;
	int listen_fd;
	socklen_t len;
	struct sockaddr_un srv_un;
	struct sockaddr_un clt_un;

	listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("socket: ");
		return -1;
	}

	memset(&srv_un, 0, sizeof(srv_un));
	srv_un.sun_family = AF_UNIX;
	strncpy(srv_un.sun_path, UNIX_DOMAIN, sizeof(srv_un.sun_path) - 1);
	unlink(UNIX_DOMAIN);
	rv = bind(listen_fd, (struct sockaddr *)&srv_un, sizeof(srv_un));
	if (rv < 0) {
		perror("bind: ");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
		return -1;
	}

	rv = listen(listen_fd, 1);
	if (rv < 0) {
		perror("listen: ");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
		return -1;
	}

	len = sizeof(clt_un);
	while (1) {
		com_fd = accept(listen_fd, (struct sockaddr *)&clt_un, &len);
		if (errno == EINTR && com_fd == -1) {
			continue;
		}
		break;
	}

	return 0;
}

static void print_cmd_help(void)
{
	printf("CAMMAND NOT SUPPORT!.\n");
	printf("********** Help Info **********\n");
	printf("  1. Send -------- send a packet to commad.\n");
	printf("  2. Quit -------- quit the program.\n");
	printf("**********    End    **********\n");
}

static int shmwrite(void *shm)
{
	int rv;
	shared_use_t *shared;

	rv = semaphore_p();
	if (rv < 0) {
		return -1;
	}

	shared = (shared_use_t *) shm;
	shared->pktlen = sizeof(ucast_pkt);
	memcpy(shared->pkt, ucast_pkt, sizeof(ucast_pkt));

	rv = semaphore_v();
	if (rv < 0) {
		return -1;
	}

	return 0;
}

int main()
{
	int rv;
	int shmid;
	char cmd[10];

	rv = semaphore_init();
	if (rv < 0) {
		printf("Init a semaphore fail.\n");
		return 0;
	}

	shmid = sharemmy_init();
	if (shmid == -1) {
		printf("Init a share memory fail.\n");
		return 0;
	}

	rv = domain_server();
	if (rv < 0) {
		printf("Create domain server socket fail.\n");
		return 0;
	}
	printf("Server init success!\n");
	printf("\n");

	while (1) {
		memset(cmd, 0, sizeof(cmd));
		printf("Enter a command:");
		scanf("%s", cmd);
		if (cmd[9] != 0) {
			printf
			    ("Error : Command is too long, at most 9 char allowed!!!\n");
			continue;
		} else if (strstr(cmd, "send") != NULL
			   || strstr(cmd, "Send") != NULL) {
			rv = shmwrite(shm);
			if (rv < 0) {
				printf("Write packet to share memory fail.\n");
				break;
			}
			rv = send(com_fd, cmd, sizeof(cmd), 0);
			if (rv < 0) {
				printf("Send command send fail.\n");
				continue;
			}
		} else if (strstr(cmd, "quit") != NULL
			   || strstr(cmd, "Quit") != NULL) {
			printf
			    ("**********Stop Send Packet To Client.***********\n");
			rv = send(com_fd, cmd, sizeof(cmd), 0);
			if (rv < 0) {
				printf("Quit commadn send fail.\n");
			}
			break;
		} else {
			print_cmd_help();
		}
		printf("\n");
	}

	usleep(1000);
	rv = sharemmy_destroy(shmid, shm);
	if (rv < 0) {
		printf("Destroy share memory fail.\n");
		return 0;
	}

	return 1;
}
