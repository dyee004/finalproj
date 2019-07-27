/*
 * final.c
 *
 * Created: 7/22/2019 11:48:31 AM
 * Author : danih
 */ 

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

#include <util/delay.h>
#define HC595_PORT   PORTA
#define HC595_DDR    DDRA

#define HC595_DS_POS PA0      //Data pin (DS) pin location
#define  HC595_DS_POS2 PA3
#define HC595_SH_CP_POS PA1      //Shift Clock (SH_CP) pin location 
#define HC595_ST_CP_POS PA2      //Store Clock (ST_CP) pin location

/***************************************
Configure Connections ***ENDS***
****************************************/

//Initialize HC595 System

void HC595Init()
{
   //Make the Data(DS), Shift clock (SH_CP), Store Clock (ST_CP) lines output
   HC595_DDR|=((1<<HC595_SH_CP_POS)|(1<<HC595_ST_CP_POS)|(1<<HC595_DS_POS));
}

void HC595Init2()
{
	//Make the Data(DS), Shift clock (SH_CP), Store Clock (ST_CP) lines output
	HC595_DDR|=((1<<HC595_SH_CP_POS)|(1<<HC595_ST_CP_POS)|(1<<HC595_DS_POS2));
}


//Low level macros to change data (DS)lines
#define HC595DataHigh() (HC595_PORT|=(1<<HC595_DS_POS))

#define HC595DataLow() (HC595_PORT&=(~(1<<HC595_DS_POS)))

#define HC595DataHigh2() (HC595_PORT|=(1<<HC595_DS_POS2))

#define HC595DataLow2() (HC595_PORT&=(~(1<<HC595_DS_POS2)))

//Sends a clock pulse on SH_CP line
void HC595Pulse()
{
   //Pulse the Shift Clock

   HC595_PORT|=(1<<HC595_SH_CP_POS);//HIGH

   HC595_PORT&=(~(1<<HC595_SH_CP_POS));//LOW

}

//Sends a clock pulse on ST_CP line
void HC595Latch()
{
   //Pulse the Store Clock

   HC595_PORT|=(1<<HC595_ST_CP_POS);//HIGH
   _delay_loop_1(1);

   HC595_PORT&=(~(1<<HC595_ST_CP_POS));//LOW
   _delay_loop_1(1);
}


/*

Main High level function to write a single byte to
Output shift register 74HC595. 

Arguments:
   single byte to write to the 74HC595 IC

Returns:
   NONE

Description:
   The byte is serially transfered to 74HC595
   and then latched. The byte is then available on
   output line Q0 to Q7 of the HC595 IC.

*/
void HC595Write2(uint8_t data, uint8_t data2)
{
   //Send each 8 bits serially

   //Order is MSB first
   for(uint8_t i=0;i<8;i++)
   {
      //Output the data on DS line according to the
      //Value of MSB
      if(data & 0b10000000)
      {
         //MSB is 1 so output high

         HC595DataHigh();
      }
      else
      {
         //MSB is 0 so output high
         HC595DataLow();
		 
      }
	  if(data2 & 0b10000000){
		HC595DataHigh2();
	  }
	  else{
		HC595DataLow2();
	  }

      HC595Pulse();  //Pulse the Clock line
      data=data<<1;  //Now bring next bit at MSB position
	  data2 = data2<<1;

   }

   //Now all 8 bits have been transferred to shift register
   //Move them to output latch at one
   HC595Latch();
}



/*

Simple Delay function approx 0.5 seconds

*/

void Wait()
{
   for(uint8_t i=0;i<30;i++)
   {
      _delay_loop_2(0);
   }
}


enum button_States {Start1, Init1, Up, Down, Wait1} button_state;
unsigned char button;
unsigned char position;	
void button_tick(){
	button = ~PINB & 0x03;
	switch(button_state){
		case Start1:
			button_state = Init1;
			break;
		case Init1:
			if(button == 0x01){
				button_state = Up;
			}
			else if(button == 0x02){
				button_state = Down;
			}
			else{
				button_state = Init1;
			}
			break;
		case Up:
			button_state = Wait1;
			break;
		case Down:
			button_state = Wait1;
			break;
		case Wait1:
			if(button == 0x00){
				button_state = Init1;
			}
			else{
				button_state = Wait1; 
			}
			break;
		default:
			button_state = Start1;
			break;
	}
	switch(button_state){
		case Start1:
			position = 3;
			break;
		case Init1:
			break;
		case Up:
			if(position < 5){
				position = position + 1;
			}
			break;
		case Down:
			if(position > 0){
				position = position - 1;
			}
			break;
		case Wait1:
			break;
		default:
			 break;
	}			
}

