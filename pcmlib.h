#ifndef _PCH_H_
#define _PCM_H_
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

#include <sys/shm.h>

#define ILASER_PIN 3
#define OLASER_PIN 4

#define BITWIDTH   12

int pcmdelay_s(int *c, int t);
int pcmdelay_ss(int *c, int t, int m);
int pcmdelay_f(int t);

int initgpio_read(void); 

int readgpio_ls(char *buf, int len);

#endif
