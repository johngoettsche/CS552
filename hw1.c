/*
 * John Goettsche
 * CS 552
 * hw1.c
 */

#include <inttypes.h>
#include <avr/io.h>

void delay (unsigned int dly){
	int i;
	for(i = dly; i != 0; i--);
}

int main(void){
	uint8_t count = 0;
	uint8_t notOn = 0xff;
	uint8_t isOn = 0x00;
	unsigned int dly = 1000U;
	unsigned int adjust = 500U;
	uint8_t i;
	DDRB = 0xff;  /* enable port B for output */
	DDRD = 0x00;  /* enable port D for input */
	while(1){
		if(PIND != notOn){
			while(PIND != notOn){
				count++;
				delay(dly);
			}
			delay(65000U);
			PORTB = isOn;
			for(i = count; i != 0; i--){
				delay(dly);
			}
			PORTB = notOn;
		}
	}
	return(0);
}
