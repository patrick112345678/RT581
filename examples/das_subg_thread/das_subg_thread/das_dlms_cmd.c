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
#include "FreeRTOS.h"
#include "cm3_mcu.h"
#include "log.h"
#include "main.h"
#include "mbedtls/gcm.h"
#include "task.h"
#include "timers.h"
#include "uart_stdio.h"
#include "shell.h"
#include "cli.h"
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
#define QUEUE_SIZE 20
#define COMMAND_LENGTH 1500
#define APP_COMMISSION_DATA_FLASH_START 0xF1000
#define ConfigurationPage APP_COMMISSION_DATA_FLASH_START
//=============================================================================
//                Private Global Variables
//=============================================================================
static uint8_t actionkey_response[128], Key_Restart = 0;
static uint8_t Broadcast_savebuff_flag = 0, Broadcast_flag = 0;
static uint8_t continuebusytime = 120;
static uint8_t Channel_num = 1;
static uint8_t flag_Power_Off = 0; // 1: Power off 0: Power on
static uint8_t gettime_flag = 0;
static uint8_t get_id_done = 1;
static uint8_t hitHANDguN = 0;
static uint8_t keyObtainedStatusIncorrecttimeout = 5;
static uint8_t KeyErrorflag = 0;
static uint8_t Leaderweight_num = 1;
static uint8_t meter_fix_cnt = 0x41;
static uint8_t ondemandreadingtime = 3;
static uint8_t On_Demand_Retry = 0; // on demand Retransmission
static uint8_t On_Demand_Type = 0; // On Demand Datatype 3: Load Profile, 4: Midnight, 5: ALT, 6: Event Log,
// 7:immediate REG
static uint8_t PingCheckMeter = 1; // 0:success 1:fail
static uint8_t registerRetrytime = 1;
static uint8_t receivebusytime = 3;
static uint8_t Rafael_reset = 1;
static uint8_t RFretrytime = 1;
static uint8_t sst_flag = 0;
static uint8_t start_dis = 0;
static uint8_t sendbusyflag = 0;
static uint8_t target_pos = 0;     // 1:leader 2:router 3:child
static uint8_t read_targetpos = 0; // 1:did read status 0:didn't read status
static uint8_t register_steps = 0; // am1 register flag
static uint8_t riva_fix_cnt = 0;
static uint8_t Verification_AA_done = 1;

static uint8_t ACTION_CHANGE_KEY = 0;
static uint8_t AUTO_AA = 0,
               AUTO_AA_FLAG = 1; // Do AA first when auto reading the meter,
// only do once AA 1:start 0:enter
static uint8_t DISC_FLAG = 0;
// Power off record 0:power on 1:power off ; 1:power off 15 Second; Actively
// read loadprofile every hour; Read midnight at 12 midnight; Read ALT
// notification every two hours while running only once.
static uint8_t EVENT_LOG_POWER_ON = 0, NOT_COMPLETELY_POWER_OFF = 0,
               FIRST_NOT_COMPLETELY_POWER_ON_FLAG = 1,
               auto_read_loadprofile = 0, auto_read_Midnight = 0,
               auto_read_ALT = 0, POWERONFLAG = 0,
               POWER_ON_NOTIFICATION_FLAG = 0, POWERON_Restart = 0;
static uint8_t CONTINUE_BUSY = 0; // Continuous transmission flag
static uint8_t FLAG_PING_METER = 0;
static uint8_t MAA_flag = 0;
static uint8_t MAAFIRST = 0;
static uint8_t MC_done = 0;
static uint8_t RE_AA = 0;
static uint8_t RESETFLAG = 0;
static uint8_t RESTART_FLAG = 0;
static uint8_t SETACTION = 0;
static uint8_t STEP_CONTINUE_FLAG = 0; // Do it slowly in while
static uint8_t SET_TOU_C2_Start = 0;
static uint8_t SET_TOU_C1_INDEX = 0;
static uint8_t SET_TIME_C0_INDEX = 0; // 0621
static uint8_t SET_BUSY_FLAG = 0;
static uint8_t SQBsettingNUM = 0;

static uint16_t ic_count = 0;
static uint16_t On_Demand_length = 0; // Group command length
static uint16_t Resume_index = 1;
static uint16_t Refuse_Command = 0;
static uint16_t SETdata_index = 0;
static uint16_t SETdata_RECEIVE_Tatol_LEN = 0;
static uint16_t uart_len1 = 0;

static int8_t LoadprofileAB = 0;

static int Broadcastmeterdelay = 0;
static int HANDSQB = 0;
static int KEYresponse_len = 0;
static int Networkidtime_num = 120;
static int ondemand_idle = 0;
static int printf_off = 0;
static int PANID_num = 30;
static int setdataLen = 0;
static int sendIndex = 0;
static int SQBsetStart = 0;
static int SETQBtmp = 1;
static int SQBCTLEN = 0;
static int TASK_ID = 0;

static size_t ipv6_length = 0;
static size_t virtual_ipv6_length = 0;

// Task code : Automatic meter reading, Number of tasks Reading tasks Do AA
// before A3
static uint8_t Tesk_ID = 0, On_Demand_ID = 0, Auto_Reading = 0, Task_count = 0,
               On_Demand_Reading_Type = 0, Task_count_all = 0,
               ON_DEMAND_AA_FLAG = 0;
static unsigned char On_Demand_Task_Command[200] = {0},
        On_Demand_Task_Buff[1000] = {0}; // A3一堆命令 A3一堆回應

/* Key flag  */
static uint8_t TPC_GUK_FLAG = 0, TPC_AK_FLAG = 0, TPC_MK_FLAG = 0;

/* timer flag  */
static uint32_t Broadcast_delaysend_start = 0;
static uint32_t continuebusyflagstart = 0;
static uint32_t ObtainIPv6StartTime = 0;
static uint32_t ondemandreadingstart = 0;
static uint32_t receivebusyflagstart = 0;
static uint32_t RegistrationprocessStart = 0;
static uint32_t Rafael_time = 0;
static uint32_t sendbusyflagstart = 0;
static uint32_t UART7InitializationSTART = 0;

static uint8_t ucMeterPacket[350];
static unsigned char Configuration_Page[180];
static unsigned char event_notification_data[80];
static unsigned char Initial_Value[256];
static unsigned char saveSETdata[2000] = {0x00};
static unsigned char uart_read_data2[1500];

unsigned char flash_test[200] =
{
    0x41, 0x43, 0x31, 0x30, 0x38, 0x32, 0x31, 0x30, 0x38, 0x30, 0x32,
    0x30,                   /*factory ID            0~11*/
    0x80,                   /*leader weight 12*/
    0x00, 0x78,             /*networkid 13~14*/
    0x01, 0x2c,             /*PANid         15~16*/
    0x03,                   /*Channel  17*/
    0x00, 0x01,             /*send els61 timeout 18~19*/
    0x00, 0x01,             /*send rf  timeout   20~21*/
    0x00, 0x01,             /*receive timeout    22~23*/
    0x00, 0x3C,             /*continue timeout      24~25*/
    0X0F,                   /*loadprofile        26*/
    0x64, 0x61, 0x73, 0x00, /*server ip                 27~30*/
    0x07, 0xD1, 0x04, 0x64, 0x61, 0x73, 0x38, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, /*socket port                   31~32*/
    0x07, 0xD1, 0x04, 0x64, 0x61, 0x73, 0x38, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, /*APN               33~47*/
    0x07, 0xD1, 0x04, 0x64, 0x61, 0x73, 0x38, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*tpc guk 48~63*/
    0x07, 0xD1, 0x04, 0x64, 0x61, 0x73, 0x38, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             /*tpc gcm ak 64~80*/
    0x07, 0xD1, 0x04, 0x64, 0x61, 0x73, 0x00, /*tpc mk 80~95*/
    0x00, 0x03,                               /*ondemandreadingtime 96~97*/
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, /*keyObtainedStatusIncorrecttimeout
                                                       138*/
    0x01,                                           /*registerRetrytime 139*/
    0x14,                                           /*Rafael_reset 140*/
    0x96,                                           /*Broadcastmeterdelay 141*/
    0xff
};

static uint8_t NOTIFICATION_CODE[6] = {0, 0, 0, 0, 0, 0};

static uint8_t tpc_mkey[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                               0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1F, 0x20
                              };

static uint8_t tpc_guk[16] =
    //{0xCD,0x78,0x12,0xE5,0xAA,0xD6,0xE9,0x9E,0xA1,0x52,0xD0,0xA6,0x20,0x1D,0x0A,0x4C};
    ////Meter 70 GUK  18701666 {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    //0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f}; // test
    //{0xDC,0xB0,0x06,0xB9,0x82,0xE8,0xEC,0xF9,0x69,0xA6,0x6B,0x5D,0x55,0x81,0x22,0x31};
    //// GD18667512
{
    0x03, 0x4E, 0x49, 0x0E, 0xA9, 0x33, 0xE0, 0xA7,
    0xA9, 0xBC, 0x40, 0xAA, 0xCE, 0xB5, 0x14, 0xD8
}; // GD18667593
//{0xEE, 0x67, 0xEB, 0x5F, 0xAF, 0xB7, 0x1A, 0x39, 0xD5, 0xFF, 0x41, 0x34, 0x3C,
//0xED, 0x76, 0x30};//GD18667515
//{0x94,0x12,0xd1,0xd7,0xb0,0x5e,0x47,0x5a,0xbb,0xeb,0x65,0xe1,0x92,0x47,0x47,0x27};
////tpc test 00030040
static uint8_t tpc_dk[16] = {0xB5, 0x07, 0xE3, 0x86, 0xEB, 0x00,
                             0x66, 0xF4, 0xFA, 0x0B, 0xDB, 0x14,
                             0x63, 0xB6, 0x69, 0xFB
                            }; // DK

static uint8_t tpc_c_iv[12] = {0x4D, 0x41, 0x4E, 0x01,
                               0x23, 0x45, 0x67, 0x89,  // Client System Title
                               0x00, 0x00, 0x00, 0x01
                              }; // IC

static uint8_t tpc_c_iv_h[12] = {0x48, 0x41, 0x4E, 0x01,
                                 0x21, 0xA8, 0x8C, 0x02,  // Client System Title
                                 0x00, 0x00, 0x00, 0x01
                                }; // IC

static uint8_t tpc_s_iv[12] = {0x20, 0x54, 0x30, 0x00,
                               0x00, 0xBC, 0x61, 0x4E,  // Server System Title
                               0x00, 0x00, 0x00, 0x01
                              }; // IC

static uint8_t tpc_s_iv_h[12] = {0x20, 0x54, 0x30, 0x00,
                                 0x00, 0xBC, 0x61, 0x4E, // Server System Title
                                 0x00, 0x00, 0x00, 0x01
                                };

static uint8_t tpc_gcm_ak[17] =
{
    0x30, // SC
    // 0xB3,0xC2,0x99,0x5E,0x4A,0x63,0x75,0xDE,0x5F,0x05,0x04,0xD2,0x06,0x1F,0x68,0x48};//18701666
    // 0xE6,0x9C,0xD0,0xFF,0x07,0xB8,0x05,0x32,0x9E,0xB1,0x23,0x64,0x0B,0x9B,0x2F,0x8B};//GD18667512
    // 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    // 0xdb, 0xdc, 0xdd, 0xde, 0xdf}; // test 0xD2, 0xCB, 0xCC, 0xA5,
    // 0x04, 0xBC, 0x0C, 0x8A, 0xC8, 0xED, 0x60, 0xDD, 0xDF, 0xB1, 0xA7,
    // 0x78};//GD18667515
    0x90, 0x68, 0x41, 0x85, 0x03, 0x9F, 0x8C, 0xAB, 0x7B, 0x4E, 0x86, 0x64,
    0xB1, 0x41, 0x25, 0x3C
}; // GD18667593
// 0x0e,0x09,0x90,0x5c,0x1d,0x09,0xed,0xdf,0x90,0xda,0xf0,0xf9,0x95,0xed,0x30,0x62
// };//tpc test 00030040
static uint8_t tpc_gmac_ak[25] =
{
    0x10, // SC
    // 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    // 0xdb, 0xdc, 0xdd, 0xde, 0xdf, // Meter 70 AK
    // 0xD2, 0xCB, 0xCC, 0xA5, 0x04, 0xBC, 0x0C, 0x8A, 0xC8, 0xED, 0x60, 0xDD,
    // 0xDF, 0xB1, 0xA7, 0x78,//GD18667515
    // 0xB3,0xC2,0x99,0x5E,0x4A,0x63,0x75,0xDE,0x5F,0x05,0x04,0xD2,0x06,0x1F,0x68,0x48,//18701666
    // 0xE6,0x9C,0xD0,0xFF,0x07,0xB8,0x05,0x32,0x9E,0xB1,0x23,0x64,0x0B,0x9B,0x2F,0x8B,//GD18667512
    0x90, 0x68, 0x41, 0x85, 0x03, 0x9F, 0x8C, 0xAB, 0x7B, 0x4E, 0x86, 0x64,
    0xB1, 0x41, 0x25, 0x3C, // GD18667593
    // 0x0e,0x09,0x90,0x5c,0x1d,0x09,0xed,0xdf,0x90,0xda,0xf0,0xf9,0x95,0xed,0x30,0x62,//tpc
    // test 00030040
    0x8F, 0x45, 0x32, 0xC8, 0x79, 0x5B, 0xFA,
    0x3C
}; // StoC (Change data from AARE)

static uint16_t dlms_rx_length = 0;
static uint8_t dlms_rx_buf[1200]; // to support 1000-byte udp pkt send
//=============================================================================
//                Public Global Variables
//=============================================================================
uint8_t ELS61IOType = 0, ELS61inPointer = 0, ELS61outPointer = 0,
        ELS61_IO_Difference = 0, ELS61_MESSAGE_LOCK = 0, ELS_CSQ = 0,
        Get_CSQ_FLAG = 0;
