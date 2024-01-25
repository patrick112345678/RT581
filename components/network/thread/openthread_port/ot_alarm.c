/**
 * @file ot_alarm.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-07-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "openthread-system.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <openthread/config.h>
#include <openthread/thread.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/diag.h>
#include <openthread/link.h>
#include <openthread_port.h>
#include "common/logging.hpp"


#include "utils/code_utils.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "cm3_mcu.h"

static TimerHandle_t otAlarm_timerHandle = NULL;
static uint32_t      otAlarm_offset = 0;

static void otPlatALarm_msTimerCallback( TimerHandle_t xTimer ) 
{
    OT_NOTIFY(OT_SYSTEM_EVENT_ALARM_MS_EXPIRED);
}

void ot_alarmInit(void) 
{
    otAlarm_timerHandle = xTimerCreate("ot_timer", 1, pdFALSE, (void *)otAlarm_timerHandle, otPlatALarm_msTimerCallback);
}

void ot_alarmTask(ot_system_event_t sevent)
{
    if (!(OT_SYSTEM_EVENT_ALARM_ALL_MASK & sevent)) 
    {
        return;
    }

    if (OT_SYSTEM_EVENT_ALARM_MS_EXPIRED & sevent) 
    {
        otPlatAlarmMilliFired(otrGetInstance());
    }

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    if (OT_SYSTEM_EVENT_ALARM_US_EXPIRED & sevent) 
    {
        otPlatAlarmMicroFired(otrGetInstance());
    }
#endif
}

uint32_t otPlatTimeGetXtalAccuracy(void)
{
    return SystemCoreClock;
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    BaseType_t ret;

    uint32_t elapseTime = otPlatAlarmMilliGetNow() - aT0;

    if (elapseTime < aDt) {
        ret = xTimerChangePeriod( otAlarm_timerHandle, pdMS_TO_TICKS(aDt - elapseTime), 0 );
        configASSERT(ret == pdPASS);

        return;
    }

    OT_NOTIFY(OT_SYSTEM_EVENT_ALARM_MS_EXPIRED);
}

void otPlatAlarmMilliStop(otInstance *aInstance) 
{
    if (xTimerIsTimerActive(otAlarm_timerHandle) == pdTRUE) {
        xTimerStop(otAlarm_timerHandle, 0 );
    }
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return xTaskGetTickCount() * portTICK_RATE_MS;
}

uint64_t otPlatTimeGet(void)
{
    return (uint64_t)(Timer_25us_Tick*25);
}
