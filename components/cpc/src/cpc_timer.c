/**
 * @file cpc_timer.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-08-03
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "cpc.h"
#include "cpc_timer.h"
#include "cm3_mcu.h"

static TimerHandle_t xTimers = NULL;
static cpc_timer_callback_t g_callback = NULL;
static void *g_callback_data = NULL;

uint32_t cpc_timer_ms_to_tick(uint16_t time_ms)
{
    return time_ms;
}

uint32_t cpc_timer_tick_to_ms(uint32_t tick)
{
    return tick;
}

uint32_t cpc_timer_get_tick_count(void)
{
    return xTaskGetTickCount();
}

uint64_t cpc_timer_get_tick_count64(void)
{
    uint64_t tick = xTaskGetTickCount();

    return tick;
}

status_t cpc_timer_start_timer(cpc_timer_handle_t *handle,
                                  uint32_t timeout,
                                  cpc_timer_callback_t callback,
                                  void *callback_data)
{
    // info("%s\n", __func__);
    status_t status = CPC_STATUS_OK;

    return status;
}

status_t cpc_timer_start_timer_ms(cpc_timer_handle_t *handle,
                                     uint32_t timeout_ms,
                                     cpc_timer_callback_t callback,
                                     void *callback_data)
{
    // info("%s\n", __func__);
    status_t status = CPC_STATUS_OK;

    return status;
}

static void vTimerCallback(TimerHandle_t xTimer)
{
    // info("TCB\n");
    g_callback(NULL, g_callback_data);
}

status_t cpc_timer_restart_timer(cpc_timer_handle_t *handle,
                                    uint32_t timeout,
                                    cpc_timer_callback_t callback,
                                    void *callback_data)
{
    // info("%s handel %X timout %d\n", __func__, handle, timeout);
    status_t status = CPC_STATUS_OK;

    if (xPortIsInsideInterrupt())
    {
        xTimerChangePeriodFromISR(xTimers, timeout, NULL);
        xTimerStartFromISR(xTimers, NULL);
    }
    else
    {
        xTimerChangePeriod(xTimers, timeout, 0);
        xTimerStart(xTimers, 0);
    }

    g_callback = callback;
    g_callback_data = callback_data;

    return status;
}

status_t cpc_timer_stop_timer(cpc_timer_handle_t *handle)
{
    // info("%s\n", __func__);
    status_t status = CPC_STATUS_OK;

    if (xTimerIsTimerActive(xTimers) == pdTRUE)
        xTimerStop(xTimers, 0);

    return status;
}
void cpc_timer_init(void)
{
    if (!xTimers)
    {
        xTimers = xTimerCreate("cpc_Timer", pdMS_TO_TICKS(10), false, (void *)0, vTimerCallback);
    }
}