int Event_Notification;
//=============================================================================
//                Private Definition of Compare/Operation/Inline function/
//=============================================================================
//=============================================================================
//                Functions
//=============================================================================
static void WriteFlashData(uint32_t WriteAddress, unsigned char *data, uint8_t num)
{
    static uint8_t program_data[0x100], read_buf[0x100];
    flash_read_page((uint32_t)(program_data), WriteAddress);
    memcpy(&program_data, data, num);

    while (flash_check_busy());
    flash_erase(FLASH_ERASE_SECTOR, WriteAddress);
    while (flash_check_busy());
    flush_cache();

    while (flash_check_busy());
    flash_write_page((uint32_t)&program_data, WriteAddress);
    while (flash_check_busy());
}

static void SaveKey(void)
{
    int i = 0;
    printf("\nsave key\n");
    for (i = 0; i < 16; i++)
    {
        Configuration_Page[i + 48] = tpc_guk[i];
    }
    for (i = 0; i < 16; i++)
    {
        Configuration_Page[i + 64] = tpc_gcm_ak[i + 1];
    }
    for (i = 0; i < 16; i++)
    {
        Configuration_Page[i + 80] = tpc_mkey[i];
    }

    WriteFlashData(ConfigurationPage, Configuration_Page, 180);
}

static int thread_extaddr_setting(otInstance *instance, uint8_t *Param, uint8_t lens)
{
    int ret = 0;
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
        /* set extaddr to equal extAddress*/
        otExtAddress extAddress;
        memcpy(extAddress.m8, extaddr, OT_EXT_ADDRESS_SIZE);
        if (otLinkSetExtendedAddress(instance, &extAddress) != OT_ERROR_NONE)
        {
            ret = 1;
            break;
        }

        /* set mle eid to equal extAddress*/
        otIp6InterfaceIdentifier iid;
        memcpy(iid.mFields.m8, extAddress.m8, OT_EXT_ADDRESS_SIZE);
        if (otIp6SetMeshLocalIid(instance, &iid) != OT_ERROR_NONE)
        {
            ret = 1;
            break;
        }
    } while (0);

    return ret;
}

static void Configuration_Page_init(otInstance *instance)
{
    printf("\n\rConfiguration_Page \r\n");
    static uint8_t read_buf[0x100];
    uint8_t factory_id_null[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    flash_read_page((uint32_t)(read_buf), ConfigurationPage);

    memcpy(Configuration_Page, read_buf, 180);

    if (Configuration_Page[17] == 0xFF)
    {
        printf("no flash!\n");
        printf("\n\rflash_set_test\n\r");
        if (memcmp(&Configuration_Page[2], factory_id_null, 10) != 0)
        {
            memcpy(&flash_test[2], &read_buf[2], 10);
        }
        WriteFlashData(ConfigurationPage, flash_test, 180);
        while (flash_check_busy());
        flash_read_page((uint32_t)(read_buf), ConfigurationPage);
        memcpy(Configuration_Page, read_buf, 180);
    }
    /*set thread extadder*/
    if (thread_extaddr_setting(instance, &Configuration_Page[2], 10))
    {
        printf("set thread extadder fail \r\n");
    }

#if 0 //use ot dataset command directily
    // Leaderweight
    Leaderweight_num = Configuration_Page[12];

    // networkid
    Networkidtime_num = Configuration_Page[13] * 256 + Configuration_Page[14];

    // panid
    PANID_num = Configuration_Page[15] * 256 + Configuration_Page[16];

    // channel
    Channel_num = Configuration_Page[17];
#endif
    // send els61 timeout
    // sendbusytime = Configuration_Page[18]*256 +  Configuration_Page[19];

    // send rf timeout
    RFretrytime = Configuration_Page[20] * 256 + Configuration_Page[21];

    // receive timeout
    receivebusytime = Configuration_Page[22] * 256 + Configuration_Page[23];

    // continue busy timeout
    continuebusytime = Configuration_Page[24] * 256 + Configuration_Page[25];

    LoadprofileAB = Configuration_Page[26];

    ondemandreadingtime = Configuration_Page[96] * 256 + Configuration_Page[97];

    keyObtainedStatusIncorrecttimeout = Configuration_Page[138];

    registerRetrytime = Configuration_Page[139];

    Rafael_reset = Configuration_Page[140];

    Broadcastmeterdelay = Configuration_Page[141];
}

static void _cli_cmd_factory_id_set(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    int i = 0;
    printf("save factory id \r\n");
    if (argc > 1)
    {
        printf("factory id : ");
        memcpy(&Configuration_Page[2], argv[1], 10);
        for (i = 0; i < 10; i++)
        {
            printf("%02X", Configuration_Page[2 + i]);
        }
        printf("\r\n");
        WriteFlashData(ConfigurationPage, Configuration_Page, 180);
    }
    else
    {
        printf("factory id : ");
        for (i = 0; i < 10; i++)
        {
            printf("%02X", Configuration_Page[2 + i]);
        }
        printf("\r\n");
    }
}

static unsigned short UpdateCRC16(unsigned short crc, unsigned char value)
{
    unsigned short CRC16Table[] =
    {
        0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
        0x8408, 0x9489, 0xa50a, 0xb58b, 0xc60c, 0xd68d, 0xe70e, 0xf78f
    };

    crc = (crc >> 4) ^ CRC16Table[(crc & 0xf) ^ (value & 0xf)];
    crc = (crc >> 4) ^ CRC16Table[(crc & 0xf) ^ (value >> 4)];

    return crc;
}

static uint16_t CRC_COMPUTE(char *recv_buff, uint16_t len)
{
    int i;
    char strbuf[128], crc;
    unsigned short crc16;
    unsigned char ch;
    unsigned char crcbuf[2];
    crc16 = 0xFFFF;

    printf("CRC_Buff=");
    for (i = 0; i < len; i++)
    {
        ch = *(recv_buff + i);
        sprintf(strbuf, "%02X ", ch); // test only
        printf(strbuf);
    }
    printf("\n");

    for (i = 0; i < len; i++)
    {
        ch = *(recv_buff + i);
        crc16 = UpdateCRC16(crc16, ch);
    }
    crc16 = ~crc16;
    crcbuf[0] = crc16 & 0xff;
    crcbuf[1] = (crc16 >> 8) & 0xff;
    printf("CRC=");
    for (i = 0; i < 2; i++)
    {
        crc = crcbuf[i];
        sprintf(strbuf, "%02X ", crc); // test only
        printf(strbuf);
    }
    printf("\n");

    return crc16;
}

static int32_t udf_AES_GCM_Decrypt(uint8_t *gcm_ek, uint8_t *gcm_aad,
                                   uint8_t *gcm_iv, uint8_t *gcm_ct,
                                   uint8_t *gcm_tag, uint8_t *gcm_pt,
                                   uint16_t gcm_ctlen, uint16_t gcm_aadlen)
{
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);
    int32_t ret = 0;
    mbedtls_cipher_id_t cipher = MBEDTLS_CIPHER_ID_AES;

    ret = mbedtls_gcm_setkey(&ctx, cipher, gcm_ek, 128);

    ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_DECRYPT, gcm_ctlen,
                                    gcm_iv, 12, gcm_aad, gcm_aadlen, gcm_ct,
                                    gcm_pt, 12, gcm_tag);

    mbedtls_gcm_free(&ctx);
    return ret;
}

int32_t udf_AES_GCM_Encrypt(uint8_t *gcm_ek, uint8_t *gcm_aad, uint8_t *gcm_iv,
                            uint8_t *gcm_pt, uint8_t *gcm_ct, uint8_t *gcm_tag,
                            uint16_t gcm_ptlen, uint16_t gcm_aadlen)
{
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);
    int32_t ret = 0;
    mbedtls_cipher_id_t cipher = MBEDTLS_CIPHER_ID_AES;

    mbedtls_gcm_init(&ctx);
    // 128 bits, not bytes!
    ret = mbedtls_gcm_setkey(&ctx, cipher, gcm_ek, 128);

    ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, gcm_ptlen,
                                    gcm_iv, 12, gcm_aad, gcm_aadlen, gcm_pt,
                                    gcm_ct, 12, gcm_tag);
    mbedtls_gcm_free(&ctx);
    return ret;
}

static void udf_Rafael_data(uint16_t task_id, uint8_t type, char *meter_data,
                            uint8_t Rafael_command, uint16_t meter_len)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    uint8_t i = 0;
    uint16_t payload_len = 0;
    uint8_t *payload = NULL;
    otIp6Address LeaderIp;

    otIp6AddressToString(otThreadGetMeshLocalEid(instance), string,
                         sizeof(string));
    LeaderIp = *otThreadGetRloc(instance);
    LeaderIp.mFields.m8[14] = 0xfc;
    LeaderIp.mFields.m8[15] = 0x00;
}

payload_len = 1 + strlen(string) + 1 + 2 + meter_len;

payload = pvPortMalloc(payload_len);
if (payload)
{
    uint8_t *tmp = payload;
    *tmp++ = strlen(string);
    memcpy(tmp, string, strlen(string));
    tmp += strlen(string);
    *tmp++ = type;
    *tmp++ = task_id / 256;
    *tmp++ = task_id % 256;
    memcpy(tmp, meter_data, meter_len);
    tmp += meter_len;
    if ((tmp - payload) != payload_len)
    {
        printf("send lens error %u %u \n", (tmp - payload), payload_len);
    }
    else
    {
        app_udpSend(THREAD_UDP_PORT, LeaderIp, payload, payload_len);
    }
}

if (payload)
{
    vPortFree(payload);
}
}

static void ACK_function(uint8_t num)
{
    char ack_buff[1] = {0x00};
    ack_buff[0] = num;
    udf_Rafael_data(TASK_ID, 0x11, &ack_buff[0], 3, 1);
}

static uint8_t Check_Meter_Res(char *recv_buff, uint16_t len)
{
    char ack_buff[1] = {0x01};
    if (len < 10)
    {
        if (*(recv_buff + 5) == 0x1f) // Meter Disconnet
        {
            printf("Meter_Disconnet_ok\n\r");
            start_dis = 1;

            return 88;
        }
        else if (*(recv_buff + 5) == 0x73)   // Metet UA
        {
            if (DISC_FLAG == 0)
            {
                return 73;
            }
            else
            {
                return 88;
            }
        }
    }
    else if (*(recv_buff + 5) == 0x13 &&
             *(recv_buff + 11) == 0x61) // Meter AARE
    {
        return 61;
    }
    else if ((uint8_t) * (recv_buff + 9) == 0xE7 &&
             (uint8_t) * (recv_buff + 11) == 0xC4 &&
             (uint8_t) * (recv_buff + 13) == meter_fix_cnt)
    {
        return 1;
    }
    else if ((uint8_t) * (recv_buff + 9) == 0xE7 &&
             (uint8_t) * (recv_buff + 11) == 0xD4) // Get Response
    {
        return 2;
    }
    else if ((uint8_t) * (recv_buff + 9) == 0xE7 &&
             (uint8_t) * (recv_buff + 11) == 0xD5) // Set Response
    {
        return 21;
    }
    else if ((uint8_t) * (recv_buff + 9) == 0xE7 &&
             (uint8_t) * (recv_buff + 11) == 0xD7) // Action Response
    {
        return 99;
    }
    else if ((uint8_t) * (recv_buff + 9) == 0xE7 &&
             (uint8_t) * (recv_buff + 11) == 0x2E)
    {
        printf("=====WTF======\n\r");
        KeyErrorflag = 1; // key error
        ACK_function(1);
        return 33;
    }
    else if ((uint8_t) * (recv_buff + 11) == 0xCF)
    {
        return 11;
    }
    else if ((uint8_t) * (recv_buff + 11) == 0xCA)
    {
        return 12;
    }

    return 0;
}

