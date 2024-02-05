/**
 * @file app_udp.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-10-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <openthread/udp.h>
#include <openthread/thread.h>
#include <openthread_port.h>

#include "FreeRTOS.h"

#include <main.h>
#include "log.h"

static otUdpSocket          appSock;
static void (*app_udpHandler) (otMessage *, const otMessageInfo *);
static uint16_t appUdpPort = THREAD_UDP_PORT;

static void otUdpReceive_handler(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    log_info("UPD Packet received, port: %d, len: %d", aMessageInfo->mPeerPort, otMessageGetLength(aMessage));

    if (app_udpHandler)
    {
        app_udpHandler(aMessage, aMessageInfo);
    }

    otMessageFree(aMessage);

}

otError app_udpSend(uint16_t PeerPort, otIp6Address PeerAddr, uint8_t *data, uint8_t data_lens)
{
    otError error = OT_ERROR_NONE;
    otMessage *message = NULL;
    otMessageInfo messageInfo;
    otMessageSettings messageSettings = {true, OT_MESSAGE_PRIORITY_NORMAL};
    char *buf = NULL;
    printf("send 1 \r\n");
    do
    {
        printf("send 2 \r\n");
        memset(&messageInfo, 0, sizeof(messageInfo));

        messageInfo.mPeerPort = PeerPort;
        messageInfo.mPeerAddr = PeerAddr;
        printf("send 3 \r\n");
        otInstance *instance = otrGetInstance();
        printf("send 4 \r\n");
        if (instance)
        {
            printf("send 5 \r\n");
            message = otUdpNewMessage(instance, &messageSettings);
            if (message == NULL)
            {
                error = OT_ERROR_NO_BUFS;
                break;
            }
            error = otMessageAppend(message, data, data_lens);
            if (error != OT_ERROR_NONE)
            {
                printf("otMessageAppend fail \r\n");
                break;
            }

            error = otUdpSend(instance, &appSock, message, &messageInfo);
            if (error != OT_ERROR_NONE)
            {
                printf("otUdpSend fail \r\n");
                break;
            }
        }
        printf("send 6 \r\n");
        message = NULL;

    } while (0);

    if (message != NULL)
    {
        otMessageFree(message);
    }
    return error;
}

uint8_t app_sockInit(otInstance *instance, void (*handler)(otMessage *, const otMessageInfo *), uint16_t udp_port)
{
    otSockAddr sockAddr;

    uint8_t ret;

    memset(&appSock, 0, sizeof(otUdpSocket));
    memset(&sockAddr, 0, sizeof(otSockAddr));

    ret = otUdpOpen(instance, &appSock, otUdpReceive_handler, instance);

    if (OT_ERROR_NONE == ret)
    {
        app_udpHandler = handler;
        sockAddr.mPort = udp_port;
        ret = otUdpBind(instance, &appSock, &sockAddr, OT_NETIF_THREAD);
        if (OT_ERROR_NONE == ret)
        {
            log_info("UDP PORT           : 0x%x", udp_port);
        }
    }

    return ret;
}
