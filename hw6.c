/*
 * John Goettsche
 * CS 552
 * hw5.c
 * size: 940
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

uint8_t notOn = 0xff;
uint8_t isOn = 0x00;

void init(){
	DDRB = 0xff;				//Set Port B as Output
	DDRD = 0x00;  				//Set Port D as Input
	//TCCR0 = (1<<CS02)|(1<<CS00);	//Timer clock = Sysclk/1024
	TCCR0 = (0<<CS02)|0<<CS01|(1<<CS00);	//clock 1 ms click
	TIFR = 1<<TOV0;				//Clear TOV0, any pending interupts
	TIMSK = 1<<TOIE0;			//Enable Timer0 Overflow interrupt
}

void delay(unsigned int thisDly){
	int i;
	for(i = 0; i < thisDly; i++);
}

void msDelay(unsigned int thisDly){
	TCNT0 = 0;
	while(TCNT0 < thisDly);
}

ISR(TIMER0_OVF_vect){
	flash();
}

void flash(){
	PORTB = isOn;
	delay(40000U);
	PORTB = notOn;
	delay(40000U);
}

int main(void){
	init();
	sei();
	PORTB = notOn;
	while(1){
		
	}
	return(0);
}
