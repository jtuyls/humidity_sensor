/* 
 * File:   initialize.c
 * Author: Florian de Roose
 *
 * Created on May 18, 2017, 10:00 AM
 */

#include "constants.h"
#include "LCD_code.h"
#include <xc.h>
void initializeTimer(void)
{
    PSA = 1; // Prescaler assigned to timer 0
    TMR0CS = 0; // Clock comes from the oscillator, also used for instructions

    PS0 = 1; // Prescaler = 256 => hit om de 40ns * 4 * 256 = 40.960 us => 25 kHz
    PS1 = 1;
    PS2 = 1;
}

void initializeADC(void)
{
	// References for the ADC
	ADPREF0 = 0;
	ADPREF1 = 0;
	ADNREF  = 0;

	//Select the justification as right
	ADFM = 1;

        TSRNG = 1; // Only for Vdd > 3.6
}

void initializeLED(void)
{
        LED      = 1; // Led off
	LED_TRIS = 0; // Output
	LED_ANS  = 0; // Digital

}

void initializeOscillator(void)
{
	OscOn_TRIS  = 0;
	OscOn_ANS   = 0;
	OscOn       = 0;

	OscOut_TRIS = 1;
	OscOut_ANS  = 0;
}

void initializeScreen(void)
{
	LCD_RS = LCD_EN = LCD_DB4 = LCD_DB5 = LCD_DB6 = LCD_DB7 = 0;
	LCD_RS_TRIS = LCD_EN_TRIS = LCD_DB4_TRIS
		        = LCD_DB5_TRIS = LCD_DB6_TRIS = LCD_DB7_TRIS = 0;
	LCD_RS_ANS = LCD_EN_ANS = LCD_DB4_ANS = LCD_DB5_ANS
               = LCD_DB6_ANS = LCD_DB7_ANS = 0;

	initialize_LCD();
}

void initializeInterrupt(void)
{
	TMR0IF = 0;     // Flag interrupt timer 0
	//TMR0IE = 1;   // Enable interrupts on timer 0
	TMR0IE = 0;     // Disable interrupts on timer 0

	TMR1IF = 0;
	TMR1IE = 1;     // Timer 1 Enable
        //TMR1IE = 0;
        TMR1GE = 0; // Timer 1 always on when TMR1ON == 1
        TMR1CS0 = 1; //Select fosc as the clock speed
        TMR1CS1 = 0;
        T1CKPS0 = 0; // No prescaler
        T1CKPS1 = 0;

	IOCIE  = 1;     // Interrupt-On-Change Enable Flag

	IOCAF  = 0x00; // port A flag
	IOCAP  = 0x00; // port A positive edge
	IOCAN  = 0x00; // port A negative edge

	IOCBF  = 0x00; // port B flag
	IOCBP  = 0x00; // port B positive edge
	IOCBN  = 0x00; // port B negative edge

        OscOut_Flag = 0;
        OscOut_Edge = 1;

	GIE    = 1; // Enable interrupts generally
	PEIE   = 1; // Periferal interrupt enable
}

void initializePB(void)
{
    // Input/output
    pb1 = pb2 = pb3 = 0;
    pb1_TRIS = pb2_TRIS = pb3_TRIS = 1; // READ
    pb1_ANS = pb2_ANS = pb3_ANS = 0;

    // Pullup
    pb1_WPU = pb2_WPU = pb3_WPU = 1;
    WPUB4 = 0;
    nWPUEN = 0;
}

void initializeBatADC(void)
{
    batTRIS = 1; // read
    batANS  = 1; // analog
}

