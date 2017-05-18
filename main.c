/* 
 * File:   main.c
 * Author: Florian de Roose, Thomas Valkaert, Jorn Tuyls
 *
 * Created on May 18, 2017, 10:03 AM
 */


#include "constants.h"
#include "LCD_code.h"
#include "initialize.h"
#include <xc.h>

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1
#pragma config FOSC = HS    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = ON   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF       // PLL Enable (4x PLL enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

typedef struct menuItems {
	const char *text1;
	unsigned int *number;
	const char *text2;
	int scrollDown;
        int scrollUp;
	int enter;
	void (*actionScrollDown)( void );
        void (*actionScrollUp)  ( void );
	void (*actionEnter)     ( void );

} menuItems;

unsigned int TMR1overflow, counter2;
unsigned int TMRhigh, TMRlow, periodMeasured;
unsigned int tempCal, tempCal10, tempADC, tempMeasured;
__eeprom unsigned int tempADC_PROM, tempMeasured_PROM;
__eeprom unsigned int period0p, period100p;
bit START, STOP, BUSY;

void waitForPb1 (void);
void waitForPb2 (void);
void waitForPb3 (void);

void tempCalUp10(void);
void tempCalDown10(void);
void tempCalUp(void);
void tempCalDown(void);
void tempCalAct(void);

void tempMeasureAct(void);

void testFun(void);

void checkBattery(void);

void measureCapacitance(void);
void capacitanceCap0p(void);
void capacitanceCap100p(void);

void calibrate(void);

unsigned int divide2(unsigned long dividend, unsigned int divisor);

unsigned int offset;
unsigned int rico;
unsigned int empty;

menuItems menu[] = {
    {"",0,"",0,0,0,0,0,0},                                                  //0
    {"1. Bienvenida\n",0,"",2,4,1,0,0,&testFun},                            //1
    {"2. Medir",0,"",3,1,5,0,0,0,},                                       //2
    {"3. Calibrar",0,"",4,2,8,0,0,0},                                      //3
    {"4. Battery check",0,"",1,3,4,0,0,&checkBattery},                      //4

    {"2.1 Measure\n capacitor",0,"",6,7,5,0,0,&measureCapacitance},         //5
    {"2.2 Measure\n temperature",0,"",7,5,14,0,0,&tempMeasureAct},          //6
    {"2.3 Return",0,"",5,6,1,0,0,0},                                        //7

    {"3.1 Calibrate\n without cap",0,"",9,11,8,0,0,&calibrate},             //8 &capacitanceCap0p
    {"3.2 Calibrate\n with 100 pF cap",0,"",10,8,9,0,0,&capacitanceCap100p},//9
    {"3.3 Calibrate\n temperature",0,"",11,9,12,0,0,0},                     //10
    {"3.4 Return",0,"",8,10,1,0,0,0},                                       //11

    {"Temperature\n  ",&tempCal10,"XºC",12,12,13,&tempCalUp10,&tempCalDown10,0},        //12
    {"Temperature\n  ",&tempCal  ,"ºC" ,13,13,1,&tempCalUp,&tempCalDown,&tempCalAct},   //13
    //{"Measurement\n ",&tempADC,"",1,14,14,0,0,0}

    {"The temperature\nis ",&tempMeasured,"ºC",14,14,6,0,0,0},              //14
    {"",0,"",0,0,0,0,0,0}
};

void measureFrequency(void)
{
    LED =!LED;
    TMR1 = 0;
    counter2 = 0; // External CCO (capacity controlled oscillator) cycle count
    TMR1overflow = 0;
    OscOn = 1;

    /*
     From here on the oscillator starts running and will place an interrupt
     every positive edge. This will increase the value of counter2. As soon as
     the oscillator had 10 successful oscillations, we enable the timer 1.
     We then wait for x oscillations to turn the timer back off again. As such
     we also put the oscillator off.

     To increase the range of TMR1, an extra overflow variable TMR1overflow has
     been created. This is also increased by interrupt.
    */

    while(OscOn==1); // Wait until the interrupts have done their job

    // Ik moet een soort van auto-averaging toevoegen, die enkele metingen doet, en checkt of ze wel dicht genoeg bij elkaar liggen. Nog niet gebeurd

    TMRhigh = TMR1overflow;
    TMRlow = TMR1;

    unsigned int TMRlow_1 = TMRlow >> 9;
    unsigned int TMRhigh_1 = TMRhigh << 7;
    periodMeasured = TMRlow_1 + TMRhigh_1;
    LED =!LED;
}


