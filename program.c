/* EECE259 Assignment 4 Solution
 * Final Project: "Hit the Balloons!"
 * Date: April 8, 2011
 *
 * @author: Navid Fattahi		<nfattahi (at) interchange.ubc.ca>
 *			Shayan Farahbakhsh 	<shayanf23 (at) hotmail.com>
 *
 */

#include "259macros.h"

#define MAX_MISS 20			//Maximum number of missed hits before losing

unsigned char hex[10] =
{DIGIT0, DIGIT1, DIGIT2, DIGIT3, DIGIT4,
DIGIT5, DIGIT6, DIGIT7, DIGIT8, DIGIT9};

/* Global Variables */
//General Variables
int time = 0;

//GUI variables
unsigned short BLACK, WHITE, RED, GREEN, BLUE;
unsigned short dragColor;
unsigned int xCircle = 0;
unsigned int yCircle = 0;

//Game variables
int circleRad = 30;
int missed = 0;
int restart = 0;
int score = 0;
int increment = 1;
int pause = 0;

//Devices
int mouse_status;
int mouse_button;
int xMouse, yMouse, dxMouse, dyMouse;
int musicCounter = 0;


/* Function Declarations */
//Graphics, I/O functions
void drawCircle(int x, int y, int radius, unsigned short color);
void drawCursor(int x, int y);
void printStr(unsigned int x, unsigned int y, char* number);
void welcomeMessage();
void emptyScreen();
void display7Seg(int num);
void playMusic(short left, short right);
unsigned short getColor(int x, int y);
//Math and helper functions
void reverse(char *str,int len);
double random();
int abs(int num);
char* itoa(unsigned int number);
//Interrupt-related functions
void balloonISR();
void enableBalloonIRQ( int interval_cycles, ptr_to_function newISR );
void keyISR();
void enableKeyIRQ( int keys_to_watch, ptr_to_function newISR );


short music[] = {
#include "piano.h"
};
int musicLength = sizeof(music);

short effect[] = {
#include "effect.h"
};
int effectLength = sizeof(effect);

/* Generates a standard page for the boot-up of the program */
void welcomeMessage()
{
	int i = 0;
	//in order to prevent the GUI stuff show up...
	disableInterrupt(IRQ_COUNTER);
	fillScreen(BLUE);
	printStr(12, 11, "Hit the Balloon!");
	printStr(9, 13, "EECE 259 Final Project");
	printStr(10, 17, "By: Navid Fattahi");
	printStr(10, 18, "Shayan Farahbakhsh");
	for(i = 0; i < 40000000; i++);
	//in order to get the GUI stuff show up again!
	enableInterrupt(IRQ_COUNTER);
	printStr(12, 11, "                ");

	printStr(9, 13, "                      ");
	printStr(10, 17, "                 ");
	printStr(10, 18, "                  ");
}

/* Sends the sounds from the buffer to the right and left sound outputs */
void playMusic(short left, short right)
{

	int sampleSound;

	do{
		sampleSound = *pSNDRDY;
		sampleSound = sampleSound>>24;
	 } while(sampleSound == 0);

	*pSNDL = left;
	*pSNDR = right;
}

/* Displays "num" on the 7segment LEDs */
void display7Seg(int num)
{
	unsigned int dig3,dig2,dig1,dig0;
	unsigned int temp = num;

	
	dig3=temp/1000;		//extract each digit by dividing by 10^n
	dig3=hex[dig3];		//find the corresponding HEX LED value from the table
	dig3 = dig3 << 24;	//shift the digit to the corresponding space

	temp=temp%1000;
	dig2=temp/100;
	dig2=hex[dig2];
	dig2 = dig2 << 16;

	temp=temp%100;
	dig1=temp/10;
	dig1=hex[dig1];
	dig1 =dig1 << 8;

	dig0=temp%10;
	dig0=hex[dig0];

	//OR all digits together and load onto the 7seg LEDs
	*pHEX7SEG = dig3 | dig2 | dig1 | dig0;
}

