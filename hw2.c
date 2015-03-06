/*
 * John Goettsche
 * CS 552
 * hw2.c
 * program size: 535
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

uint8_t notOn = 0xff;
uint8_t isOn = 0x00;
uint8_t count = 0;
uint8_t second = 4;

void init(){
	DDRB = 0xFF;			//Set Port B as Output
	DDRD = 0x00;  			//Set Port D as Input
	TCCR0 = (1<<CS02)|(1<<CS00);	//Timer clock = Sysclk/1024
	TIFR = 1<<TOV0;			//Clear TOV0, any pending interupts
	TIMSK = 1<<TOIE0;		//Enable Timer0 Overflow interrupt
}

//void interrupt [TIMER0_OVF_vect] ISR_TOV0 (void){
ISR(TIMER0_OVF_vect){
	count++;
}

void delay (uint8_t timeDly){
	count = 0;
	while((timeDly - count) > 0){
	}
}

void working(){
	PORTB = isOn; /* to show that the system is on */
	delay(second / 2);
	PORTB = notOn; /* turn off LEDs */
	delay(second / 4);
	PORTB = isOn; /* to show that the system is on */
	delay(second / 2);
	PORTB = notOn; /* turn off LEDs */
	delay(second);
}

int monitor (){
	count = 0;
	PORTB = isOn;  /* to show that a button has been pressed */
	while(PIND != notOn){
	}
	PORTB = notOn; /* turn off LEDs */
	return count;
}

void display(uint8_t timeCount){
	count = 0;
	PORTB = isOn; /* display */
	while((timeCount - count) > 0){
	}
	PORTB = notOn; /* turn off LEDs */
}

int main(void){
	init();
	sei();
	uint8_t pressTime = 0;
	working(); //all systems go signal
	while(1){
		if(PIND != notOn){ /* button is pressed */
			count = 0;
			pressTime = monitor();
			delay(second);
			display(pressTime);
		}
	}
	return(0);
}
