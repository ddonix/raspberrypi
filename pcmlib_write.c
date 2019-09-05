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

int initgpio_write()
{
	wiringPiSetup();
	pinMode(OLASER_PIN, OUTPUT);
	digitalWrite(OLASER_PIN, HIGH);
}

int writegpio_ls(char *buf, int len)
{
	int i, j, k, v;
	int time_cur, t;
	char c, d;

	time_cur = pcmdelay_f(1);
	for (i = 0; i < len; i++) {
		digitalWrite(OLASER_PIN, HIGH);
		pcmdelay_s(&time_cur, BITWIDTH*2);

		c = buf[i];
		d = 1;
		for (j = 0; j < 8; j++) {
			if (c & d) {
				digitalWrite(OLASER_PIN, LOW);
				pcmdelay_s(&time_cur, BITWIDTH/3);
				digitalWrite(OLASER_PIN, HIGH);
				pcmdelay_s(&time_cur, BITWIDTH*2/3);
			}
			else {
				digitalWrite(OLASER_PIN, HIGH);
				pcmdelay_s(&time_cur, BITWIDTH/3);
				digitalWrite(OLASER_PIN, LOW);
				pcmdelay_s(&time_cur, BITWIDTH*2/3);
			}
			d = d << 1;
		}
		digitalWrite(OLASER_PIN, LOW);
		pcmdelay_s(&time_cur, 5);
	}
	return len;
}

int writegpio_fz(char *buf, int len)
{
	int i, j, k, v;
	int time_cur, t;
	char c, d;

	digitalWrite(OLASER_PIN, HIGH);
	time_cur = pcmdelay_f(1);
	for (i = 0; i < len; i++) {
		digitalWrite(OLASER_PIN, LOW);
		pcmdelay_s(&time_cur, BITWIDTH*2);

		c = buf[i];
		d = 1;
		for (j = 0; j < 8; j++) {
			if (c & d) {
				digitalWrite(OLASER_PIN, HIGH);
				pcmdelay_s(&time_cur, BITWIDTH/3);
				digitalWrite(OLASER_PIN, LOW);
				pcmdelay_s(&time_cur, BITWIDTH*2/3);
			}
			else {
				digitalWrite(OLASER_PIN, LOW);
				pcmdelay_s(&time_cur, BITWIDTH/3);
				digitalWrite(OLASER_PIN, HIGH);
				pcmdelay_s(&time_cur, BITWIDTH*2/3);
			}
			d = d << 1;
		}
		digitalWrite(OLASER_PIN, HIGH);
		pcmdelay_s(&time_cur, 5);
	}
	digitalWrite(OLASER_PIN, HIGH);
	return len;
}
