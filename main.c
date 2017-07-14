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

#include "constants.h"
#include "LCD_code.h"
#include "initialize.h"
#include <math.h>
#include <xc.h>


typedef struct menuItems {
    const char *text1;

    int scrollDown;
    int scrollUp;
    int enter;

    void (*actionScrollDown)( void );
    void (*actionScrollUp)  ( void );
    void (*actionEnter)     ( void );

} menuItems;


// globals

int admin;
unsigned int TMR1overflow, counter2;
unsigned int TMRhigh, TMRlow, periodMeasured;
unsigned int tempCal, tempADC, tempMeasured;
__eeprom unsigned int tempADC_PROM, tempMeasured_PROM;
__eeprom double a, b, c, d;
__eeprom unsigned int empty;
int accessCode[3];
bit START, STOP, BUSY;

unsigned int metingen[3];

void waitForPb1 (void);
void waitForPb2 (void);
void waitForPb3 (void);

void tempCalAct(void);

void tempMeasureAct(void);

unsigned int humedadPergamino(void);
unsigned int humedadOro(void);

void checkBattery(void);

void measureCapacitance(void);     // ? void measureFrequency(void)

//void calibrate(void);

unsigned int inputTemperature(void);

void calibrateTemperature(void);

void calibrateCoffee(void);

void inputCoefficients(void);
void testCoeff(void);

void showMeasuredTemp(void);

double power(float ground, int exponent);

unsigned int calibrateWarning(void);

unsigned int testHumidity(void);

unsigned int inputHumidity(void);

int inputCodeDigits(void);

void getCoefficientsTest(void);

void getCoefficients(void);

int checkCode(int code[]);

void test(void);

menuItems menu[] = {
    {"",0,0,0,0,0,0},                                                   //0
    {"1. Bienvenido\n",2,4,1,0,0,0},                                    //1
    {"2. Medir\nCafe Pergamino",3,1,2,0,0,&humedadPergamino},           //2
    {"3. Medir\nCafe Oro",4,2,3,0,0,&humedadOro},                       //3
    {"4. Extra",1,3,5,0,0,0},                                           //4
    {"4.1 Calibrar\nTemperatura",6,10,5,0,0,&calibrateTemperature},      //5
    {"4.2 Calibrar\nCafe",7,5,6,0,0,&calibrateCoffee},                  //6
    {"4.3 Adaptar\ncoefficientes",8,6,7,0,0,&inputCoefficients},        //7
    {"4.4 Mirar\ncoefficientes",9,7,8,0,0,&getCoefficients},            //8
    {"4.5 Test",10,8,9,0,0,&test},                                          //9
    {"4.6 Regresar",5,9,4,0,0,0},                                       //10
    {"",0,0,0,0,0,0}
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

    // wordt nu niets geprint want geen waitforpb
    LCD_clear();
    LCD_write_number(TMRlow);
    LCD_write_string("\n");
    LCD_write_number(TMRhigh);
    // waitForPb2();
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

    tempMeasured = tempMeasured_PROM+(((signed int) tempADC-(signed int)tempADC_PROM)*1000)/1080;
}

