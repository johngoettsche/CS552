/*
 * John Goettsche
 * CS 552
 * hw4.c
 * size: ?
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

uint8_t notOn = 0xff;
uint8_t isOn = 0x00;

//uint8_t colFilter = 0x74;		// 01110100
uint8_t colFilter = 0x74;		// 01110100
uint8_t pina;
uint8_t pinaClear;
uint8_t col[4] = {0x04, 0x10, 0x20, 0x40};
uint8_t row[4] = {0x80, 0x08, 0x02, 0x01};

void init(){
	DDRB = 0xff;			//Set Port B as Output
	PORTB = notOn;
	DDRA = colFilter; 		//Set Columns to output and
					//Rows to input in Port 
					//C for the keypad

	PORTA = ~colFilter;		//Set the Columns to high 
					//in Port C
	pinaClear = PINA;
}

void correct(){
	PORTB = isOn;
}

void delay (unsigned int thisDly){
	volatile int i;
	for(i = thisDly; i != 0; i--);
}

uint8_t getKey(){
	int c, r;
	uint8_t key = notOn;
	while(PINA == pinaClear){
	}
	delay(30U);
	for(c = 0; c < 4; c++){
		PORTA = ~col[c];
		pina = PINA;
		for(r = 0; r < 4; r++){
			if(PINA == (pina & ~row[r])) key = row[r] + col[c];
		}
	}
	PORTA = ~colFilter;
	while (PINA != pinaClear){
	}
	delay(30U);
	return key;
}

int getCode(){
	int k;
	int check = 0;
	uint8_t code[4] = {0x04 + 0x80, 0x10 + 0x80, 0x20 + 0x80, 0x40 + 0x80};
//uint8_t row[4] = {0x80, 0x08, 0x02, 0x01};
	uint8_t keys[4];
	PORTB = notOn;
	for(k = 0; k < 4; k++){
		keys[k] = getKey();
		PORTB = PORTB << 1;
		if(keys[k] == code[k]) check++;
	}
	if (check == 4) return 1;
	else return 0;
}

int main(void){
	int success;
	init();
	while(1){
		success = getCode();	
		if(success) {
			correct();	
			delay(65000U);
		}
	}
	return(0);
}
