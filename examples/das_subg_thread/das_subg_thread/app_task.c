/**
 * @file app_task.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-07-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "main.h"
#include "log.h"
#include "ot_ota_handler.h"
#include "timers.h"
#include <time.h>
#define RAFAEL_REGISTER_TASK_STACK_SIZE (2 * configMINIMAL_STACK_SIZE)
static SemaphoreHandle_t    appSemHandle          = NULL;
app_task_event_t g_app_task_evt_var = EVENT_NONE;
// Declare a global variable to hold the task handle of the RafaelRegisterTask.
TaskHandle_t xRafaelRegisterTaskHandle = NULL;
TimerHandle_t xLightTimer;
void __app_task_signal(void)
{
    if (xPortIsInsideInterrupt())
    {
        BaseType_t pxHigherPriorityTaskWoken = pdTRUE;
        xSemaphoreGiveFromISR( appSemHandle, &pxHigherPriorityTaskWoken);
    }
    else
    {
        xSemaphoreGive(appSemHandle);
    }
}

void vLightTimerCallback(TimerHandle_t xTimer)
{
    if (GreenLight_Flag)
    {
        gpio_pin_write(14, 1);
        GreenLight_Flag = 0;
    }
    else
    {
        gpio_pin_write(14, 0);
        GreenLight_Flag = 1;
    }
    if (target_pos == OT_DEVICE_ROLE_CHILD || target_pos ==  OT_DEVICE_ROLE_ROUTER || target_pos == OT_DEVICE_ROLE_LEADER)
    {
        gpio_pin_write(14, 1);
        xTimerStop(xLightTimer, portMAX_DELAY);
    }
}
static void ot_stateChangeCallback(otChangedFlags flags, void *p_context)
{
    otInstance *instance = (otInstance *)p_context;
    uint8_t *p;
    if (flags & OT_CHANGED_THREAD_ROLE)
    {

        otDeviceRole role = otThreadGetDeviceRole(p_context);
        switch (role)
        {
        case OT_DEVICE_ROLE_CHILD:
            target_pos = OT_DEVICE_ROLE_CHILD;
            SuccessRole = 1;
            break;
        case OT_DEVICE_ROLE_ROUTER:
            target_pos = OT_DEVICE_ROLE_ROUTER;
            SuccessRole = 1;
#if NET_MGM_ENABLED
            nwk_mgm_child_register_post();
#endif
            break;
        case OT_DEVICE_ROLE_LEADER:
            target_pos = OT_DEVICE_ROLE_LEADER;
            SuccessRole = 1;
            break;

        case OT_DEVICE_ROLE_DISABLED:
            target_pos = OT_DEVICE_ROLE_DISABLED;
        case OT_DEVICE_ROLE_DETACHED:
            target_pos = OT_DEVICE_ROLE_DETACHED;
        default:
            break;
        }
        if (SuccessRole)
        {
            APP_EVENT_NOTIFY(EVENT_SEND_QUEUE);
        }
        if (role == OT_DEVICE_ROLE_DISABLED || role == OT_DEVICE_ROLE_DETACHED)
        {
            uint32_t timerPeriod = pdMS_TO_TICKS(200);
            if (xLightTimer == NULL)
            {
                xLightTimer = xTimerCreate("LightTimer", timerPeriod, pdTRUE, (void *)0, vLightTimerCallback);
                if (xLightTimer != NULL)
                {
                    xTimerStart(xLightTimer, 0);
                }
            }
        }
        if (role)
        {
            log_info("Current role       : %s", otThreadDeviceRoleToString(otThreadGetDeviceRole(p_context)));

            p = (uint8_t *)(otLinkGetExtendedAddress(instance)->m8);
            log_info("Extend Address     : %02x%02x-%02x%02x-%02x%02x-%02x%02x",
                     p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);

            p = (uint8_t *)(otThreadGetMeshLocalPrefix(instance)->m8);
            log_info("Local Prefx        : %02x%02x:%02x%02x:%02x%02x:%02x%02x",
                     p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);

            p = (uint8_t *)(otThreadGetLinkLocalIp6Address(instance)->mFields.m8);
            log_info("IPv6 Address       : %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                     p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);

            log_info("Rloc16             : %x", otThreadGetRloc16(instance));

            p = (uint8_t *)(otThreadGetRloc(instance)->mFields.m8);
            log_info("Rloc               : %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                     p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
        }
    }
}

static void app_udp_cb(otMessage *otMsg, const otMessageInfo *otInfo)
{
    uint8_t *p = NULL;
    bool is_multicast = 0;
    uint16_t len;

    otInstance *instance = otrGetInstance();
    char string[OT_IP6_ADDRESS_STRING_SIZE];

    otIp6AddressToString(&otInfo->mPeerAddr, string, sizeof(string));
    len = otMessageGetLength(otMsg) - otMessageGetOffset(otMsg);

    p = pvPortMalloc(len);

    do
    {
        if (p == NULL)
        {
            break;
        }

        memset(p, 0x0, len);
        otMessageRead(otMsg, otMessageGetOffset(otMsg), p, len);

        /*check is udp ack data*/
        if (memcmp(p, "ACK", sizeof(char) * 3) != 0)
        {
            /* print payload*/
            log_info("Received len     : %d ", len);
            log_info("Received ip      : %s ", string);
            log_info("Received port    : %d ", otInfo->mSockPort);
            /*process data and send to meter (p, len) */
            app_udp_received_queue_push(p, len);

            if (otInfo->mSockAddr.mFields.m8[0] == 0xff)
            {
                if (otInfo->mSockAddr.mFields.m8[1] == 0x02 ||
                        otInfo->mSockAddr.mFields.m8[1] == 0x03)
                {
                    is_multicast = true;
                }
            }

            if (!is_multicast)
            {
                memcpy((char *)p, "ACK", sizeof(char) * 3);
                app_udpSend(otInfo->mSockPort, otInfo->mPeerAddr, p, 3);
            }
        }
    } while (0);

    if (p != NULL)
    {
        vPortFree(p);
    }
}
// Implementation of the RafaelRegisterTask function
static void RafaelRegisterTask(void *pvParameters)
{
    // Wait for notification indefinitely
    for (;;)
    {
        // Check if MAA_flag is 3 or more before waiting for notification
        if (MAA_flag >= 3)
        {
            // Optionally: Log or perform any cleanup before stopping the task
            // Example: log_info("Stopping RafaelRegisterTask as MAA_flag reached 3.");

            // Delete this task
            vTaskDelete(NULL); // This will terminate the RafaelRegisterTask
        }

        // Wait for notification to execute Rafael_Register
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY))
        {
            // Upon receiving a notification, execute Rafael_Register
            Rafael_Register();
        }
    }
}