/* Interrupts every 1 second and draws a circle on the screen (randomly)
   and updates the GUI variables (i.e. score etc.); it also updates the
   7seg LEDs */
void balloonISR()
{
	/* remember: no waiting in an ISR */
	time ++;
	*pLEDG = time;

	//	show the level on LEDR
	if(increment == 1)
		*pLEDR = 1;
	else if(increment == 2)
		*pLEDR = 2;
	else if(increment == 3)
		*pLEDR = 4;


	char* string = 0;

	fillScreen(BLACK);
	xCircle = (int)(random() * 1200);
	yCircle = (int)(random() * 900);
	// Darw the balloon and update the cursor location */		
	drawCircle(xCircle, yCircle, circleRad, RED);
	drawPixel( xMouse, yMouse, WHITE );
	//Create the user menu
	printStr(1, 1, "Level:  ");
	string = itoa(increment);
	printStr(9, 1, string);

	printStr(1, 2, "Score:  ");
	string = itoa(score);
	printStr(9, 2, string);

	printStr(1, 3, "Missed: ");
	string = itoa(missed);
	printStr(9, 3, string);

	printStr(1, 4, "Time:   ");
	string = itoa(time);
	printStr(9, 4, string);
/*
	string = itoa(xCircle);
	printStr(1, 4, string);

	string = itoa(yCircle);
	printStr(1, 5, string);
*/
	display7Seg(score);

	/* clear source of COUNTER interrupt before returning */
	*pCOUNTER_STATUS = 1; /* Device: send interrupts, clear existing interrupt */
}

/* This routine configures device-specific details about interrupts */
void enableBalloonIRQ( int interval_cycles, ptr_to_function newISR )
{
	registerISR( IRQ_COUNTER, newISR ); /* specify which ISR to call with COUNTER interrupts */
	
	*pCOUNTER		= -interval_cycles; /* initial counter value */
	*pCOUNTER_RELOAD	= -interval_cycles; /* on overflow, start with this value */
	
	*pCOUNTER_STATUS	= 1; /* Device: send interrupts, clear existing interrupt */
	enableInterrupt( IRQ_COUNTER ); /* CPU: allow it to receive COUNTER interrupts */
}

/* This function clears the GUI variables from the screen */
void emptyScreen()
{
	int i;
	for(i = 0; i < 5; i++)
		printStr(1, i, "              ");
}


/* This ISR gets called whenever a KEY is pressed down:
	KEY1 restarts the program
	KEY2 starts a new game after a GAME OVER
*/
void keyISR()
{
	

	/* remember: no waiting in an ISR */

	int keypress = *pKEY_EDGECAPTURE;

	int i = 0;

	int x, y;


	//Reset Key

	if( keypress & 0x02 )

	{

		score = 0;

		missed = 0;
	
		time = 0;

		emptyScreen();

		fillScreen(BLUE);

		printStr(12, 15, "GAME RESTARTED!");

		for(i = 0; i < 30000000; i++);

		
		printStr(12, 15, "               ");

	}

	
	//New game key

	if( keypress & 0x04 && missed >= MAX_MISS)

	{

		score = 0;

		missed = 0;

		time = 0;

		emptyScreen();

		fillScreen(BLUE);

		printStr(7, 15, "Good Luck in the new game!");

		
		for(i = 0; i < 30000000; i++);
		printStr(7, 15, "                          ");

	}
	
	/* clear source of KEY interrupt before returning */

	*pKEY_EDGECAPTURE = 0;

	
}


	
/* This routine configures device-specific details about interrupts */

void enableKeyIRQ( int keys_to_watch, ptr_to_function newISR )

{

	registerISR( IRQ_KEY, newISR );	
	/* specify which ISR to call with KEY interrupts */

	*pKEY_IRQENABLE = keys_to_watch;
	
	/* Device: to send interrupts for KEY3,KEY2 */

	enableInterrupt( IRQ_KEY );
	
	/* CPU: allow it to receive KEY interrupts */

}

