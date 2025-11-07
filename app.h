#ifndef APP_H_
#define APP_H_

typedef enum {
	APP_STATE_WAITING = 0,
	APP_STATE_RECEIVED,
	APP_STATE_TRANSMIT,
} app_state_t;

void app_init(void);
void app_process(void);

#endif // APP_H_
