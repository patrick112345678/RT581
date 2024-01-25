/**
 * @file das_hex_cmd.c
 * @author Jiemin Cao (jiemin.cao@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023
 *
 */



//=============================================================================
//                Include
//=============================================================================
#include <stdio.h>
#include <string.h>
#include "openthread-core-config.h"
#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/ip6.h>
#include "cm3_mcu.h"
#include "uart_stdio.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "log.h"
// #include "ota_handler.h"
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
//                Private Function Declaration
//=============================================================================

//=============================================================================
//                Private Global Variables
//=============================================================================
static bool das_hex_cmd_udp_ip_set;
static char das_hex_cmd_udp_ip[40];

static uint16_t das_rx_length = 0;
static uint8_t das_rx_buf[1200]; // to support 1000-byte udp pkt send

static TimerHandle_t get_meter_id_timer = NULL;
static bool is_meter_id_geted = false;
//=============================================================================
//                Public Global Variables
//=============================================================================
extern otUdpSocket socket;
//=============================================================================
//                Private Definition of Compare/Operation/Inline function/
//=============================================================================
static void meter_id_is_geted(bool is_geted);
//=============================================================================
//                Functions
//=============================================================================
static void das_hex_cmd_send(uint8_t *cmd_data, uint16_t cmd_data_lens)
{
#if (CONFIG_UART_STDIO_PORT == 0)
    app_uart_data_send(2, cmd_data, cmd_data_lens);
#else
    app_uart_data_send(0, cmd_data, cmd_data_lens);
#endif
}

/*use gpio 30 to switch hex command*/
bool das_hex_cmd_status_check()
{
    return (gpio_pin_get(30) == 0);
}

static uint8_t das_hex_cmd_crc_calculate(uint8_t *hex_ptr, uint16_t hex_ptr_lens)
{
    uint8_t crc_8 = 0;
    uint16_t i = 0;
    for (i = 0; i < hex_ptr_lens; i++)
    {
        crc_8 += hex_ptr[i];
    }
    return crc_8;
}

/*hex cmd function*/
/*cmd 0x00*/
void das_hex_command_response(bool state, uint8_t cmd_id)
{
    uint8_t message[10];
    uint8_t ack_crc = 0x97;

    message[0] = 0x93;
    message[1] = 0;
    message[2] = 0x4;
    message[3] = 0;
    message[4] = cmd_id;

    if (state)
    {
        message[5] = 0x1;
        ack_crc += (cmd_id + 1);
    }
    else
    {
        message[5] = 0x0;
        ack_crc += (cmd_id);
    }
    message[6] = ack_crc;
    das_hex_cmd_send(message, 7);
}

/*cmd 0x01*/
static uint8_t das_hex_cmd_store_udpip(uint8_t *Param, uint16_t lens)
{
    uint8_t i;

    for (i = 0; i < lens; i++)
    {
        if (Param[i] < 48 || (Param[i] > 58 && Param[i] < 65) || (Param[i] > 70 && Param[i] < 97) || Param[i] > 102)
        {
            return 1;
        }
        das_hex_cmd_udp_ip[i] = (char)Param[i];

    }

    das_hex_cmd_udp_ip[i+1] = '\0';
    das_hex_cmd_udp_ip_set = 1;

    return 0;
}

/*cmd 0x02*/
static uint8_t das_hex_cmd_store_udpport(uint8_t *Param, uint16_t lens)
{
    /*this command isn't used and has been replaced by default*/
    return 0;    
}

/*cmd 0x03*/
static uint8_t das_hex_cmd_udp_data_send(uint8_t *Param, uint16_t lens)
{
    otError error = OT_ERROR_NONE;
    otIp6Address dst_peerAddr;
    otDeviceRole mRole = 0;
    OT_THREAD_SAFE(
        otInstance *instance = otrGetInstance();
        if(instance)
        {
            mRole = otThreadGetDeviceRole(instance);
        }
    )

    do
    {
        if (!das_hex_cmd_udp_ip_set)
        {
            error = OT_ERROR_FAILED;
            break;
        }

        if (mRole == OT_DEVICE_ROLE_DISABLED || mRole == OT_DEVICE_ROLE_DETACHED )//|| (ota_get_state() > OTA_IDLE))
        {
            error = OT_ERROR_FAILED;
            break;
        }

        otIp6AddressFromString(das_hex_cmd_udp_ip, &dst_peerAddr);

        error = app_udpSend(THREAD_UDP_PORT, dst_peerAddr, Param, lens);
        
    } while (0);

    return (error != OT_ERROR_NONE);
}