static void udf_Send_AARQ(uint8_t type)
{
    unsigned char cmdbuf[] =
    {
        0x7E, 0xA0, 0x74, 0x03, 0x23, 0x13, 0x3F, 0xC9, 0xE6, 0xE6,
        0x00, 0x60, 0x66, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74,
        0x05, 0x08, 0x01, 0x03, 0xA6, 0x0A, 0x04, 0x08, 0x4D, 0x41,
        0x4E, 0x01, 0x23, 0x45, 0x67, 0x89, 0x8A, 0x02, 0x07, 0x80,
        0x8B, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x02, 0x05, 0xAC,

        0x0A, 0x80, 0x08, 0xAE, 0x0E, 0x52, 0x87, 0x56, 0x52, 0x15,
        0xD2, 0xBE, 0x34, 0x04, 0x32, 0x21, 0x30, 0x30, 0x00, 0x00,
        0x00, 0x01, 0x01, 0x01, 0x10, 0xB5, 0x07, 0xE3, 0x86, 0xEB,
        0x00, 0x66, 0xF4, 0xFA, 0x0B, 0xDB, 0x14, 0x63, 0xB6, 0x69,
        0xFB, 0x00, 0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00, 0x60, 0x1C,

        0x9F, 0x03, 0x00, 0x0F, 0xD9, 0xE6, 0x00, 0xBA, 0x44, 0x93,
        0xC1, 0x5F, 0x17, 0xBE, 0x08, 0x50, 0xF3, 0x7E
    }; // 118B
    unsigned char no_en_aarq[] =
    {
        0x7E, 0xA0, 0x40, 0x03, 0x21, 0x13, 0x91, 0xC4, 0xE6, 0xE6, 0x00,
        0x60, 0x32, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08,
        0x01, 0x01, 0xA6, 0x0A, 0x04, 0x08, 0x56, 0x45, 0x52, 0x30, 0x30,
        0x30, 0x30, 0x31, 0x8B, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x02,
        0x00, 0xBE, 0x10, 0x04, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x06, 0x5F,
        0x1F, 0x04, 0x00, 0x00, 0x10, 0x14, 0x03, 0x00, 0x5D, 0x67, 0x7E
    };
    int i;
    char strbuf[128];
    unsigned char cipher[128], tag[16];
    unsigned short crc16, CRC_check;
    unsigned char ch;

    if (type == 1 /*get_id_done==0*/) // verification_aarq
    {
        printf("\n\rSend_NO_EN_AARQ\n");

        crc16 = CRC_COMPUTE((char *)&no_en_aarq[1], 5);
        no_en_aarq[6] = crc16 & 0xff;
        no_en_aarq[7] = (crc16 >> 8) & 0xff;
        crc16 = CRC_COMPUTE((char *)&no_en_aarq[1], 62);
        no_en_aarq[sizeof(no_en_aarq) - 3] = crc16 & 0xff;
        no_en_aarq[sizeof(no_en_aarq) - 2] = (crc16 >> 8) & 0xff;
        for (i = 0; i < sizeof(no_en_aarq); i++)
        {
            sprintf(strbuf, "%02X ", no_en_aarq[i]);
            printf(strbuf);
        }

        printf("\n\r");
        app_uart_data_send(1, no_en_aarq, sizeof(no_en_aarq));
    }
    else   // if(type==2/*get_id_done==1*/)
    {
        if (type == 2)
        {
            printf("\n\rEN_AARQ_M\n");
            cmdbuf[4] = 0x23;
        }
        else if (type == 4)
        {
            printf("\n\rEN_AARQ_HAN\n\r");
            cmdbuf[4] = 0x27;
            cmdbuf[28] = 0x48;
            cmdbuf[29] = 0x41;
            cmdbuf[30] = 0x4E;
            cmdbuf[31] = 0x01;
            cmdbuf[32] = 0x21;
            cmdbuf[33] = 0xA8;
            cmdbuf[34] = 0x8C;
            cmdbuf[35] = 0x02;

            cmdbuf[53] = 0x8A;
            cmdbuf[54] = 0x34;
            cmdbuf[55] = 0x76;
            cmdbuf[56] = 0x22;
            cmdbuf[57] = 0x36;
            cmdbuf[58] = 0x5D;
            cmdbuf[59] = 0xB0;
            cmdbuf[60] = 0x91;
            /*cmdbuf[75]=0x91;
            cmdbuf[76]=0x2d;
            cmdbuf[77]=0x06;
            cmdbuf[78]=0x21;
            cmdbuf[79]=0xcc;
            cmdbuf[80]=0x0a;
            cmdbuf[81]=0x01;
            cmdbuf[82]=0xb2;
            cmdbuf[83]=0xcd;
            cmdbuf[84]=0x20;
            cmdbuf[85]=0xad;
            cmdbuf[86]=0x26;
            cmdbuf[87]=0x4a;
            cmdbuf[88]=0x63;
            cmdbuf[89]=0xfc;
            cmdbuf[90]=0x36;
            cmdbuf[91]=0x00;
            cmdbuf[92]=0x00;
            cmdbuf[93]=0x06;
            cmdbuf[94]=0x5f;
            cmdbuf[95]=0x1f;
            cmdbuf[96]=0x04;
            cmdbuf[97]=0x00;
            cmdbuf[98]=0x00;
            cmdbuf[99]=0x10;
            cmdbuf[100]=0x16;
            cmdbuf[101]=0x03;
            cmdbuf[102]=0x00;*/
        }
        CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
        cmdbuf[6] = CRC_check & 0xff;
        cmdbuf[7] = (CRC_check >> 8) & 0xff;
        // data encrypt
        if (type == 2)
        {
            for (i = 0; i < 4; i++)
            {
                tpc_c_iv[8 + i] = cmdbuf[68 + i];
            }

            udf_AES_GCM_Encrypt(tpc_guk, tpc_gcm_ak, tpc_c_iv, &cmdbuf[72],
                                &cipher[0], &tag[0], 31, 17);
        }
        if (type == 4)
        {
            for (i = 0; i < 4; i++)
            {
                tpc_c_iv_h[8 + i] = cmdbuf[68 + i];
            }

            udf_AES_GCM_Encrypt(tpc_guk, tpc_gcm_ak, tpc_c_iv_h, &cmdbuf[72],
                                &cipher[0], &tag[0], 31, 17);
        }
        for (i = 0; i < 31; i++) // update cipher to buffer
        {
            cmdbuf[72 + i] = cipher[i];
        }

        for (i = 0; i < 12; i++) // update tag to buffer
        {
            cmdbuf[103 + i] = tag[i];
        }

        // CRC
        crc16 = 0xFFFF;
        for (i = 1; i < sizeof(cmdbuf) - 3; i++)
        {
            ch = cmdbuf[i];
            crc16 = UpdateCRC16(crc16, ch);
        }

        crc16 = ~crc16;
        cmdbuf[sizeof(cmdbuf) - 3] = crc16 & 0xFF;
        cmdbuf[sizeof(cmdbuf) - 2] = (crc16 >> 8) & 0xFF;

        for (i = 0; i < sizeof(cmdbuf); i++)
        {
            sprintf(strbuf, "%02X ", cmdbuf[i]);
            printf(strbuf);
        }
        printf("\n\r");

        app_uart_data_send(1, cmdbuf, sizeof(cmdbuf));
    }
}

static void udf_Management_Client(uint8_t type)
{
    unsigned char cmdbuf[] =
    {
        0x7E, 0xA0, 0x3F, 0x03, 0x23, 0x13, 0x9D, 0x1F, 0xE6, 0xE6, // 0~9
        0x00, 0xCB, 0x31, 0x30, 0x00, 0x00, 0x00, 0x03,             // 10~17
        0xC3, 0x01, 0x41,                                           // 18~20
        0x00, 0x0F, 0x00, 0x00, 0x28, 0x00, 0x02, 0xFF, 0x01, 0x01, // 21~30
        0x09, 0x11, 0x10, 0x00, 0x00, 0x00, 0x02, 0xF9, 0x20, 0x62,
        0xB8, 0x99, 0xF5, 0xA4, 0x0A, 0x4E, 0xCF, 0xC2, 0x8A, // 31~49
        0x00, 0xEA, 0xDB, 0x61, 0x8B, 0x34, 0x98, 0x1A, 0xBE, 0x16,
        0x66, 0xF2, 0xE3, // tag
        0x1E, 0x7E
    };      // 46B

    int i;
    char strbuf[128];
    unsigned char cipher[128], tag[16];
    unsigned short crc16, CRC_check;
    unsigned char ch;
    ic_count = 4;

    printf("\n\rManagement Client\n\r");
    CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
    cmdbuf[6] = CRC_check & 0xff;
    cmdbuf[7] = (CRC_check >> 8) & 0xff;
    // get GMAC tag
    for (i = 0; i < 4; i++)
    {
        tpc_c_iv[8 + i] = cmdbuf[34 + i];
    }

    udf_AES_GCM_Encrypt(tpc_guk, tpc_gmac_ak, tpc_c_iv, &cmdbuf[0], &cipher[0],
                        &tag[0], 0, 25);

    for (i = 0; i < 12; i++) // update GMAC tag to buffer
    {
        cmdbuf[38 + i] = tag[i];
    }

    // data encrypt
    for (i = 0; i < 4; i++)
    {
        tpc_c_iv[8 + i] = cmdbuf[14 + i];
    }

    udf_AES_GCM_Encrypt(tpc_guk, tpc_gcm_ak, tpc_c_iv, &cmdbuf[18], &cipher[0],
                        &tag[0], 32, 17);

    for (i = 0; i < 32; i++) // update cipher to buffer
    {
        cmdbuf[18 + i] = cipher[i];
    }

    for (i = 0; i < 12; i++) // update tag to buffer
    {
        cmdbuf[50 + i] = tag[i];
    }

    // CRC
    crc16 = 0xFFFF;
    for (i = 1; i < sizeof(cmdbuf) - 3; i++)
    {
        ch = cmdbuf[i];
        crc16 = UpdateCRC16(crc16, ch);
    }

    crc16 = ~crc16;
    cmdbuf[sizeof(cmdbuf) - 3] = crc16 & 0xFF;
    cmdbuf[sizeof(cmdbuf) - 2] = (crc16 >> 8) & 0xFF;

    for (i = 0; i < sizeof(cmdbuf); i++)
    {
        sprintf(strbuf, "%02X ", cmdbuf[i]);
        printf(strbuf);
    }
    printf("\n\r");

    app_uart_data_send(1, cmdbuf, sizeof(cmdbuf));
}

static void udf_Han_Client(void)
{
    unsigned char cmdbuf[] =
    {
        0x7E, 0xA0, 0x3F, 0x03, 0x27, 0x13, 0x9D, 0x1F, 0xE6, 0xE6, // 0~9
        0x00, 0xCB, 0x31, 0x30, 0x00, 0x00, 0x00, 0x02,             // 10~17
        0xC3, 0x01, 0x41,                                           // 18~20
        0x00, 0x0F, 0x00, 0x00, 0x28, 0x00, 0x04, 0xFF, 0x01, 0x01, // 21~30
        0x09, 0x11, 0x10, 0x00, 0x00, 0x00, 0x01, 0xF9, 0x20, 0x62,
        0xB8, 0x99, 0xF5, 0xA4, 0x0A, 0x4E, 0xCF, 0xC2, 0x8A, // 31~49
        0x00, 0xEA, 0xDB, 0x61, 0x8B, 0x34, 0x98, 0x1A, 0xBE, 0x16,
        0x66, 0xF2, 0xE3, // tag
        0x1E, 0x7E
    };      // 46B

    int i;
    char strbuf[128];
    unsigned char cipher[128], tag[16];
    unsigned short crc16, CRC_check;
    unsigned char ch;
    ic_count = 2;

    printf("\nHAN_Clinet\n\r");
    CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
    cmdbuf[6] = CRC_check & 0xff;
    cmdbuf[7] = (CRC_check >> 8) & 0xff;
    // get GMAC tag
    for (i = 0; i < 4; i++)
    {
        tpc_c_iv_h[8 + i] = cmdbuf[34 + i];
    }

    udf_AES_GCM_Encrypt(tpc_guk, tpc_gmac_ak, tpc_c_iv_h, &cmdbuf[0],
                        &cipher[0], &tag[0], 0, 25);

    for (i = 0; i < 12; i++) // update GMAC tag to buffer
    {
        cmdbuf[38 + i] = tag[i];
    }

    // data encrypt
    for (i = 0; i < 4; i++)
    {
        tpc_c_iv_h[8 + i] = cmdbuf[14 + i];
    }

    udf_AES_GCM_Encrypt(tpc_guk, tpc_gcm_ak, tpc_c_iv_h, &cmdbuf[18],
                        &cipher[0], &tag[0], 32, 17);

    for (i = 0; i < 32; i++) // update cipher to buffer
    {
        cmdbuf[18 + i] = cipher[i];
    }

    for (i = 0; i < 12; i++) // update tag to buffer
    {
        cmdbuf[50 + i] = tag[i];
    }

    // CRC
    crc16 = 0xFFFF;
    for (i = 1; i < sizeof(cmdbuf) - 3; i++)
    {
        ch = cmdbuf[i];
        crc16 = UpdateCRC16(crc16, ch);
    }

    crc16 = ~crc16;
    cmdbuf[sizeof(cmdbuf) - 3] = crc16 & 0xFF;
    cmdbuf[sizeof(cmdbuf) - 2] = (crc16 >> 8) & 0xFF;

    for (i = 0; i < sizeof(cmdbuf); i++)
    {
        sprintf(strbuf, "%02X ", cmdbuf[i]);
        printf(strbuf);
    }
    printf("\n\r");

    app_uart_data_send(1, cmdbuf, sizeof(cmdbuf));
}

static void Decide_Once(unsigned char *pt, unsigned char *meter_data)
{
    int i = 0;
    char strbuf[128];
    printf("Decide_Once\n");

    sprintf(strbuf, "pt[26] = %d\n", pt[26]);
    printf(strbuf);
    if (pt[26] == 0x01) // Voltage too low
    {
        if (NOTIFICATION_CODE[0] == 0)
        {
            printf("Event code = 1\n");
            for (i = 0; i < 60; i++)
            {
                event_notification_data[i] = meter_data[i];
            }
            udf_Rafael_data(1, 8, (char *)&event_notification_data[0], 3, 60);
            // Notification_Buff(&event_notification_data[0]);
            NOTIFICATION_CODE[0] = 1;
        }
    }
    else if (pt[26] == 0x02)   // no power
    {
        if (NOTIFICATION_CODE[1] == 0)
        {
            printf("Event code = 2\n");
            for (i = 0; i < 60; i++)
            {
                event_notification_data[i] = meter_data[i];
            }
            udf_Rafael_data(1, 8, (char *)&event_notification_data[0], 3, 60);
            // Notification_Buff(&event_notification_data[0]);
            NOTIFICATION_CODE[1] = 1;
        }
    }
    else if (pt[26] == 0x03)   // memory error
    {
        if (NOTIFICATION_CODE[2] == 0)
        {
            printf("Event code = 3\n");
            for (i = 0; i < 60; i++)
            {
                event_notification_data[i] = meter_data[i];
            }
            udf_Rafael_data(1, 8, (char *)&event_notification_data[0], 3, 60);
            // Notification_Buff(&event_notification_data[0]);
            // auto_send_type_buff(8,&event_notification_data[0]);
            NOTIFICATION_CODE[2] = 1;
        }
    }
    else if (pt[26] == 0x04)   // Non-volatile memory data error
    {
        if (NOTIFICATION_CODE[3] == 0)
        {
            printf("Event code = 4\n");
            for (i = 0; i < 60; i++)
            {
                event_notification_data[i] = meter_data[i];
            }
            udf_Rafael_data(1, 8, (char *)&event_notification_data[0], 3, 60);
            // Notification_Buff(&event_notification_data[0]);
            NOTIFICATION_CODE[3] = 1;
        }
    }
    else if (pt[26] == 0x07)   // Voltage low potential
    {
        if (NOTIFICATION_CODE[4] == 0)
        {
            printf("Event code = 7\n");
            for (i = 0; i < 60; i++)
            {
                event_notification_data[i] = meter_data[i];
            }
            udf_Rafael_data(1, 8, (char *)&event_notification_data[0], 3, 60);
            // Notification_Buff(&event_notification_data[0]);
            NOTIFICATION_CODE[4] = 1;
        }
    }
    else if (pt[26] == 0x64)   // The meter is not programmed
    {
        if (NOTIFICATION_CODE[5] == 0)
        {
            printf("Event code = 100\n");
            for (i = 0; i < 60; i++)
            {
                event_notification_data[i] = meter_data[i];
            }
            udf_Rafael_data(1, 8, (char *)&event_notification_data[0], 3, 60);
            // Notification_Buff(&event_notification_data[0]);
            NOTIFICATION_CODE[5] = 1;
        }
    }
    else   // If you receive anything else, just throw it away.
    {
        for (i = 0; i < 60; i++)
        {
            event_notification_data[i] = meter_data[i];
        }
        udf_Rafael_data(1, 8, (char *)&event_notification_data[0], 3, 60);
        // Notification_Buff(&event_notification_data[0]);
    }
}

