/*
 * John Goettsche
 * CS 552
 * UIK.c
 * size: ?
 */

#include "stdlib.h"
#include "stddef.h"
#include "string.h"
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//#define EXT_INT0 PINB
#define TICK 50
#define MAXTASK 10

#define DISPLED 4
#define DISPLCD 5
#define FLASH 6
#define COUNTER 2
#define READKEY 3
#define IDLE 9

#define RUNNING 101
#define BLOCKED 102
#define READY 103

//structs
typedef union arg{
	char *str;
	int i;
}arg;

typedef struct list{
	uint8_t max;
	int size;
	struct task *head;
	struct task *tail;
}list;

typedef struct task{
	struct task *next;
	struct task *prev;
	uint8_t priority;
	int nargs;
	arg *args;
	int status;
	uint16_t funcPtr;
}task;

//Initialization
void Initialize();
void initTimer0();
void initTimer1();
void initLCD();
void initKeypad();
//void initTasks(){

//Scheduler
task *AddTask(void *functionPtr, uint8_t priority, int nargs, arg *args);
void Scheduler();
void Run(task *t);

//Queue
void createReadyQueue();
void addToQueue(list *q, task *newTask, uint8_t priority);
void priorityAdjust(list *q);
void removeFromList(list *l, task *t);
task *front(list *q);
void pop(list *q);
int inList(list *l, task *t);

//delays
void delay(unsigned int thisDly);
void msDelay(unsigned int thisDly);

//LCD functions
void writeInst(uint8_t inst);
void writeData(char data);

//keypad functions
uint8_t getKey();
//char *getValue(uint8_t key);
void addTaskCode(uint8_t key);

//Tasks
void DisplayLCD();
void DisplayLED();
void Flash();
void Counter();
void ReadKeyPad();
void Idle();

uint8_t notOn = 0xff;
uint8_t isOn = 0x00;
uint8_t allOut = 0xff;

void (*functionPtr)();
task *displayLCD;
task *displayLED;
task *flash;
task *counter;
task *readKeyPad;
task *idle;

list *ReadyQueue;
task *running;

uint8_t colFilter = 0x74;			// 01110100
uint8_t pina;

int semaphore = 1;
uint16_t tick = 0;

void Initialize(){
	//initTimer0();
	initTimer1();

	DDRB = allOut;				//Set Port A as Output
	PORTB = notOn;				//turn off lights

	//void initLCD();
	initKeypad();

	createReadyQueue();
	initTasks();
	addToQueue(ReadyQueue, idle, IDLE);
	sei();
}

/*void initTimer0(){
	TCCR0 = (1<<CS02)|(1<<CS00);		//Timer clock = Sysclk/1024
	//TCCR0 = (0<<CS02)|0<<CS01|(1<<CS00);	//clock 1 ms click
	TIFR = 1<<TOV0;				//Clear TOV0, any pending interupts
	TIMSK |= (1<<TOIE0);			//Enable Timer0 Overflow interrupt
}*/

void initTimer1(){
	TCCR1B |= (1 << WGM12)|(1 << CS12)|(1 << CS10);
						// set up timer with prescaler = 1024 and CTC mode
	TCNT1 = 0;				// initialize counter
	OCR1A = TICK;				// initialize compare value
	TIMSK |= (1 << OCIE1A);			// enable compare interrupt
}
/*
void initLCD(){
	DDRC = allOut;	 			//Set Port C as Output to LCD Control
	DDRD = allOut;	 			//Set Port D as Output to LCD Display

	writeInst(0x38);			//Function Set
	writeInst(0x0f);			//Turn on display
	writeInst(0x01);			//Clear display
	writeInst(0x02);			//Carage Return
}*/

void initKeypad(){
	DDRA = colFilter; 			//Set Columns to output and
						//Rows to input in Port 
						//B for the keypad
	PORTA = ~colFilter;			//Set the Columns to high in Port B
	pina = PINA;				//capture empty keypad
}

void initTasks(){
	arg *args;

	displayLCD = AddTask(&DisplayLCD, DISPLCD, 1, args);
	displayLCD->args[0].str = (char *)calloc(20, sizeof(char));
	displayLED = AddTask(&DisplayLED, DISPLED, 1, args);
	displayLED->args[0].i = isOn;
	flash = AddTask(&Flash, FLASH, 1, args);
	flash->args[0].i = 0;
	counter = AddTask(&Counter, COUNTER, 1, args);
	counter->args[0].i = 0;
	readKeyPad = AddTask(&ReadKeyPad, READKEY, 0, args);
	idle = AddTask(&Idle, 9, 0, args);
}