unsigned int divide2(unsigned long dividend, unsigned int divisor)
{
    unsigned long divisor2 = divisor;

    if (dividend < divisor2)
        {return 0;}

    unsigned int result = 0;
    while(1)
    {
        char i = 0; // calculate the index of the first significant bit of the quotient
        while(i<16){
            if(dividend >= (divisor2 << i) & dividend < (divisor2 << (i+1)))
            {break;}
            i++;
        }
        if( i !=16)
        {
            result += 1<<i; //writing new digit of quotient
            dividend = dividend - (divisor2 << i);
        }
        else
        {    return result;}
    }

    return 666;
}

void testFun(void)
{
    unsigned long vochtigheid;
    unsigned int vochtigheidwhole;
    unsigned int vochtigheiddec;

    measureFrequency();

    if (periodMeasured>19000)
    {
        vochtigheid=periodMeasured-19000;
        vochtigheid=divide2(vochtigheid*10,913)+112;
        vochtigheiddec=vochtigheid%10;
        vochtigheidwhole=vochtigheid/10;

        LCD_clear();
        LCD_write_number(periodMeasured);
        LCD_write_string("\n");
        LCD_write_number(vochtigheidwhole);
        LCD_write_string(".");
        LCD_write_number(vochtigheiddec);
        //LCD_write_string(" ");
        //LCD_write_number(TMRlow);
    }
    else
    {
        LCD_clear();
        LCD_write_number(periodMeasured);
        LCD_write_string("\n");
        LCD_write_string("damiaan = banaan");
    }

    waitForPb2();
}

void measureCapacitance(void)
{
    measureFrequency();
    LCD_clear();
    LCD_write_string("Edges: ");
    LCD_write_number(periodMeasured);
    LCD_write_string("\n");

#define accuracy 100

    unsigned int cap0p = 0;
    unsigned int cap100p = 100; // Alleen gehele getallen => Wrs prescalen en descalen nodig,
        // Range is tussen 0 en 65535

    unsigned int x  = periodMeasured;
    unsigned int x1 = period0p;
    unsigned int x2 = period100p;
    unsigned int y1 = cap0p;
    unsigned int y2 = cap100p;

    if (x2 <= x1){
        LCD_write_string("ERROR");
        waitForPb2();
    }
    if (x <= x1){
        LCD_write_string("ERROR");
        waitForPb2();
    }
    if (y2 <= y1){
        LCD_write_string("ERROR");
        waitForPb2();
    }

    // Convention: minuscule = integer (2 byte), capital = long (4 byte)

    unsigned int deltax2x1 = x2 - x1;
    unsigned long deltaX2X1 = deltax2x1;
    unsigned int deltay2y1 = y2 - y1;
    unsigned long deltaY2Y1 = deltay2y1;
    unsigned int deltaxx1 = x - x1;
    unsigned long deltaXX1 = deltaxx1;

    unsigned long deltaMult = deltaXX1 * deltaY2Y1 * accuracy;

    unsigned int deltayy1 = divide2(deltaMult, deltax2x1);
    unsigned int y = y1 + deltayy1;

    unsigned int y_whole =  y / accuracy;
    unsigned int y_fraction = y - accuracy*y_whole;

    LCD_write_number(y_whole);
    LCD_write_string(".");
    LCD_write_number(y_fraction);
    LCD_write_string(" pF");

    waitForPb2();
}

void capacitanceCap100p(void)
{
    measureFrequency();
    period100p = periodMeasured;

    LCD_clear();
    LCD_write_string("Edges: ");
    LCD_write_number(period100p);
    LCD_write_string("\nDone");
    waitForPb2();
}

void capacitanceCap0p(void)
{
    measureFrequency();
    period0p = periodMeasured;

    LCD_clear();
    LCD_write_string("Edges: ");
    LCD_write_number(period0p);
    LCD_write_string("\nDone");
    waitForPb2();
}

unsigned int tempADC_exec(void)
{
    ADON = 1; // Boot the ADC

    CHS4 = 1; // Select temperature module
    CHS3 = 1;
    CHS2 = 1;
    CHS1 = 0;
    CHS0 = 1;

    TSEN = 1; // Enable temperature sensor

    _delay(125000); // 20 ms

    ADGO = 1;
    while(ADGO);

    TSEN = 0;
    ADON = 0;

    return ADRES; // A-to-D result
}

unsigned int battADC_exec(void)
{
    ADON = 1; // Boot the ADC

    CHS4 = 0; // Select temperature module
    CHS3 = 1;
    CHS2 = 0;
    CHS1 = 1;
    CHS0 = 0;

    _delay(125000); // 20 ms

    ADGO = 1;
    while(ADGO);

    ADON = 0;

    return ADRES;
}