static void udf_GetData_Request(uint8_t *obis_data, uint16_t ic_index,
                                uint8_t source)
{
    if (obis_data[0] == 0x01 && obis_data[1] == 0x07 && obis_data[2] == 0x01 &&
            obis_data[3] == 0x00 && obis_data[4] == 0x63 && obis_data[5] == 0x01 &&
            obis_data[6] == 0x00 && obis_data[7] == 0xFF && obis_data[8] == 0x02 &&
            obis_data[9] == 0x00) // The first bit was originally 0x00 and changed
        // to 0x01 for identification.
    {
        uint16_t CRC_check;
        unsigned char cmdbuf[] =
        {
            0x7E, 0xA0, 0x5F, 0x03, 0x23, 0x13, 0xF1, 0xF9, 0xE6, 0xE6, //[0-9]
            0x00, 0xD0, 0x51, 0x30, 0x00, 0x00, 0x00, 0x05, //[10-17]
            0xC0, 0x01, 0x42, 0x00,                         //[18-21]
            0x07, 0x01, 0x00, 0x63, 0x01, 0x00, 0xFF, 0x02, //[22-29]
            0x01, 0x01,                                     //[30-31]
            0x02, 0x04,                                     //[32-33]
            0x02, 0x04,                                     //[34-35]
            0x12, 0x00, 0x08,                               //[36-38]
            0x09, 0x06, 0x00, 0x00, 0x01, 0x00, 0x00, 0xFF, 0x0F, 0x02,
            0x12, 0x00, 0x00, //[39-51]
            0x09, 0x0C, 0x07, 0xE1, 0x07, 0x15, 0x05, 0x07, 0x00, 0x00,
            0xFF, 0x80, 0x00, 0x00, //[52-65]
            0x09, 0x0C, 0x07, 0xE1, 0x07, 0x15, 0x05, 0x08, 0x00, 0x00,
            0xFF, 0x80, 0x00, 0x00, //[66-79]
            0x01, 0x00,             //[80-81]
            0xEA, 0xDB, 0x61, 0x8B, 0x34, 0x98, 0x1A, 0xBE, 0x16, 0x66,
            0xF2, 0x12, 0xE3, 0x1E, 0x7E
        };
        int i;
        char strbuf[128];
        unsigned char cipher[128], tag[16];
        unsigned short crc16;
        unsigned char ch;
        uint8_t pt_len;

        if (source == 1)
        {
            cmdbuf[4] = 0x23;
        }
        if (source == 2)
        {
            cmdbuf[4] = 0x27;
        }

        CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
        cmdbuf[6] = CRC_check & 0xff;
        cmdbuf[7] = (CRC_check >> 8) & 0xff;

        // update buffer
        cmdbuf[16] = (ic_index >> 8) & 0xFF;
        cmdbuf[17] = ic_index & 0xFF;

        cmdbuf[20] = ((ic_index - 3) % 0x40) + 0x40;

        // uart_read_data2[44]

        for (i = 0; i <= 7; i++) // Modify date match FDM
        {
            cmdbuf[i + 54] = uart_read_data2[44 + i];
            cmdbuf[i + 68] = uart_read_data2[58 + i];
        }

        // data encrypt
        pt_len = cmdbuf[12] - 17;
        if (source == 1) // M_GET
        {
            for (i = 0; i < 4; i++)
            {
                tpc_c_iv[8 + i] = cmdbuf[14 + i];
            }

            udf_AES_GCM_Encrypt(tpc_dk, tpc_gcm_ak, tpc_c_iv, &cmdbuf[18],
                                &cipher[0], &tag[0], pt_len, 17);
        }
        if (source == 2) // HAN_GET
        {
            for (i = 0; i < 4; i++)
            {
                tpc_c_iv_h[8 + i] = cmdbuf[14 + i];
            }

            udf_AES_GCM_Encrypt(tpc_dk, tpc_gcm_ak, tpc_c_iv_h, &cmdbuf[18],
                                &cipher[0], &tag[0], pt_len, 17);
        }
        for (i = 0; i < pt_len; i++) // update cipher to buffer
        {
            cmdbuf[18 + i] = cipher[i];
        }

        for (i = 0; i < 12; i++) // update tag to buffer
        {
            cmdbuf[18 + pt_len + i] = tag[i];
        }

        // CRC
        crc16 = 0xFFFF;
        for (i = 1; i < sizeof(cmdbuf) - 3; i++)
        {
            ch = cmdbuf[i];
            crc16 = UpdateCRC16(crc16, ch);
        }

        crc16 = ~crc16;
        cmdbuf[sizeof(cmdbuf) - 3] = crc16 & 0xFF;
        cmdbuf[sizeof(cmdbuf) - 2] = (crc16 >> 8) & 0xFF;

        for (i = 0; i < sizeof(cmdbuf); i++)
        {
            sprintf(strbuf, "%02X ", cmdbuf[i]);
            printf(strbuf);
        }
        printf("\n\r");

        app_uart_data_send(1, cmdbuf, sizeof(cmdbuf));
    }
    else
    {
        unsigned char cmdbuf[] =
        {
            0x7E, 0xA0, 0x2C, 0x03, 0x23, 0x13, 0xF1, 0xF9, 0xE6, 0xE6,
            0x00, 0xD0, 0x1E, 0x30, 0x00, 0x00, 0x00, 0x05, 0xC0, 0x01,
            0x42, 0x00, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0xFF, 0x02,
            0x00, 0xEA, 0xDB, 0x61, 0x8B, 0x34, 0x98, 0x1A, 0xBE, 0x16,
            0x66, 0xF2, 0x12, 0xE3, 0x1E, 0x7E
        }; // 46B

        int i;
        char strbuf[128];
        unsigned char cipher[128], tag[16];
        unsigned short crc16, CRC_check;
        unsigned char ch;
        uint8_t pt_len;
        if (source == 1)
        {
            cmdbuf[4] = 0x23;
        }
        else if (source == 2)
        {
            cmdbuf[4] = 0x27;
        }

        CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
        cmdbuf[6] = CRC_check & 0xff;
        cmdbuf[7] = (CRC_check >> 8) & 0xff;

        // update buffer
        cmdbuf[16] = (ic_index >> 8) & 0xFF;
        cmdbuf[17] = ic_index & 0xFF;

        cmdbuf[20] = ((ic_index - 3) % 0x40) + 0x40;

        for (i = 0; i < 10; i++)
        {
            cmdbuf[21 + i] = obis_data[i];
        }

        // data encrypt
        pt_len = cmdbuf[12] - 17;

        if (source == 1) // M_GET
        {
            for (i = 0; i < 4; i++)
            {
                tpc_c_iv[8 + i] = cmdbuf[14 + i];
            }

            udf_AES_GCM_Encrypt(tpc_dk, tpc_gcm_ak, tpc_c_iv, &cmdbuf[18],
                                &cipher[0], &tag[0], pt_len, 17);
        }
        if (source == 2) // HAN_GET
        {
            for (i = 0; i < 4; i++)
            {
                tpc_c_iv_h[8 + i] = cmdbuf[14 + i];
            }

            udf_AES_GCM_Encrypt(tpc_dk, tpc_gcm_ak, tpc_c_iv_h, &cmdbuf[18],
                                &cipher[0], &tag[0], pt_len, 17);
        }

        for (i = 0; i < pt_len; i++) // update cipher to buffer
        {
            cmdbuf[18 + i] = cipher[i];
        }

        for (i = 0; i < 12; i++) // update tag to buffer
        {
            cmdbuf[18 + pt_len + i] = tag[i];
        }

        // CRC
        crc16 = 0xFFFF;
        for (i = 1; i < sizeof(cmdbuf) - 3; i++)
        {
            ch = cmdbuf[i];
            crc16 = UpdateCRC16(crc16, ch);
        }

        crc16 = ~crc16;
        cmdbuf[sizeof(cmdbuf) - 3] = crc16 & 0xFF;
        cmdbuf[sizeof(cmdbuf) - 2] = (crc16 >> 8) & 0xFF;

        for (i = 0; i < sizeof(cmdbuf); i++)
        {
            sprintf(strbuf, "%02X ", cmdbuf[i]);
            printf(strbuf);
        }
        printf("\n\r");

        app_uart_data_send(1, cmdbuf, sizeof(cmdbuf));
    }
}

static void udf_Get_New_Number_of_pens(uint16_t ic_count, uint8_t *obis,
                                       uint8_t obis_number) // Latest data
{
    uint16_t CRC_check;
    unsigned char cmdbuf[] =
    {
        0x7E, 0xA0, 0x3F, 0x03, 0x23, 0x13, 0xF1, 0xF9, 0xE6, 0xE6, //[0-9]
        0x00, 0xD0, 0x31, 0x30, 0x00, 0x00, 0x00, 0x05,             //[10-17]
        0xC0, 0x01, 0x42, 0x00,                                     //[18-21]
        0x07, 0x00, 0x00, 0x63, 0x62, 0x00, 0xFF, 0x02,             //[22-29]
        0x01, 0x02,                                                 //[30-31]
        0x02, 0x04,                                                 //[32-33]
        0x06, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x00, 0x00, 0x05, 0x12,
        0x00, 0x01, 0x12, 0x00, 0x00, 0xEA, 0xDB, 0x61, 0x8B, 0x34, 0x98,
        0x1A, 0xBE, 0x16, 0x66, 0xF2, 0x12, 0xE3, 0x1E, 0x7E
    };
    int i;
    char strbuf[128];
    unsigned char cipher[128], tag[16];
    unsigned short crc16;
    unsigned char ch;
    uint8_t pt_len;
    for (i = 0; i <= 7; i++)
    {
        cmdbuf[i + 22] = obis[i];
    }
    if (obis_number == 1)
    {
        printf("Power ON\n\r");
    }
    else
    {
        for (i = 34; i <= 43; i++)
        {
            cmdbuf[i] = obis[i - 25];
        }
    }
    CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
    cmdbuf[6] = CRC_check & 0xff;
    cmdbuf[7] = (CRC_check >> 8) & 0xff;

    // update buffer
    cmdbuf[16] = (ic_count >> 8) & 0xFF;
    cmdbuf[17] = ic_count & 0xFF;

    cmdbuf[20] = ((ic_count - 3) % 0x40) + 0x40;

    // uart_read_data2[44]

    //      for(i = 0;i <= 7;i++) //Modify date match FDM
    //      {
    //          cmdbuf[i + 54] = uart_read_data2[44+i];
    //          cmdbuf[i + 68] = uart_read_data2[58+i];
    //      }

    // data encrypt
    pt_len = cmdbuf[12] - 17;

    for (i = 0; i < 4; i++)
    {
        tpc_c_iv[8 + i] = cmdbuf[14 + i];
    }

    udf_AES_GCM_Encrypt(tpc_dk, tpc_gcm_ak, tpc_c_iv, &cmdbuf[18], &cipher[0],
                        &tag[0], pt_len, 17);

    for (i = 0; i < pt_len; i++) // update cipher to buffer
    {
        cmdbuf[18 + i] = cipher[i];
    }

    for (i = 0; i < 12; i++) // update tag to buffer
    {
        cmdbuf[18 + pt_len + i] = tag[i];
    }

    // CRC
    crc16 = 0xFFFF;
    for (i = 1; i < sizeof(cmdbuf) - 3; i++)
    {
        ch = cmdbuf[i];
        crc16 = UpdateCRC16(crc16, ch);
    }

    crc16 = ~crc16;
    cmdbuf[sizeof(cmdbuf) - 3] = crc16 & 0xFF;
    cmdbuf[sizeof(cmdbuf) - 2] = (crc16 >> 8) & 0xFF;

    for (i = 0; i < sizeof(cmdbuf); i++)
    {
        sprintf(strbuf, "%02X ", cmdbuf[i]);
        printf(strbuf);
    }
    printf("\n\r");

    app_uart_data_send(1, cmdbuf, sizeof(cmdbuf));
}

static void udf_SMIP_Res(uint8_t *meter_data)
{
    char send_buff[128] = {0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0D,
                           0xC4, 0x01, 0x43, 0x00, 0x0A, 0x08, 0x31, 0x36,
                           0x36, 0x39, 0x33, 0x37, 0x35, 0x31
                          };
    char strbuf[128];

    uint16_t i = 0;

    send_buff[10] = riva_fix_cnt;

    for (i = 0; i < 8; i++)
    {
        send_buff[i + 14] = meter_data[i + 17];
    }

    printf("Send_buff= ");

    for (i = 0; i < 22; i++)
    {
        sprintf(strbuf, "%02X ", send_buff[i]); // test only
        printf(strbuf);
    }
    printf("\n\r");

    for (i = 0; i < 22; i++)
    {
        ucMeterPacket[i] = send_buff[i];
    }
#if (CONFIG_UART_STDIO_PORT == 0)
    app_uart_data_send(2, ucMeterPacket, 22);
#else
    app_uart_data_send(0, ucMeterPacket, 22);
#endif
}