//interupts
/*ISR(TIMER0_OVF_vect){
	if(counting) addToQueue(ReadyQueue, counter, COUNTER);
	if(flash && flashCount <= tick) {
		flash = 0;
		displayLED->args[0].i = notOn;
		addToQueue(ReadyQueue, displayLED, DISPLED);
	}
	sei();
}*/

ISR(TIMER1_COMPA_vect){
	tick++;
	if(counter->status == RUNNING && tick % 100) addToQueue(ReadyQueue, counter, COUNTER);
	if(flash->status == RUNNING) addToQueue(ReadyQueue, flash, FLASH);
	//if(tick % 5 == 0)addToQueue(ReadyQueue, readKeyPad, READKEY);
	if(PINA != pina){
		uint8_t key = getKey();
		PORTB = ~key;
		addTaskCode(key);
		key = 0;
	}

	PORTB = 0x0f & PORTB;
	PORTB = ~(ReadyQueue->head->priority << 4) | PORTB;

	Scheduler();
	/*priorityAdjust(ReadyQueue);
	if(running->priority < 10){
		running->priority++;
	}*/
	TCNT1 = 0;
}

//Semaphore
void TakeSemaphore(){
	semaphore--;
}

void ReleaseSemaphore(){
	semaphore++;
}

task *AddTask(void *functionPtr, uint8_t priority, int nargs, arg *args){
	task *newTask = (task *)calloc(1, sizeof(task));
	newTask->next = NULL;
	newTask->prev = NULL;
	newTask->priority = priority;
	newTask->nargs = nargs;
	newTask->status = BLOCKED;
	if(nargs > 0) newTask->args = (arg *)calloc(nargs, sizeof(arg));
	newTask->funcPtr = (uint16_t)functionPtr;
	return newTask;
}

//Scheduler
void Scheduler(){
	if(front(ReadyQueue) != NULL){
		if(running->priority >= front(ReadyQueue)->priority){
			if(semaphore){
/*  swithch out the data */
				Run(front(ReadyQueue));
				pop(ReadyQueue);
			}
		}
	}
}


void Run(task *t){
	t->status = RUNNING;
	running = t;
	functionPtr = (void *)running->funcPtr;
	(*functionPtr)();
}

//Queue Procedures
void createReadyQueue(){
	ReadyQueue = (list*)calloc(1, sizeof(list));
	ReadyQueue->max = MAXTASK;
	ReadyQueue->size = 0;
}

void addToQueue(list *q, task *newTask, uint8_t priority){
	if(!inList(q, newTask)){
		newTask->priority = priority;
		newTask->status = READY;
		newTask->next = NULL;
		newTask->prev = NULL;
		if (q->size == 0){
			q->head = newTask;
			q->tail = newTask;
			q->size++;
		} else {
			task *current = q->tail;
			int place = 0;
			while (current != NULL && place == 0){
				if(newTask->priority < current->priority){
					current = current->prev;
				}		
				newTask->prev = current;
				newTask->next = current->next;
				current->next = newTask;
				q->size++;
				place = 1;
				
			}
		}
	}
}

void priorityAdjust(list *q){
	task *current = ReadyQueue->head;
	while(current != NULL){
		if (current->priority > 1) current->priority--;
		current = current->next;
	}
}

void removeFromList(list *l, task *t){
	task *current = l->head;
	while(current != t && current != NULL) current = current->next;
	if(current != NULL){
		if(current->prev != NULL) current->prev->next = current->next;
		if(current->next != NULL) current->next->prev = current->prev;
	}
	l->size--;
}

task *front(list *q){
	return q->head;
}

void pop(list *q){
	if(q->head != NULL)q->head = q->head->next;
	q->size--;
}

int inList(list *l, task *t){
	task *current = l->head;
	while(current != NULL){
		if(current == t) return 1;
		current = current->next;
	}
	return 0;
}

//delay functions

void delay(unsigned int thisDly){
	volatile int i;
	for(i = 0; i < thisDly; i++);
}

void msDelay(unsigned int thisDly){
	TCNT0 = 0;
	while(TCNT0 < thisDly);
}

//LCD functions
void writeInst(uint8_t inst){
	PORTC = 0x00;
	msDelay(1U);
	PORTC = 0x01;
	msDelay(1U);
	PORTD = inst;
	msDelay(1U);
	PORTC = 0x00;
	msDelay(100U);	
}

