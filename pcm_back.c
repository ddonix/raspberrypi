#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>


#define IR_PIN 		1
#define OLED_PIN 	2
#define OLB_PIN  	3
#define IMP_PIN  	4

int h,l;
int mp(void)
{
	int i,j;
	for(i=0; i < 10000000; i++)
	{
        	if(HIGH == digitalRead(IMP_PIN))
		{
			h++;
			digitalWrite(OLB_PIN, HIGH);
			digitalWrite(OLED_PIN, HIGH);
                	delayMicroseconds(8000);
			printf("HIGH %d\n",h);
		}
		else
		{
			l++;
			digitalWrite(OLB_PIN, LOW);
			digitalWrite(OLED_PIN, LOW);
                	delayMicroseconds(8000);
			printf("LOW %d\n",l);
		}
	}
}

int main(int argc ,char *argv[])
{
	wiringPiSetup();
    
	pinMode(OLED_PIN, OUTPUT);
	pinMode(OLB_PIN, OUTPUT);
	
	pinMode(IMP_PIN, INPUT);
    	pullUpDnControl(IMP_PIN, PUD_DOWN);

	uint32_t i,j;
	h = l = 0;
        delayMicroseconds(100000);
	mp();
	digitalWrite(OLED_PIN, LOW);
        digitalWrite(OLB_PIN, LOW);
	return 0;
}
