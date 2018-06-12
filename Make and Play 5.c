/*
 * Make and Play 5.1.c
 *
 * Created: 6/05/2018 1:35:51 PM
 * Author : Rogelio Macedo
 * UCR Computer Science | Undergraduate
 * The code controls LED matrix with 2 shift reg on PORTC. It controls 3 buttons on A2, A3, and A4, 
 * as well as joystick on A0 and A1;
 * Depending on the Mode you are in, an external LED will light on PORTD.
 * Build/SetSong mode, LED Matrix enabled, and A2 can play that note's sound 
 * Speaker connected to PB6 for Play state (PWM)
 * Disconnect the speaker when programming the microcontroller
 * A2: play for 4s, A3: go to setSong, A4: exit/set_the_song
 */

/* Notes
for SetBit, parameters: PORTx, pin number, bit value
getBit: PINx, pin number
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "bit.h"
#include "scheduler.h"

//PWM variables
#define c4 261.63
#define d4 293.66
#define e4 329.63
#define f4 349.23
#define g4 392.00
#define a4 440.00
#define b4 493.88
#define c5 523.25

//shared variables
double note[] = {c4,d4,e4,f4,g4,a4,b4,c5};
double song[8];
unsigned char ledCol[8];
unsigned char ledVal[8];
unsigned char setSong = 0; 
unsigned char start_song = 0;
unsigned char song_index;
unsigned char note_index;
#define A2 (~PINA & 0x04)
#define A3 (~PINA & 0x08)
#define A4 (~PINA & 0x10)
//select the LED to light
unsigned char column_select = 0x01;
unsigned char column_val = 0x01;
//end shared variables

void setTheNote(){
	switch (column_select){
		case 0x01:
		song_index = 0;
		break;
		case 0x02:
		song_index = 1;
		break;
		case 0x04:
		song_index = 2;
		break;
		case 0x08:
		song_index = 3;
		break;
		case 0x10:
		song_index = 4;
		break;
		case 0x20:
		song_index = 5;
		break;
		case 0x40:
		song_index = 6;
		break;
		case 0x80:
		song_index = 7;
		break;
	}
	switch (column_val){
		case 0x01:
		note_index = 0;
		break;
		case 0x02:
		note_index = 1;
		break;
		case 0x04:
		note_index = 2;
		break;
		case 0x08:
		note_index = 3;
		break;
		case 0x10:
		note_index = 4;
		break;
		case 0x20:
		note_index = 5;
		break;
		case 0x40:
		note_index = 6;
		break;
		case 0x80:
		note_index = 7;
		break;
	}
	song[song_index] = note[note_index];
	ledCol[song_index] = column_select;
	ledVal[song_index] = column_val;
	return;
}


//The following functions are sourced from UCR CS120B
void set_PWM(double frequency) {
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; } //stops timer/counter
		else { TCCR3B |= 0x03; } // resumes/continues timer/counter
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		// prevents OCR0A from underflowing, using prescaler 64     // 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR3A = 0x0000; }

		// set OCR3A based on desired frequency
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }
		TCNT3 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}

}
void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB3 on compare match between counter and OCR0A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM02: When counter (TCNT0) matches OCR0A, reset counter
	// CS01 & CS30: Set a prescaler of 64
	set_PWM(0);
}
void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

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
	// set RCLK = 1. Rising edge copies data from ?Shift? register to ?Storage? register
	PORTC |= 0x40;
	// clears all lines in preparation of a new transmission
	PORTC = 0x00;
}
//end UCR CS120B functions

/*Function below courtesy of: http://learn.parallax.com/tutorials/language/propeller-c/propeller-c-simple-devices/joystick */
/*and the ATmega1284 data sheet on ADC */
void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADSCRA: ADC status and control register A
	// ADEN: Writing '1' to ADEN enables ADC
	// ADSC: Analog-digital start conversion: ADEN must be enabled first or at the same time
	//		 Writing '1' to this starts conversion, hardware sets back to '0'
	// ADATE:AD Auto-Trigger enable. ADC will start conversion automatically on a positive edge of trigger signal
}


enum SM1_States {joystick_wait, joystick_display} ;

int Joystick_Tick(int state) {

	ADC_init();
	ADMUX = !ADMUX;
	unsigned short move;
	move = ADC;

	switch (state) {
		case joystick_wait:
			if (setSong){
				state = joystick_display;
			}
			else{
				state = joystick_wait;
			}
			break;
		case joystick_display:
			if (setSong){
				state = joystick_display;
			}
			else{
				state = joystick_wait;
			}
			break;
		default:
		state = joystick_wait;
		break;
	}
	
	switch (state) {
		case joystick_wait:
			
			break;
		
		case joystick_display:
		if(move < 225 && ADMUX == 0){ //left
			column_select = (column_select <= 0x01) ? 0x01: column_select >> 1;
		}
		if(move > 0 && move < 225 && ADMUX == 1){ //down
			column_val = (column_val <= 0x01) ? 0x01: column_val >> 1;
		}
		if(move > 700 && ADMUX == 0){ //right
			column_select = (column_select >= 0x80) ? 0x80: column_select << 1;
		}
		if(move > 700 && ADMUX == 1){ //up
			column_val = (column_val >= 0x40) ? 0x80: column_val << 1;
		}
		break;

		default:
		break;
	}
	if (setSong){
		transmit_data(~column_select); // PORTA displays column pattern
		transmit_data1(column_val); // PORTB selects column to display pattern
	}
	else if (start_song){
		//do nothing
	}
	else {
		transmit_data(0); // PORTA displays column pattern
		transmit_data1(0); // PORTB selects column to display pattern
	}
	return state;
}