static void on_Demand_Task_Assembly(unsigned char *auto_data, uint16_t ct_len,
                                    unsigned char *pt_confirm)
{
    unsigned char auto_send_data[1000];
    char strbuf[128];
    int i = 0, data_len, j = 0; //,check_sum = 0
    unsigned char obis_data[10] = {0x00, 0x07, 0x01, 0x00, 0x63,
                                   0x01, 0x00, 0xFF, 0x02, 0x00
                                  };
    data_len = ct_len;
    for (i = 0; i < data_len; i++)
    {
        auto_send_data[i] = pt_confirm[i];
    }

    sprintf(strbuf, "pt[1] = %d , pt[3] = %d\n\r", pt_confirm[1],
            pt_confirm[3]);
    printf(strbuf);
    if ((pt_confirm[1] == 0x02 && pt_confirm[3] == 0x01) ||
            (pt_confirm[1] == 0x02 && pt_confirm[3] == 0x00) ||
            (pt_confirm[1] == 0x01 && pt_confirm[3] == 0x00) ||
            pt_confirm[1] == 0x01)
    {
        if (Task_count_all == Task_count)
        {
            for (i = 0, j = 0; i < data_len; i++, j++)
            {
                On_Demand_Task_Buff[j] = pt_confirm[i];
                sprintf(strbuf, "%02X ", On_Demand_Task_Buff[j]);
                printf(strbuf);
            }
            On_Demand_length = On_Demand_length + data_len;
            sprintf(strbuf, "On_Demand_length = %d \n", On_Demand_length);
            printf(strbuf);
        }
        else
        {
            for (i = 0, j = On_Demand_length; i < data_len; i++, j++)
            {
                On_Demand_Task_Buff[j] = pt_confirm[i];
                sprintf(strbuf, "%02X ", On_Demand_Task_Buff[j]);
                printf(strbuf);
            }
            On_Demand_length = On_Demand_length + data_len;
            sprintf(strbuf, "On_Demand_length = %d \n", On_Demand_length);
            printf(strbuf);
        }

        Task_count--;

        if (Task_count > 0)
        {
            sprintf(strbuf, "Task_count = %d FIRST = %02X  END = %02X \n",
                    Task_count, (Task_count_all - Task_count) * 9 + 1,
                    (Task_count_all - Task_count) * 9 + 9);
            printf(strbuf);

            for (i = (Task_count_all - Task_count) * 9, j = 1;
                    i < (Task_count_all - Task_count) * 9 + 9; i++, j++)
            {
                obis_data[j] = On_Demand_Task_Command[i];
            }
            udf_GetData_Request(obis_data, ic_count, 1);
        }
        if (Task_count == 0)
        {
            printf("On_Demand_Task_Buff = ");
            for (i = 0; i < On_Demand_length; i++)
            {
                sprintf(strbuf, "%02X ", On_Demand_Task_Buff[i]);
                printf(strbuf);
            }
            printf("\n");
            udf_Rafael_data(TASK_ID, 7, (char *)&On_Demand_Task_Buff[0], 3,
                            On_Demand_length);
            memset(On_Demand_Task_Buff, 0, sizeof(On_Demand_Task_Buff));
            On_Demand_length = 0;
            On_Demand_Reading_Type = 0;
            ondemand_idle = 0; // frank,201811
        }
    }
    else
    {
        On_Demand_Retry++;
        if (On_Demand_Retry < 2)
        {
            for (i = (Task_count_all - Task_count) * 9 + 1, j = 1;
                    i < (Task_count_all - Task_count) * 9 + 10; i++, j++)
            {
                obis_data[j] = On_Demand_Task_Command[i];
            }
            udf_GetData_Request(obis_data, ic_count, 1);
        }
        else
        {
            memset(On_Demand_Task_Buff, 0, sizeof(On_Demand_Task_Buff));
            On_Demand_length = 0;
            On_Demand_Reading_Type = 0;
            ondemand_idle = 0;
            On_Demand_Retry = 0;
        }
    }
}

static void Power_On_Event_Log_Return(
    unsigned char *Notification_pt) // Send reply message
{
    int i = 0, j = 0, k = 0;
    unsigned short crc16;
    unsigned char cipher[128], tag[16];
    // uint8_t pt_len;
    char strbuf[128];
    unsigned char ch;
    uint8_t NEW_POWER_ON_FLAG = 0, FLAG_POWER_ON = 0;
    unsigned char obis_Power_On_Event_Code[] = {0x12, 0x00, 0x0f},
            Power_On_data_Buff[15] = {0};
    unsigned char Power_On_Event_Buff[] =
    {
        0x7E, 0xA0, 0x3A, 0x23, 0x03, 0x13, 0xC2, 0x51, 0xE6, 0xE6, 0x00, 0xCA,
        0x2C, 0x30, 0x00, 0x00, 0x00, 0x01, 0xc2, 0x01, 0x0c, 0x07, 0xe2, 0x09,
        0x0a, 0xff, 0x0e, 0x03, 0x04, 0xff, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x00, 0x60, 0x0b, 0x00, 0xff, 0x02, 0x12, 0x00, 0x07, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x00, 0x00, 0x7e
    };
    printf("pt[i] = ");
    if (Notification_pt[7] == 4) // one way
    {
        for (i = 0; i <= 136; i++)
        {
            if (Notification_pt[i] == obis_Power_On_Event_Code[0])
            {
                if (Notification_pt[i + 1] == obis_Power_On_Event_Code[1])
                {
                    if (Notification_pt[i + 2] == obis_Power_On_Event_Code[2])
                    {
                        if (Notification_pt[i - 1] == 0x00 &&
                                Notification_pt[i - 2] == 0x00 &&
                                Notification_pt[i - 3] == 0x80 &&
                                Notification_pt[i - 4] == 0xff)
                        {
                            for (j = i - 12, k = 0; j <= i + 2 & k <= 14;
                                    j++, k++)
                            {
                                if (NEW_POWER_ON_FLAG == 0)
                                {
                                    Power_On_data_Buff[k] = Notification_pt[j];
                                    sprintf(strbuf, "%02X ",
                                            Power_On_data_Buff[k]);
                                    printf(strbuf);
                                    FLAG_POWER_ON = 1;
                                }
                            }
                        }
                        NEW_POWER_ON_FLAG = 1;
                    }
                }
            }
        }
    }
    else if (Notification_pt[7] == 5)   // Two-way
    {
        for (i = 0; i <= 161; i++)
        {
            if (Notification_pt[i] == obis_Power_On_Event_Code[0])
            {
                if (Notification_pt[i + 1] == obis_Power_On_Event_Code[1])
                {
                    if (Notification_pt[i + 2] == obis_Power_On_Event_Code[2])
                    {
                        if (Notification_pt[i - 1] == 0x00 &&
                                Notification_pt[i - 2] == 0x00 &&
                                Notification_pt[i - 3] == 0x80 &&
                                Notification_pt[i - 4] == 0xff)
                        {
                            for (j = i - 12, k = 0; j <= i + 2 & k <= 14;
                                    j++, k++)
                            {
                                if (NEW_POWER_ON_FLAG == 0)
                                {
                                    Power_On_data_Buff[k] = Notification_pt[j];
                                    sprintf(strbuf, "%02X ",
                                            Power_On_data_Buff[k]);
                                    printf(strbuf);
                                    FLAG_POWER_ON = 1;
                                }
                            }
                        }
                        NEW_POWER_ON_FLAG = 1;
                    }
                }
            }
        }
    }
    // IWDG_ReloadCounter();
    if (FLAG_POWER_ON == 1)
    {
        for (i = 21; i <= 32; i++)
        {
            Power_On_Event_Buff[i] = Power_On_data_Buff[i - 21];    // time
        }
        for (i = 42; i <= 44; i++)
        {
            Power_On_Event_Buff[i] = Power_On_data_Buff[i - 30];    // event code
        }

        printf("\nNotification = ");
        for (i = 0; i < sizeof(Power_On_Event_Buff); i++)
        {
            sprintf(strbuf, "%02X ", Power_On_Event_Buff[i]);
            printf(strbuf);
        }
        printf("\n");
        for (i = 0; i < 4; i++)
        {
            tpc_s_iv[8 + i] = Power_On_Event_Buff[14 + i];
        }

        udf_AES_GCM_Encrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv,
                            &Power_On_Event_Buff[18], &cipher[0], &tag[0], 27,
                            17);
        for (i = 0; i < 27; i++) // update cipher to buffer
        {
            Power_On_Event_Buff[18 + i] = cipher[i];
        }
        printf("tag[i] = ");
        for (i = 0; i < 12; i++) // update tag to buffer
        {
            Power_On_Event_Buff[45 + i] = tag[i];
            sprintf(strbuf, "%02X ", tag[i]);
            printf(strbuf);
        }
        printf("\n");
        // CRC
        crc16 = 0xFFFF;
        for (i = 1; i < sizeof(Power_On_Event_Buff) - 3; i++)
        {
            ch = Power_On_Event_Buff[i];
            crc16 = UpdateCRC16(crc16, ch);
        }

        crc16 = ~crc16;
        Power_On_Event_Buff[sizeof(Power_On_Event_Buff) - 3] = crc16 & 0xFF;
        Power_On_Event_Buff[sizeof(Power_On_Event_Buff) - 2] =
            (crc16 >> 8) & 0xFF;

        printf("\n Encrypt Notification = ");
        for (i = 0; i < sizeof(Power_On_Event_Buff); i++)
        {
            sprintf(strbuf, "%02X ", Power_On_Event_Buff[i]);
            printf(strbuf);
        }
        printf("\n");
        udf_Rafael_data(
            1, 8, (char *)&Power_On_Event_Buff[0], 3,
            (Power_On_Event_Buff[1] - 0xA0) * 256 + Power_On_Event_Buff[2] + 2);
        // Notification_Buff(&Power_On_Event_Buff[0]);
    }
    else
    {
        printf("NO Power ON NOTIFICATION\n");
    }
}

static void udf_Set_Transmit(unsigned char obisTXT[], unsigned char plainTXT[],
                             uint16_t plainTXT_Len, uint32_t frameNum,
                             uint8_t lastNum)
{
    unsigned char cmdTXT[779] =
    {
        0x7E, 0xA0, 0x2C, 0x03, 0x23, 0x13, 0x6C, 0x8D, 0xE6, 0xE6,
        0x00, 0xD1, 0x1E, 0x30, 0x00, 0x00, 0x00, 0x01, 0xC1, 0x01,
        0x49, 0x00, 0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 50
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 100
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 150
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 200
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 250
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 300
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 350
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 400
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 450
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 500
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 550
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 600
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 650
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 700
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, // 750
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x7E
    }; // 779B

    uint8_t shift = 0, shiftTrans = 0, shiftOBIS = 0;
    uint16_t i = 0, frameLen = 0, cipherLen = 0, plainLen = 0;
    unsigned short crc16;
    unsigned char ch;
    char strbuf[777];
    unsigned char cipher[744], tag[16];
    // int riva_cnt = 0x40;
    printf("in Set Transmit\n\r");
    SET_BUSY_FLAG = 1;
    plainLen = plainTXT_Len;
    cipherLen = plainLen + 21; // 5 + 4 + 12
    frameLen = cipherLen + 14; // 12 + 2

    // Total frame length modification
    if (frameLen >= 256)
    {
        if (cipherLen >= 256) // encrypt plain text length modification
        {
            shift = 2;
            cmdTXT[12] = 0x82; // extra
            cmdTXT[15] = 0x30;
        }
        else if (cipherLen >= 128)
        {
            shift = 1;
            cmdTXT[12] = 0x81; // extra
        }
    }
    else if (frameLen >= 128)
    {
        if (cipherLen >= 128)
        {
            shift = 1;
            cmdTXT[12] = 0x81; // extra
        }
        else
        {
            shift = 0;
        }
    }
    else
    {
        shift = 0;
    }

    cmdTXT[14 + shift] = 0x00;
    cmdTXT[15 + shift] = 0x00;
    cmdTXT[16 + shift] = (ic_count >> 8) & 0xFF;
    cmdTXT[17 + shift] = ic_count & 0xFF;
    cmdTXT[18 + shift] = 0xC1;
    cmdTXT[19 + shift] = 01;
    cmdTXT[20 + shift] = ((ic_count - 3) % 0x40) + 0x40;
    ; // 40�}�l
    cmdTXT[21 + shift] = (lastNum > 0) ? 01 : 00;

    for (i = 0; i < 9; i++)
    {
        cmdTXT[22 + shift + i] = obisTXT[i]; // OBIS insertion
    }
    shiftOBIS = 9;

    // Continuing transmit set up
    if (frameNum != 0)
    {
        if (frameNum == 1)
        {
            cmdTXT[19 + shift] = 0x02; // transmit command
            cmdTXT[31 + shift] = 0x00; // extra "1"
            shiftOBIS++;
        }
        else
        {
            cmdTXT[19 + shift] = 0x03; // transmit command
            shiftOBIS -= 9;
        }
        cmdTXT[22 + shift + shiftOBIS] = (frameNum >> 24) & 0xFF; // extra
        cmdTXT[23 + shift + shiftOBIS] = (frameNum >> 16) & 0xFF; // extra
        cmdTXT[24 + shift + shiftOBIS] = (frameNum >> 8) & 0xFF;  // extra
        cmdTXT[25 + shift + shiftOBIS] = frameNum & 0xFF;         // extra
        if (plainLen >= 256)
        {
            cmdTXT[26 + shift + shiftOBIS] = 0x82;                   // extra
            cmdTXT[27 + shift + shiftOBIS] = (plainLen >> 8) & 0xFF; // extra
            cmdTXT[28 + shift + shiftOBIS] = plainLen & 0xFF;        // extra
            shiftTrans = 7;                                          // 4 + 3
        }
        else if (plainLen >= 128)
        {
            cmdTXT[26 + shift + shiftOBIS] = 0x81;            // extra
            cmdTXT[27 + shift + shiftOBIS] = plainLen & 0xFF; // extra
            shiftTrans = 6;                                   // 4 + 2
        }
        else
        {
            cmdTXT[26 + shift + shiftOBIS] = plainLen & 0xFF; // extra
            shiftTrans = 5;                                   // 4 + 1
        }
    }

    // Length adjustment
    frameLen += (shift + shiftTrans + shiftOBIS);
    cipherLen += (shiftTrans + shiftOBIS);
    cmdTXT[1] = ((frameLen >> 8) & 0xFF) + 0xA0;
    cmdTXT[2] = (frameLen) & 0xFF;
    switch (shift)
    {
    case 0:
        cmdTXT[12] = cipherLen;
        cmdTXT[13] = 0x30;
        break;
    case 1:
        cmdTXT[13] = cipherLen;
        cmdTXT[14] = 0x30;
        break;
    case 2:
        cmdTXT[13] = ((cipherLen) >> 8) & 0xFF; // extra
        cmdTXT[14] = (cipherLen) & 0xFF;
        break;
    default:
        break;
    }

    // Byte1~5 CRC
    crc16 = 0xFFFF;
    for (i = 1; i < 6; i++)
    {
        ch = cmdTXT[i];
        crc16 = UpdateCRC16(crc16, ch);
    }

    // Byte1~5 CRC  insertion
    crc16 = ~crc16;
    cmdTXT[6] = crc16 & 0xFF;
    cmdTXT[7] = (crc16 >> 8) & 0xFF;

    // plainTXT insertion
    for (i = 0; i < plainLen; i++)
    {
        cmdTXT[22 + shift + shiftTrans + shiftOBIS + i] = plainTXT[i];
    }

    // Data encrypt
    for (i = 0; i < 4; i++)
    {
        tpc_c_iv[8 + i] = cmdTXT[14 + shift + i]; // ��count�a�J��iv��
    }

    udf_AES_GCM_Encrypt(tpc_dk, tpc_gcm_ak, tpc_c_iv, &cmdTXT[18 + shift],
                        &cipher[0], &tag[0],
                        plainLen + shiftTrans + shiftOBIS + 4,
                        17); // ��Jcmdbuf,���ͧ������cipher����

    // Cipher insertion
    for (i = 0; i < 4 + shiftOBIS + shiftTrans + plainLen; i++)
    {
        cmdTXT[18 + shift + i] = cipher[i];
    }

    // Tag insertion
    for (i = 0; i < 12; i++)
    {
        cmdTXT[22 + shift + shiftTrans + shiftOBIS + plainLen + i] = tag[i];
    }

    // Total CRC
    crc16 = 0xFFFF;
    for (i = 1; i <= frameLen - 2; i++)
    {
        ch = cmdTXT[i];
        crc16 = UpdateCRC16(crc16, ch);
    }

    // Total CRC    insertion
    crc16 = ~crc16;
    cmdTXT[frameLen - 1] = crc16 & 0xFF;
    cmdTXT[frameLen] = (crc16 >> 8) & 0xFF;
    cmdTXT[frameLen + 1] = 0x7E; // end flag

    /*for (i = 0; i < frameLen + 2 ; i++)
    {
        sprintf(strbuf, "%02X ", cmdTXT[i]);
        printf(strbuf);
    }
    printf("\n\r");*/
    for (i = 0; i < frameLen + 2; i++)
    {
        sprintf(strbuf, "%02X ", cmdTXT[i]);
        printf(strbuf);
    }
    printf("\n\r");

    app_uart_data_send(1, cmdTXT, (frameLen + 2));
}

