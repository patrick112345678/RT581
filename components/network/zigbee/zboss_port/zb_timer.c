/**
 * @file zb_timer.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
//=============================================================================
//                Include
//=============================================================================
#include "FreeRTOS.h"
#include "timers.h"

#include "cm3_mcu.h"

#include "zb_common.h"
#include "zigbee_platform.h"
//=============================================================================
//                Private Definitions of const value
//=============================================================================

//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================

//=============================================================================
//                Private Global Variables
//=============================================================================
static TimerHandle_t zbAlarm_timerHandle = NULL;
//=============================================================================
//                Functions
//=============================================================================

static void zb_15msTimerCallback( TimerHandle_t xTimer ) 
{
    ZB_TIMER_CTX().timer++;
    ZB_NOTIFY(ZB_SYSTEM_EVENT_ALARM_MS_EXPIRED);
}

void zb_TimerInit(void)
{
    zbAlarm_timerHandle = xTimerCreate("zb_timer", 15, pdTRUE, 
            (void *)zbAlarm_timerHandle, zb_15msTimerCallback);    
}

void zb_TimerStart(void)
{
    BaseType_t pxHigherPriorityTaskWoken = pdTRUE;
    if (xPortIsInsideInterrupt())
    {
        xTimerStartFromISR( zbAlarm_timerHandle, &pxHigherPriorityTaskWoken);
    }
    else
    {
        xTimerStart(zbAlarm_timerHandle, 0 );
    }
}
void zb_TimerStop(void)
{
    BaseType_t pxHigherPriorityTaskWoken = pdTRUE;
    if (xPortIsInsideInterrupt())
    {
        if (xTimerIsTimerActive(zbAlarm_timerHandle) == pdTRUE) 
            xTimerStopFromISR( zbAlarm_timerHandle, &pxHigherPriorityTaskWoken);
    }
    else
    {
        if (xTimerIsTimerActive(zbAlarm_timerHandle) == pdTRUE) 
            xTimerStop(zbAlarm_timerHandle, 0 );
    }
}
zb_time_t osif_transceiver_time_get(void)
{
    return Timer_25us_Tick;
}
zb_time_t osif_sub_trans_timer(zb_time_t t2, zb_time_t t1)
{
    return ZB_TIME_SUBTRACT(t2, t1);
}

void osif_sleep_using_transc_timer(zb_time_t timeout)
{

}