/* Generates a random number between 0 and 1 (double) */
double random()
{
	unsigned int num = *pCOUNTER;// & 0xFF;
	int hi, lo;
	
	//high 16 bits
	hi=num >> 16;
	lo = num & 0xffff;
	num = hi ^ lo;
	hi=num >> 8;
	//low 16 bits
	lo = num & 0xff;
	//hi XOR lo
	num = hi ^ lo;
	//this will get us a number between 0 and 1
	double rand = (double)num/1000;

	return rand;
}

/* Takes the absolute value of num */
int abs(int num)
{
	if (num >= 0)
		return num;
	else
		return -num;
}

/* Draws a circle with radius "radius" pixels with the center
	at position (x,y) and fills it with color "color" */
void drawCircle(int x, int y, int radius, unsigned short color)
{
	int xPos = x;
	int yPos = y;

	//Max radius is 100 pix
	if (radius >= 100)
		radius = 100;

	for(x = -100; x < 100; x++)
	{
		for(y = -100; y < 100; y++)
		{
			//using the formula x^2+y^2=c^2
			if(x*x + y*y < radius*radius)
				drawPixel(x+xPos, y+yPos, color);
		}
	}
}

/*Draws a cursor on the screen */
void drawCursor(int x, int y)
{
	int xPos = x;
	int yPos = y;

	for(x = -20; x < 20; x++)
	{
		for(y = -20; y < 20; y++)
		{
			if(x*x + y*y < 3)
				drawPixel(x+xPos, y+yPos, WHITE);
		}
	}
}

/* reorders an array of characters in reverse; this function
   is used by itoa function */
void reverse(char *str,int len)
{
	int i=0;
	char ch;
	for(i=0;i<=(len-1)/2;i++)
	{
		ch=str[i];
		str[i]=str[len-1-i];
		str[len-1-i]=ch;
	}
}

/* takes an integer as input and converts it into an array of
   characters (a string); we mainly use this function to print
   out our variables on the screen (extremely helpful for 
   debugging purposes) */
char* itoa(unsigned int number)
{
	char *str=malloc(sizeof(char)*20);
	int negFlag=0,pos=0;
	//taking care of the negative number situations
	//(although the input of this function is an unsigned
	//integer, having the function more generalized can be
	//useful in the future)
	if(number<0)
	{
		negFlag=1;
		number=-number;
	}
	//devide and modulus applications would get us the digits (reversed order)
	while(number>0)
	{
		str[pos++]='0'+number%10;
		number=number/10;
	}
	if(negFlag)
	{
		str[pos++]='-';
	}
	str[pos]='\0';
	//reverse the order (to get the original thing)
	reverse(str,pos);
	return str;
}

/* prints an array of characters (string) on the screen */
void printStr(unsigned int x, unsigned int y, char* str)
{
	int i = 0;
	
	if(str[i] == '\0')
		drawChar (x+i, y, '0');
	
	//use a loop to draw all the chars one by one
	while(str[i] != '\0')
	{
		int ch = str[i];
		drawChar (x+i, y, ch);
		i++;
	}
}

/* Gets the color of the pixel at position (x,y) and
   returns it as an unsigned short value (color format) */
unsigned short getColor(int x, int y)
{
	//this is kind of the reverse of drawPixel() on 259macros.h
	unsigned short color;
	volatile unsigned short *pPixel;
	pPixel = getPixelAddr(x,y);
	color = *pPixel;

	return color;
}