/*cmd 0x04*/
void das_hex_cmd_udp_ack()
{
    uint8_t message[5];
    message[0] = 0x93;
    message[1] = 0;
    message[2] = 0x02;
    message[3] = 0x04;
    message[4] = 0x99;
    das_hex_cmd_send(message, 5);
}

/*cmd 0x10*/
void das_hex_cmd_get_udp_ip(uint8_t *Param, uint16_t lens)
{
    uint16_t message_len = lens + 5;
    uint8_t *message = pvPortMalloc(message_len);
    uint8_t cmd_crc = 0xff;
    if(message)
    {
        message[0] = 0x93;
        message[1] = ((lens + 2) >> 8) & 0xff;
        message[2] = (lens + 2) & 0xff;
        message[3] = 0x10;

        for (uint16_t i = 0; i < lens; i++)
        {
            message[(4 + i)] = (int)Param[i];
        }

        cmd_crc =  das_hex_cmd_crc_calculate((uint8_t*)message, (message_len - 1));
        message[message_len-1] = cmd_crc;

        das_hex_cmd_send(message, message_len);
    }
    if(message) vPortFree(message);
}

/*cmd 0x20*/
void das_hex_cmd_get_udp_port(uint8_t *Param, uint16_t lens)
{
    /*this command isn't used and has been replaced by default*/
}

/*cmd 0x30*/
void das_hex_cmd_get_udp_received_data(uint8_t *Param, uint16_t lens)
{
    uint16_t message_len = lens + 5;
    uint8_t *message = pvPortMalloc(message_len);
    uint8_t cmd_crc = 0xff;
    if(message)
    {
        message[0] = 0x93;
        message[1] = ((lens + 2) >> 8) & 0xff;
        message[2] = (lens + 2) & 0xff;
        message[3] = 0x30;

        for (uint16_t i = 0; i < lens; i++)
        {
            message[(4 + i)] = (int)Param[i];
        }

        cmd_crc =  das_hex_cmd_crc_calculate((uint8_t*) message, (message_len - 1));
        message[message_len-1] = cmd_crc;

        das_hex_cmd_send(message, message_len);
    }
    if(message) vPortFree(message);
}

/*cmd 0x40*/
void das_hex_cmd_command_bootup()
{
    uint8_t message[5];

    message[0] = 0x93;
    message[1] = 0;
    message[2] = 0x2;
    message[3] = 0x40;
    message[4] = 0xD5;

    das_hex_cmd_send(message, 5);
}

/*cmd 0x50*/
void das_hex_cmd_state_change(uint8_t state)
{
    uint8_t message[6];
    uint8_t crc = 0xE6;

    message[0] = 0x93;
    message[1] = 0;
    message[2] = 0x3;
    message[3] = 0x50;
    message[4] = state;
    crc += state;
    message[5] = crc;

    das_hex_cmd_send(message, 6);
}


/*cmd 0x51 and generate 0x52*/
static uint8_t das_hex_cmd_get_thread_state(uint8_t *Param, uint16_t lens)
{
    uint8_t message[5 + 1];
    otDeviceRole mRole = 0;
    OT_THREAD_SAFE(
        otInstance *instance = otrGetInstance();
        if(instance)
        {
            mRole = otThreadGetDeviceRole(instance);
        }
    )
    
    message[0] = 0x93;
    message[1] = 0x0;
    message[2] = 0x03;
    message[3] = 0x52;
    message[4] = mRole & 0xff;
    message[5] = (message[0] + message[2] + message[3] + message[4]) & 0xff;

    das_hex_cmd_send(message, 6);

    return 0;
}

