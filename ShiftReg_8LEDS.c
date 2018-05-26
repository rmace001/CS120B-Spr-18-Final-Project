/*
 * Make and Play.c
 *
 * Created: 5/23/2018 11:35:54 AM
 * Author : Rogith
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include "bit.h"
//#include "scheduler.h"
//state enumerations
enum SHIFT_STATES {Init, Shift} state;
//end state enums
void ShiftTick(void);



// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
unsigned char tmpB = 0x00;
unsigned char temp = 0;
unsigned char i = 0;
void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

//end timer stuff








//shift register function overrides


void transmit_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		// does this mean it sets SRCLK low?
		// does this set RCLK low?
		PORTC = 0x08;
		// set SER = next bit of data to be sent.
		PORTC |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTC |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTC |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTC = 0x00;
}

//end shift functions



//shared global variables
unsigned char idx;
unsigned char tempOutput;
#define upperBound 0x08
//end shared global variables 




//user defined ticks()
void ShiftTick(void){
	switch(state){ //transition switch
		case Init:
			state = Shift;
			break;
		case Shift:
			state = Shift;
			break;
		default:
			state = Init;
			break;
	}
	switch(state){ //action switch
		case Init:
			tempOutput = 0x01;
			idx = 0;
			transmit_data(tempOutput);
			break;
			
			
		case Shift:
			if (idx >= upperBound){ //upperBound = 8
				idx = 0;
			}
			tempOutput = (0x01 << idx);
			idx++;
			transmit_data(tempOutput);
			break;
			
			
		default: 
			state = Init;
			break;
	}
}

//end user defined ticks()



int main(void){
	state = -1;// force the default case
	DDRC = 0xFF; PORTC = 0x00; 
	TimerSet(500);
	TimerOn();
	while (1){
		ShiftTick();
		while (!TimerFlag){}	// Wait 0.5 sec
		TimerFlag = 0;
	}
}