/* Main Function */
int main( int argc, char *argv[] )
{
	//Color constants' initilizations
	BLACK = makeColour( 0, 0, 0 );
	WHITE = makeColour(63,63,63 );
	RED   = makeColour(63, 0, 0 );
	GREEN = makeColour( 0,63, 0 );
	BLUE  = makeColour( 0, 0,63 );

	//Initializations
	initInterrupts();
	initScreen();
	initChars();

	//Enable Interrupts
	enableBalloonIRQ(1000*ONE_MS, balloonISR );
	int keys_to_watch = 0x04 | 0x02 ;
	enableKeyIRQ( keys_to_watch, keyISR );


//	fillScreen(BLACK);
	int i = 0;
	welcomeMessage();

	mouse_status = resetPS2();

	int left, right;
	int lastTime;

	while(1)
	{
		//in case of losing (past 20 misses)
		if(missed >= MAX_MISS)
		{

			disableInterrupt(IRQ_COUNTER);

			fillScreen(RED);
			emptyScreen();

			printStr(10, 10, "Too many misses...");

			printStr(14, 13, "GAME OVER!");
			printStr(8, 16, "Press KEY2 to restart");
			for(i = 0; i < 30000000; i++);
//			while(missed >= 20);
//			printStr(8, 20, "HOOOOOORAY");
			enableInterrupt(IRQ_COUNTER);
			printStr(10, 10, "                  ");
	
			printStr(14, 13, "          ");
			printStr(8, 16, "                     ");
		}

		drawChar (1, 6, ' ');

		disableInterrupt(IRQ_COUNTER);
		if (*pSWITCH != 0)
		{
			if (*pSWITCH&1)
			{
				increment = 1;
				circleRad = 30;
			}
			else if (*pSWITCH&2)
			{
				increment = 2;
				circleRad = 20;

			}
			else if (*pSWITCH&4)
			{
				increment = 3;
				circleRad = 15;
			}
		}
		else
		{
			increment = 1;
			circleRad = 30;
		}		

		enableInterrupt(IRQ_COUNTER);

		/// MUSIC PLAYER ///
//		drawChar (1, 5, 16);
		if(musicCounter == 0)
		{
			left = 0;
			right = 1;
		}
		if(musicCounter < musicLength/4)
		{
			i = musicCounter;
			for(i; i<musicCounter+200; i++)
			{
				playMusic(music[left], music[right]);
				left+=2;
				right+=2;
			}
			musicCounter += 200;
		}
		else
		{
			musicCounter = 0;
			left = 0;
			right = 1;
		}
		/// MUSIC ///

		if ( mouse_status == PS2_MOUSE ) 
		{
//			drawChar (1, 5, 22);
			if( getMouseMotion( &dxMouse, &dyMouse, &mouse_button ) ) 
			{
				if( mouse_button & MOUSE_BUTTON1 )
				{
					if(getColor(xMouse+1, yMouse) == RED)
					{
						score += increment;
						drawCircle(xCircle, yCircle, 30, BLACK);
						/// SOUND EFFECT ///
						int sleft = 0;
						int sright = 1;
						int i;
						for(i=0; i<effectLength/4; i++)
						{
							playMusic(effect[sleft], effect[sright]);
							sleft+=2;
							sright+=2;
						}
						sleft = 0;
						sright = 1;
						/// SOUND EFFECT ///
					}
					else
					{
						if(time != lastTime)
						{
							missed++;
							time = lastTime;
						}
					}
				}

	/*			if( mouse_button & MOUSE_BUTTON2 )
					fillScreen( BLACK );
	
				if( mouse_button & MOUSE_BUTTON3 )
				{
					printStr(17, 13, "PAUSED!");
					disableInterrupts(IRQ_COUNTER);
					while( 1 )
					{
						if( mouse_button & MOUSE_BUTTON2 )
						{
							enableInterrupts(IRQ_COUNTER);
							printStr(17, 13, "       ");
							break;
						}
					}
				}
	*/


				if(getColor(xMouse+1, yMouse) == RED)
					dragColor = RED;
				else
					dragColor = BLACK;

				drawPixel( xMouse, yMouse, dragColor );

				xMouse += dxMouse;
				yMouse -= dyMouse;
				xMouse = max( 0, min( xMouse, MAX_X_PIXELS-1 ) );
				yMouse = max( 0, min( yMouse, MAX_Y_PIXELS-1 ) );

				drawPixel( xMouse, yMouse, WHITE );
				//drawCursor(xMouse, yMouse);
			}
		}
	}
	
	return 0;
}
