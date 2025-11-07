#include "messages.h"

static const char g_commands[MESSAGE_COMMAND_COUNT][MESSAGE_COMMAND_LENGTH + 1u] = {
	"SPDUP_0001",
	"SPDDN_0002",
	"TURNL_0003",
	"TURNR_0004",
	"BRAKE_0005",
	"IDLE__0006",
};

static const char g_test_string[] = "NCSU  #1";

static const char g_baud_labels[][9] = {
	"115,200",
	"  9,600",
};

static const char *const g_splash_lines[4] = {
	"   NCSU   ",
	" WOLFPACK ",
	"  ECE306  ",
	"  GP I/O  ",
};

const char *messages_get_command(uint8_t index) {
	if (index >= MESSAGE_COMMAND_COUNT) {
		return 0;
	}
	return g_commands[index];
}

const char *messages_get_test_string(void) {
	return g_test_string;
}

const char *messages_get_baud_label(uart_baud_t baud) {
	switch (baud) {
		case UART_BAUD_115200:
			return g_baud_labels[0];
		case UART_BAUD_9600:
			return g_baud_labels[1];
		default:
			return g_baud_labels[0];
	}
}

const char *messages_get_splash_line(uint8_t index) {
	if (index >= 4u) {
		return 0;
	}
	return g_splash_lines[index];
}
