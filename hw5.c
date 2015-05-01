/*
 * John Goettsche
 * CS 552
 * hw5.c
 * size: 940
 */

#include <inttypes.h>
#include <avr/io.h>
//#include <avr/interrupt.h>

void writeInst(uint8_t inst);

uint8_t notOn = 0xff;
uint8_t isOn = 0x00;
uint8_t allOut = 0xff;

char line1[8] = {'H', 'e', 'l', 'l', 'o', ' ', ' ', ' '};
char line2[8] = {'W', 'o', 'r', 'l', 'd', '!', ' ', ' '};

void init(){
	DDRB = allOut;				//Set Port B as Output to LED
	PORTB = notOn;

	DDRA = allOut;	 			//Set Port A as Output to Control
	DDRD = allOut;	 			//Set Port D as Output to Display

	TCCR0 = (0<<CS02)|0<<CS01|(1<<CS00);	//clock 1 ms click

	writeInst(0x38);			//Function Set
	writeInst(0x0f);			//Turn on display
	writeInst(0x01);			//Clear display
	writeInst(0x02);			//Carage Return
}

void delay(unsigned int thisDly){
	volatile int i, j;
	for(i = thisDly; i != 0; i--)
		for (j = 0; j < 65; j++);
}

void msDelay(unsigned int thisDly){
	TCNT0 = 0;
	while(TCNT0 < thisDly);
}

void ready(){
	PORTB = isOn;
	delay(100U);
	PORTB = notOn;
	delay(100U);
	PORTB = isOn;
	delay(100U);
	PORTB = notOn;
}

void checkBusyFlag(){
	PORTA = 0x02;
	msDelay(1U);
	PORTA = 0x06;
	msDelay(1U);
	PORTA = 0x07;
	msDelay(1U);
	while(PORTD >= 0x80){
		PORTA = 0x02;
		msDelay(1U);
		PORTA = 0x06;
		msDelay(1U);
		PORTA = 0x07;
		msDelay(1U);
	}
}

void writeInst(uint8_t inst){
	//checkBusyFlag();
	PORTA = 0x00;
	msDelay(1U);
	PORTA = 0x01;
	msDelay(1U);
	PORTD = inst;
	msDelay(1U);
	PORTA = 0x00;
	msDelay(100U);	
}

void writeData(char data){
	//checkBusyFlag();
	PORTA = 0x02;
	msDelay(1U);
	PORTA = 0x03;
	msDelay(1U);
	PORTD = data;
	msDelay(1U);
	PORTA = 0x02;
	msDelay(100U);	
}

int main(void){
	int c;
	init();
	ready();
	//while(1){
		writeInst(0x80); 		//set DD RAM first line first cell
		for(c = 0; c < 8; c++)
			writeData(line1[c]);
		writeInst(0xc0);		//set DD RAM second line first cell
		for(c = 0; c < 8; c++)
			writeData(line2[c]);
	//}
	return(0);
}
