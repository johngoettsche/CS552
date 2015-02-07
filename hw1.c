/*
 * John Goettsche
 * CS 552
 * hw1.c
 */

#include <inttypes.h>
#include <avr/io.h>

uint8_t notOn = 0xff;
uint8_t isOn = 0x00;
unsigned int second = 65000U;
unsigned int dly = 5000U;

void delay (unsigned int thisDly){
	volatile int i;
	for(i = thisDly; i != 0; i--);
}

int monitor (){
	uint8_t count = 0;
	PORTB = isOn;  /* to show that a button has been pressed */
	while(PIND != notOn){
		count++;
		delay(dly);
	}
	PORTB = notOn; /* turn off LEDs */
	return count;
}

void display(unsigned int count){
	volatile int i;
	PORTB = isOn; /* display */
	for(i = count; i != 0; i--){
		asm volatile(	"nop\n\t"
				"nop\n\t"
				"nop\n\t"
				"nop\n\t"
				);
		delay(dly);
	}
	PORTB = notOn; /* turn off LEDs */
}

int main(void){
	uint8_t pressTime = 0;
	DDRB = 0xff;  /* enable port B for output */
	DDRD = 0x00;  /* enable port D for input */
	PORTB = isOn; /* to show that the system is on */
	delay(second);
	PORTB = notOn; /* turn off LEDs */
	while(1){
		if(PIND != notOn){ /* button is pressed */
			pressTime = monitor();
			delay(second);
			display(pressTime);
		}
	}
	return(0);
}
