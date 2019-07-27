/*
 * testing_lcd.c
 *
 * Created: 7/25/2019 11:44:05 AM
 * Author : danih
 */ 


#include <util/delay.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <scheduler.h>
#include <timer.h>
#include <io.c>
#include <bit.h>

void LCD_Custom(unsigned char *msg, unsigned char loc){
	unsigned char i;
	LCD_WriteCommand(0x40 + (loc * 8));
	for(i = 0; i < 8; i++){
		LCD_WriteData(msg[i]);
	}
}


enum lcd_States {Start2, Display, Win, Lose, Highscore, Wait2} lcd_state;
const unsigned short custom_character[] = {0b01010,0b00000,0b10001,0b01110,0b00000,0b00000,0b00000,0b00000};
unsigned char userScore;
unsigned char computerScore;
unsigned char countdown;
unsigned char highscore;
unsigned char reset;
void lcd_tick(){
	static unsigned char i = 0;
	switch(lcd_state){
		case Start2:
		lcd_state = Display;
		break;
		case Display:
		if(countdown <= 0){
			if(userScore > computerScore){
				LCD_ClearScreen();
				lcd_state = Win;
			}
			else if(userScore <= computerScore){
				LCD_ClearScreen();
				lcd_state = Lose;
			}
		}
		else {
			lcd_state = Display;
		}
		break;
		case Win:
		if(userScore > highscore){
			lcd_state = Highscore;
		}
		else if(reset == 0x04){
				lcd_state = Wait2;
		}
		else{
			lcd_state = Win;
		}
		break;
		case Lose:
		if(userScore > highscore){
			lcd_state = Highscore;
		}
		else if (reset == 0x04){
			lcd_state = Wait2;
		}
		else{
				lcd_state = Lose;
		}
		break;
		case Highscore:
		if(reset == 0x04){
			lcd_state = Wait2;
		}
		else{
			lcd_state = Highscore;
		}
		break;
		case Wait2:
		if(reset == 0x00){
			lcd_state = Start2;
		}
		else {
			lcd_state = Wait2;
		}
		break;
		default:
		lcd_state = Start2;
		break;
	}
	switch(lcd_state){
		case Start2:
		LCD_init();
		break;
		case Display:
		LCD_DisplayString(2, "UserScore");
		LCD_Cursor(13);
		LCD_WriteData(userScore + '0');
		LCD_DisplayString(17, " CompScore: ");
		LCD_Cursor(29);
		LCD_WriteData(computerScore + '0');
		break;
		case Win:
		LCD_Cursor(1);
		LCD_DisplayString(1, "Winner");
		LCD_Cursor(31);
		break;
		case Lose:
		LCD_Cursor(1);
		LCD_DisplayString(1, "Loser");
		LCD_Custom(custom_character,1);
		LCD_Cursor(20);
		LCD_WriteData(1);
		break;
		case Highscore:
		LCD_Cursor(17);
		LCD_DisplayString(17, "New: ");
		LCD_Cursor(23);
		LCD_WriteData(highscore + '0');
		break;
		case Wait2:
		break;
		default:
		break;
	}
}

enum reset_States {Start3,Countdown,Stop, Wait3} reset_state;
	unsigned char j = 0;
void reset_tick(){
	switch(reset_state){
		case Start3:
		reset_state = Countdown;
		break;
		case Countdown:
		if(j > 0){
			reset_state = Countdown;
		}
		else{
			reset_state = Stop;
		}
		break;
		case Stop:
		if(reset == 0x04){
			reset_state = Wait3;
		}
		else{
			reset_state = Stop;
		}
		break;
		case Wait3:
		if(reset == 0x00){
			j = 4;
			reset_state = Start3;
		}
		else{
			reset_state = Wait3;
		}
		break;
		default:
		break;
	}
	switch(reset_state){
		case Start3:
		break;
		case Countdown:
		countdown = j;
		j = j - 1;
		break;
		case Stop:
		break;
		case Wait3:
		break;
		default:
		break;
		
	}
}

int main(void)
{
	/* Replace with your application code */

	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0x00; PORTB = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;

	reset = ~PINB & 0x04;
	
	
	static task task1, task2;
	task *tasks[] = { &task1, &task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(100,10);
	unsigned long int GCD = tmpGCD;

	// Task 1
	task1.state = Start2;
	task1.period = 100;
	task1.elapsedTime = task1.period;
	task1.TickFct = &lcd_tick;

	// Task 2
	task2.state = Start3;
	task2.period = 10;
	task2.elapsedTime = task2.period;
	task2.TickFct = &reset_tick;

	

	TimerSet(GCD);
	TimerOn();
	LCD_init();
	
	unsigned short i;
	while(1) {
		for ( i = 0; i < numTasks; i++ ) {
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag){};
		TimerFlag = 0;
	}
	return 0;
}