void tempMeasureAct(void)
{
    tempADC = tempADC_exec();

    tempMeasured = tempMeasured_PROM+((tempADC-tempADC_PROM)*1000)/1080;
}

void tempCalAct(void)
{
    tempADC = tempADC_exec();
    tempADC_PROM = tempADC;

    tempMeasured = tempCal;
    tempMeasured_PROM = tempMeasured;
}

void tempCalUp10(void)
{
    if (tempCal < 30)
    {
        tempCal = tempCal+10;
    }
    tempCal10 = tempCal/10;
}
void tempCalDown10(void)
{
    if (tempCal > 19)
    {
        tempCal = tempCal-10;
    }
    tempCal10 = tempCal/10;
}
void tempCalUp(void)
{
    if (tempCal < 43)
    {
        tempCal = tempCal+1;
    }
    tempCal10 = tempCal/10;
}
void tempCalDown(void)
{
    if (tempCal > 10)
    {
        tempCal = tempCal-1;
    }
    tempCal10 = tempCal/10;
}


void checkBattery(void)
{
    unsigned int adcOut = battADC_exec(); // Measure ADC output for battery
    unsigned int temp = adcOut*supply;    //
    unsigned int batteryInt = temp >> 9;  // The integral part: divide by 10 bit, times 2 (resistive divider)
    temp = temp & 511;                     //Choose the remaining bits which haven't been converted yet
    temp = temp*100;
    temp = temp/512;

    if(temp>100)                           // Useless error checking
    {
        LCD_clear();
        LCD_write_string("ERROR 1");
    }
    if( temp%10 > 4){                      // Rounding the last digit
        temp = temp/10 + 1;
    } else {
        temp = temp/10;
    }

    LCD_clear();
    LCD_write_string("Battery Voltage is\n ");
    LCD_write_number(batteryInt);
    LCD_write_string(".");
    LCD_write_number(temp);
    LCD_write_string("V");

    waitForPb2();
}

void interrupt ISR(void)
{
    if (OscOut_Flag && OscOut_Edge) // oscillator
    {
        if(counter2 == 10)
        {
                TMR1ON	= 1;
        }
        if(counter2 == 1010)
        {
                TMR1ON 	= 0;
                OscOn	= 0;
        }
        counter2++;  // CCO oscillator counter increment
        OscOut_Flag = 0;
    }


    if (TMR1IF && TMR1IE )
    {
	TMR1overflow++;
	TMR1IF = 0;  // Interrupt Flag
    }
}


void waitForPb1 (void)
{
    while(pb1 != 1);
    while( 1 ){
        if(pb1 == 0)
        {
            _delay(150000); // 24 ms
            if(pb1 == 0)
            {
                _delay(150000); // 24 ms
                if(pb1 == 0)
                {
                    break;
                }
            }
        }
    }
}

void waitForPb2 (void)
{
    while(pb2 != 1);

    while( 1 ){
        if(pb2 == 0)
        {
            _delay(150000); // 24 ms
            if(pb2 == 0)
            {
                _delay(150000); // 24 ms
                if(pb2 == 0)
                {
                    break;
                }
            }
        }
    }
}

void waitForPb3 (void)
{
    while(pb3 != 1); // Wait until pb3 is released
    while( 1 ){
        if(pb3 == 0)
        {
            _delay(150000); // 24 ms
            if(pb3 == 0)
            {
                _delay(150000); // 24 ms
                if(pb3 == 0)
                {
                    break;
                }
            }
        }
    }
}

char waitForPbs(void)
{
    while( !(pb1 && pb2 && pb3)); // Wait till all buttons are released

    while( 1 ){
        if(pb1 == 0) // Button debouncing
        {
            _delay(150000); // 24 ms
            if(pb1 == 0)
            {
                _delay(150000); // 24 ms
                if(pb1 == 0)
                {
                    return 1;
                }
            }
        }
        if(pb2 == 0)
        {
            _delay(150000); // 24 ms
            if(pb2 == 0)
            {
                _delay(150000); // 24 ms
                if(pb2 == 0)
                {
                    return 2;
                }
            }
        }
        if(pb3 == 0)
        {
            _delay(150000); // 24 ms
            if(pb3 == 0)
            {
                _delay(150000); // 24 ms
                if(pb3 == 0)
                {
                    return 3;
                }
            }
        }
    }

    return 0;
}