enum SET_NOTE_STATES {sns_wait, sns_set};
int SetTick (int state){
	switch(state){
		case sns_wait:
			if ( !(A2 && setSong) ){
				state = sns_wait;
			}
			else if(A2 && setSong){
				state = sns_set;
			}
			break;
		case sns_set:
			if (A2){
				state = sns_set;
			}
			else {
				state = sns_wait;
				setTheNote();
			}
			break;
		default:
			state = sns_wait;
			break;
	}
	switch(state){
		case sns_wait:
			if (setSong){
				set_PWM(0);
			}
		break;
		case sns_set:
		setTheNote();
		set_PWM(note[note_index]);
		break;
		
		default:
		state = sns_wait;
		break;
	}
	return state;
}


enum smPLAY_STATES {smWait, smPlay};
int PlayTick(int state){
	static unsigned char i;
	static unsigned char j;
	switch (state){
		case smWait:
		if (start_song){
			state = smPlay;
			i = 0;
			j = 0;
		}
		else{
			state = smWait;
		}
		break;
		case smPlay:
		if (i < 85){
			state = smPlay;
		}
		else {
			state = smWait;
		}
		break;
		default:
		state = smWait;
		break;
	}
	switch (state){
		case smWait:
		if (!setSong){
			set_PWM(0);
		}	
		break;
		case smPlay:
		if (i%5 == 0){
			set_PWM(song[j]);
			transmit_data(~(ledCol[j])); //  displays column pattern
			transmit_data1(ledVal[j]);   //  selects column to display pattern
			j = (j > 7) ? 0 : j+1;
		}
		i++;
		break;
		default:
		break;
	}
	return state;
}

enum MODE_STATES {Wait, Play, SetNote, WaitRelease};
int buttonTask (int state){
	static unsigned char i;
	switch(state){ // transitions
		case Wait:
		if (A2){
			state = Play;
			PORTD = 0x02;
			start_song = 1;
			i = 0;
		}
		else if (A3){
			state = SetNote;
			PORTD = 0x04;
			setSong = 1;
		}
		else{
			state = Wait;
		}
		break;
		case Play:
		if (i < 43){ // i < 4s
			state = Play;
		}
		else{
			state = Wait;
			start_song = 0;
		}
		break;
		case SetNote:
		if (!A4){
			state = SetNote;
		}
		else if (A4){
			state = WaitRelease;
		}
		break;
		case WaitRelease:
		if (A4)
		{
			state = WaitRelease;
		}
		else if (!A4){
			state = Wait;
			setSong = 0;
		}
		break;
		default:
		state = Wait;
		break;
	}
	switch(state){ // actions
		case Wait:
		PORTD = 0x01;
		break;
		case Play:
		i++;
		break;
		case SetNote:
		break;
		case WaitRelease:
		break;
		default:
		state = Wait;
		break;
	}
	return state;
}

int main(void)
{
	//set data direction registers
	DDRA = 0x00; PORTA = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	//=====================================================================================
	// The task initialization methods were grabbed from UCR CS120B sample scheduler Lab 10
	//=====================================================================================
	//period for the tasks
	unsigned long int tick1_calc = 50; // sm1: joystick task
	unsigned long int tick2_calc = 100;// input buttons task
	unsigned long int tick3_calc = 50; // play song task
	unsigned long int tick4_calc = 50; // build song task
	//calculating the GCD
	unsigned long int tempGCD = 1;
	tempGCD = findGCD(tempGCD, tick1_calc);
	tempGCD = findGCD(tempGCD, tick2_calc);
	tempGCD = findGCD(tempGCD, tick3_calc);
	tempGCD = findGCD(tempGCD, tick4_calc);
	

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tempGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = tick1_calc/GCD; // sm1: joystick task
	unsigned long int SMTick2_period = tick2_calc/GCD; // input buttons task
	unsigned long int SMTick3_period = tick3_calc/GCD; // play song task
	unsigned long int SMTick4_period = tick4_calc/GCD; // build/set song task


	//testing
	static task task1, task2, task3, task4;
	task *tasks[] = { &task1, &task2, &task3, &task4};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	task1.state = -1; //task init state
	task1.period = SMTick1_period;
	task1.elapsedTime = task1.period;
	task1.TickFct = &Joystick_Tick;
	
	task2.state = -1; //task init state
	task2.period = SMTick2_period;
	task2.elapsedTime = task2.period;
	task2.TickFct = &buttonTask;
	
	task3.state = -1; //task init state
	task3.period = SMTick3_period;
	task3.elapsedTime = task3.period;
	task3.TickFct = &PlayTick;
	
	task4.state = -1; //task init state
	task4.period = SMTick4_period;
	task4.elapsedTime = task4.period;
	task4.TickFct = &SetTick;
	


	TimerSet(GCD);
	TimerOn();
	unsigned char i;
	PWM_on();
	while(1)
	{
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
}