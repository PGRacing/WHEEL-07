#define UBUTTONS_COUNT 3
#define UBUTTONS_DEBOUNCE_THRESHOLD 20
#define UBUTTONS_NOTIFICATION_PERIOD 1000
#define UBUTTONS_PRESS_NOTIFICATION_PERIOD 100
#define UBUTTONS_NOTIFICATION_ENABLED 1

/* FreeRTOS buttons task */
void ubuttonsTaskStart(void* arguments);