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
//#include "m32def.inc"
//#include ".m32def.inc"

//#define EXT_INT0 PINB
#define TICK 50
#define COUNTERSPEED 20
#define FLASHLENGTH 10

#define MAXTASK 10

#define DISPLED 2
#define DISPLCD 3
#define FLASH 4
#define COUNTER 5
#define READKEY 1
#define IDLE 9

#define RUNNING 101
#define BLOCKED 102
#define READY 103

//structs
typedef union arg{
	char *str;
	int i;
}arg;

typedef struct context{
	uint16_t sreg;
}context;

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
	uint16_t TCBPtr;
}task;

#define initStack()                  \
asm volatile (                       \
  "LDI	R16, LOW(RAMEND)       \n\t" \
  "OUT	SPL, R16               \n\t" \
  "LDI	R16, HIGH(RAMEND)      \n\t" \
  "OUT	SPH, R16               \n\t" \
);

#define saveContext()                \
asm volatile (	                     \
  "push  r0                    \n\t" \
  "in    r0, __SREG__          \n\t" \
  "cli                         \n\t" \
  "push  r0                    \n\t" \
  "push  r1                    \n\t" \
  "clr   r1                    \n\t" \
  "push  r2                    \n\t" \
  "push  r3                    \n\t" \
  "push  r4                    \n\t" \
  "push  r5                    \n\t" \
  "push  r6                    \n\t" \
  "push  r7                    \n\t" \
  "push  r8                    \n\t" \
  "push  r9                    \n\t" \
  "push  r10                   \n\t" \
  "push  r11                   \n\t" \
  "push  r12                   \n\t" \
  "push  r13                   \n\t" \
  "push  r14                   \n\t" \
  "push  r15                   \n\t" \
  "push  r16                   \n\t" \
  "push  r17                   \n\t" \
  "push  r18                   \n\t" \
  "push  r19                   \n\t" \
  "push  r20                   \n\t" \
  "push  r21                   \n\t" \
  "push  r22                   \n\t" \
  "push  r23                   \n\t" \
  "push  r24                   \n\t" \
  "push  r25                   \n\t" \
  "push  r26                   \n\t" \
  "push  r27                   \n\t" \
  "push  r28                   \n\t" \
  "push  r29                   \n\t" \
  "push  r30                   \n\t" \
  "push  r31                   \n\t" \
  "lds   r26, pxCurrentTCB     \n\t" \
  "lds   r27, pxCurrentTCB + 1 \n\t" \
  "in    r0, __SP_L__          \n\t" \
  "st    x+, r0                \n\t" \
  "in    r0, __SP_H__          \n\t" \
  "st    x+, r0                \n\t" \
);

#define restorContext()              \
asm volatile (	                     \
  "lds  r26, pxCurrentTCB      \n\t" \
  "lds  r27, pxCurrentTCB + 1  \n\t" \
  "ld   r28, x+                \n\t" \
  "out  __SP_L__, r28          \n\t" \
  "ld   r29, x+                \n\t" \
  "out  __SP_H__, r29          \n\t" \
  "pop  r31                    \n\t" \
  "pop  r30                    \n\t" \
  "pop  r29                    \n\t" \
  "pop  r28                    \n\t" \
  "pop  r27                    \n\t" \
  "pop  r26                    \n\t" \
  "pop  r25                    \n\t" \
  "pop  r24                    \n\t" \
  "pop  r23                    \n\t" \
  "pop  r22                    \n\t" \
  "pop  r21                    \n\t" \
  "pop  r20                    \n\t" \
  "pop  r19                    \n\t" \
  "pop  r18                    \n\t" \
  "pop  r17                    \n\t" \
  "pop  r16                    \n\t" \
  "pop  r15                    \n\t" \
  "pop  r14                    \n\t" \
  "pop  r13                    \n\t" \
  "pop  r12                    \n\t" \
  "pop  r11                    \n\t" \
  "pop  r10                    \n\t" \
  "pop  r9                     \n\t" \
  "pop  r8                     \n\t" \
  "pop  r7                     \n\t" \
  "pop  r6                     \n\t" \
  "pop  r5                     \n\t" \
  "pop  r4                     \n\t" \
  "pop  r3                     \n\t" \
  "pop  r2                     \n\t" \
  "pop  r1                     \n\t" \
  "pop  r0                     \n\t" \
  "out  __SREG__, r0           \n\t" \
  "pop  r0                     \n\t" \
);

//Initialization
void Initialize();
void initTimer0();
void initTimer1();
void initLCD();
void initKeypad();
void initTasks();

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
//void DisplayLCD();
//void DisplayLED();
void Flash();
void Counter();
void ReadKeyPad();
void Idle();

uint8_t notOn = 0xff;
uint8_t isOn = 0x00;
uint8_t allOut = 0xff;

void (*functionPtr)();
//task *displayLCD;
//task *displayLED;
task *flash;
task *counter;
task *readKeyPad;
task *idle;

list *ReadyQueue;
task *running;

uint8_t colFilter = 0x74;			// 01110100
uint8_t pina;

uint8_t stkLow;
uint8_t stkHigh;

int semaphore = 1;
uint16_t tick = 0;