enum lcd_States {Start2, Display, Win, Lose, Highscore, Wait2} lcd_state;	
const unsigned short custom_character[] = {0b01010,0b00000,0b10001,0b01110,0b00000,0b00000,0b00000,0b00000};
unsigned char userScore;
unsigned char computerScore;
unsigned short countdown;	
unsigned char highscore;
unsigned char reset;
void lcd_tick(){
	static unsigned char i = 0;
	switch(lcd_state){
		case Start2:
			lcd_state = Display;
			break;
		case Display:
			if( countdown <= 0){
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
			else {
				if(reset == 0x04){
					lcd_state = Wait2;
				}
				else{
					lcd_state = Win;
				}
			}
			break;
		case Lose:
			if(userScore > highscore){
				lcd_state = Highscore;
			}
			else {
				if(reset == 0x04){
					lcd_state = Wait2;
				}
				else{
					lcd_state = Lose;
				}
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
		LCD_ClearScreen();
			LCD_init();
			break;
		case Display:
			highscore = eeprom_read_byte((uint8_t*)1);
			LCD_Cursor(1);
			LCD_DisplayString(1, " Your score: ");
			LCD_Cursor(13);
			LCD_WriteData(userScore + '0');
			LCD_Cursor(17);
			LCD_DisplayString(17, " Comp score: ");
			LCD_Cursor(30);
			LCD_WriteData(computerScore + '0');
			break;
		case Win:
			LCD_Cursor(1);
			LCD_DisplayString(1, "Winner!");
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
			eeprom_update_byte((uint8_t*)1, (uint8_t) highscore);
			LCD_Cursor(31);
			break;
		case Wait2:
			break;
		default:
			break;
	}
}

enum reset_States {Start3,Countdown,Stop, Wait3} reset_state;
void reset_tick(){
	static unsigned i = 10;
	switch(reset_state){
		case Start3:
			reset_state = Countdown;
			break;
		case Countdown:
			 if(i > 0){
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
			i = 10;
			break;
		case Countdown:
			countdown = i;
			i = i - 1;
			break;
		case Stop:
			break;
		case Wait3:
			break;
		default:
			break;
			
	}
}

enum paddle_States{Start4, Move, Reset4, Wait4} paddle_state;	
unsigned char paddleRow[6] = {0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0};
unsigned char paddleCol[1] = {0xFE};	
unsigned char paddleIndex;	
void paddle_tick(){
	switch(paddle_state){
		case Start4:
			paddle_state = Move;
			break;
		case Move:
			if(countdown <= 0){
				paddle_state = Reset4;
			}
			else{
				paddle_state = Move;
			}
			break;
		case Reset4:
			if(reset == 0x04){
				paddle_state = Wait4;
			}
			else{
				paddle_state = Reset4;
			}
			break;
		case Wait4:
			if(reset == 0x00){
				paddle_state = Start4;
			}
			else{
				paddle_state = Wait4;
			}
			break;
		default:
			paddle_state = Start4;
		}
	switch(paddle_state){
		case Start4:
			paddleIndex = 3;
			break;
		case Move:
			paddleIndex = position;
			break;
		case Reset4:
			break;
		case Wait4:
			break;
		default:
			break;	
	}
}

enum comp_States {Start6, Up6, Down6, Reset6, Wait6} comp_state;
unsigned char compIndex;
unsigned char compRow[6] = {0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0};
unsigned char compCol[1] = {0x7F};
void comp_tick(){
	switch(comp_state){
		case Start6:
			comp_state = Up6;
			break;
		case Up6:
			if(compIndex < 5){
				comp_state = Up6;
			}
			if(countdown <= 0){
				comp_state = Reset6;
			}
			else{
				comp_state = Down6;
			}
			break;
		case Down6:
			if(compIndex > 0){
				comp_state = Down6;
			}
			else if(countdown <= 0){
				comp_state = Reset6;
			}
			else {
				comp_state = Up6;	
			}
			break;
		case Reset6:
			if(reset == 0x04){
				comp_state = Wait6;
			}
			else{
				comp_state = Reset6;
			}
			break;
		case Wait6:
			if(reset == 0x00){
				comp_state = Start6;
			}
			else{
				comp_state = Wait6;
			}
			break;
	}
	switch(comp_state){
		case Start6:
			compIndex = 4;
			break;
		case Up6:
			compIndex = compIndex + 1;
			break;
		case Down6:
			compIndex = compIndex - 1;
			break;
		case Reset6:
			break;
		case Wait6:
			break;
	}
}	

enum ball_States{Start5, Init5, Move5} ball_state;
unsigned char ballRow[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
unsigned char ballCol[8] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};
unsigned char ballRIndex;
unsigned char ballCIndex;		
unsigned char score;  
unsigned char row = 1;
unsigned char col = -1;  
void ball_tick(){
	switch(ball_state){
		case Start5:
			ball_state = Init5;
			break;
		case Init5:
			if(countdown > 0){
				ball_state = Move5;
			}
			else{
				ball_state = Init5;
			}
			break;
		case Move5:
			if(ballCIndex == 0 && score == 1){
				computerScore = computerScore + 1;
				ball_state = Start5;
			}
			else if(ballCIndex == 7 && score == 2){
				userScore = userScore + 1;
				ball_state = Start5;
			}
			else{
				ball_state = Move5;
			}
			break;
	}
	switch(ball_state){
		case Start5:
			userScore = 0;
			computerScore = 0;
			break;
		case Init5:
			if(score == 0){
				row = 0;
				col = 1;
				ballCIndex = 4;
				ballRIndex = 4;
			}
			else if(score == 1){
				row = 0;
				col = 1;
				ballCIndex = 3;
				ballRIndex = 3;
			}
			else if(score == 2){
				row = 0;
				col = -1;
				ballCIndex = 5;
				ballRIndex = 5;
			}
			break;
		case Move5:
			score = 0;
			ballRIndex = ballRIndex + row;
			ballCIndex = ballCIndex + col;
			if(ballCIndex == 1 || ballCIndex == 0){
				if(ballRIndex == 0){
					if(paddleIndex == 0){
						row = 1;
						col = 1;
					}
				}
				else if(ballRIndex == 1){
					if(paddleIndex == 0){
						row = 0;
						col = 1;
					}
					else if(paddleIndex == 1){
						row = 1;
						col = 1;
					}
				}
				else if(ballRIndex == 2){
					if(paddleIndex == 0){
						row = 1;
						col = 1;
					}
					else if(paddleIndex == 1){
						row = 0;
						col = 1;
					}
					else if(paddleIndex == 2){
						row = 1;
						col = -1;
					}
				}
				else if(ballRIndex == 3){
					if(paddleIndex == 1){
						row = 1;
						col = 1;
					}
					else if(paddleIndex == 2){
						row = 0;
						col = 1;
					}
					else if(paddleIndex == 3){
						row = 1;
						col = -1;
					}
				}
				else if(ballRIndex == 4){
					if(paddleIndex == 2){
						row = 1;
						col = 1;
					}
					else if(paddleIndex == 3){
						row = 0;
						col = 1;
					}
					else if(paddleIndex == 4){
						row = 1;
						col = -1;
					}
				}
				else if(ballRIndex == 5){
					if(paddleIndex == 3)
					{
						row = 1;
						col = 1;
					}
					else if(paddleIndex == 4){
						row = 0;
						col = 1;
					}
					else if(paddleIndex == 5){
						row = 1;
						col = -1;
					}
				}
				else if(ballRIndex == 6){
					if(paddleIndex == 4){
						row = 0;
						col = 1;
					}
					else if(paddleIndex == 5){
						row = 1;
						col = -1;
					}
				}
				else if(ballRIndex == 7){
					if(paddleIndex == 5)
					{
						row = 1;
						col = -1;
					}
				}
				else{
					score = 2;	
				}
			}
			else if(ballCIndex == 6 || ballCIndex == 7){
				if(ballRIndex == 0){
					if(compIndex == 0){
						row = 1;
						col = -1;
					}
				}
				else if(ballRIndex == 1){
					if(compIndex == 0){
						row = 0;
						col = -1;
					}
					else if(compIndex == 1){
						row = -1;
						col = -1;
					}
				}
				else if(ballRIndex == 2){
					if(compIndex == 0){
						row = -1;
						col = 1;
					}
					else if(compIndex == 1){
						row = 0;
						col = -1;
					}
					else if(compIndex == 2){
						row = -1;
						col = -1;
					}
				}
				else if(ballRIndex == 3){
					if(compIndex == 1){
						row = 1;
						col = -1;
					}
					else if(compIndex == 2){
						row = 0;
						col = 1;
					}
					else if(compIndex == 3){
						row = -1;
						col = -1;
					}
				}
				else if(ballRIndex == 4){
					if(compIndex == 2)
					{
						row = 1;
						col = -1;
					}
					else if(compIndex == 3){
						row = 0;
						col = -1;
					}
					else if(compIndex == 4){
						row = -1;
						col = -1;
					}
				}
				else if(ballRIndex == 5){
					if(compIndex == 3){
						row = 1;
						col = -1;
					}
					else if(compIndex == 4){
						row = 0;
						col = -1;
					}
					else if(compIndex == 5){
						row = -1;
						col = -1;
					}
				}
				else if(ballRIndex == 6){
					if(compIndex == 4){
						row = -1;
						col = -1;
					}
					else if(compIndex == 5){
						row = 0;
						col = -1;
					}
				}
				else if(ballRIndex == 7){
					if(compIndex == 5){
						row = -1;
						col = -1;
					}
				}
				else{
					score = 1;
				}
			}
			if(ballRIndex < 1){
				row = 1;
			}
			if(ballRIndex > 6){
				row = -1;
			}
			break;
	}
}	

enum matrix_States {Start7, Updateball, Updatepaddle} matrix_state;
unsigned char ball;
unsigned char user;
unsigned char comp;	
void matrix_tick(){
	switch(matrix_state){
		case Start7:
			matrix_state = Updateball;
			break;
		case Updateball:
			matrix_state = Updatecomp;
			break;
		case Updatepaddle:
			matrix_state = Updateball;
			break;
	}
	switch(matrix_state){
		case Start7:
			user = paddleRow[3];
			HC595Write2(user,paddleCol[1]);
			comp = compRow[4];
			HC595Write2(comp, compCol[1]);
			break;
		case Updateball:
			HC595Write2(ballRow[ballRIndex], ballCol[ballCIndex]);
			break;
		case Updatepaddle:
			HC595Write2(compRow[compIndex], compCol[1]);
			HC595Write2(paddleRow[paddleIndex], paddleCol[1]);
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
	
    
    static task task1, task2, task3, task4, task5, task6, task7;
    task *tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6, &task7};
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
    
    unsigned long int tmpGCD = 1;
    tmpGCD = findGCD(100,100);
    unsigned long int GCD = tmpGCD;

    // Task 1
    task1.state = Start2;
    task1.period = 100;
    task1.elapsedTime = task1.period;
    task1.TickFct = &lcd_tick;

    // Task 2
    task2.state = Start3;
    task2.period = 100;
    task2.elapsedTime = task2.period;
    task2.TickFct = &reset_tick;

    // Task 3
    task3.state = Start1;
    task3.period = 100;
    task3.elapsedTime = task3.period;
    task3.TickFct = &button_tick;
	
	// Task 4
	task4.state = Start4;
	task4.period = 100;
	task4.elapsedTime = task4.period;
	task4.TickFct = &paddle_tick;

    // Task 5
    task5.state = Start5;
    task5.period = 100;
    task5.elapsedTime = task5.period;
    task5.TickFct = &ball_tick;

	// Task 6
	task6.state = Start6;
	task6.period = 100;
	task6.elapsedTime = task6.period;
	task6.TickFct = &comp_tick;
	
	// Task 7
	task6.state = Start7;
	task6.period = 100;
	task6.elapsedTime = task6.period;
	task6.TickFct = &matrix_tick;
	


    TimerSet(GCD);
    TimerOn();
	  LCD_init();
    HC595Init();
    HC595Init2();
	
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
