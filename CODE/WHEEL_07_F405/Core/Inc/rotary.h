#define MUXSW_COUNT 1
#define MUXSW_STATE_COUNT 14
#define MUXSW_NOTIFICATION_PERIOD 1000
//#define MUXSW_NOTIFICATION_ENABLED 1

/*!< Possible MUXSW states */
typedef enum {
    /* ! This order must be unchanged ! begin */
    SW_POSITION_1           = 0U,
    SW_POSITION_2           = 1U,
    SW_POSITION_3           = 2U,
    SW_POSITION_4           = 3U,
    SW_POSITION_5           = 4U,
    SW_POSITION_6           = 5U,
    SW_POSITION_7           = 6U,
    SW_POSITION_8           = 7U,
    SW_POSITION_9           = 8U,
    SW_POSITION_10          = 9U,
    SW_POSITION_11          = 10U,
    SW_POSITION_12          = 11U,
    SW_UNKNOWN              = 12U,
    SW_INVALID              = 13U,
} muxswStatesEnum;


/**
* @brief Function implementing the muxswTask thread.
* @param argument: Not used
* @retval None
*/
void muxswTaskStart(void *argument);
void muxswNotifyTaskStart(void *argument);