static void Set_TOU_C2(uint8_t start)
{
    unsigned char obis_tou[9] = {20, 0, 0, 13, 0, 0, 255, 0, 0};
    uint16_t dataLen = 0; //,ip_len=0;
    // uint16_t i = 0;//rf_Len = 0,
    /*05                                        saveSETdata[0]
    6(Calendar name)    00 08 saveSETdata[1~3]
    7(Season profile)   01 23 saveSETdata[4~6]
    8(Week profile)     01 23 saveSETdata[7~9]
    9(Day profile)      01 23 saveSETdata[10~12]
    10(Active time)     01 23 saveSETdata[13~15]  */
    printf("\n\r---Set_TOU_C2---\n\r");

    dataLen = (saveSETdata[2 + (start - 1) * 3] << 8) +
              saveSETdata[3 + (start - 1) * 3];

    SET_TOU_C2_Start++;
    // SET_TOU_C2_Start = (SET_TOU_C2_Start == 5) ? 0:SET_TOU_C2_Start;//5�k�s

    obis_tou[7] = saveSETdata[1 + (start - 1) * 3]; // attribute_id
    udf_Set_Transmit(obis_tou, &saveSETdata[16 + SETdata_index], dataLen, 0, 0);

    SETdata_index += dataLen;
}

void SQBrespone(uint8_t type, unsigned char *auto_data)
{
    unsigned char SQB_send_data[1000];
    int i, data_len, check_sum = 0;

    printf("SQB!!!!!respone!!!!!!!!!111\n\r");
    SQB_send_data[0] = 0x8E;

    for (i = 0; i <= 3; i++)
    {
        SQB_send_data[i + 1] = Initial_Value[i];
    }
    SQB_send_data[5] = 0xFF;
    SQB_send_data[6] = 0xFF;
    SQB_send_data[7] = 0xFF;
    SQB_send_data[8] = 0xFF;

    data_len = ((auto_data[1] - 0xA0) * 256 + auto_data[2] + 2); // ��ƪ���
    SQB_send_data[9] = data_len / 256;
    SQB_send_data[10] = (data_len % 256) + 1;
    SQB_send_data[11] = type;
    for (i = 0; i <= data_len; i++)
    {
        SQB_send_data[i + 12] = auto_data[i];
    }
    for (i = 1; i <= data_len + 11; i++)
    {
        check_sum += SQB_send_data[i];
    }
    SQB_send_data[data_len + 12] = check_sum;
    SQB_send_data[data_len + 13] = 0x8E;
    // 4G�ǰe
    //      if(ACTION_CHANGE_KEY == 1)
    //          RF_MessageQueue(SQB_send_data, data_len+14, 1);
    //      else
    //          RF_MessageQueue(SQB_send_data, data_len+14, 1);

    SET_BUSY_FLAG = 0; // bob
    ondemand_idle = 0; // frank,201811
}

void set_Test()
{
    int sendPoint = 700, i = 0, lastnum = 0;
    unsigned char TOUobisTXT[9] = {0};

    SETACTION = (setdataLen > 700) ? 1 : 0;
    lastnum = (setdataLen - sendPoint * SETACTION > 0) ? 0 : 1;
    for (i = 0; i < 9; i++)
    {
        TOUobisTXT[i] = saveSETdata[i];
    }
    /*/ if(TOUobisTXT[0] == 8 && TOUobisTXT[1] == 0 && TOUobisTXT[2] == 0 &&
    //       TOUobisTXT[3] == 1 && TOUobisTXT[4] == 0 && TOUobisTXT[5] == 0 &&
    //       TOUobisTXT[6] == 255 && TOUobisTXT[7] == 2 && TOUobisTXT[8] == 0)
    //      {
    //          begin.tm_year = (saveSETdata[10] * 256 + saveSETdata[11] -
    1900);
    //          begin.tm_mon  = saveSETdata[12] - 1;
    //          begin.tm_mday = saveSETdata[13];
    //          begin.tm_hour = saveSETdata[15];
    //          begin.tm_min  = saveSETdata[16];
    //          begin.tm_sec  = saveSETdata[17];
    //          now_time = mktime(&begin);//19
    //
    //          //Modification time + time difference = current time
    //          now_time  = mktime(&begin);
    //      }*/
    if (SETACTION == 0)   // No need to continue uploading
    {
        udf_Set_Transmit(TOUobisTXT, &saveSETdata[9], setdataLen - 9, SETACTION,
                         0);
        setdataLen = 0;
        memset(saveSETdata, 0, sizeof(saveSETdata));
    }
    else if (SETACTION == 1)
    {
        hitHANDguN = 1;
        udf_Set_Transmit(TOUobisTXT, &saveSETdata[9], sendPoint, SETACTION,
                         lastnum);
    }
    else
    {
        udf_Set_Transmit(TOUobisTXT, &saveSETdata[9 + sendPoint * SETACTION],
                         setdataLen - sendPoint * SETACTION, SETACTION,
                         lastnum);
        if (lastnum == 1)   // reset parameter
        {
            SETACTION = 0;
            setdataLen = 0;
            hitHANDguN = 0;
            memset(saveSETdata, 0, sizeof(saveSETdata));
        }
    }
}

void udf_Alarm_Register1(void)
{
#if 0 // for arter  
    unsigned char cmdbuf[] =
    {
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
        0x00, 0x09,
        0xC4, 0x01, 0x43, 0x00,
        0x06,
        0x00, 0x00, 0x00, 0x00
    };
    unsigned int write_len;
    int i;
    char strbuf[128];

    cmdbuf[10] = uart_read_data2[10];
    write_len = 17;
    cmdbuf[10] = uart_read_data2[10];

    if (Event_Notification == 1)
    {
        cmdbuf[15] = 0x20;
    }
    else
    {
        cmdbuf[15] = 0x00;
    }

    printf("Send Alarm Register1\n\r");
    for (i = 0; i < write_len; i++)
    {
        ucMeterPacket[i] = cmdbuf[i];
        USART_SendData(USART2, ucMeterPacket[i]); // stay for test
        while (!(USART2->STS & USART_FLAG_TXE));

        sprintf(strbuf, "%02X ", ucMeterPacket[i]);
        printf(strbuf);
    }

    Event_Notification = 0;
    printf("\n\r");
#endif
}

void udf_Send_DISC(uint8_t type)
{
    unsigned char cmdbuf[] = {0x7E, 0xA0, 0x07, 0x03, 0x23,
                              0x53, 0xB3, 0xF4, 0x7E
                             };
    int i;
    char strbuf[128];
    uint16_t CRC_check;

    if (type == 1)
    {
        printf("\n\rVerification_DISC\n");
        cmdbuf[4] = 0x21;
    }
    else if (type == 2)
    {
        DISC_FLAG = 1;
        printf("\n\rManagement_DISC\n");
        cmdbuf[4] = 0x23;
    }

    CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
    cmdbuf[6] = CRC_check & 0xff;
    cmdbuf[7] = (CRC_check >> 8) & 0xff;
    printf("\n\rDISC-DM\n");
    for (i = 0; i < sizeof(cmdbuf); i++)
    {
        sprintf(strbuf, "%02X ", cmdbuf[i]);
        printf(strbuf);
    }
    printf("\n\r");

    app_uart_data_send(1, cmdbuf, sizeof(cmdbuf));
}

void udf_Send_SNRM(uint8_t type)
{
    unsigned char cmdbuf[] = {0x7E, 0xA0, 0x07, 0x03, 0x23,
                              0x93, 0xBF, 0x32, 0x7E
                             };
    int i;
    char strbuf[128];
    uint16_t CRC_check;
    if (type == 1)
    {
        printf("\n\rVerification_SNRM\n");
        cmdbuf[4] = 0x21;
    }
    else if (type == 2)
    {
        printf("\n\rManagement_SNRM\n");
        cmdbuf[4] = 0x23;
    }
    else if (type == 3)
    {
        printf("\n\rHAN_SNRM\n\r");
        cmdbuf[4] = 0x27;
    }

    CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
    cmdbuf[6] = CRC_check & 0xff;
    cmdbuf[7] = (CRC_check >> 8) & 0xff;
    for (i = 0; i < sizeof(cmdbuf); i++)
    {
        sprintf(strbuf, "%02X ", cmdbuf[i]);
        printf(strbuf);
    }
    printf("\n");

    app_uart_data_send(1, cmdbuf, sizeof(cmdbuf));
}

