#include "constants.h"
#include <xc.h>

void writeLCDnibble(char nibble)
{
    if (nibble & 1)
    {   LCD_DB4 = 1;
    }else{
        LCD_DB4 = 0;
    }
    if (nibble & 2)
    {   LCD_DB5 = 1;
    }else{
        LCD_DB5 = 0;
    }
    if (nibble & 4)
    {   LCD_DB6 = 1;
    }else{
        LCD_DB6 = 0;
    }
    if (nibble & 8)
    {   LCD_DB7 = 1;
    }else{
        LCD_DB7 = 0;
    }
}

void pulse_en(void)
{
    _delay(625); // 100 탎
    LCD_EN = 1;
    _delay(625); // 100 탎
    LCD_EN = 0;
    _delay(625); // 100 탎
}

void pulse_en_long(void)
{
    _delay(12500); // 2000 탎
    LCD_EN = 1;
    _delay(12500); // 2000 탎
    LCD_EN = 0;
    _delay(12500); // 2000 탎
}

void LCD_cmd( char command )
{
    LCD_RS = 0; // LCD R/W = 0

    // Send upper nibble
    char toPort;
    toPort = (command >> 4) & 0x0F;
    writeLCDnibble(toPort);

    pulse_en_long();

    // Send lower nibble
    toPort = command & 0x0F;
    writeLCDnibble(toPort);

    pulse_en_long();

}

void LCD_char( char command )
{
    LCD_RS = 1; // LCD R/W = 1

    // Send upper nibble
    char toPort;
    toPort = (command >> 4) & 0x0F;
    writeLCDnibble(toPort);

    pulse_en();

    // Send lower nibble
    toPort = command & 0x0F;
    writeLCDnibble(toPort);

    pulse_en();
}

void initialize_LCD(void)
{
        _delay(625000); // 100 ms

    writeLCDnibble(0x3);
    pulse_en();

        _delay(125000); // 20 ms

    writeLCDnibble(0x3);
    pulse_en();

        _delay(6250); // 1 ms

    writeLCDnibble(0x3);
    pulse_en();

    writeLCDnibble(0x2);
    pulse_en();

    LCD_cmd(0x28); // function set
    LCD_cmd(0x01); // clear display
        _delay(31250); // 5 ms
    LCD_cmd(0x06); // Set entry mode to increment, display freeze

    LCD_cmd(0x0F); // Put display on, cursor on and blinking on
    LCD_cmd(0x0C); // Put display on, cursor off and blinking off
    
}

void LCD_clear(void)
{
    LCD_cmd(0x01);
}

void LCD_home(void)
{
    LCD_cmd(0x02);
}

void LCD_second_row(void)
{
    LCD_cmd(0xC0);
}

void LCD_display_cursor(void)
{
    LCD_cmd(0x0E);
}

void LCD_remove_cursor(void)
{
    LCD_cmd(0x0C);
}
void LCD_decrement_cursor(void)
{
    LCD_cmd(0x10);
}

void LCD_increment_cursor(void)
{
    LCD_cmd(0x14);
}

void LCD_write_string(const char *string)
{
    while( *string != '\0')
    {
        if(*(string)=='\n')
        {   LCD_second_row();
        }else if ( *string == '�')
        {
            LCD_char(0xDF);
        }else{
        LCD_char(*string);
        }
        string++;
    }
}

void LCD_write_number(unsigned long input)
{
    int buffer[10];
    for(char i = 0; i<10; i++)
    {
        buffer[i] = input % 10;
        input = (input - buffer[i]) /10;
    }
    int startWriting = 0;
    for(char j = 0; j<10; j++)
    {
        int towrite = buffer[9-j];
        if( towrite != 0 || startWriting)
        {
            LCD_char(towrite+0x30);
            startWriting = 1;
        }
        if(j == 9 && !startWriting)
        {   LCD_char(0x30); }
    }
}
/*
void LCD_write_number_decimal(long whole, long decimal)
{
    int buffer[10];
    for(char i = 0; i<10; i++)
    {
        buffer[i] = whole % 10;
        whole = (whole - buffer[i]) /10;
    }
    int startWriting = 0;
    for(char j = 0; j<10; j++)
    {
        int towrite = buffer[9-j];
        if( towrite != 0 || startWriting)
        {
            LCD_char(towrite+0x30);
            startWriting = 1;
        }
        if(j == 9 && !startWriting)
        {   LCD_char(0x30); }
        LCD_char(0x2D);
    }
}
*/