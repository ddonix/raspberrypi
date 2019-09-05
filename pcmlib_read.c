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
#include <assert.h>

#include <sys/shm.h>
#include "pcmlib.h"

int inline pcmdelay_s(int *c, int t)
{
	int tt;
	while (1) {
		tt = micros();
		if (tt < *c+t)
			continue;
		else
		{	
			*c += t;
			return tt;
		}
	}
}

int inline pcmdelay_f(int t)
{
	int tt,c;
	c = micros();
	while (1) {
		tt = micros();
		if (tt < c+t)
			continue;
		else
			return tt;
	}
}


int initgpio_read(void)
{
	wiringPiSetup();
	pinMode(ILASER_PIN, INPUT);
	pullUpDnControl(ILASER_PIN, PUD_DOWN);
	return 0;
}

int readgpio_ls(char *buf, int len)
{
	int time_cur, tag, t;
	int i,j;
	char c, d;

	tag = 0;
	for (i = 0; i < len; i++) {
		while (1) {
			while(1) {
				if (LOW == digitalRead(ILASER_PIN))
					break;
			}
			while(1) {
				if (HIGH == digitalRead(ILASER_PIN))
					break;
			}
			time_cur = micros();
			while(1) {
				if (LOW == digitalRead(ILASER_PIN))
					break;
				t = micros();
				if (t < time_cur + BITWIDTH*5/3)
				{
					continue;
				}
				else
				{
					tag = 1;
					break;
				}
			}
			if (1 == tag)
				break;
		}
		c = 0;
		d = 1;
		for (j = 0; j < 8; j++) {
			pcmdelay_f(BITWIDTH); 

			if (HIGH == digitalRead(ILASER_PIN)) {
				c |= d;
			} else {
			}
			d = d << 1;
		}
		buf[i] = c;
		tag = 0;
	}
	return 0;
}