static void udf_Meter_Process(uint8_t *meter_data, uint16_t data_len)
{
    int i = 0;
    char strbuf[128];
    uint8_t tag[12], pt[1024] = {0};
    uint8_t ex_byte = 0, Res_type;
    uint16_t ct_len = 0;
    uint8_t EN_done = 0;
    unsigned char obis_notification_data[] = {0x07, 0x00, 0x00, 0x63,
                                              0x62, 0x00, 0xFF, 0x02
                                             };
    unsigned char obis_data[10] = {0x00, 0x07, 0x01, 0x00, 0x63,
                                   0x01, 0x00, 0xFF, 0x02, 0x00
                                  },
                                  ack_buff[1] = {0x01};

    if (meter_data[3] == 0x27) // PASS HAN // frank,201810
    {
#if (CONFIG_UART_STDIO_PORT == 0)
        app_uart_data_send(2, meter_data, data_len);
#else
        app_uart_data_send(0, meter_data, data_len);
#endif
        return;
    }

    ///////////////////////////Event
    ///Notification///////////////////////////////////////////////
    if (MC_done == 1 && data_len == 60 &&
            meter_data[11] == 0xCA) // Event Notification
    {
        printf("send notification\n");
        EN_done = 1;
#if TEST_NOTIFICATION == 1
        if (flag_Power_Off == 0)
        {
            memcmp(event_notification_data, meter_data, 60);
            udf_Rafael_data(1, 8, (char *)&event_notification_data[0], 3, 60);
            // Notification_Buff(&event_notification_data[0]);
        }
        if (flag_Power_Off == 1) // Power off
        {
        }
        if (MAA_flag == 0)
        {
            AAcheck = 1; // start AA
            flag_MAA = 0;
            MAAFIRST = 1;
        }
#elif TEST_NOTIFICATION == 0 // Only generated once per day.
        // Replace IV
        memcmp(&tpc_s_iv[8], &meter_data[14], 4);

        // Replace Tag
        memcmp(tag, &meter_data[45], 12);
        if (udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv,
                                (uint8_t *)&meter_data[18], tag, &pt[0], 27,
                                17))
        {
            printf("AES_GCM_Decrypt fail \r\n");
        }
        if (flag_Power_Off == 0)
        {
            Decide_Once(&pt[0], &meter_data[0]);
        }
        if (flag_Power_Off == 1) // Power off
        {
        }
#endif
    }
    else if (EN_done == 0 && meter_data[0] == 0x7E)
    {
        if (ic_count == 65535)
        {
            ic_count = 5;
        }
        else
        {
            ic_count++;
        }
        for (i = 0; i < data_len; i++)
        {
            sprintf(strbuf, "%02X ", meter_data[i]);
            printf(strbuf);
        }
        printf("\n\r");
        Res_type = Check_Meter_Res((char *)&meter_data[0], data_len);
        sprintf(strbuf, "Res Type: %d\n\r", Res_type);
        printf(strbuf);

        switch (Res_type)
        {
        case 73:
            printf("Meter_UA\n\r");
            if (meter_data[3] == 0x23)
            {
                udf_Send_AARQ(2);
            }
            if (meter_data[3] == 0x21)
            {
                if (FLAG_PING_METER == 1)
                {
                    PingCheckMeter = 0;
                    udf_Send_AARQ(1);
                }
                else
                {
                    udf_Send_AARQ(1);
                }
            }
            if (meter_data[3] == 0x27)
            {
                udf_Send_AARQ(4);
            }
            MC_done = 1;
            break;

        case 61: // RES-UA
            if (meter_data[3] == 0x23)
            {
                printf("Meter_AARE_M\n\r");
                // Replace GMAC_AK
                for (i = 0; i < 8; i++)
                {
                    tpc_gmac_ak[17 + i] = meter_data[65 + i];
                }

                // Replace Server Sys-T
                for (i = 0; i < 8; i++)
                {
                    tpc_s_iv[i] = meter_data[40 + i];
                }

                // Replace IV
                for (i = 0; i < 4; i++)
                {
                    tpc_s_iv[8 + i] = meter_data[80 + i];
                }

                // Replace Tag
                for (i = 0; i < 12; i++)
                {
                    tag[i] = meter_data[98 + i];
                }

                udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv,
                                    &meter_data[84], tag, &pt[0], 14, 17);
                udf_Management_Client(1);
            }
            else if (meter_data[3] == 0x27)
            {
                printf("Meter_AARE_H\n\r");
                // Replace GMAC_AK
                for (i = 0; i < 8; i++)
                {
                    tpc_gmac_ak[17 + i] = meter_data[65 + i];
                }

                // Replace Server Sys-T
                for (i = 0; i < 8; i++)
                {
                    tpc_s_iv_h[i] = meter_data[40 + i];
                }

                // Replace IV
                for (i = 0; i < 4; i++)
                {
                    tpc_s_iv_h[8 + i] = meter_data[80 + i];    // 80
                }

                // Replace Tag
                for (i = 0; i < 12; i++)
                {
                    tag[i] = meter_data[98 + i];    // 98
                }

                udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv_h,
                                    &meter_data[84], tag, &pt[0], 14, 17);
                udf_Han_Client();
            }
            else if (meter_data[3] == 0x21)
            {
                printf("Meter_AARE_V\n\r");
            }

            break;

        case 11: // RES-Client AA & Change Key RES
            if ((unsigned char)meter_data[12] > 128)
            {
                ex_byte = meter_data[12] & 0x0F;
            }
            else
            {
                ex_byte = 0;
            }

            if (ex_byte == 2)
                ct_len = (meter_data[11 + ex_byte] * 256) +
                         meter_data[12 + ex_byte] - 17;
            else
            {
                ct_len = meter_data[12 + ex_byte] - 17;
            }
            if (meter_data[3] == 0x23)
            {
                // Replace IV
                for (i = 0; i < 4; i++)
                {
                    tpc_s_iv[8 + i] = meter_data[14 + ex_byte + i];
                }

                // Replace Tag
                for (i = 0; i < 12; i++)
                {
                    tag[i] = meter_data[18 + ex_byte + ct_len + i];
                }

                udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv,
                                    &meter_data[18], tag, &pt[0], ct_len,
                                    17);
                if (RE_AA == 1)
                {
                    MAA_flag = 3;
                    RE_AA = 0;
                }
                if (MAAFIRST == 1)
                {
                    MAA_flag = 1;
                    RegistrationprocessStart =
                        0; // (u32)now_time + RTC_CNT; What does this do?
                    sst_flag = 1;
                    if (memcmp(tpc_guk, &Configuration_Page[48], 16) == 0 &&
                            memcmp(&tpc_gcm_ak[1], &Configuration_Page[64],
                                   16) == 0 &&
                            memcmp(tpc_mkey, &Configuration_Page[80], 16) ==
                            0)
                    {
                        printf(
                            "No need to write the same key into flash if "
                            "it is already the same.\n");
                    }
                    else
                    {
                        SaveKey();
                    }
                    KeyErrorflag = 0; // key correct
                    register_steps = 2;
                    if (target_pos == 2 || target_pos == 3)
                    {
                        read_targetpos = 0;
                    }
                    MAAFIRST = 2;
                }
                if (ACTION_CHANGE_KEY == 1)
                {
                    if (pt[0] == 0xC7 && pt[3] == 0x00 &&
                            pt[4] == 0x00) // right key and change
                    {
                        if (TPC_AK_FLAG == 1)
                        {
                            //                              for(i=0;i<17;i++)
                            //                              {
                            //                                  tpc_gcm_ak[i]
                            //= new_tpc_gcm_ak[i];                                  tpc_gmac_ak[i] =
                            //new_tpc_gmac_ak[i];
                            //                              }
                            TPC_AK_FLAG = 2;
                        }
                        if (TPC_GUK_FLAG == 1)
                        {
                            //                              for(i = 0;i <
                            //16; i++)                                  tpc_guk[i] = new_tpc_guk[i];
                            TPC_GUK_FLAG = 2;
                        }
                        if (TPC_MK_FLAG == 1)
                        {
                            //                              for(i=0 ;i<16
                            //;i++)                                     tpc_mkey[i] = new_tpc_mkey[i];
                            TPC_MK_FLAG = 2;
                        }
                        for (i = 0; i < meter_data[2] + 2;
                                i++) // copy response
                        {
                            actionkey_response[i] = meter_data[i];
                        }
                        KEYresponse_len = meter_data[2] + 2;
                        udf_Rafael_data(TASK_ID, 0x1A,
                                        (char *)&meter_data[0], 3,
                                        uart_len1);
                        // SQBrespone(0x1A, &meter_data[0]);
                        RE_AA = 1;
                        // udf_Send_SNRM(2);
                    }
                    else
                    {
                        for (i = 0; i < meter_data[2] + 2;
                                i++) // copy response
                        {
                            actionkey_response[i] = meter_data[i];
                        }
                        TPC_GUK_FLAG = 0;
                        TPC_AK_FLAG = 0;
                        TPC_MK_FLAG = 0;
                        udf_Rafael_data(TASK_ID, 0x1A,
                                        (char *)&meter_data[0], 3,
                                        uart_len1);
                    }
                    ACTION_CHANGE_KEY = 0;
                    MAA_flag = 3;
                }
                else if (ON_DEMAND_AA_FLAG == 1)
                {
                    On_Demand_Reading_Type = 1;
                    ondemandreadingstart =
                        0; // (u32)now_time + RTC_CNT; What does this do?
                    for (i = 14; i <= 22; i++)
                    {
                        obis_data[i - 13] = On_Demand_Task_Command[i - 14];
                    }
                    // Task_count --;
                    udf_GetData_Request(obis_data, ic_count, 1);
                    MAA_flag = 3;
                    ON_DEMAND_AA_FLAG = 0; // frank,201811
                    AUTO_AA = 0;
                    AUTO_AA_FLAG = 1;
                    ondemand_idle = 0; // frank,201811
                }
                else if (AUTO_AA == 1)
                {
                    AUTO_AA = 0;
                    AUTO_AA_FLAG = 1;
                    ondemand_idle = 0; // frank,201811
                    MAA_flag = 3;
                }

                if (NOT_COMPLETELY_POWER_OFF == 1)
                {
                    printf("NOT_COMPLETELY_POWER_OFF = 1\n");
                    udf_Get_New_Number_of_pens(
                        ic_count, &obis_notification_data[0], 1);
                }
            }
            if (meter_data[3] == 0x27)
            {
                // Replace IV
                for (i = 0; i < 4; i++)
                {
                    tpc_s_iv_h[8 + i] = meter_data[14 + ex_byte + i];
                }

                // Replace Tag
                for (i = 0; i < 12; i++)
                {
                    tag[i] = meter_data[18 + ex_byte + ct_len + i];
                }

                udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv_h,
                                    &meter_data[18], tag, &pt[0], ct_len,
                                    17);
            }
            MC_done = 1;
            break;

        case 1:
            if (get_id_done ==
                    0) // Reserve, Unable to obtaon V AA connection ID
            {
                printf("Meter_ID\n\r");
                udf_SMIP_Res(meter_data);
                Verification_AA_done = 1;
                get_id_done = 1;
            }
            else
            {
                printf("Send_Meter_Res\n\r");
                udf_SMIP_Res(meter_data);
            }
            break;

        case 2: // Read_RES
            if ((unsigned char)meter_data[12] > 128)
            {
                ex_byte = meter_data[12] & 0x0F;
            }
            else
            {
                ex_byte = 0;
            }

            if (ex_byte == 2)
                ct_len = (meter_data[11 + ex_byte] * 256) +
                         meter_data[12 + ex_byte] - 17;
            else
            {
                ct_len = meter_data[12 + ex_byte] - 17;
            }
            if (meter_data[3] == 0x23)
            {
                // Replace IV
                for (i = 0; i < 4; i++)
                {
                    tpc_s_iv[8 + i] = meter_data[14 + ex_byte + i];
                }

                // Replace Tag
                for (i = 0; i < 12; i++)
                {
                    tag[i] = meter_data[18 + ex_byte + ct_len + i];
                }

                udf_AES_GCM_Decrypt(tpc_dk, tpc_gcm_ak, tpc_s_iv,
                                    &meter_data[18 + ex_byte], tag, &pt[0],
                                    ct_len, 17);

                if (pt[1] == 0x02 && MAA_flag == 3 &&
                        flag_Power_Off == 0) // Continue transmission
                {
                    sprintf(strbuf,
                            "===========================On_Demand_ID : %d, "
                            "On_Demand_Reading_Type : "
                            "%d===============================\n\r ",
                            On_Demand_ID, On_Demand_Reading_Type);
                    CONTINUE_BUSY = (pt[3] == 0x00) ? 1 : 0;
                    continuebusyflagstart =
                        0; //(u32)now_time + RTC_CNT; What does this do?
                    if (CONTINUE_BUSY == 1)
                    {
                        ELS61_MESSAGE_LOCK = 1; // LOCK 4G QUEUE
                        if (On_Demand_Reading_Type == 1)
                        {
                            on_Demand_Task_Assembly(&pt[0], ct_len, &pt[0]);
                        }
                        else if (Auto_Reading == 1 &&
                                 On_Demand_Reading_Type == 0)
                        {
                            udf_Rafael_data(TASK_ID, On_Demand_Type,
                                            (char *)&meter_data[0], 3,
                                            uart_len1);
                            // auto_send_type_buff(On_Demand_Type,&meter_data[0]);
                        }
                        else if (On_Demand_ID == 1 &&
                                 On_Demand_Reading_Type == 0)
                        {
                            sprintf(strbuf,
                                    "task id =%d , On_Demand_Type = %d\n",
                                    TASK_ID, On_Demand_Type);
                            printf(strbuf);
                            udf_Rafael_data(TASK_ID, On_Demand_Type,
                                            (char *)&meter_data[0], 3,
                                            uart_len1);
                            // On_Demand_type_buff(On_Demand_Type,&meter_data[0]);
                        }
                        else
                        {
                            printf("\nERROR\n");
                            CONTINUE_BUSY = 0;
                            ELS61_MESSAGE_LOCK = 0;
                            Resume_index = 1;
                        }
                    }
                    else
                    {
                        CONTINUE_BUSY = 1;
                        if (On_Demand_Reading_Type == 1)
                        {
                            on_Demand_Task_Assembly(&pt[0], ct_len, &pt[0]);
                        }
                        else if (Auto_Reading == 1 &&
                                 On_Demand_Reading_Type == 0)
                        {
                            udf_Rafael_data(TASK_ID, On_Demand_Type,
                                            (char *)&meter_data[0], 3,
                                            uart_len1);
                            // auto_send_type_buff(On_Demand_Type,&meter_data[0]);
                            Auto_Reading = 0;
                        }
                        else if (On_Demand_ID == 1 &&
                                 On_Demand_Reading_Type == 0)
                        {
                            udf_Rafael_data(TASK_ID, On_Demand_Type,
                                            (char *)&meter_data[0], 3,
                                            uart_len1);
                            // On_Demand_type_buff(On_Demand_Type,&meter_data[0]);
                            On_Demand_ID = 0;
                        }
                        else
                        {
                            printf("\nERROR COMMAND\n");
                        }
                        CONTINUE_BUSY = 0;
                    }
                    STEP_CONTINUE_FLAG = 0;

                    if (CONTINUE_BUSY == 0)
                    {
                        Resume_index = 1;
                        ELS61_MESSAGE_LOCK = 0; // UNLOCK 4G QUEUE
                    }
                }
                else if (MAA_flag == 3 && flag_Power_Off == 0 &&
                         NOT_COMPLETELY_POWER_OFF == 0)
                {
                    if (On_Demand_Reading_Type == 1)
                    {
                        on_Demand_Task_Assembly(&pt[0], ct_len, &pt[0]);
                    }
                    else if (Auto_Reading == 1 &&
                             On_Demand_Reading_Type == 0)
                    {
                        udf_Rafael_data(TASK_ID, On_Demand_Type,
                                        (char *)&meter_data[0], 3,
                                        uart_len1);
                        // auto_send_type_buff(On_Demand_Type,&meter_data[0]);
                        Auto_Reading = 0;
                    }
                    else if (On_Demand_ID == 1 &&
                             On_Demand_Reading_Type == 0)
                    {
                        sprintf(strbuf,
                                "task id =%d , On_Demand_Type = %d\n",
                                TASK_ID, On_Demand_Type);
                        printf(strbuf);

                        udf_Rafael_data(TASK_ID, On_Demand_Type,
                                        (char *)&meter_data[0], 3,
                                        uart_len1);
                        // On_Demand_type_buff(On_Demand_Type,&meter_data[0]);
                        On_Demand_ID = 0;
                    }
                    else
                    {
                        printf("\nERROR COMMAND2\n");
                    }
                    CONTINUE_BUSY = 0;
                    ELS61_MESSAGE_LOCK = 0;
                    Resume_index = 1;
                }
                if (MAA_flag == 2 ||
                        NOT_COMPLETELY_POWER_OFF == 1 && sendbusyflag == 0)
                {
                    MAA_flag = 3;
                    Power_On_Event_Log_Return(
                        &pt[0]); // Return Negative charge data
                    NOT_COMPLETELY_POWER_OFF = 0;
                    printf_off = 1;
                }
            }
            if (meter_data[3] == 0x27)
            {
                // Replace IV
                for (i = 0; i < 4; i++)
                {
                    tpc_s_iv_h[8 + i] = meter_data[14 + ex_byte + i];
                }

                // Replace Tag
                for (i = 0; i < 12; i++)
                {
                    tag[i] = meter_data[18 + ex_byte + ct_len + i];
                }

                udf_AES_GCM_Decrypt(tpc_dk, tpc_gcm_ak, tpc_s_iv_h,
                                    &meter_data[18 + ex_byte], tag, &pt[0],
                                    ct_len, 17);
            }
            break;

        case 21: // SET_RES
            if ((unsigned char)meter_data[12] > 128)
            {
                ex_byte = meter_data[12] & 0x0F;
            }
            else
            {
                ex_byte = 0;
            }

            if (ex_byte == 2)
                ct_len = (meter_data[11 + ex_byte] * 256) +
                         meter_data[12 + ex_byte] - 17;
            else
            {
                ct_len = meter_data[12 + ex_byte] - 17;
            }

            // Replace IV
            for (i = 0; i < 4; i++)
            {
                tpc_s_iv[8 + i] = meter_data[14 + ex_byte + i];
            }

            // Replace Tag
            for (i = 0; i < 12; i++)
            {
                tag[i] = meter_data[18 + ex_byte + ct_len + i];
            }

            udf_AES_GCM_Decrypt(tpc_dk, tpc_gcm_ak, tpc_s_iv,
                                &meter_data[18 + ex_byte], tag, &pt[0],
                                ct_len, 17);
            SETACTION++;
            // 0615//bob
            if (pt[0] == 0xC5 && pt[3] == 0 && SET_TOU_C2_Start != 0 &&
                    SET_TOU_C2_Start <= saveSETdata[0] /*0620*/) // tou_c2 ok
            {
                // 0617
                printf("\n\ragain\n\r");
                Set_TOU_C2(SET_TOU_C2_Start);
            }
            else if (pt[0] == 0xC5 && pt[3] == 0 &&
                     SET_TOU_C2_Start >
                     saveSETdata[0] /*0620*/) // tou_c2 last, ok sent
                // ack
            {
                printf("\n\rSET_TOU_C2 OK!!!!\n\r");
                SET_TOU_C2_Start = 0;
                SETdata_index = 0;
                memset(saveSETdata, 0, sizeof(saveSETdata));
                udf_Rafael_data(TASK_ID, 0xB2, (char *)&meter_data[0], 3,
                                uart_len1); // Packaged for 4G
            }
            else if (pt[0] == 0xC5 && pt[3] != 0 &&
                     SET_TOU_C2_Start != 0) // tou_c2 ok
            {
                printf("\n\rSET_TOU_C2 errrrrrr!!!!\n\r");
                udf_Rafael_data(TASK_ID, 0xB2, (char *)&meter_data[0], 3,
                                uart_len1); // 0717

                SET_TOU_C2_Start = 0;
                SETdata_index = 0;
                memset(saveSETdata, 0, sizeof(saveSETdata));
            }
            // 0615//bob
            // 0617//bob
            if (pt[0] == 0xC5 && pt[3] == 0 &&
                    SET_TOU_C1_INDEX == 1) // tou_c1 last, ok sent ack
            {
                // SET_TOU_C1_INDEX
                printf("\n\rSET_TOU_C1 OK!!!!\n\r");
                SET_TOU_C1_INDEX = 0;
                udf_Rafael_data(TASK_ID, 0xB1, (char *)&meter_data[0], 3,
                                meter_data[2] + 2); // Packaged for 4G
            }
            else if (pt[0] == 0xC5 && pt[3] != 0 &&
                     SET_TOU_C1_INDEX == 1) // tou_c1 error
            {
                printf("\n\rSET_TOU_C1 error!!!!\n\r");
                SET_TOU_C1_INDEX = 0;
                udf_Rafael_data(TASK_ID, 0xB1, (char *)&meter_data[0], 3,
                                meter_data[2] + 2); // Packaged for 4G
            }
            // 0617//bob
            // 0621
            if (pt[0] == 0xC5 && pt[3] == 0 &&
                    SET_TIME_C0_INDEX == 1) // set time
            {
                // SET_TOU_C1_INDEX
                printf("\n\rSET_TIME_C0_INDEX OK!!!!\n\r");
                SET_TIME_C0_INDEX = 0;
                udf_Rafael_data(TASK_ID, 0xB0, (char *)&meter_data[0], 3,
                                uart_len1); // Packaged for 4G
            }
            else if (pt[0] == 0xC5 && pt[3] != 0 &&
                     SET_TIME_C0_INDEX == 1)
            {
                printf("\n\rSET_TIME_C0_INDEX error!!!!\n\r");
                SET_TIME_C0_INDEX = 0;
                udf_Rafael_data(TASK_ID, 0xB0, (char *)&meter_data[0], 3,
                                uart_len1); // Packaged for 4G
            }
            // 0621
            if (pt[3] != 0 && pt[0] == 0xC5 && SQBsetStart == 1) // error
            {
                SQBrespone(0x1B, &meter_data[0]);
                SQBsettingNUM = 0xFF; // break
                SQBsetStart = 0;

                SETQBtmp = 1;
                HANDSQB = 0;
                memset(saveSETdata, 0, sizeof(saveSETdata));
                //      memset( SaveSETheader
                //,0,sizeof(SaveSETheader));//bob
            }

            if (SETACTION != 0 && hitHANDguN == 1)
            {
                set_Test();
            }
            break;

        case 3:
            printf("GET Alarm\n\r");
            udf_Alarm_Register1();
            break;
        case 99: // ACTION_RES
            printf("GET Action_res\n\r");
            if (Auto_Reading == 1)
            {
                auto_read_loadprofile = 0;
                auto_read_Midnight = 0;
                auto_read_ALT = 0;
                Auto_Reading = 0;
            }
            if (POWERON_Restart == 1) // 0205 matt
            {
                POWERON_Restart = 0;
            }
            if ((unsigned char)meter_data[12] > 128)
            {
                ex_byte = meter_data[12] & 0x0F;
            }
            else
            {
                ex_byte = 0;
            }

            if (ex_byte == 2)
                ct_len = (meter_data[11 + ex_byte] * 256) +
                         meter_data[12 + ex_byte] - 17;
            else
            {
                ct_len = meter_data[12 + ex_byte] - 17;
            }

            // Replace IV
            for (i = 0; i < 4; i++)
            {
                tpc_s_iv[8 + i] = meter_data[14 + ex_byte + i];
            }

            // Replace Tag
            for (i = 0; i < 12; i++)
            {
                tag[i] = meter_data[18 + ex_byte + ct_len + i];
            }

            udf_AES_GCM_Decrypt(tpc_dk, tpc_gcm_ak, tpc_s_iv,
                                &meter_data[18 + ex_byte], tag, &pt[0],
                                ct_len, 17);
            udf_Rafael_data(TASK_ID, 19, (char *)&meter_data[0], 3,
                            uart_len1);
            // On_Demand_type_buff(0x19,&meter_data[0]);//20191104
            break;
        case 33: // Error
            if (MAA_flag == 0 && sst_flag == 1)
            {
                // NVIC_SystemReset(); // 0211 matt
                printf("Key_Error or SST Fail\n\r");
                // udf_Rafael_data(1,1,(char *)&ack_buff[0],3,1);

                RESTART_FLAG = 1; // key error 1hr restart 0211 matt
            }
            if (TPC_GUK_FLAG == 1 || TPC_AK_FLAG == 1 || TPC_MK_FLAG == 1)
            {
                TPC_GUK_FLAG = 0;
                TPC_AK_FLAG = 0;
                TPC_MK_FLAG = 0;
            }
            if (POWERON_Restart == 1) // 0205 matt
            {
                printf("NVIC_SystemReset:4\n\r");
                NVIC_SystemReset();
            }

            if (MAA_flag == 3)
            {
                Refuse_Command++;
                if (CONTINUE_BUSY == 1 || ELS61_MESSAGE_LOCK == 1)
                {
                    CONTINUE_BUSY = 0;
                    ELS61_MESSAGE_LOCK = 0;
                }

                if (Refuse_Command >= 3)
                {
                    printf("NVIC_SystemReset:5\n\r");
                    NVIC_SystemReset();
                }

                udf_Send_DISC(2);
            }
            break;
        case 12: // EVENT NOTIFICATION
            if (meter_data[3] == 0x27)
            {
                // Replace IV
                for (i = 0; i < 4; i++)
                {
                    tpc_s_iv_h[8 + i] = meter_data[14 + i];
                }

                // Replace Tag
                for (i = 0; i < 12; i++)
                {
                    tag[i] = meter_data[45 + i];
                }
                printf("\r\nGET Event Notification\r\n");
                Event_Notification = 1;
                MC_done = 1;
                udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv_h,
                                    &meter_data[18], tag, &pt[0], 27, 17);
            }
            if (AUTO_AA == 1)
            {
                udf_Send_DISC(2);
            }
            break;
        case 88:
            udf_Send_SNRM(2);
            DISC_FLAG = 0;
            break;
        default:
            break;
        }

        gettime_flag = 0;
    }
}

