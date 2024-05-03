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
#include <openthread/link.h>
#define NET_MGM_ENABLED 1

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
        if (xLightTimer != NULL)
        {
            // Attempt to stop the timer
            BaseType_t result = xTimerStop(xLightTimer, portMAX_DELAY);
            if (result == pdPASS)
            {
                // Timer stopped successfully, now try to delete it
                result = xTimerDelete(xLightTimer, portMAX_DELAY);
                if (result == pdPASS)
                {
                    // Timer deleted successfully, set handle to NULL
                    xLightTimer = NULL;
                }
                else
                {
                    // Failed to delete the timer, log the error and consider retrying
                    log_error("Failed to delete timer, will retry...");
                    // Retry logic or wait before retrying could be implemented here
                }
            }
            else
            {
                // Failed to stop the timer, log the error and consider retrying
                log_error("Failed to stop timer, will retry...");
                // Retry logic or wait before retrying could be implemented here
            }
        }
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

int8_t app_get_parent_rssi()
{
    int8_t rssi = (-128);
    otRouterInfo parentInfo;
    otNeighborInfo         neighborInfo;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    otInstance *instance = otrGetInstance();
    if (otThreadGetParentInfo(instance, &parentInfo) == OT_ERROR_NONE)
    {
        while (otThreadGetNextNeighborInfo(instance, &iterator, &neighborInfo) == OT_ERROR_NONE)
        {
            if (neighborInfo.mRloc16 == parentInfo.mRloc16)
            {
                rssi = neighborInfo.mAverageRssi;
                break;
            }
        }
    }
    return rssi;
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

    p = mem_malloc(len);

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
            // app_udp_received_queue_push(p, len);
            evaluate_commandAM1(p, len);

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
        mem_free(p);
    }
}
void mac_received_cb(uint8_t *data, uint16_t lens, int8_t rssi, uint8_t *src_addr)
{
    log_info("rssi %d \r\n", rssi);
    log_debug_hexdump("Src addr : ", src_addr, 8);
    log_debug_hexdump("MacR : ", data, lens);
}
// Implementation of the RafaelRegisterTask function
void otrInitUser(otInstance *instance)
{
    ota_bootloader_info_check();
#ifdef CONFIG_NCP
    otAppNcpInit((otInstance * )instance);
#else
    static char aNetworkName[] = "Thread_RT58X";
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x00, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00};
#if NETWORKKEY_USE == 0
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = {0xfe, 0x77, 0x44, 0x8a, 0x67, 0x29, 0xfe, 0xab,
                                           0xab, 0xfe, 0x29, 0x67, 0x8a, 0x44, 0x88, 0xff
                                          };
#endif
#if NETWORKKEY_USE
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = {0x16, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x66,
                                          };
#endif
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

    radio_mac_received_callback(mac_received_cb);

    log_info("Thread version     : %s", otGetVersionString());

    log_info("Link Mode           %d, %d, %d",
             otThreadGetLinkMode(instance).mRxOnWhenIdle,
             otThreadGetLinkMode(instance).mDeviceType,
             otThreadGetLinkMode(instance).mNetworkData);
    log_info("Network name        : %s", otThreadGetNetworkName(instance));
    log_info("PAN ID              : %x", otLinkGetPanId(instance));

    log_info("channel             : %d", otLinkGetChannel(instance));

    otSetStateChangedCallback(instance, ot_stateChangeCallback, instance);

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
    Register_Timeout = 500;
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