/*cmd 0x60 and generate 0x61*/
static uint8_t das_hex_cmd_get_link_local_addr(uint8_t *Param, uint16_t lens)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    uint8_t message[5 + sizeof(string)];
    uint8_t cmd_crc = 0, i = 0;

    OT_THREAD_SAFE(
        otInstance *instance = otrGetInstance();
        if(instance)
        {
            otIp6AddressToString(otThreadGetLinkLocalIp6Address(instance), string, sizeof(string));
        }
    )

    message[0] = 0x93;
    message[1] = 0;
    message[3] = 0x61;
    cmd_crc = message[0] + message[1] + message[3];

    while (string[i] != '\0')
    {
        message[4+i] = (int)string[i];
        cmd_crc += message[(4 + i)];
        i++;
    }
    message[2] = i + 2;
    cmd_crc += message[2];
    message[(4 + i)] = cmd_crc;

    das_hex_cmd_send(message, (i + 5));

    return 0;
}

/*cmd 0x70 and generate 0x71*/
static uint8_t das_hex_cmd_get_mesh_local_addr(uint8_t *Param, uint16_t lens)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    uint8_t message[5 + sizeof(string)];
    uint8_t cmd_crc = 0, i = 0;

    OT_THREAD_SAFE(
        otInstance *instance = otrGetInstance();
        if(instance)
        {
            otIp6AddressToString(otThreadGetMeshLocalEid(instance), string, sizeof(string));
        }
    )

    message[0] = 0x93;
    message[1] = 0;
    message[3] = 0x71;
    cmd_crc = message[0] + message[1] + message[3];

    while (string[i] != '\0')
    {
        message[4+i] = (int)string[i];
        cmd_crc += message[(4 + i)];
        i++;
    }
    message[2] = i + 2;
    cmd_crc += message[2];
    message[(4 + i)] = cmd_crc;

    das_hex_cmd_send(message, (i + 5));
}

/*cmd 0x80 and generate 0x81*/
static uint8_t das_hex_cmd_get_routing_locator_addr(uint8_t *Param, uint16_t lens)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    uint8_t message[5 + sizeof(string)];
    uint8_t cmd_crc = 0, i = 0;
    OT_THREAD_SAFE(
        otInstance *instance = otrGetInstance();
        if(instance)
        {
            otIp6AddressToString(otThreadGetRloc(instance), string, sizeof(string));
        }
    )

    message[0] = 0x93;
    message[1] = 0;
    message[3] = 0x81;
    cmd_crc = message[0] + message[1] + message[3];

    while (string[i] != '\0')
    {
        message[4+i] = (int)string[i];
        cmd_crc += message[(4 + i)];
        i++;
    }
    message[2] = i + 2;
    cmd_crc += message[2];
    message[(4 + i)] = cmd_crc;

    das_hex_cmd_send(message, (i + 5));
}

/*cmd 0x90*/
void das_hex_cmd_get_meter_id()
{
    if (das_hex_cmd_status_check())
    {
        uint8_t message[5];
        memset(message, 0x0, 5);
        message[0] = 0x93;
        message[1] = 0x00;
        message[2] = 0x02;
        message[3] = 0x90;
        message[4] = 0x25;

        das_hex_cmd_send(message, 5);
    }
}