// Function to trigger notification to RafaelRegisterTask
static void TriggerRafaelRegisterCallback()
{
    if (xRafaelRegisterTaskHandle != NULL)
    {
        // Notify the RafaelRegisterTask to execute Rafael_Register function
        xTaskNotifyGive(xRafaelRegisterTaskHandle);
    }
}

// Task to check the SuccessRole variable and trigger notification
static void CheckSuccessRoleTask(void *pvParameters)
{
    const TickType_t xDelay = pdMS_TO_TICKS(1000); // Delay for 1 second to prevent continuous checking

    while (1)
    {
        if (SuccessRole == 1 && MAA_flag < 3)
        {
            // If SuccessRole is 1, trigger the RafaelRegisterTask notification
            TriggerRafaelRegisterCallback();
            // Optionally reset SuccessRole here to prevent repeated notifications
            // SuccessRole = 0; // Uncomment if you wish to reset SuccessRole after notification
        }
        else if (MAA_flag >= 3)
        {
            // Optional: Log or perform any cleanup before stopping the task
            // Example: Log_info("MAA_flag has reached 3, stopping CheckSuccessRoleTask.");

            // Stop the task from further execution
            printf("MAA_flag >= 3\r\n");
            break; // This will exit the while loop
        }

        // Delay for a period to prevent continuous checking
        vTaskDelay(xDelay);
    }
    // Optionally delete the task if it should no longer run
    vTaskDelete(NULL); // Uncomment if you want to delete the task when finished
}

