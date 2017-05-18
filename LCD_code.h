/* 
 * File:   LCD_code.h
 * Author: Florian de Roose
 *
 * Created on May 18, 2017, 10:02 AM
 */

#ifndef LCD_code
#define LCD_code

void LCD_cmd( char command );
void LCD_char( char command );
void initialize_LCD(void);

void LCD_clear(void);
void LCD_home(void);
void LCD_second_row(void);
void LCD_write_string(const char string[]);
void LCD_write_number(unsigned long input);
void LCD_write_number_decimal(long whole, long decimal);

#endif