void tempCalAct(void)
{
    tempADC = tempADC_exec();
    tempADC_PROM = tempADC;

    tempMeasured = tempCal;
    tempMeasured_PROM = tempMeasured;
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
                TMR1ON  = 1;
        }
        if(counter2 == 1010)
        {
                TMR1ON  = 0;
                OscOn   = 0;
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

int checkCode(int inputCode[3])
{
    if (inputCode[0] == accessCode[0] && inputCode[1] == accessCode[1] && inputCode[2] == accessCode[2])
    {   admin = 1;
        return 1;}

    return 0;
}


int inputCodeDigits(void)
{
    unsigned int codeIndex=0;
    unsigned int i;

    // Code vector and his length
    unsigned int length_code = 3;
    int inputCode[3]={0,0,0};

    char button;

    if(admin)
        {return 1; }


    for (;;)
    {
        LCD_clear();
        LCD_write_string("Codigo entrada\n");
        LCD_write_number(inputCode[0]);
        LCD_write_number(inputCode[1]);
        LCD_write_number(inputCode[2]);
        LCD_display_cursor();

        for(i=0; i<(length_code-codeIndex); i++)
        {  LCD_decrement_cursor(); }

        button = waitForPbs();

        if(button == 3)                  // Return button
            {
            if (codeIndex == 0)
                {
                LCD_remove_cursor();
                return 0;
                }
            else
                {
                codeIndex = codeIndex - 1;
                }
            }

        if(button == 2)                  // Enter button
            {
                codeIndex = codeIndex + 1;

                if (codeIndex == length_code)
                {
                    LCD_remove_cursor();

                 if (checkCode(inputCode))
                 {   LCD_clear();
                     LCD_write_string("Codigo correcto");
                     waitForPbs();

                     return 1;
                    }
                 else
                    {
                     LCD_clear();
                     LCD_write_string("Codigo no\ncorrecto");
                     waitForPbs();

                     return 0;
                    }
                }
            }

         if(button == 1)                         // Increase/forward button
            {
            inputCode[codeIndex] = (inputCode[codeIndex] + 1) % 5;
            }

         }

}

unsigned int inputTemperature(void)
{
    unsigned int temperatureIndex=0;
    unsigned int i;

    // Code vector and his length
    unsigned int length_temperature = 2;
    int temperature[2]={0,0};

    char button;

    for (;;)
    {
        LCD_clear();
        LCD_write_string("Temperatura\n");
        LCD_write_number(temperature[0]);
        LCD_write_number(temperature[1]);
        //LCD_write_string(".");
        //LCD_write_number(temperature[2]);
        LCD_write_string("ºC");
        LCD_display_cursor();

        for(i=0; i<(length_temperature - temperatureIndex + 2); i++)
        {  LCD_decrement_cursor(); }

        button = waitForPbs();

        if(button == 3)                  // Return button
            {
            if (temperatureIndex == 0)
                {
                LCD_remove_cursor();
                return 0;
                }
            else
                {
                temperatureIndex = temperatureIndex - 1;
                }
            }

        if(button == 2)                  // Enter button
            {
               temperatureIndex = temperatureIndex + 1;

                if (temperatureIndex == length_temperature)
                {
                    LCD_remove_cursor();
                    return (10*temperature[0]+temperature[1]);
                }
            }

         if(button == 1)                         // Increase/forward button
            {
            temperature[temperatureIndex] = (temperature[temperatureIndex] + 1) % 10;
            }

         }
}

void calibrateTemperature(void)
{
    if (inputCodeDigits())
    {
       tempCal = inputTemperature();
       LCD_clear();
       LCD_write_string("Temp calibrata\n");
       LCD_write_number(tempCal);
       LCD_write_string("ºC");
       waitForPbs();

       // To store the calibrated temperature in memory
       tempCalAct();
    }
}

void showMeasuredTemp(void)
{
    // Measure temperature
    // Global variable tempMeasured will be changed
    tempMeasureAct();

    LCD_clear();
    LCD_write_string("Temp medida\n");
    LCD_write_number(tempMeasured);
    LCD_write_string("ºC");
    waitForPbs();

}

double power(float ground, int exponent)
{
    float result=1;

    if (exponent>0)
    {
       for (exponent; exponent>0; exponent--)
       {
           result = result*ground;
       }
    }

    else
    {
        for (exponent; exponent<0; exponent++)
        {
            result = result/ground;
        }
    }

    return result;
            
}


void inputCoefficients(void)
{
      unsigned int coefficientIndex = 0;
      unsigned int coefficientNumber = 0;
      unsigned int i, j, k, m;

      unsigned int nb_coefficients = 4;
      unsigned int length_coefficient = 8;

       unsigned int counter = 0;

        // to store the coefficients a,b,c,d
        double temp[4]={0,0,0,0};

        // to store the coefficients and their lengths (should this be global?)
        unsigned int coefficientMatrix[4][8]={{0,0,10,0,0,0,0,0},
                                      {0,0,10,0,0,0,0,0},
                                      {0,0,10,0,0,0,0,0},
                                      {0,0,10,0,0,0,0,0}};
        

        char button;

        if (inputCodeDigits())
        {
            LCD_clear();
            LCD_write_string("Insertar\nnumeros del pc");
            waitForPbs();

            for (coefficientNumber; coefficientNumber<nb_coefficients; coefficientNumber++)
            {
                for(;;)
                {
                LCD_clear();
                LCD_write_string("Insertar n.º ");
                LCD_write_number(coefficientNumber+1);
                LCD_write_string("\n");
                for (i=0; i<length_coefficient; i++)
                    {
                        if(coefficientMatrix[coefficientNumber][i]<10){
                            LCD_write_number(coefficientMatrix[coefficientNumber][i]);
                        } else {
                            LCD_write_string(".");
                        }
                    }
                LCD_display_cursor();

                for(m=0; m<(length_coefficient-coefficientIndex); m++)
                {  LCD_decrement_cursor(); }

                button = waitForPbs();

                if(button == 3)                  // Return button
                    {
                    if (coefficientIndex == 0)
                        {
                        LCD_clear();
                        LCD_remove_cursor();
                        LCD_write_string("Imposibele\nvolver");
                        waitForPbs();
                        }
                    else
                        {
                        coefficientIndex = coefficientIndex - 1;
                        }
                    }

                if(button == 2)                  // Enter button
                    {
                        coefficientIndex = coefficientIndex + 1;

                        if (coefficientIndex == length_coefficient)
                        {
                            LCD_remove_cursor();
                            coefficientIndex = 0;
                            break;
                        }

                     }

                 if(button == 1)                         // Increase/forward button
                    {
                     if(coefficientMatrix[coefficientNumber][coefficientIndex]<10){
                        coefficientMatrix[coefficientNumber][coefficientIndex] = \
                            (coefficientMatrix[coefficientNumber][coefficientIndex] + 1) % 10;
                     }
                    }
                   }
              }

             for (j=0; j<nb_coefficients; j++)
              {
                
                 counter = 0;
                 
                 for (k=0; k<length_coefficient; k++)
                {
                     
                    if(coefficientMatrix[j][k]<10)
                    {
                        temp[j] = temp[j] + coefficientMatrix[j][k] * power(10,-(k-1-counter));
                     }
                    else
                    {
                        counter = 1;
                    }
                      
                }
              }
  
        }


            a = temp[0];
            b = temp[1];
            c = temp[2];
            d = temp[3];
            

}

unsigned int calibrateWarning(void)
{
    char button;

    LCD_clear();
    LCD_write_string("Reinicializar?\n");
    LCD_write_string("(Si/No/-)");

     for (;;)                         // Pushing button 1 doesn't have any effect
     {
        button = waitForPbs();

         if (button == 3)
        { break; }

         if (button == 2)
         {
             return 0;
         }
     }

    LCD_clear();
    LCD_write_string("Borar datos?\n");
    LCD_write_string("(Si/No/-)");

    for (;;)                         // Pushing button 1 doesn't have any effect
     {
        button = waitForPbs();

         if (button == 3)
        { break; }

         if (button == 2)
         {
             return 0;
         }
     }

    LCD_clear();
    LCD_write_string("Esta seguro?\n");
    LCD_write_string("(Si/No/-)");

    for (;;)                         // Pushing button 1 doesn't have any effect
     {
        button = waitForPbs();

         if (button == 3)
         {
             return 1;
         }

         if (button == 2)
         {
             return 0;
         }
     }
}

void calibrateCoffee(void)
{
    unsigned int measurementIndex = 1;
    unsigned char i;

    char button;

    double sumPeriod;
    double sumTemperature;

    #define M 2

    if (inputCodeDigits())
    {
        if (calibrateWarning())
        {
            LCD_clear();
            LCD_write_string("Iniciar la\ncalibracion");
            waitForPbs();

            LCD_clear();
            LCD_write_string("Medida sin cafe:\n");
            waitForPbs();
            measureFrequency();
            empty = periodMeasured;

            tempMeasureAct();

            LCD_clear();
            LCD_write_string("Inserte en pc:\n");
            LCD_write_string("Ciclos ");
            LCD_write_number(periodMeasured);
            waitForPbs();

            LCD_clear();
            LCD_write_string("Inserte en pc:\n");
            LCD_write_string("Temperatura ");
            LCD_write_number(tempMeasured);
            LCD_write_string("ºC");
            waitForPbs();

            for(;;)
            {
                LCD_clear();
                LCD_write_string("Medida con cafe:\n");
                LCD_write_string("Numero ");
                LCD_write_number(measurementIndex);
                waitForPbs();

                for(i=0; i<M; i++)
                {
                    LCD_clear();
                    LCD_write_string("Poner granos\ncafe: numero ");// granos de cafe?
                    LCD_write_number(i+1);
                    waitForPbs();

                    // flanken meten
                    measureFrequency();
                    periodMeasured = periodMeasured-empty;
                    sumPeriod = sumPeriod + periodMeasured;       // tijdelijke variabele

                    // temperatuur meten
                    tempMeasureAct();
                    sumTemperature = sumTemperature + tempMeasured;

                    LCD_clear();
                    LCD_write_string("Vaciar\n");
                    waitForPbs();
                }

                // uitmiddelen
                periodMeasured = sumPeriod/M;
                sumPeriod = 0;
                tempMeasured = sumTemperature/M;
                sumTemperature = 0;

                LCD_clear();
                LCD_write_string("Inserte en pc:\n");
                LCD_write_string("Ciclos ");
                LCD_write_number(periodMeasured);
                waitForPbs();

                LCD_clear();
                LCD_write_string("Inserte en pc:\n");
                LCD_write_string("Temperatura ");
                LCD_write_number(tempMeasured);
                LCD_write_string("ºC");
                waitForPbs();

                measurementIndex = measurementIndex + 1;

                if (measurementIndex > 3)
                    {
                     LCD_clear();
                     LCD_write_number(measurementIndex-1);
                     LCD_write_string(" medidas\nrealizadas");
                     waitForPbs();

                     LCD_clear();
                     LCD_write_string("Extra medida?\n");
                     LCD_write_string("(Si/No/-)");

                     for (;;)                         // Pushing button 1 doesn't have any effect
                     {
                         button = waitForPbs();

                         if (button == 3)
                        { break; }

                         if (button == 2)
                         {
                         inputCoefficients();
                         break;
                         }
                     }

                     if (button == 2)
                     {
                         break;
                     }
                }
            }
        }
    }

}

void getCoefficientsTest(void)
{
     LCD_clear();
     LCD_write_string("Coeff 1\n");
     LCD_write_number(a);
     waitForPbs();

     LCD_clear();
     LCD_write_string("Coeff 2\n");
     LCD_write_number(a*10000);
     waitForPbs();

     LCD_clear();
     LCD_write_string("Coeff 3\n");
     LCD_write_number(a*10000000000);
     waitForPbs();

     LCD_clear();
     LCD_write_string("Coeff 3\n");
     LCD_write_number(a*10000000000);
     waitForPbs();
   
}

void getCoefficients(void)
{
    LCD_clear();
    LCD_write_string("Primero coeff * 100000\n");
    LCD_write_number(a*100000);
    waitForPbs();

    LCD_clear();
    LCD_write_string("Segundo coeff * 100000\n");
    LCD_write_number(b*100000);
    waitForPbs();

    LCD_clear();
    LCD_write_string("Tercero coeff* 100000\n");
    LCD_write_number(c*100000);
    waitForPbs();

    LCD_clear();
    LCD_write_string("Quarto coeff * 1000000\n");
    LCD_write_number(d*100000);
    waitForPbs();
}

unsigned int humedadPergamino(void)
{
    // Coefficients a and b will be used for calibration of cafe pergamino
    // First the farmer needs to take an empty measurement
    // Then he fills the device with cafe pergamino
    // humedadPergamino will calculate the humidity

    signed long vochtigheid;
    unsigned int vochtigheidwhole;
    unsigned int vochtigheiddec;

    unsigned int emptyperiods;
    unsigned int periods;

    LCD_clear();
    LCD_write_string("Vacie medidor\n");
    waitForPb2();

    measureFrequency();
    emptyperiods = periodMeasured;
     
    LCD_clear();
    LCD_write_string("Pone pergamino\n");
    waitForPb2();

    measureFrequency();
    tempMeasureAct();
    periods = periodMeasured;

    if (periods>emptyperiods)
    {
        vochtigheid = periods-emptyperiods;
        vochtigheid = 10*(a*vochtigheid + b);
        vochtigheiddec = vochtigheid%10;
        vochtigheidwhole = vochtigheid/10;

        if (vochtigheid < 0)
        {
            LCD_clear();
            LCD_write_string("Humedad: 0.0%\n");
            LCD_write_string("Temperatura: ");
            LCD_write_number(tempMeasured);
            LCD_write_string(" C\n");
        }

        LCD_clear();
            LCD_clear();
            LCD_write_string("Humedad: ");
            LCD_write_number(vochtigheidwhole);
            LCD_write_string(".");
            LCD_write_number(vochtigheiddec);
            LCD_write_string("%\n");
            LCD_write_number(tempMeasured);
            LCD_write_string(" C - ");
            LCD_write_number(periods-emptyperiods);
    }

    else
    {
        LCD_clear();
        LCD_write_string("Error\n");
        LCD_write_string("Intenta de nuevo");
    }

    waitForPb2();

}

unsigned int humedadOro(void)
{
    // Coefficients c and d will be used for calibration of cafe pergamino
    // First the farmer needs to take an empty measurement
    // Then he fills the device with cafe oro
    // humedadOro will calculate the humidity

    signed long vochtigheid;
    unsigned int vochtigheidwhole;
    unsigned int vochtigheiddec;

    unsigned int emptyperiods;
    unsigned int periods;

    LCD_clear();
    LCD_write_string("Vacie medidor\n");
    waitForPb2();

    measureFrequency();
    emptyperiods = periodMeasured;

    LCD_clear();
    LCD_write_string("Pone oro\n");
    waitForPb2();

    measureFrequency();
    tempMeasureAct();
    periods = periodMeasured;

    if (periods>emptyperiods)
    {
        vochtigheid = periods-emptyperiods;
        vochtigheid = 10*(c*vochtigheid + d);
        vochtigheiddec = vochtigheid%10;
        vochtigheidwhole = vochtigheid/10;

        if (vochtigheid < 0)
        {
            LCD_clear();
            LCD_write_string("Humedad: 0.0%\n");
            LCD_write_string("Temperatura: ");
            LCD_write_number(tempMeasured);
            LCD_write_string(" C\n");
        }

            LCD_clear();
            LCD_write_string("Humedad: ");
            LCD_write_number(vochtigheidwhole);
            LCD_write_string(".");
            LCD_write_number(vochtigheiddec);
            LCD_write_string("%\n");
            LCD_write_number(tempMeasured);
            LCD_write_string(" C - ");
            LCD_write_number(periods-emptyperiods);
            
    }

    else
    {
        LCD_clear();
        LCD_write_string("Error\n");
        LCD_write_string("Intenta de nuevo");
    }

    waitForPb2();
}

void test(void)
{
    measureFrequency();

    LCD_clear();
    LCD_write_string("Periods: ");
    LCD_write_number(periodMeasured);
    waitForPb2();

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

    // Code om toegang te krijgen tot het kalibratieproces
    accessCode[0] = 1;
    accessCode[1] = 2;
    accessCode[2] = 3;

    // Om aan te geven of de toegangscode eenmaal correct werd ingevoerd, om herhalingen te vermijden
    admin = 0;

    LCD_clear();
    checkBattery();
    
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
            {   ( *(menu[menuIndex].actionScrollDown))(); }  // Execute the action
            if  (menu[menuIndex].scrollDown != 0)            // If you go to another menu item
            {
                menuIndex = menu[menuIndex].scrollDown;                 // Change index
        LCD_clear();                                            // Clear screen
        LCD_write_string(menu[menuIndex].text1);                // Write text 1
         //if (*menu[menuIndex].number != 0)                       // If there is a number to be written
      // {   LCD_write_number(*menu[menuIndex].number); }        // Write the number
        //      LCD_write_string(menu[menuIndex].text2);                // Write text 2
         }
    }
        if(button == 3) // scrollUp
    {
            if(  menu[menuIndex].actionScrollUp != 0 )
            {   ( *(menu[menuIndex].actionScrollUp))(); }
            if  (menu[menuIndex].scrollUp != 0)
            {
                menuIndex = menu[menuIndex].scrollUp;

        LCD_clear();
        LCD_write_string(menu[menuIndex].text1);
        //if (*menu[menuIndex].number != 0)
        //{   LCD_write_number(*menu[menuIndex].number); }
          //      LCD_write_string(menu[menuIndex].text2);
            }
    }
    if(button == 2) // enter
    {
            if(  menu[menuIndex].actionEnter != 0 )
            {   ( *(menu[menuIndex].actionEnter))(); }
            if( menu[menuIndex].enter != 0)
            {
        menuIndex = menu[menuIndex].enter;
        LCD_clear();
        LCD_write_string(menu[menuIndex].text1);
        //if (*menu[menuIndex].number != 0)
        //{   LCD_write_number(*menu[menuIndex].number); }
        //LCD_write_string(menu[menuIndex].text2);
            }
    }

    }


}