void Initialize(){

	initTimer0();
	initTimer1();

	DDRB = allOut;				//Set Port B as Output
	PORTB = notOn;				//turn off lights

	void initLCD();
	initKeypad();

	createReadyQueue();
	initTasks();
	initStack();
	sei();
}

void initTimer0(){
	TCCR0 = (1<<CS02)|(1<<CS00);		//Timer clock = Sysclk/1024
	//TCCR0 = (0<<CS02)|0<<CS01|(1<<CS00);	//clock 1 ms click
	TIFR = 1<<TOV0;				//Clear TOV0, any pending interupts
	TIMSK |= (1<<TOIE0);			//Enable Timer0 Overflow interrupt
}

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

	//displayLCD = AddTask(&DisplayLCD, DISPLCD, 1, args);
	//displayLCD->args[0].str = (char *)calloc(20, sizeof(char));
	//displayLED = AddTask(&DisplayLED, DISPLED, 1, args);
	//displayLED->args[0].i = notOn;
	flash = AddTask(&Flash, FLASH, 1, args);
	flash->args[0].i = 0;
	counter = AddTask(&Counter, COUNTER, 1, args);
	counter->args[0].i = 0;
	readKeyPad = AddTask(&ReadKeyPad, READKEY, 0, args);
	idle = AddTask(&Idle, 9, 0, args);
}

//interupts
ISR(TIMER0_OVF_vect){
	sei();
}

ISR(TIMER1_COMPA_vect, ISR_NAKED){
	//stkLow = //__SP_L__;
	saveContext()
/* context switch */
	//cli();
	//if(running != NULL)saveContext();
	TCNT1 = 0;
	tick++;
	//if(tick % 7 == 0)addToQueue(ReadyQueue, readKeyPad, READKEY);
	//if(flash->status == RUNNING) addToQueue(ReadyQueue, flash, FLASH);
	//if(counter->status == RUNNING && tick % COUNTERSPEED == 0) addToQueue(ReadyQueue, counter, COUNTER);
	priorityAdjust(ReadyQueue);
	sei();
	Scheduler();
	reti();
}

//Semaphore
void TakeSemaphore(){
	if(semaphore) semaphore--;
}

void ReleaseSemaphore(){
	if(!semaphore) semaphore++;
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
		if(semaphore){
			if(running != NULL) running->status = BLOCKED;
			Run(front(ReadyQueue));
		}
	}
}


void Run(task *t){
	//t->TCBPtr = stkHigh;
	t->status = RUNNING;
	running = t;
	pop(ReadyQueue);
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
		if (q->head == NULL){
			q->head = newTask;
			q->tail = newTask;
			q->size = 1;
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
		if (current != running && current->priority > 1) current->priority--;
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
	if(q->head != NULL) {
		q->head = q->head->next;
		q->size--;
	}
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
/*
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
}*/

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
			flash->args[0].i = FLASHLENGTH;
			addToQueue(ReadyQueue, flash, FLASH);
			//displayLED->args[0].i = isOn;	
			//addToQueue(ReadyQueue, displayLED, DISPLED);
			//displayLCD->args[0].str = "Flash";
			//addToQueue(ReadyQueue, displayLCD, DISPLCD);
			break;
		case 0x90 : 
			addToQueue(ReadyQueue, counter, COUNTER);
			//displayLCD->args[0].str = "Counting";
			//addToQueue(ReadyQueue, displayLCD, DISPLCD);
			break;
		/*case 0xa0 : 
			addToQueue(ReadyQueue, displayLCD, DISPLCD);
			break;*/
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
			ReadyQueue->head = NULL;
			counter->args[0].i = 0;
			//counter->status = BLOCKED;
			//flash->status = BLOCKED;
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
/*
void DisplayLED(){
	PORTB = displayLED->args[0].i;
	displayLED->status = BLOCKED;
	ReleaseSemaphore();
	Scheduler();
}

void DisplayLCD(){
	int c;
	char *str = displayLCD->args[0].str;
	writeInst(0x80); 			//set DD RAM first line first cell
	for(c = 0; c < strlen(str); c++)
		writeData(str[c]);
	displayLCD->status = BLOCKED;
	ReleaseSemaphore();
	Scheduler();
}*/

void Flash(){
	PORTB = isOn;
	while(1){
		flash->args[0].i--;
		if(flash->args[0].i <= 0){
			flash->args[0].i = 0;
			PORTB = notOn;
		}
	}
	/*flash->args[0].i--;
	if(flash->args[0].i <= 0){
		displayLED->args[0].i = notOn;
		addToQueue(ReadyQueue, displayLED, DISPLED);
	}
	ReleaseSemaphore();
	Scheduler();*/
		
}

void Counter(){
	while(1){
		if(counter->args[0].i % COUNTERSPEED == 0) {
			TakeSemaphore();
			counter->args[0].i++;
			PORTB = ~counter->args[0].i;
			ReleaseSemaphore();
		}
	}
}

void ReadKeyPad(){
	while(1){
		if(PINA != pina){
			uint8_t key = getKey();
			PORTB = ~key;
			addTaskCode(key);
			key = 0;
		}
	}
}

void Idle(){
	while(1){};
}

int main(void){
	Initialize();
	//displayLCD->args[0].str = "Ready";
	addToQueue(ReadyQueue, idle, IDLE);
	while(1){};
	return(0);
}
