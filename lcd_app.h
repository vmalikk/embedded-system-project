//------------------------------------------------------------------------------
//  Module: lcd_app.h
//  Description: High-level LCD helpers for Project 8
//------------------------------------------------------------------------------

#ifndef LCD_APP_H_
#define LCD_APP_H_

#include <stdint.h>

void lcd_print_line(uint8_t line, const char *text);
void lcd_center_line(uint8_t line, const char *text);
void lcd_clear_line(uint8_t line);
void lcd_status_waiting(const char *baud_label);
void lcd_status_received(const char *command);
void lcd_status_transmit(const char *command);
void lcd_show_baud(const char *baud_label);
void lcd_show_splash(void);

#endif // LCD_APP_H_