void writeData(char data){
	PORTC = 0x02;
	msDelay(1U);
	PORTC = 0x03;
	msDelay(1U);
	PORTD = data;
	msDelay(1U);
	PORTC = 0x02;
	msDelay(100U);	
}

//keypad functions

uint8_t getKey(){
	uint8_t col[4] = {0x04, 0x10, 0x20, 0x40};
	uint8_t row[4] = {0x80, 0x08, 0x02, 0x01};
	int c, r;
	uint8_t key = notOn;
	//msDelay(30U);
	for(c = 0; c < 4; c++){
		PORTA = ~col[c];
		uint8_t pin = PINA;
		for(r = 0; r < 4; r++){
			if(PINA == (pin & ~row[r])) key = row[r] + col[c];
		}
	}
	PORTA = ~colFilter;
	while(PINA != pina);
	return key;
}

/*
char *getValue(uint8_t key){
	switch(key){
		case 0x84 : return "1";
		case 0x90 : return "2";
		case 0xa0 : return "3";
		case 0xc0 : return "A";
		case 0x0c : return "4";
		case 0x18 : return "5";
		case 0x28 : return "6";
		case 0x48 : return "B";
		case 0x06 : return "7";
		case 0x12 : return "8";
		case 0x22 : return "9";
		case 0x42 : return "C";
		case 0x05 : return "*";
		case 0x11 : return "0";
		case 0x21 : return "#";
		case 0x41 : return "D";
		default : return "X";
	}
}*/

void addTaskCode(uint8_t key){
	switch(key){
		case 0x84 : 
			flash->args[0].i = 10;
			//displayLCD->args[0].str = "Flash";
			displayLED->args[0].i = isOn;
			//addToQueue(ReadyQueue, displayLCD, DISPLCD);
			addToQueue(ReadyQueue, displayLED, DISPLED);
			break;
		case 0x90 : 
			//displayLCD->args[0].str = "Counting";
			addToQueue(ReadyQueue, counter, COUNTER);
			//addToQueue(ReadyQueue, displayLCD, DISPLCD);
			break;
		case 0xa0 : 
			addToQueue(ReadyQueue, displayLCD, DISPLCD);
			break;
		/*case 0xc0 : return "A";
		case 0x0c : return "4";
		case 0x18 : return "5";
		case 0x28 : return "6";
		case 0x48 : return "B";
		case 0x06 : return "7";
		case 0x12 : return "8";
		case 0x22 : return "9";
		case 0x42 : return "C";
		case 0x05 : return "*";*/
		case 0x11 : 
			//displayLCD->args[0].str = "Idle";
			//addToQueue(ReadyQueue, displayLCD, DISPLCD);
			addToQueue(ReadyQueue, idle, IDLE);
			break;
		/*case 0x21 : return "#";
		case 0x41 : return "D";
		default : return "X";*/
	}
}

//tasks
void DisplayLED(){
	uint8_t l = displayLED->args[0].i;
	PORTB = l;
	displayLED->status = BLOCKED;
	ReleaseSemaphore();
	sei();
}

void DisplayLCD(){
	int c;
	char *str = displayLCD->args[0].str;
	writeInst(0x80); 			//set DD RAM first line first cell
	for(c = 0; c < strlen(str); c++)
		writeData(str[c]);
	displayLCD->status = BLOCKED;
	ReleaseSemaphore();
	sei();
}

void Flash(){
	flash->args[0].i--;
	if(flash->args[0].i <= 0){
		displayLED->args[0].i = notOn;
		addToQueue(ReadyQueue, displayLED, DISPLED);
	}
	ReleaseSemaphore();
	sei();
}

void Counter(){
	counter->args[0].i++;
	displayLED->args[0].i = counter->args[0].i;
	addToQueue(ReadyQueue, displayLED, DISPLED);
	ReleaseSemaphore();
	sei();
}

void ReadKeyPad(){
	if(PINA != pina){
		uint8_t key = getKey();
		PORTB = ~key;
		addTaskCode(key);
	}
	ReleaseSemaphore();
	sei();
}

void Idle(){
	ReadyQueue->head = NULL;
	counter->status = BLOCKED;
	flash->status = BLOCKED;
	ReleaseSemaphore();
	sei();
}

int main(void){
	Initialize();
//PORTB = 0xf0;
//	displayLCD->args[0].str = "Ready";
//	addToQueue(ReadyQueue, displayLCD, 6);
	while(1);
	return(0);
}