/*cmd 0x91*/
static uint8_t das_hex_cmd_meter_id_respone(uint8_t *Param, uint16_t lens)
{
    bool ret = 0;
    uint8_t i, k, hexDigit;
    uint64_t num = 0, temp = 0 ;
    uint8_t extaddr[OT_EXT_ADDRESS_SIZE];
    memset(extaddr, 0x0, OT_EXT_ADDRESS_SIZE);
    do
    {
        if (lens > 10)
        {
            ret = 1;
            break;
        }
        for (i = 0; i < lens; i++)
        {
            if (Param[i] >= '0' && Param[i] <= '9')
            {
                hexDigit = (Param[i] - '0');
            }
            else if (Param[i] >= 'a' && Param[i] <= 'f')
            {
                hexDigit = (Param[i] - 'a' + 10);
            }
            else if (Param[i] >= 'A' && Param[i] <= 'F')
            {
                hexDigit = (Param[i] - 'A' + 10);
            }
            else
            {
                // Invalid character in the input
                ret = 1;
                break;
            }
            temp = hexDigit;
            for (k = 0; k < (lens - i - 1); k++)
            {
                temp *= 10;
            }
            num += temp;
        }
        extaddr[3] = (num >> 32) & 0xff;
        extaddr[4] = (num >> 24) & 0xff;
        extaddr[5] = (num >> 16) & 0xff;
        extaddr[6] = (num >> 8) & 0xff;
        extaddr[7] = num & 0xff;
        OT_THREAD_SAFE(
            otInstance *instance = otrGetInstance();
            if(instance)
            {
                /* set extaddr to equal eui64*/
                otExtAddress extAddress;
                memcpy(extAddress.m8, extaddr, OT_EXT_ADDRESS_SIZE);
                if (otLinkSetExtendedAddress(instance, &extAddress) != OT_ERROR_NONE)
                {
                    ret = 1;
                    break;
                }

                /* set mle eid to equal eui64*/
                otIp6InterfaceIdentifier iid;
                memcpy(iid.mFields.m8, extAddress.m8, OT_EXT_ADDRESS_SIZE);
                if (otIp6SetMeshLocalIid(instance, &iid) != OT_ERROR_NONE)
                {
                    ret = 1;
                    break;
                }
            }
        )
        meter_id_is_geted(true);
    } while (0);

    return ret;


    return 0;
}

/*cmd 0xA0 and generate 0xA1*/
static uint8_t das_hex_cmd_get_build_version(uint8_t *Param, uint16_t lens)
{
    const char *string = otGetVersionString();
    uint8_t message[5 + strlen(string)];

    uint8_t cmd_crc = 0, i = 0;

    message[0] = 0x93;
    message[1] = 0;
    message[3] = 0xA1;
    cmd_crc = message[0] + message[1] + message[3];

    while (string[i] != '\0')
    {
        message[4+i] = (int)string[i];
        cmd_crc += message[(4 + i)];
        i++;
    }
    message[2] = i + 2;
    cmd_crc += message[2];
    message[(4 + i)] = cmd_crc;

    das_hex_cmd_send(message, (i + 5));
    return 0;
}

/*cmd 0xB0 and generate 0xB1*/
static uint8_t das_hex_cmd_get_partitionid(uint8_t *Param, uint16_t lens)
{
    uint32_t partitionid = 0;
    OT_THREAD_SAFE(
        otInstance *instance = otrGetInstance();
        if(instance)
        {
            partitionid = otThreadGetPartitionId(instance);
        }
    )
    
    uint8_t message[9];

    uint8_t cmd_crc = 0, i = 0;

    message[0] = 0x93;
    message[1] = 0;
    message[2] = 0x06;
    message[3] = 0xB1;

    message[4] = (partitionid >> 24) & 0xFF;
    message[5] = (partitionid >> 16) & 0xFF;
    message[6] = (partitionid >> 8) & 0xFF;
    message[7] = partitionid & 0xFF;

    for (i = 0 ; i < 8 ; i++)
    {
        cmd_crc += message[i];
    }
    message[8] = cmd_crc;

    das_hex_cmd_send(message, 9);

    return 0;
}
/*cmd 0xF1*/
static void das_hex_cmd_to_acsii_comand_process(uint8_t *Param, uint16_t lens)
{
    char *Rx_Buffer = pvPortMalloc(lens);
    if(Rx_Buffer)
    {
        memcpy(Rx_Buffer, (char *)Param, lens);
        Rx_Buffer[lens] = '\0';
        otCliInputLine(Rx_Buffer);
    }
    if(Rx_Buffer) vPortFree(Rx_Buffer);
}

