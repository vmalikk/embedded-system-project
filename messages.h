//------------------------------------------------------------------------------
//  Module: messages.h
//  Description: Shared string resources for Project 8
//------------------------------------------------------------------------------

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <stdint.h>
#include "uart.h"

#define MESSAGE_COMMAND_COUNT   (6u)
#define MESSAGE_COMMAND_LENGTH  (10u)

const char *messages_get_command(uint8_t index);
const char *messages_get_test_string(void);
const char *messages_get_baud_label(uart_baud_t baud);
const char *messages_get_splash_line(uint8_t index);

#endif // MESSAGES_H_