void calibrate()
{
    /*
     * y is de vochtigheid
     * x is het aantal flanken
     */
    unsigned int sumx;
    unsigned int sumxy;
    unsigned int sumy;
    unsigned int sumxx;
    unsigned char vochtigheid;
    unsigned char i;

    unsigned long vochtdec;
    unsigned long vochtwhole;

    sumx=0;
    sumxy=0;
    sumy=0;
    sumxx=0;

    // lege meting
    LCD_clear();
    LCD_write_string("meet met lege bak");
    waitForPbs();
    measureFrequency();
    empty = periodMeasured;

    // volle metingen
#define N 3
    for(i=0; i<N; i++)
    {
        // flanken bepalen
        LCD_clear();
        LCD_write_string("meet met bonen");
        waitForPbs();
        measureFrequency();

        // vochtigheid bepalen
        vochtwhole=12;
        vochtdec=0;

        for(;;)
        {
            LCD_clear(); // Dit kan beter via menustructuur, maar veel werk, eerst zo testen
            LCD_write_string("vochtigheid");
            LCD_write_number(vochtwhole);
            LCD_write_string(".");
            LCD_write_number(vochtdec);

            char button = waitForPbs();
            if(button == 1)
            {
                vochtdec = vochtdec - 1;
                if (vochtdec > 9)
                {
                    vochtdec = 9;
                    if (vochtwhole>10)
                    {
                        vochtwhole = vochtwhole - 1;
                    }
                }
            }

            if(button == 2)
            {
                break;
            }

            if(button == 3)
            {
                vochtdec = vochtdec + 1;
                if (vochtdec > 9)
                {
                    vochtdec = 0;
                    if (vochtwhole<18);
                    {
                        vochtwhole = vochtwhole + 1;
                    }
                }
            }
        }

        vochtigheid = (10 * vochtwhole) + vochtdec; // belangrijke conversie !!!!!!!!!!!!!!!
        periodMeasured = periodMeasured-empty;         // belangrijke conversie !!!!!!!!!!!!!!!

        sumx = sumx + periodMeasured;
        sumxy = sumxy + (periodMeasured * vochtigheid);
        sumy = sumy + vochtigheid;
        sumxx = sumxx + (periodMeasured * periodMeasured); /// TODO te groot getal
    }

    sumxy = sumxy - (sumx * sumy);
    sumxx = sumxx - (sumx * sumx);

    rico = divide2(sumxy, sumxx);
    offset = sumy - (rico * sumx); // gaat negatief zijn??? altijd? ONDERZOEKEN!!!
}

main()
{
    initializeLED();
    initializePB();
    initializeScreen();
    initializeOscillator();
    initializeInterrupt();
    initializeTimer();
    initializeADC();
    initializeBatADC();

    char menuIndex = 1;  // Location in menu
    tempCal = 20;
    tempCal10 = 2;

    LCD_clear();
    LCD_write_string(menu[menuIndex].text1);

    ADCS2 = 1;
    ADCS1 = 1;
    ADCS0 = 0;

    for(;;){
        char button = waitForPbs();
	if(button == 1) // scrollDown
	{
            if(  menu[menuIndex].actionScrollDown != 0 )     // If there is an action associated
            {	( *(menu[menuIndex].actionScrollDown))(); }  // Execute the action
            if	(menu[menuIndex].scrollDown != 0)            // If you go to another menu item
            {
                menuIndex = menu[menuIndex].scrollDown;                 // Change index
		LCD_clear();                                            // Clear screen
		LCD_write_string(menu[menuIndex].text1);                // Write text 1
		if (*menu[menuIndex].number != 0)                       // If there is a number to be written
		{   LCD_write_number(*menu[menuIndex].number); }        // Write the number
                LCD_write_string(menu[menuIndex].text2);                // Write text 2
            }
	}
        if(button == 3) // scrollUp
	{
            if(  menu[menuIndex].actionScrollUp != 0 )
            {	( *(menu[menuIndex].actionScrollUp))(); }
            if	(menu[menuIndex].scrollUp != 0)
            {
                menuIndex = menu[menuIndex].scrollUp;

		LCD_clear();
		LCD_write_string(menu[menuIndex].text1);
		if (*menu[menuIndex].number != 0)
		{   LCD_write_number(*menu[menuIndex].number); }
                LCD_write_string(menu[menuIndex].text2);
            }
	}
	if(button == 2) // enter
	{
            if(  menu[menuIndex].actionEnter != 0 )
            {	( *(menu[menuIndex].actionEnter))(); }
            if( menu[menuIndex].enter != 0)
            {
		menuIndex = menu[menuIndex].enter;
		LCD_clear();
		LCD_write_string(menu[menuIndex].text1);
		if (*menu[menuIndex].number != 0)
		{	LCD_write_number(*menu[menuIndex].number); }
		LCD_write_string(menu[menuIndex].text2);
            }
	}

    }


}

