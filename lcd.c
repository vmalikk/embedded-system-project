//------------------------------------------------------------------------------
//  Module: lcd.c
//  Description: High-level LCD helpers for Project 8
//------------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "lcd_app.h"
#include "LCD.h"
#include "display.h"
#include "messages.h"

#ifndef LCD_COLUMNS
#define LCD_COLUMNS (10u)
#endif

extern volatile unsigned char display_changed;
extern char display_line[4][11];

static uint8_t lcd_line_index(uint8_t line) {
    if (line == 0u) {
        return 0u;
    }
    if (line > 4u) {
        return 3u;
    }
    return (uint8_t)(line - 1u);
}

static void lcd_copy_to_line(uint8_t line_index, const char *text, uint8_t center) {
    char buffer[11];
    memset(buffer, ' ', sizeof(buffer));
    buffer[10] = '\0';

    if (text) {
        size_t len = strlen(text);
        if (len > LCD_COLUMNS) {
            len = LCD_COLUMNS;
        }
        size_t start = 0u;
        if (center && len < LCD_COLUMNS) {
            start = (LCD_COLUMNS - len) / 2u;
        }
        memcpy(&buffer[start], text, len);
    }

    memcpy(display_line[line_index], buffer, sizeof(buffer));
    display_changed = 1u;
}

void lcd_print_line(uint8_t line, const char *text) {
    lcd_copy_to_line(lcd_line_index(line), text, 0u);
}

void lcd_center_line(uint8_t line, const char *text) {
    lcd_copy_to_line(lcd_line_index(line), text, 1u);
}

void lcd_clear_line(uint8_t line) {
    lcd_copy_to_line(lcd_line_index(line), "", 0u);
}

void lcd_status_waiting(const char *baud_label) {
    lcd_center_line(1u, "Waiting");
    lcd_clear_line(2u);
    lcd_show_baud(baud_label);
    lcd_clear_line(4u);
}

void lcd_status_received(const char *command) {
    lcd_center_line(1u, "Received");
    lcd_clear_line(2u);
    if (command) {
        lcd_print_line(4u, command);
    } else {
        lcd_clear_line(4u);
    }
}

void lcd_status_transmit(const char *command) {
    lcd_center_line(1u, "Transmit");
    if (command) {
        lcd_print_line(2u, command);
    } else {
        lcd_clear_line(2u);
    }
    lcd_clear_line(4u);
}

void lcd_show_baud(const char *baud_label) {
    if (baud_label) {
        lcd_center_line(3u, baud_label);
    } else {
        lcd_clear_line(3u);
    }
}

void lcd_show_splash(void) {
    uint8_t i;
    for (i = 0u; i < 4u; i++) {
        const char *line = messages_get_splash_line(i);
        if (line) {
            lcd_center_line((uint8_t)(i + 1u), line);
        } else {
            lcd_clear_line((uint8_t)(i + 1u));
        }
    }
}
