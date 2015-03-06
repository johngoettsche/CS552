/*
 * John Goettsche
 * CS 552
 * hw3.c
 * size: 782
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

uint8_t notOn = 0xff;
uint8_t isOn = 0x00;
uint8_t setOn = 0x0f;
uint8_t setOff = 0x00;
uint8_t pressSet;
uint8_t second = 4;    		// one second
uint8_t clk = 0;		// clock timer
unsigned int timer = 0U;	// keeps track of how long button was pressed
unsigned int count = 0U;

void init(){
	DDRB = 0xff;			//Set Port B as Output
	DDRD = 0x00;  			//Set Port D as Input
	TCCR0 = (1<<CS02)|(1<<CS00);	//Timer clock = Sysclk/1024
	TIFR = 1<<TOV0;			//Clear TOV0, any pending interupts
	TIMSK = 1<<TOIE0;		//Enable Timer0 Overflow interrupt
}

//void interrupt [TIMER0_OVF_vect] ISR_TOV0 (void){
ISR(TIMER0_OVF_vect){
	count++;
	timer++;
	if(count % second == 0) {
		clk = clk + 0x10;
		if(clk > 0xf0) clk = 0x00;
		PORTB = ~(clk + pressSet); 
	}
}

void delay (unsigned int timeDly){
	timer = 0U;
	while((timeDly - timer) > 0){
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
	timer = 0;
	pressSet = setOn;
	PORTB = ~(clk + pressSet);  /* to show that a button has been pressed */
	while(PIND != notOn){
	}
	pressSet = setOff;
	PORTB = ~(clk + pressSet); /* turn off LEDs */
	return timer;
}

void display(unsigned int timeCount){
	timer = 0U;
	pressSet = setOn;
	PORTB = ~(clk + pressSet); /* display */
	while((timeCount - timer) > 0){
	}
	pressSet = setOff;
	PORTB = ~(clk + pressSet); /* turn off LEDs */
}

int main(void){
	init();
	sei();
	pressSet = setOff;
	clk = 0;
	unsigned int pressTime = 0U;
	working(); //all systems go signal
	while(1){
		if(PIND != notOn){ /* button is pressed */
			timer = 0;
			pressTime = monitor();
			delay(second);
			display(pressTime);
		}
	}
	return(0);
}
