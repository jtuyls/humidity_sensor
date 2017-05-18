/* 
 * File:   constants.h
 * Author: Florian de Roose
 *
 * Created on May 18, 2017, 10:01 AM
 */

// Pushbuttons
#define pb1 		RC5
#define pb1_TRIS	TRISC5
#define pb1_ANS		ANSC3  // because ANSC5 doesn't exist
#define pb1_WPU		WPUC5

#define pb2 		RC4
#define pb2_TRIS	TRISC4
#define pb2_ANS		ANSC3
#define pb2_WPU		WPUC4

#define pb3 		RC3
#define pb3_TRIS	TRISC3
#define pb3_ANS		ANSC3
#define pb3_WPU		WPUC3

// Display
#define LCD_RS		RB6
#define LCD_RS_TRIS	TRISB6
#define LCD_RS_ANS	ANSB6

#define LCD_EN		RB5
#define LCD_EN_TRIS	TRISB5
#define LCD_EN_ANS	ANSB5

#define LCD_DB4		RC2
#define LCD_DB4_TRIS	TRISC2
#define LCD_DB4_ANS	ANSC2

#define LCD_DB5		RC1
#define LCD_DB5_TRIS	TRISC1
#define LCD_DB5_ANS	ANSC1

#define LCD_DB6		RC0
#define LCD_DB6_TRIS	TRISC0
#define LCD_DB6_ANS	ANSC0

#define LCD_DB7		RA2
#define LCD_DB7_TRIS	TRISA2
#define LCD_DB7_ANS	ANSA2

// Oscillator
#define OscOn		RC6
#define OscOn_TRIS	TRISC6
#define OscOn_ANS	ANSC6

#define OscOut		RB7
#define OscOut_TRIS	TRISB7
#define OscOut_ANS	ANSB7
#define OscOut_Flag     IOCBF7
#define OscOut_Edge     IOCBP7

// LED
#define LED             RC7
#define LED_TRIS        TRISC7
#define LED_ANS         ANSC7

// Timekeeper oscillator
#define _XTAL_FREQ      25000000

// EEPROM
#define TEMPADC_LOWER 0x00
#define TEMPADC_UPPER 0x01
#define TEMPADC_TEMP  0x02

// Battery check
#define batTRIS TRISB4
#define batANS  ANSB4
#define supply  5