static void flash_Information(void)
{
    char strbuf[128];
    static uint8_t read_buf[0x100];
    int i = 0;
    printf("\n\rflash_get_Configuration_Page(200) = ");
    flash_read_page((uint32_t)(read_buf), ConfigurationPage);
    memcpy(Configuration_Page, read_buf, 200);
    printf("\nFan ID: ");
    for (i = 0; i <= 11; i++)
    {
        sprintf(strbuf, "%c ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("\n");
#if 0 //use ot dataset command directily
    sprintf(strbuf, "Leader Weight: %02X\n", Configuration_Page[12]);
    printf(strbuf);
    printf("Networkid: ");
    for (i = 13; i <= 14; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("\n");
    printf("PANID: ");
    for (i = 15; i <= 16; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("\n");
    sprintf(strbuf, "Channel: %02X\n", Configuration_Page[17]);
    printf(strbuf);
#endif
    printf("send els61 timeout: ");
    for (i = 18; i <= 19; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("(10 sec)");
    printf("\n");
    printf("send rf timeout: ");
    for (i = 20; i <= 21; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("(10 sec)");
    printf("\n");
    printf("receive timeout: ");
    for (i = 22; i <= 23; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("(10 sec)");
    printf("\n");
    printf("continue timeout: ");
    for (i = 24; i <= 25; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("(10 sec)");
    printf("\n");
    sprintf(strbuf, "A: %02X\n", Configuration_Page[26]);
    printf(strbuf);
    printf("Server IP: ");
    for (i = 27; i <= 30; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("\n");
    printf("Socket port: ");
    for (i = 31; i <= 32; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("\n");
    printf("APN: ");
    for (i = 33; i <= 47; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("\n");
    printf("tpc guk: ");
    for (i = 48; i <= 63; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("\n");
    printf("tpc gcm ak: ");
    for (i = 64; i <= 79; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("\n");
    printf("tpc mkey: ");
    for (i = 80; i <= 95; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("\n");
    printf("on demandreading timeout: ");

    for (i = 96; i <= 97; i++)
    {
        sprintf(strbuf, "%02X ", Configuration_Page[i]);
        printf(strbuf);
    }
    printf("(10 sec)");
    printf("\n");

    sprintf(strbuf, "keyObtainedStatusIncorrecttimeout : %d (min)\n", Configuration_Page[138]);
    printf(strbuf);
    sprintf(strbuf, "registerRetrytime : %d (min)\nRafael_reset :%d (min)\n", Configuration_Page[139], Configuration_Page[140]);
    printf(strbuf);
    sprintf(strbuf, "Broadcastmeterdelay : %d (MOD)\n", Configuration_Page[141]);
    printf(strbuf);

}

void udf_Meter_received_task(const uint8_t *aBuf, uint16_t aBufLength)
{
    const uint8_t *end;
    uint16_t cmd_length;
    end = aBuf + aBufLength;
    for (; aBuf < end; aBuf++)
    {
        if (dlms_rx_length != 0 || *aBuf == 0x7E)
        {
            dlms_rx_buf[dlms_rx_length++] = *aBuf;
        }
    }

    cmd_length = ((dlms_rx_buf[1] & 0x0F) * 256) + dlms_rx_buf[2] + 2;

    if (dlms_rx_length >= cmd_length)
    {
        udf_Meter_Process(dlms_rx_buf, dlms_rx_length);
        /*dlms_rx_buf clean*/
        memset(dlms_rx_buf, 0x0, dlms_rx_length);
        dlms_rx_length = 0;
    }
}

void udf_Meter_init(otInstance *instance)
{
    Configuration_Page_init(instance);
    flash_Information();
}

const sh_cmd_t g_cli_cmd_factoryid STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "factoryid",
    .pDescription = "factory id setting",
    .cmd_exec     = _cli_cmd_factory_id_set,
};