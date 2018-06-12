/******************************************************************************
* Rogelio Macedo
* Joystick controlling lED on 8x8 LED Matrix usings 2 shift registers on PORTC
* Courtesy of University of California, Riverside, CS120B
* June 4th, 2018
******************************************************************************/


#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "bit.h"
#include "scheduler.h"
#include <avr/interrupt.h>

unsigned char col_sel= 0x10; // grounds column to display pattern
unsigned char col_val 0x10;  // sets the pattern displayed on columns

void transmit_data(unsigned char data);
void transmit_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		PORTC = 0x08;
		// set SER = next bit of data to be sent.
		PORTC |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTC |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from ?Shift? register to ?Storage? register
	PORTC |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTC = 0x00;
}

void transmit_data1(unsigned char data) {
		int i;
		for (i = 0; i < 8 ; ++i) {
			// Sets SRCLR to 1 allowing data to be set
			// Also clears SRCLK in preparation of sending data
			PORTC = 0x80;
			// set SER = next bit of data to be sent. left shift four times to match.
			PORTC |= (((data >> i) << 4) & 0x10);
			// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
			PORTC |= 0x20;
		}
		// set RCLK = 1. Rising edge copies data from ìShiftî register to ìStorageî register
		PORTC |= 0x40;
		// clears all lines in preparation of a new transmission
		PORTC = 0x00;
	}


void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}



// ====================
// SM1: DEMO LED matrix
// ====================
enum SM1_States {sm1_display} states;

int SM1_Tick(int state) {

	// === Local Variables ===
	ADC_init();
	ADMUX = !ADMUX;


	unsigned short input;

	input = ADC;


	// === Transitions ===
	switch (state) {
		case sm1_display:
		break;
		default:
		state = sm1_display;
		break;
	}
	// === Actions ===
	switch (state) {
		case sm1_display:
			if(input < 225 && ADMUX == 0){ //left
				col_sel = (col_sel >= 0x40) ? 0x80 : col_sel << 1;
			}
			if(input < 225 && ADMUX == 1){ //down
				col_val = (col_val <= 0x01) ? 0x01 : col_val >> 1;
			}
			if(input > 700 && ADMUX == 0){ //right
				col_sel = (col_sel <= 0x01) ? 0x01 : col_sel >> 1;
			}
			if(input > 700 && ADMUX == 1){ //up
				col_val = (col_val >= 0x40) ? 0x80 : col_val << 1;
			}
			break;
		default:
			break;
			//original
			if(input < 225 && ADMUX == 0){ //left
			col_sel = (col_sel > 0xBF) ? 0x7F : (col_sel << 1) |0x01;
		}
		//if(input < 225 && ADMUX == 1){ //down
			//col_val = (col_val >= 0x80) ? 0x80 : col_val << 1;
		//}
		if(input > 700 && ADMUX == 0){ //right
			col_sel = (col_sel < 0xFD) ? 0xFE : (col_sel >> 1) |0x80;
		}
		//if(input > 700 && ADMUX == 1){ //up
			//col_val = (col_val <= 0x01) ? 0x01 : col_val >> 1;
		//}


			//end orig





	}


	//convert col_sel and col_val to an songLED[] element
	//convert col_val to note[] element
	//convert col_sel to Song[] index

	transmit_data(col_sel); // PORTA displays column pattern, took ~ off from parameter
	transmit_data1(col_val); // PORTB selects column to display pattern

	return state;
};

int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	TimerSet(50);
	TimerOn();
	//ADC_init();
	/* Replace with your application code */
	while (1)
	{

		SM1_Tick(sm1_display);
		while(!TimerFlag) { }
		TimerFlag = 0;
	}
}
