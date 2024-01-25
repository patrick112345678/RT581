/**
 * @file zb_freertos.c
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
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "zigbee_platform.h"
#include "zboss_api.h"
#include "zb_common.h"
#include "zb_mac_globals.h"
#include "log.h"

//=============================================================================
//                Private Definitions of const value
//=============================================================================
#define ZB_TRACE_FILE_ID 294
//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================

//=============================================================================
//                Private Global Variables
//=============================================================================
zb_system_event_t           zb_system_event_var = ZB_SYSTEM_EVENT_NONE;
static SemaphoreHandle_t    zb_extLock          = NULL;
static TaskHandle_t         zb_taskHandle       = NULL;
uint32_t             zboss_start_run     = 0;

//=============================================================================
//                Functions
//=============================================================================
void zb_reset(zb_uint8_t param)
{

}

void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);
    zb_ret_t z_ret = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_signal_device_annce_params_t *dev_annce_params;

    log_info(">>zdo_signal_handler: status %d signal %d\n", z_ret, sig);
    do
    {
        if (z_ret != 0)
        {
            break;
        }
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
            zboss_start_continue();
            break;

        case ZB_BDB_SIGNAL_STEERING:
            log_info("Successfull steering, start f&b target\n");
            break;

        case ZB_COMMON_SIGNAL_CAN_SLEEP:
        {

        }
        break;

        case ZB_NLME_STATUS_INDICATION:
        {

        }
        break;

        default:
            break;
        }
    } while (0);


    if (sig == ZB_BDB_SIGNAL_DEVICE_FIRST_START || sig == ZB_BDB_SIGNAL_STEERING)
    {
        if (z_ret == 0 && sig == ZB_BDB_SIGNAL_DEVICE_FIRST_START)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
    }
    else if (sig == ZB_BDB_SIGNAL_DEVICE_REBOOT)
    {

    }

    if (param)
    {
        zb_buf_free(param);
    }
}

void zbSysEventSignalPending(void)
{
    if (xPortIsInsideInterrupt())
    {
        BaseType_t pxHigherPriorityTaskWoken = pdTRUE;
        vTaskNotifyGiveFromISR( zb_taskHandle, &pxHigherPriorityTaskWoken);
    }
    else
    {
        xTaskNotifyGive(zb_taskHandle);
    }
}

void zbLock(void)
{
    if (zb_extLock)
    {
        xSemaphoreTake(zb_extLock, portMAX_DELAY);
    }
}

void zbUnlock(void)
{
    if (zb_extLock)
    {
        xSemaphoreGive(zb_extLock);
    }
}

static void zbSysProcessDrivers(void)
{
    zb_system_event_t sevent = ZB_SYSTEM_EVENT_NONE;

    ZB_GET_NOTIFY(sevent);
    zb_radioTask(sevent);
}
static void zbStackTask(void *aContext)
{
    zigbee_app_init();

    zb_TimerStart();

    while (!zb_sheduler_is_stop())
    {
        ZB_THREAD_SAFE (
            zbSysProcessDrivers();
            if (zboss_start_run)
    {
        zboss_main_loop_iteration();
        }
        );

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    vTaskDelete(NULL);
}

void zbStartRun(void)
{
    zboss_start_run = 1;
    zboss_start();
    ZB_APP_NOTIFY(ZB_SYSTEM_EVENT_APP);
}

void zbStart(void)
{
    static StackType_t  zb_stackTask_stack[ZB_TASK_SIZE];
    static StaticTask_t zb_task;
    static StaticQueue_t stackLock;

    zb_extLock = xSemaphoreCreateMutexStatic(&stackLock);
    configASSERT(zb_extLock != NULL);
    ZB_THREAD_SAFE (
        ZB_INIT("RafaelZBee");
        zb_taskHandle = xTaskCreateStatic(zbStackTask, "ZigBeeTask", ZB_TASK_SIZE, NULL, ZB_TASK_PRORITY, zb_stackTask_stack, &zb_task);
    );


}