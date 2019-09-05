#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#define OLED_PIN 2
#define OLB_PIN  3

int p(int x, int y)
{
	int i,j;
	j = x*y/10;
	printf("%d %d %d\n", j, y, j*(500000/y));
	for(i=0; i < j; i++)
	{
		digitalWrite(OLB_PIN, HIGH);
        	digitalWrite(OLED_PIN, HIGH);
                delayMicroseconds(500000/y);
        	
		digitalWrite(OLB_PIN, LOW);
        	digitalWrite(OLED_PIN, LOW);
                delayMicroseconds(500000/y);
	}
}

int main(int argc ,char *argv[])
{
	wiringPiSetup();
    
	pinMode(OLED_PIN, OUTPUT);
	pinMode(OLB_PIN, OUTPUT);

	uint32_t i,j;

	int t[25]={212,212,190,212,159,169,212,212,190,212,142,159, 212,212,106,126,159,169,190,119,119,126,159,142,159};
	int l[25]={9,3,12,12,12,24,9,3,12,12,12,24, 9,3,12,12,12,12,12,9,3,12,12,12,24};
	for (i = 0; i < 25;i ++)
		p(l[i]/7, t[i]*6);
	for (i = 0; i < 25;i ++)
		p(l[i]/7, t[i]*6);
	
	digitalWrite(OLED_PIN, LOW);
        digitalWrite(OLB_PIN, LOW);
	return 0;
}