static void das_hex_cmd_fn_exec(uint8_t cmd_id, uint8_t *Param, uint16_t lens)
{
    bool need_ack = 1;
    uint8_t cmd_ret = 0;
    switch (cmd_id)
    {
    case 0x01:
        cmd_ret = das_hex_cmd_store_udpip(Param, lens);
        break;
    case 0x03:
        cmd_ret = das_hex_cmd_udp_data_send(Param, lens);
        break;
    case 0x51:
        das_hex_cmd_get_thread_state(Param, lens);
        need_ack = 0;
        break;
    case 0x60:
        das_hex_cmd_get_link_local_addr(Param, lens);
        need_ack = 0;
        break;
    case 0x70:
        das_hex_cmd_get_mesh_local_addr(Param, lens);
        need_ack = 0;
        break;
    case 0x80:
        das_hex_cmd_get_routing_locator_addr(Param, lens);
        need_ack = 0;
        break;
    case 0x91:
        cmd_ret = das_hex_cmd_meter_id_respone(Param, lens);
        break;
    case 0xA0:
        das_hex_cmd_get_build_version(Param, lens);
        need_ack = 0;
        break;
    case 0xB0:
        das_hex_cmd_get_partitionid(Param, lens);
        need_ack = 0;
        break;
    case 0xF1:
        das_hex_cmd_to_acsii_comand_process(Param, lens);
        need_ack = 0;
        break;
    default:
        cmd_ret = 1;
        break;
    }
    if (need_ack)
    {
        das_hex_command_response(cmd_ret, cmd_id);
    }
}

static void das_hex_cmd_process(const uint8_t *aBuf, uint16_t aBufLength)
{
    uint8_t crc = 0, cmd_id = 0xFF;
    uint16_t cmd_len = 0;
    do
    {
        /*check cmd header*/
        if (aBuf[0] != 0x93)
        {
            break;
        }

        cmd_id = aBuf[3];
        cmd_len = aBuf[1] * 256 + aBuf[2];
        /*check crc*/
        crc = das_hex_cmd_crc_calculate((uint8_t*) aBuf, (aBufLength - 1));
        if (crc != aBuf[aBufLength - 1])
        {
            das_hex_command_response(1, aBuf[3]);
        }
        else
        {
            das_hex_cmd_fn_exec(cmd_id, (uint8_t*)(aBuf + 4), (cmd_len - 2));
        }
    } while (0);
}

void das_hex_cmd_received_task(const uint8_t *aBuf, uint16_t aBufLength)
{
    const uint8_t    *end;
    uint16_t cmd_length;
    end = aBuf + aBufLength;
    for (; aBuf < end; aBuf++)
    {
        if (das_rx_length != 0 || *aBuf == 0x93)
        {
            das_rx_buf[das_rx_length++] = *aBuf;
        }
    }

    if (das_rx_length > 3)
    {
        cmd_length = das_rx_buf[1] * 256 + das_rx_buf[2];
        if (das_rx_length >= (cmd_length + 3))
        {
            das_hex_cmd_process(das_rx_buf, das_rx_length);
            /*das_rx_buf clean*/
            memset(das_rx_buf, 0x0, das_rx_length);
            das_rx_length = 0;
        }
    }
}

static void meter_id_is_geted(bool is_geted)
{
    is_meter_id_geted = is_geted;
    OT_THREAD_SAFE(
        otInstance *instance = otrGetInstance();
        if(instance)
        {
            otIp6SetEnabled(instance, true);
            otThreadSetEnabled(instance, true);
        }
    )
}

static void get_meter_id_timer_handler( TimerHandle_t xTimer ) 
{
    if (is_meter_id_geted)
    {
        xTimerDelete(get_meter_id_timer,0);
    }
    else
    {
        das_hex_cmd_get_meter_id();
        xTimerStart(get_meter_id_timer, 0 );
    }
}

void das_get_meter_id_init()
{
    if (NULL == get_meter_id_timer)
    {
        get_meter_id_timer = xTimerCreate("get_meter_id_timer", 
                                                (10000), 
                                                false, 
                                                ( void * ) 0, 
                                                get_meter_id_timer_handler);

        xTimerStart(get_meter_id_timer, 0 );
    }
    else
    {
        log_info("get_meter_id_timer exist\n");
    }
}