void otrInitUser(otInstance *instance)
{
    ota_bootloader_info_check();
#ifdef CONFIG_NCP
    otAppNcpInit((otInstance * )instance);
#else
    static char aNetworkName[] = "Thread_RT58X";
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x00, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = {0xfe, 0x77, 0x44, 0x8a, 0x67, 0x29, 0xfe, 0xab,
                                           0xab, 0xfe, 0x29, 0x67, 0x8a, 0x44, 0x88, 0xff
                                          };
    uint8_t meshLocalPrefix[OT_MESH_LOCAL_PREFIX_SIZE] = {0xfd, 0x00, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00};
    uint8_t aPSKc[OT_PSKC_MAX_SIZE] = {0x74, 0x68, 0x72, 0x65,
                                       0x61, 0x64, 0x6a, 0x70,
                                       0x61, 0x6b, 0x65, 0x74,
                                       0x65, 0x73, 0x74, 0x00
                                      };

    otAppCliInit((otInstance * )instance);

    /*Set Network Configuration*/
    otOperationalDataset App_Dataset;
    memset(&App_Dataset, 0, sizeof(otOperationalDataset));

    otDatasetGetActive(instance, &App_Dataset);

    App_Dataset.mActiveTimestamp.mSeconds = 0;
    App_Dataset.mComponents.mIsActiveTimestampPresent = true;
#if 1 //use dataset flash 
    /* Set Channel */
    App_Dataset.mChannel = THREAD_CHANNEL;
    App_Dataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID */
    App_Dataset.mPanId = THREAD_PANID;
    App_Dataset.mComponents.mIsPanIdPresent = true;

    /* Set network key */
    memcpy(App_Dataset.mNetworkKey.m8, nwkkey, OT_NETWORK_KEY_SIZE);
    App_Dataset.mComponents.mIsNetworkKeyPresent = true;
#endif
    /* Set Extended Pan ID */
    memcpy(App_Dataset.mExtendedPanId.m8, extPanId, OT_EXT_PAN_ID_SIZE);
    App_Dataset.mComponents.mIsExtendedPanIdPresent = true;

    /* Set Network Name */
    size_t length = strlen(aNetworkName);
    memcpy(App_Dataset.mNetworkName.m8, aNetworkName, length);
    App_Dataset.mComponents.mIsNetworkNamePresent = true;

    memcpy(App_Dataset.mMeshLocalPrefix.m8, meshLocalPrefix, OT_MESH_LOCAL_PREFIX_SIZE);
    App_Dataset.mComponents.mIsMeshLocalPrefixPresent = true;

    /* Set the Active Operational Dataset to this dataset */
    otDatasetSetActive(instance, &App_Dataset);

    log_info("Thread version     : %s", otGetVersionString());

    log_info("Link Mode           %d, %d, %d",
             otThreadGetLinkMode(instance).mRxOnWhenIdle,
             otThreadGetLinkMode(instance).mDeviceType,
             otThreadGetLinkMode(instance).mNetworkData);
    log_info("Network name        : %s", otThreadGetNetworkName(instance));
    log_info("PAN ID              : %x", otLinkGetPanId(instance));

    log_info("channel             : %d", otLinkGetChannel(instance));

    otSetStateChangedCallback(instance, ot_stateChangeCallback, instance);

    // At system initialization or an appropriate location, create the RafaelRegisterTask
    xTaskCreate(RafaelRegisterTask, "RafaelRegister", RAFAEL_REGISTER_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xRafaelRegisterTaskHandle);

    // Create the CheckSuccessRoleTask at the start of your program to continuously monitor the SuccessRole
    xTaskCreate(CheckSuccessRoleTask, "CheckSuccess", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

#if OPENTHREAD_FTD
    app_sockInit(instance, app_udp_cb, THREAD_UDP_PORT);
#endif
#if NET_MGM_ENABLED
    nwk_mgm_init(instance);
    otThreadRegisterNeighborTableCallback(instance, nwk_mgm_neighbor_Change_Callback);
#endif
    ota_init(instance);
#endif
    udf_Meter_init(instance);


    otIp6SetEnabled(instance, true);
    otThreadSetEnabled(instance, true);


    now_time = mktime(&begin);
}


void app_task (void)
{
    appSemHandle = xSemaphoreCreateBinary();
    app_task_event_t event = EVENT_NONE;

    app_uart_init();

    log_info("DAS SubG Thread Init ability FTD \n");

    while (true)
    {
        if (xSemaphoreTake(appSemHandle, portMAX_DELAY))
        {
            APP_EVENT_GET_NOTIFY(event);

            /*app uart use*/
            __uart_task(event);

            /*app udp use*/
            __udp_task(event);

            /*das dlms use*/
            __das_dlms_task(event);
        }
    }
}


