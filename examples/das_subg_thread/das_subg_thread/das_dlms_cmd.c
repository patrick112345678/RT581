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
#include <semphr.h>
#include <time.h>
//#include "C:\\Users\\testo\\Desktop\\rafel_das_watchdog\\rafael-iot-sdk-das-Release_v1.0.0\\rafael-iot-sdk-das-Release_v1.0.0\\version.h"


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
#define MAX_QUEUE_BYTES 2450  // 假設總共可以存儲2450個字節
static int currentQueueBytes = 0;  // 目前佇列中的字節數
//typedef struct
//{
//    uint16_t dlen;
//    uint8_t *pdata;
//} _ELS61_Block_data_t ;
typedef struct
{
    uint16_t dlen;
    uint8_t *pdata;
} _ReceiveCommand_data_t ;
typedef struct
{
    uint16_t PeerPort;
    otIp6Address PeerAddr;
    uint8_t *pdata;
    uint16_t dlen;
} _SendCommand_data_t;
typedef struct
{
    uint16_t PeerPort;
    otIp6Address PeerAddr;
    uint8_t pdata[1500];
    uint16_t dlen;
} BroadcastData;
BroadcastData broadcastData;
TimerHandle_t xRegisterTimer, xLastpTimer;
#define TASK_UNFINISHED_FLAG  0
#define TASK_FINISHED_FLAG  1
static  uint8_t TASKCOMMAND = TASK_FINISHED_FLAG;
//static QueueHandle_t ELS61_Block_Queue_handle;
static QueueHandle_t ReceiveCommand_Queue_handle;
static QueueHandle_t SendCommand_Queue_handle;
static TimerHandle_t das_dlms_timer = NULL;
// static unsigned char ELS61_Block_Queue[QUEUE_SIZE][COMMAND_LENGTH];
//static unsigned char RF_Block_Queue[QUEUE_SIZE][COMMAND_LENGTH];
static char Send_RF_lastcommand[1500];
static uint8_t receiveIndex = 0;
static uint8_t receivebusyflag = 0, RF_command_current_flag = 0, RF_send_retry = 0, RF_command_retry_count = 0;
#define APP_COMMISSION_DATA_FLASH_START 0xF1000
#define ConfigurationPage APP_COMMISSION_DATA_FLASH_START
#define TEST_NOTIFICATION 1 // test 1:每次產生就發生 0:每天產生一次
#define LOGLEVEL2CLOSE 0
#define LOGLEVEL2OPEN 1
static uint8_t LOGLEVEL2 = LOGLEVEL2CLOSE;
//=============================================================================
//                Private Global Variables
//=============================================================================
time_t now_time;
static uint8_t kek_chipher[24] = {0x8E, 0xAC, 0x01, 0x11, 0x05, 0x21, 0x15, 0xAA, 0x3A, 0x99, 0x22, 0x06, 0xF8, 0x7D, 0x63, 0x9B, 0x80, 0xC6, 0xE5, 0x07, 0xD8, 0xB1, 0xDC, 0x99}, kek_plaint[16];
uint32_t now_timer, RTC_CNT;
static uint8_t rememberAddressFlag = 0 ;
static uint32_t PingMeterSec = 0;
static uint8_t ondemand_busy = 0;
static uint16_t KeyResCount = 0;
static uint16_t RFCOUNT = 0, WaitBlockCount = 0;
struct tm begin = {7, 16, 00, 16, 11 - 1, 2022 - 1900};
static uint16_t cmdTXT_Len = 0, blockType = 0;
static unsigned char selfAddress[4] = {0}; //for AM2 address now
static unsigned char rememberAddress[4] = {0}; //Block used for reTransmit
//static unsigned char cmdTXT[793] = {0};//Store data generated from all the function and Will be inserted into the RF_MessageQueue
static uint8_t CheckFirewall = 1 ;
static unsigned char VersionNumber[3] = {0x13, 0x0B, 0x01}, FirewallSetData[2] = {0}, FlowSetData[2] = {0}; //版本號 firewall設定值 流量設定值
static uint8_t lengthCount = 0;
static uint8_t actionkey_response[128], Key_Restart = 0;
static uint8_t Broadcast_savebuff_flag = 0, Broadcast_flag = 0;
static uint8_t continuebusytime = 120;
static uint8_t Channel_num = 1;
uint8_t flag_Power_Off = 0; // 1: Power off 0: Power on
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
static uint8_t sendbusytime = 3;
static uint8_t RFretrytime = 1;
static uint8_t sst_flag = 0;
static uint8_t start_dis = 0;
static uint8_t sendbusyflag = 0;
uint8_t target_pos = 0;     // 1:leader 2:router 3:child
static uint8_t register_steps = 0; // am1 register flag
static uint8_t ipv6_first = 0;//fist Acquire IPv6
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
uint8_t MAA_flag = STATUS_NOT_OBTAINED_KEY;
uint8_t GreenLight_Flag = 0;
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
static uint16_t Continue_lastindex = 0;
static uint16_t Continue_index = 1;
static uint16_t Refuse_Command = 0;
static uint16_t SETdata_index = 0;
static uint16_t SETdata_RECEIVE_Tatol_LEN = 0;
static uint16_t uart_len1 = 0;

static int8_t LoadprofileAB = 0;

static int Broadcastmeterdelay = 0, Broadcast_meterdelay = 0;
static int HANDSQB = 0;
static int KEYresponse_len = 0;
static int Networkidtime_num = 120;
static int ondemand_idle = 0;
static int PANID_num = 30;
static int setdataLen = 0;
static uint8_t sendIndex = 0;
static int SQBsetStart = 0;
static int SETQBtmp = 1;
static int SQBCTLEN = 0;
static int TASK_ID = 0;
static int Continue_TASK_ID = 0;
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
static uint8_t continuebusyflagstart = 0;
static uint32_t Rafael_time = 0;

static uint8_t ucMeterPacket[350];
static unsigned char Configuration_Page[180];
static unsigned char event_notification_data[80];
static unsigned char Initial_Value[256];
static unsigned char saveSETdata[2000] = {0x00};
static unsigned char uart_read_data2[300];

unsigned char flash_test[200] =
{
    0x41, 0x43, 0x31, 0x30, 0x38, 0x32, 0x31, 0x30, 0x38, 0x30, 0x32,
    0x30,                   /*factory ID            0~11*/
    0x80,                   /*leader weight 12*/
    0x00, 0x78,             /*networkid 13~14*/
    0x01, 0x2c,             /*PANid         15~16*/
    0x03,                   /*Channel  17*/
    0x00, 0x24,             /*send els61 timeout 18~19*/
    0x00, 0x01,             /*send rf  timeout   20~21*/
    0x00, 0x01,             /*receive timeout    22~23*/
    0x00, 0x78,             /*continue timeout      24~25*/
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
static uint8_t new_tpc_guk[16] = {0x00};
static uint8_t new_tpc_gcm_ak[17] = {0x30};
static uint8_t new_tpc_gmac_ak[25] =
{
    0x10, // SC
    0x90, 0x68, 0x41, 0x85, 0x03, 0x9F, 0x8C, 0xAB, 0x7B, 0x4E, 0x86, 0x64, 0xB1, 0x41, 0x25, 0x3C, //GD18667593
    0x8F, 0x45, 0x32, 0xC8, 0x79, 0x5B, 0xFA, 0x3C
}; // StoC (Change data from AARE)
static uint8_t new_tpc_mkey[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1F, 0x20};
static uint8_t system_ekey[16] = {0x2d, 0x53, 0x45, 0xcb, 0x0c, 0x5b, 0xfe, 0x81, 0xab, 0xb0, 0xbe, 0xa9, 0x2e, 0xc6, 0x07, 0x73};
static uint8_t system_add[17] = {0x30, 0x1a, 0x13, 0x2e, 0xe5, 0x58, 0xcb, 0xc7, 0xb7, 0x8f, 0xc3, 0x28, 0xcc, 0x0f, 0x23, 0xb7, 0x06};
static uint8_t system_ic[12] = {0xbd, 0xb9, 0x78, 0xae, 0x5d, 0xdb, 0xf4, 0x68, 0xda, 0xa1, 0x83, 0x3c};

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
unsigned char Meter_ALL_ID[48] = {0x00};
typedef enum
{
    DISC,
    SNRM,
    AARQ,
    District_ID,
    Customer_ID,
    Meter_ID,
    Server_CODE,
    Get_time,
    Preliminary_Work_Completed
} MeterBootSteps;

MeterBootSteps meterBootStep = DISC;
int denominator = 0;
uint8_t SuccessRole = 0, SENCOND_REGISGER = 1;
static uint8_t  IPv6Flag = 0, HESKEY = 0, ack_flag = 0/*回傳註冊ack*/, RE_REG = 0;
char Serial_Number[12] = {0x41, 0x43, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x32, 0x33, 0x34};
static uint8_t FIRST_POWER_ON_FLAG = 1, POWERONDATAREADY = 0;//首次上電 復電buff組裝完成
static int  flag_loadprofile_it = 0, flag_midnight_it = 0, flag_Alt_it = 0, flag_MAA = 0, flag_event_notification = 0;
//斷電紀錄 0:上電 1:斷電 15 秒 每小時主動讀loadprofile 午夜12點讀取midnight 每兩小時讀取ALT notification while 只跑一次
static uint8_t registerCheck = 0, AAcheck = 0, A2_TIMESynchronize = 0, SETOUpket = 0, SQBCT = 0, A1_TIMESynchronize = 0;
//static char ipv6_output[40];
static char Broadcast_ip[7] = {0x66, 0x66, 0x30, 0x33, 0x3a, 0x3a, 0x31};
static uint8_t ResetRF_Flag = 0, ResetRFcount = 0;
uint16_t Register_Timeout = 0;
static uint16_t receivecount = 0, sendcount = 0, OnDemandReadingcount = 0, MAANO3count = 0, CointbusyCount = 0, BroadcastCount = 0, LastpSendCount = 0;
static uint16_t RafaelRoleCount = 0;
static uint16_t PowerResetAnomalyCount = 0;
static uint8_t PowerOnCount = 0;
static uint8_t LastpSendFlag = 0;
static uint8_t GetPowerOnFlag = 0;
static uint8_t meterBootStepCount = 0;
typedef enum
{
    Registerflag,
    receiveflag,
    sendflag,
    ondemandreadflag,
    continueflag,
    resetflag,
    broadcastflag,
} _ERRORflag;
_ERRORflag _Errorprintf = Registerflag;
uint8_t REGISTER_FLAG = 1;
uint8_t First_COUNT = 0;
uint16_t  RegisterCount = 0;
//=============================================================================
//                Private Definition of Compare/Operation/Inline function/
//=============================================================================
//=============================================================================
//                Functions
//=============================================================================
/**
 * 創建或重置一個定時器。
 *
 * @param timerHandle 指向定時器句柄的指針。如果*timerHandle為NULL，將創建一個新的定時器。
 * @param name 定時器名稱。
 * @param periodMs 定時器週期，單位為毫秒。
 * @param isPeriodic 定時器是否週期執行。pdFALSE為單次，pdTRUE為週期。
 * @param callback 定時器超時回調函數。
 * @param callbackArg 傳遞給回調函數的參數。
 * @return pdPASS表示成功，其他值表示失敗。
 */
BaseType_t CreateOrResetTimer(TimerHandle_t *timerHandle, const char *name, uint32_t periodMs, BaseType_t isPeriodic, TimerCallbackFunction_t callback, void *callbackArg)
{
    if (timerHandle == NULL)
    {
        return pdFAIL;
    }

    // 如果定時器尚未創建，創建一個新的定時器
    if (*timerHandle == NULL)
    {
        *timerHandle = xTimerCreate(name, pdMS_TO_TICKS(periodMs), isPeriodic, callbackArg, callback);
        if (*timerHandle == NULL)
        {
            // 定時器創建失敗
            return pdFAIL;
        }
    }
    else
    {
        // 定時器已存在，改變定時器週期
        if (xTimerChangePeriod(*timerHandle, pdMS_TO_TICKS(periodMs), 0) != pdPASS)
        {
            return pdFAIL;
        }
    }

    // 啟動或重啟定時器
    if (xTimerStart(*timerHandle, 0) != pdPASS)
    {
        // 定時器啟動失敗
        return pdFAIL;
    }

    return pdPASS;
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

    printf("=============Encrypt=========================\r\n");
    if (LOGLEVEL2 == LOGLEVEL2OPEN)
    {
        log_info_hexdump("Ciphertext", gcm_ct, gcm_ptlen);
    }
    log_info_hexdump("Plaintext", gcm_pt, gcm_ptlen);

    // Print EK (Encryption Key)
    if (LOGLEVEL2 == LOGLEVEL2OPEN)
    {
        printf("EK: \r\n");
        for (int i = 0; i < 16; i++) // Corrected to iterate over 16 bytes (128 bits)
        {
            printf("%02X ", gcm_ek[i]);
        }
        printf("\r\n");

        // Print IV (Initialization Vector)
        printf("IV: \r\n");
        for (int i = 0; i < 12; i++)
        {
            printf("%02X ", gcm_iv[i]);
        }
        printf("\r\n");

        // Print AAD (Additional Authenticated Data)
        printf("AAD: \r\n");
        for (int i = 0; i < gcm_aadlen; i++)
        {
            printf("%02X ", gcm_aad[i]);
        }
        printf("\r\n");
        // Log the authentication tag
        log_info_hexdump("Tag", gcm_tag, 12);
    }
    printf("=============================================\r\n");
    mbedtls_gcm_free(&ctx);
    return ret;
}

static uint16_t CRC_COMPUTE(char *recv_buff, uint16_t len)
{
    int i;
    char crc;
    unsigned short crc16;
    unsigned char ch;
    unsigned char crcbuf[2];
    crc16 = 0xFFFF;

    for (i = 0; i < len; i++)
    {
        ch = *(recv_buff + i);

    }

    for (i = 0; i < len; i++)
    {
        ch = *(recv_buff + i);
        crc16 = UpdateCRC16(crc16, ch);
    }
    crc16 = ~crc16;
    crcbuf[0] = crc16 & 0xff;
    crcbuf[1] = (crc16 >> 8) & 0xff;

    for (i = 0; i < 2; i++)
    {
        crc = crcbuf[i];

    }


    return crc16;
}
/*
static void send_queue(uint16_t PeerPort, otIp6Address PeerAddr, char *recvbuf, int send_length)
{
    if (sendIndex >= QUEUE_SIZE)
    {

        printf("Send Queue is full or overflow: %d / %d\n", sendIndex, QUEUE_SIZE);
        return;
    }
    sendbusyflag = 1;

    _SendCommand_data_t SendCommand_data;

    // 為要發送的數據分配內存
    SendCommand_data.pdata = mem_malloc(send_length);
    if (SendCommand_data.pdata)
    {
        // 儲存UDP通信所需的所有信息
        SendCommand_data.PeerPort = PeerPort;
        SendCommand_data.PeerAddr = PeerAddr; // 確保這個賦值是有效的，可能需要適當的複製或引用
        memcpy(SendCommand_data.pdata, (uint8_t *)recvbuf, send_length);
        SendCommand_data.dlen = send_length;

        // 將包含所有必要信息的結構體發送到隊列
        while (xQueueSend(SendCommand_Queue_handle, (void *)&SendCommand_data, portMAX_DELAY) != pdPASS);
        sendIndex ++;
        sendbusyflag = 0;  // 清除忙碌標記
        APP_EVENT_NOTIFY(EVENT_SEND_QUEUE);  // 通知系統已發送隊列事件


        printf("Save Send Queue: %d / %d\r\n", sendIndex, QUEUE_SIZE);
    }
    else
    {
        // 處理動態內存分配失敗的情況
    }


}
*/


static void send_queue(uint16_t PeerPort, otIp6Address PeerAddr, char *recvbuf, int send_length)
{


    if (currentQueueBytes + send_length > MAX_QUEUE_BYTES)
    {
        printf("Send Queue is full or overflow: %d bytes needed, %d bytes available\n", send_length, MAX_QUEUE_BYTES - currentQueueBytes);
        APP_EVENT_NOTIFY(EVENT_SEND_QUEUE);  // 通知系統已發送隊列事件
        return;
    }
    sendbusyflag = 1;

    _SendCommand_data_t SendCommand_data;

    // 為要發送的數據分配內存
    SendCommand_data.pdata = mem_malloc(send_length);
    if (SendCommand_data.pdata)
    {
        // 儲存UDP通信所需的所有信息
        SendCommand_data.PeerPort = PeerPort;
        SendCommand_data.PeerAddr = PeerAddr;
        memcpy(SendCommand_data.pdata, (uint8_t *)recvbuf, send_length);
        SendCommand_data.dlen = send_length;

        // 將包含所有必要信息的結構體發送到隊列
        while (xQueueSend(SendCommand_Queue_handle, (void *)&SendCommand_data, portMAX_DELAY) != pdPASS);
        sendIndex ++;
        currentQueueBytes += send_length;  // 更新目前佇列使用的字節數
        sendbusyflag = 0;  // 清除忙碌標記
        APP_EVENT_NOTIFY(EVENT_SEND_QUEUE);  // 通知系統已發送隊列事件

        printf("Saved in Send Queue: %d bytes, %d bytes total in queue\n", send_length, currentQueueBytes);
    }
    else
    {
        // 處理動態內存分配失敗的情況
    }
}



static void udf_Rafael_data(uint16_t task_id, uint8_t type, char *meter_data,
                            uint8_t Rafael_command, uint16_t meter_len)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    uint8_t i = 0;
    uint16_t payload_len = 0;
    uint8_t *payload = NULL;

    otIp6Address LeaderIp;

    otInstance *instance = otrGetInstance();
    if (instance)
    {
        otIp6AddressToString(otThreadGetMeshLocalEid(instance), string,
                             sizeof(string));
        LeaderIp = *otThreadGetRloc(instance);
        LeaderIp.mFields.m8[14] = 0xfc;
        LeaderIp.mFields.m8[15] = 0x00;
    }
    payload_len = 1 + strlen(string) + 1 + 2 + meter_len;
    log_info("payload_len = %d", payload_len);
    payload = mem_malloc(payload_len);
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
            if (flag_Power_Off == 0)
            {
                if (On_Demand_ID == 1)
                {
                    if (CONTINUE_BUSY == 0 && POWERONFLAG == 0)
                    {
                        if (Broadcast_savebuff_flag == 0)
                        {
                            send_queue(THREAD_UDP_PORT, LeaderIp, payload, payload_len);
                            On_Demand_ID = 0;
                        }
                        else
                        {
                            // 準備數據結構
                            broadcastData.PeerPort = THREAD_UDP_PORT;
                            memcpy(&broadcastData.PeerAddr, &LeaderIp, sizeof(otIp6Address));
                            if (payload_len <= sizeof(broadcastData.pdata))
                            {
                                memcpy(broadcastData.pdata, payload, payload_len);
                                broadcastData.dlen = payload_len;
                            }
                            Broadcast_flag = 1;
                            Broadcast_savebuff_flag = 0;
                            On_Demand_ID = 0;

                        }
                    }
                    else
                    {
                        Rafael_printFunction(payload, payload_len);
                        // 直接發送數據邏輯
                        app_udpSend(THREAD_UDP_PORT, LeaderIp, payload, payload_len);
                    }
                }
                else
                {
                    if (CONTINUE_BUSY == 0 && POWERONFLAG == 0)
                    {
                        send_queue(THREAD_UDP_PORT, LeaderIp, payload, payload_len);
                        On_Demand_ID = 0;
                    }
                    else
                    {
                        Rafael_printFunction(payload, payload_len);
                        // 直接发送数据的逻辑
                        app_udpSend(THREAD_UDP_PORT, LeaderIp, payload, payload_len);
                    }
                }
            }
            else
            {
                Rafael_printFunction(payload, payload_len);
                app_udpSend(THREAD_UDP_PORT, LeaderIp, payload, payload_len);
            }
        }
    }
    if (payload)
    {
        mem_free(payload);
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

    //      log_info_hexdump("Found Bootup Event",cmdbuf,sizeof(cmdbuf));
    //      printf("Notification!\r\n");
    app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));
}


static void Power_Off_Function(void)
{
    char LastpBuffer[4];
    LastpBuffer[0] = now_timer >> 24;
    LastpBuffer[1] = now_timer >> 16;
    LastpBuffer[2] = now_timer >> 8;
    LastpBuffer[3] = now_timer;
    TASK_ID = 1;
    if (LastpSendCount >= (denominator % 20))
    {
        if (target_pos == 2 || target_pos == 3)
        {
            udf_Rafael_data(TASK_ID, 0x1c, (char *)&LastpBuffer[0], 3, 4);
        }
        LastpSendFlag = 1;
        LastpSendCount = 0;

    }
}



void user_gpio29_isr_handler(uint32_t pin, void *isr_param)
{
    if (flag_Power_Off == 0)
    {
        flag_Power_Off = 1;

        LastpSendFlag = 0;
        LastpSendCount = 0;
        NOT_COMPLETELY_POWER_OFF = 0;
        GetPowerOnFlag = 0;
        PowerResetAnomalyCount = 0;
        PowerOnCount = 0;
    }
    else
    {
        flag_Power_Off = 0;
        NOT_COMPLETELY_POWER_OFF = 1;

        LastpSendFlag = 0;
        LastpSendCount = 0;
        GetPowerOnFlag = 0;
        PowerOnCount = 0;
        PowerResetAnomalyCount = 0;
    }

}
//29(lastp 腳位) 中斷啟動
void setup_gpio29(void)
{
    gpio_int_disable(29);
    // Configure GPIO29 as an input and set a pull-up resistor of 100K
    pin_set_pullopt(29, MODE_PULLUP_100K); // Set the pull-up resistor for GPIO29


    gpio_cfg_input(29, GPIO_PIN_INT_EDGE_FALLING); // Configured for falling edge trigge

    gpio_debounce_enable(29);//去彈跳

    // Register an interrupt service routine (ISR) for GPIO29
    gpio_register_isr(29, user_gpio29_isr_handler, NULL);

    // Enable the interrupt for GPIO29
    gpio_int_enable(29);
}
// Broadcast 定時器回條函式
void Broadcast_function_timeout_handlr(void)
{

    printf("==========================   Broadcast transmission: =======================================\n\r");
    Rafael_printFunction(broadcastData.pdata, broadcastData.dlen);
    app_udpSend(broadcastData.PeerPort, broadcastData.PeerAddr, broadcastData.pdata, broadcastData.dlen);
    Broadcast_flag = 0;

}
uint8_t HandleBusyFlagAndCount(uint8_t  *busyFlag, uint16_t *count, uint16_t busyTime, uint8_t Error_flag)
{

    _Errorprintf = Error_flag;
    if (*busyFlag)
    {
        (*count)++;
        if (*count >= busyTime )
        {
            *busyFlag = 0;
            *count = 0;

            switch (_Errorprintf)
            {
            case Registerflag:
                log_info("===================================");
                printf("Retry Register\r\n");
                log_info("===================================\r\n");
                break;
            case receiveflag:
                log_info("===================================");
                printf("Receiveflag Error\r\n");
                log_info("===================================\r\n");
                break;
            case sendflag:
                log_info("===================================");
                printf("Sendflag Error\r\n");
                log_info("===================================\r\n");
                break;
            case ondemandreadflag:
                log_info("===================================");
                printf("Ondemandreading Error\r\n");
                log_info("===================================\r\n");
                break;
            case continueflag:
                log_info("===================================");
                printf("Continueflag Error\r\n");
                log_info("===================================\r\n");
                break;
            case resetflag:
                log_info("===================================");
                printf("Reset Fan\r\n");
                log_info("===================================\r\n");
                break;
            default:
                break;
            }

            return 1;
        }
    }
    else
    {
        *count = 0;
    }
    return 0;
}
void register_command(void)
{
    int i = 0;
    char register_buff[100];
    printf("REGISTER \r\n");
    On_Demand_ID = 1;
    for (i = 0; i < 26; i++)
    {
        register_buff[i] = Meter_ALL_ID[i];
    }
    for (i = 0; i < 12; i++)
    {
        register_buff[26 + i] = Serial_Number[i];
    }
    udf_Rafael_data(1, 1, &register_buff[0], 3, 38);

    register_steps = 2;
}


static void Light_controller_function(void)
{
    if (MAA_flag < STATUS_REGISTRATION_COMPLETE && (target_pos == OT_DEVICE_ROLE_CHILD || target_pos ==  OT_DEVICE_ROLE_ROUTER || target_pos == OT_DEVICE_ROLE_LEADER) )
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
    }
    else if (MAA_flag == STATUS_REGISTRATION_COMPLETE  && (target_pos == OT_DEVICE_ROLE_CHILD || target_pos ==  OT_DEVICE_ROLE_ROUTER || target_pos == OT_DEVICE_ROLE_LEADER))
    {
        if (gpio_pin_get(14) == 0 && flag_Power_Off == 0)
        {
            gpio_pin_write(14, 1);
        }
    }
}
void Continue_function_timeout_handlr(void)
{
    CONTINUE_BUSY = 0;
    continuebusyflagstart = 0;
    Resume_index = 1;
}
void udf_Send_DISC(uint8_t type)
{
    unsigned char cmdbuf[] = {0x7E, 0xA0, 0x07, 0x03, 0x23,
                              0x53, 0xB3, 0xF4, 0x7E
                             };
    int i;
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
    printf("\n\rDISC-DM :\n");
    for (i = 0; i < sizeof(cmdbuf); i++)
    {
        printf("%02X ", cmdbuf[i]);
    }
    printf("\n\r");

    app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));
}


static void Auto_MAA(void)
{
    //  if( p->tm_min == 59 && p->tm_sec == 00 && flag_MAA == 0)
    //  {
    udf_Send_DISC(2);
    flag_MAA = 1;
    //      flag_snrm = 0;
    //}

}


static void ReadFlashData_Normal(uint32_t ReadAddress, uint8_t *dest_Data, uint32_t num)
{
    int i = 0;
    while (i < num)
    {
        *(dest_Data + i) = *(__IO uint16_t *) ReadAddress;
        ReadAddress += 2;//get even address
        printf("%02X ", dest_Data[i]);
        i++;
    }
}
static unsigned short calculate_checksum(char *data, int len)
{
    unsigned long sum = 0;
    int i = 0, checksum1 = 0;

    for (i = 0; i < len; i++)
    {
        sum = sum + data[i];
    }
    checksum1 = sum % 256;
    // printf("checksum = %02X\n\r",checksum1);
    return checksum1;
}

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
    int i;
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
    for (i = 2; i < 12; i++)
    {
        Serial_Number[i] = Configuration_Page[i];
    }
    // send els61 timeout
    sendbusytime = Configuration_Page[18] * 256 +  Configuration_Page[19];

    // send rf timeout
    RFretrytime = Configuration_Page[20] * 256 + Configuration_Page[21];

    // receive timeout
    receivebusytime = Configuration_Page[22] * 256 + Configuration_Page[23];

    // continue busy timeout
    continuebusytime = Configuration_Page[24] * 256 + Configuration_Page[25];

    LoadprofileAB = Configuration_Page[26];

    ondemandreadingtime = Configuration_Page[96] * 256 + Configuration_Page[97];
    log_info("ondemandreadingtime = %d", ondemandreadingtime);
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
        gpio_pin_write(15, 0);
        NVIC_SystemReset();
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
//static void _cli_cmd_factory_id_set(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
//{
//    int i = 0;
//    printf("save factory id \r\n");
//    if (argc > 1)
//    {
//        printf("factory id : ");
//        memcpy(&Configuration_Page[2], argv[1], 10);
//        for (i = 0; i < 10; i++)
//        {
//            printf("%02X", Configuration_Page[2 + i]);
//        }
//        printf("\r\n");
//        WriteFlashData(ConfigurationPage, Configuration_Page, 180);
//    }
//    else
//    {
//        printf("factory id : ");
//        for (i = 0; i < 10; i++)
//        {
//            printf("%02X", Configuration_Page[2 + i]);
//        }
//        printf("\r\n");
//    }
//}
/*
static uint8_t fan_number(void)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    uint8_t message[5 + sizeof(string)];
    uint8_t  i = 0;


    otInstance *instance = otrGetInstance();
    if (instance)
    {
        otIp6AddressToString(otThreadGetMeshLocalEid(instance), string, sizeof(string));
    }

    if (string[0] != '\0')
    {
        while (string[i] != '\0')
        {
            ipv6_output[i] = string[i];
            i++;
        }
        if (MAA_flag == STATUS_NOT_OBTAINED_KEY)
        {
            register_steps = 1;
        }
        log_info("IPV6 : %s", ipv6_output);
        ipv6_length = strlen(ipv6_output);
        log_info("IPV6 : %s ipv6_length: %d", ipv6_output, ipv6_length);
    }
    else
    {
        fan_number();
    }
}

*/
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
    printf("=============Decrypt=========================\r\n");
    if (LOGLEVEL2 == LOGLEVEL2OPEN)
    {
        log_info_hexdump("Ciphertext", gcm_ct, gcm_ctlen);
    }
    log_info_hexdump("Plaintext", gcm_pt, gcm_ctlen);

    // Print EK (Encryption Key)
    if (LOGLEVEL2 == LOGLEVEL2OPEN)
    {
        printf("EK: \r\n");
        for (int i = 0; i < 16; i++) // Corrected to iterate over 16 bytes (128 bits)
        {
            printf("%02X ", gcm_ek[i]);
        }
        printf("\r\n");

        // Print IV (Initialization Vector)
        printf("IV: \r\n");
        for (int i = 0; i < 12; i++)
        {
            printf("%02X ", gcm_iv[i]);
        }
        printf("\r\n");

        // Print AAD (Additional Authenticated Data)
        printf("AAD: \r\n");
        for (int i = 0; i < gcm_aadlen; i++)
        {
            printf("%02X ", gcm_aad[i]);
        }
        printf("\r\n");
    }
    printf("=============================================\r\n");
    mbedtls_gcm_free(&ctx);
    return ret;
}

static void flashsetfuntion(unsigned char *flashdata)
{
    int i = 0, j = 0;
    char flashbuffer[151] = {0};
    static uint8_t read_buf[0x100];
    printf("\n\rflash_get_Configuration_Page(200) = ");
    flash_read_page((uint32_t)(read_buf), ConfigurationPage);
    memcpy(Configuration_Page, read_buf, 200);
    printf("\nConfiguration_Page[12] =");
    for (i = 12, j = 0; i <= 26; i++, j++)
    {
        Configuration_Page[i] = flashdata[j];
        printf(" %02X ", flashdata[j]);
    }
    printf("\n");
    printf("Configuration_Page[96] =");
    for (i = 96, j = 15; i <= 97; i++, j++)
    {
        Configuration_Page[i] = flashdata[j];
        printf(" %02X ", flashdata[j]);
    }
    printf("\n");
    printf("Configuration_Page[138] =");
    for (i = 138, j = 17; i <= 141; i++, j++)
    {
        Configuration_Page[i] = flashdata[j];
        printf(" %02X ", flashdata[j]);
    }
    printf("\n");

    WriteFlashData(ConfigurationPage, Configuration_Page, 143);

    Leaderweight_num = Configuration_Page[12];

    //networkid
    Networkidtime_num = Configuration_Page[13] * 256 +  Configuration_Page[14];

    //panid
    PANID_num = Configuration_Page[15] * 256 +  Configuration_Page[16];

    //channel
    Channel_num = Configuration_Page[17];

    //send els61 timeout
    sendbusytime = Configuration_Page[18] * 256 +  Configuration_Page[19];

    //send rf timeout
    RFretrytime =  Configuration_Page[20] * 256 +  Configuration_Page[21];

    //receive timeout
    receivebusytime = Configuration_Page[22] * 256 +  Configuration_Page[23];

    //continue busy timeout
    continuebusytime =  Configuration_Page[24] * 256 +  Configuration_Page[25];

    LoadprofileAB = Configuration_Page[26];//0616

    ondemandreadingtime = Configuration_Page[96] * 256 + Configuration_Page[97];

    keyObtainedStatusIncorrecttimeout = Configuration_Page[138];

    registerRetrytime = Configuration_Page[139];

    Rafael_reset = Configuration_Page[140];

    Broadcastmeterdelay = Configuration_Page[141];

    flash_read_page((uint32_t)(read_buf), ConfigurationPage);
    memcpy(Configuration_Page, read_buf, 200);

    flashbuffer[0] = 0x01;
    for (i = 0; i < 150; i++)
    {
        flashbuffer[i + 1] = Configuration_Page[i];
    }
    udf_Rafael_data(TASK_ID, 0xDD, &flashbuffer[0], 3, 143);

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

        //log_info_hexdump("Send to Meter", cmdbuf, sizeof(cmdbuf));
        app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));
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
        //log_info_hexdump("Send to Meter", cmdbuf, sizeof(cmdbuf));
        app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));
    }
}

static void Check_Demand_Type(unsigned char *obis)
{
    if (obis[0] == 0x07 && obis[1] == 0x01 && obis[2] == 0x00 &&
            obis[3] == 0x63 && obis[4] == 0x01 && obis[5] == 0x00 && obis[6] == 0xFF )//loadprofile
    {
        printf("loadprofile type\r\n");
        On_Demand_Type = 3;
    }
    else if (obis[0] == 0x07 && obis[1] == 0x00 && obis[2] == 0x00 &&
             obis[3] == 0x15 && obis[4] == 0x00 && obis[5] == 0x05 && obis[6] == 0xFF )//Midnight
    {
        printf("Midnight type\r\n");
        On_Demand_Type = 4;
    }
    else if (obis[0] == 0x07 && obis[1] == 0x00 && obis[2] == 0x00 &&
             obis[3] == 0x15 && obis[4] == 0x00 && obis[5] == 0x08 && obis[6] == 0xFF )//ALT
    {
        printf("Alt type\r\n");
        On_Demand_Type = 5;
    }
    else if (obis[0] == 0x07 && obis[1] == 0x00 && obis[2] == 0x00 &&
             obis[3] == 0x63 && obis[4] == 0x62 && obis[5] == 0x00 && obis[6] == 0xFF )//Event Log
    {
        printf("event type\r\n");
        On_Demand_Type = 6;
    }
    else if (obis[0] == 0x07 && obis[1] == 0x00 && obis[2] == 0x00 &&
             obis[3] == 0x15 && obis[4] == 0x00 && obis[5] == 0x07 && obis[6] == 0xFF)//»Ý¶q´_Âk
    {
        printf("demand reset type\r\n");
        On_Demand_Type = 10;
    }
    else if (obis[0] == 0x0B && obis[1] == 0x00 && obis[2] == 0x00 &&
             obis[3] == 0x0B && obis[4] == 0x00 && obis[5] == 0x00 && obis[6] == 0xFF)//special day
    {
        printf("special day type\r\n");
        On_Demand_Type = 11;
    }
    else if (obis[0] == 0x14 && obis[1] == 0x00 && obis[2] == 0x00 &&
             obis[3] == 0x0d && obis[4] == 0x00 && obis[5] == 0x00 && obis[6] == 0xFF)//Activity calendar for tou
    {
        printf("Activity calendar for tou type\r\n");
        On_Demand_Type = 12;
    }
    else //other
    {
        printf("Other type\r\n");
        On_Demand_Type = 7;
    }
}
static void On_Demand_date(uint16_t ic_count, unsigned char *obis_time_data) //®É¶¡­­¨îbuffer¶Ç°e
{
    uint16_t CRC_check;
    unsigned char cmdbuf[] =
    {
        0x7E, 0xA0, 0x5D, 0x03, 0x23, 0x13, 0xF1, 0xF9, 0xE6, 0xE6, //[0-9]
        0x00, 0xD0, 0x4F, 0x30, 0x00, 0x00, 0x00, 0x05, //[10-17]
        0xC0, 0x01, 0x42, 0x00, //[18-21]
        0x07, 0x01, 0x00, 0x63, 0x01, 0x00, 0xFF, 0x02,//[22-29]
        0x01, 0x01, //[30-31]
        0x02, 0x04, //[32-33]
        0x02, 0x04, //[34-35]
        0x12, 0x00, 0x08, //[36-38]
        0x09, 0x06, 0x00, 0x00, 0x01, 0x00, 0x00, 0xFF, 0x0F, 0x02, 0x12, 0x00, 0x00, //[39-51]
        0x19, 0x07, 0xE2, 0x07, 0x01, 0xFF, 0x01, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00, //[52-65]
        0x19, 0x07, 0xE2, 0x07, 0x0a, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00, //[65-79]
        0x01, 0x00, //[80-81]
        0xEA, 0xDB, 0x61, 0x8B, 0x34, 0x98, 0x1A, 0xBE, 0x16, 0x66, 0xF2, 0x12,
        0xE3, 0x1E, 0x7E
    };
    int i/*,last_hours_time*/;
    unsigned char cipher[128], tag[16];
    unsigned short crc16;
    unsigned char ch;
    uint8_t pt_len;


    for (i = 0; i <= 8; i++)
    {
        cmdbuf[i + 22] = obis_time_data[i];
    }

    for (i = 53; i <= 60; i++)
    {
        cmdbuf[i] = obis_time_data[i - 44];             //9 10 11 12 13 14 15 16
        //printf("%02X ",obis_time_data[i-44]);
    }
    for (i = 66; i <= 73; i++)
    {
        cmdbuf[i] = obis_time_data[i - 49];    //17 18 19 20 21 22 23 24 25
    }



    CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
    cmdbuf [6] = CRC_check & 0xff;
    cmdbuf [7] = (CRC_check >> 8) & 0xff;


    // update buffer
    cmdbuf[16] = (ic_count >> 8) & 0xFF;
    cmdbuf[17] = ic_count & 0xFF;

    cmdbuf[20] = ((ic_count - 3) % 0x40) + 0x40;


    pt_len = cmdbuf[12] - 17;

    for (i = 0; i < 4; i++)
    {
        tpc_c_iv[8 + i] = cmdbuf[14 + i];
    }

    udf_AES_GCM_Encrypt(tpc_dk, tpc_gcm_ak, tpc_c_iv, &cmdbuf[18], &cipher[0], &tag[0], pt_len, 17);

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

    // for (i = 0; i < sizeof(cmdbuf); i++)
    // {
    //     printf("%02X ", cmdbuf[i]);
    // }
    // printf("\n\r");

    log_info_hexdump("Send Command to Meter", cmdbuf, sizeof(cmdbuf));

    app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));
}


static void AC_Command(char *recvbuf)
{
    unsigned char obis_number_of_pens_data[18] = {0};
    unsigned char obis_time_data[25] = {0};
    unsigned char obis_data[10] = { 0x00, 0x07, 0x01, 0x00,  0x63, 0x01, 0x00, 0xFF, 0x02, 0x00};

    int i = 0;
    On_Demand_ID = 1;
    Check_Demand_Type((unsigned char *)&recvbuf[0]);
    //log_info("Send Type = %d", On_Demand_Type);

    if (recvbuf[8] == 0x00)
    {
        for (i = 0 ; i <= 8; i++)
        {
            obis_data[i + 1] = recvbuf[i];
        }
        printf("Read OBIS CODE\r\n");
        udf_GetData_Request(obis_data, ic_count, 1);
    }
    else if (recvbuf[8] == 0x01)
    {
        for (i = 0 ; i <= 24; i++)
        {
            obis_time_data[i] = recvbuf[i];
        }
        printf("Time Range Reading!\r\n");
        On_Demand_date(ic_count, &obis_time_data[0]);
    }
    else if (recvbuf[8] == 0x02)
    {
        for (i = 0; i <= 18; i++)
        {
            obis_number_of_pens_data[i] = recvbuf[i];
        }
        printf("Reading Record Count!\r\n");
        udf_Get_New_Number_of_pens(ic_count, &obis_number_of_pens_data[0], 0);
    }
    else
    {
        printf("OBIS CODE ERROR!\r\n");
    }

}


static void _cli_cmd_get_fan_status(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    int i = 0;
    double countdown = 0;
    struct tm *p;
    unsigned char obis_notification_data[] = { 0x07, 0x00, 0x00, 0x63, 0x62, 0x00, 0xFF, 0x02};
    unsigned char recvbuf[81] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x1A, 0x66, 0x64, 0x30, 0x30, 0x3A, 0x64, 0x62, 0x38, 0x3A, 0x30, 0x3A, 0x30, 0x3A, 0x30, 0x3A, 0x30, 0x3A, 0x34, 0x32, 0x63, 0x33, 0x3A, 0x36, 0x30, 0x34, 0x32, 0xAC, 0x00, 0x00, 0x07, 0x01, 0x00, 0x63, 0x01, 0x00, 0xFF, 0x02, 0x01, 0x07, 0xE8, 0x02, 0x0F, 0xFF, 0x0F, 0x00, 0x01, 0x07, 0xE8, 0x02, 0x0F, 0xFF, 0x10, 0x01, 0x00, 0x63, 0x01, 0x6B, 0xA8, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    otInstance *instance = otrGetInstance();
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    p = localtime(&now_timer);
#if NETWORKKEY_USE
    log_info("\nNETWORKEY version");
#elif NETWORKKEY_USE == 0
    log_info("\nNO NETWORKEY version");
#endif

    log_info("%d / %d / %d   %d : %d : %d", (p->tm_year + 1900), (p->tm_mon + 1), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec );
    log_info("Rafel Thread version                            :      %s", otGetVersionString());
//    log_info("Das version                                     :      %s", VERSION);
    printf  ("Role                                            : ");
    switch (target_pos)
    {
    case 0:
        log_info("     Disabled");
        break;
    case 1:
        log_info("     Detached");
        break;
    case 2:
        log_info("     Child");
        break;
    case 3:
        log_info("     Router");
        break;
    case 4:
        log_info("     Leader");
        break;
    }
    switch (MAA_flag)
    {
    case 0:
    {
        log_info("MAA_flag                                        :      STATUS_NOT_OBTAINED_KEY ");
        break;
    }
    case 1:
    {
        log_info("MAA_flag                                        :      STATUS_OBTAINED_KEY ");
        break;
    }
    case 2:
    {
        log_info("MAA_flag                                        :      STATUS_SENT_ACK ");
        break;
    }
    case 3:
    {
        log_info("MAA_flag                                        :      STATUS_REGISTRATION_COMPLETE ");
        break;
    }
    default:
    {
        break;
    }
    }

    if (meterBootStep != Preliminary_Work_Completed)
    {
        countdown = 10 - meterBootStepCount;
        countdown = countdown < 0 ? 0 : countdown;
        log_info("Incomplete acquisition of startup information from the meter. \r\nRestart countdown:                              :      %-6d      (s)", (int)countdown);
    }
    if ((target_pos != 4 && target_pos != 2 && target_pos != 3))
    {
        countdown = (Rafael_reset * 60.0) - RafaelRoleCount;
        countdown = countdown < 0 ? 0 : countdown;
        log_info("Rafael not receiving status. \r\nResetting countdown timer                       :      %-6d      (s)", (int)countdown);
    }
    else
    {
        otIp6AddressToString(otThreadGetMeshLocalEid(instance), string, sizeof(string));
        log_info("The real IPv6 address is                        :      %s", string);
    }
    switch (meterBootStep)
    {
    case DISC:
    {
        log_info("meterBootStep                                   :      DISC ");
        break;
    }
    case SNRM:
    {
        log_info("meterBootStep                                   :      SNRM ");
        break;
    }
    case AARQ:
    {
        log_info("meterBootStep                                   :      AARQ ");
        break;
    }
    case District_ID:
    {
        log_info("meterBootStep                                   :      District_ID ");
        break;
    }
    case Customer_ID:
    {
        log_info("meterBootStep                                   :      Customer_ID ");
        break;
    }
    case Meter_ID:
    {
        log_info("meterBootStep                                   :      Meter_ID ");
        break;
    }
    case Server_CODE:
    {
        log_info("meterBootStep                                   :      Server_CODE ");
        break;
    }
    case Get_time:
    {
        log_info("meterBootStep                                   :      Get_time ");
        break;
    }
    case Preliminary_Work_Completed:
    {
        log_info("meterBootStep                                   :      Preliminary_Work_Completed ");
        break;
    }
    default:
        break;
    }
    if (KeyErrorflag == 1 && HESKEY == 1 && MAA_flag == STATUS_NOT_OBTAINED_KEY)
    {
        log_info("The server provided an incorrect key.\n");
    }

    if (MAA_flag == STATUS_NOT_OBTAINED_KEY && SuccessRole)
    {
        log_info("Re-registration in progress. Countdown          : %7d        (s)", (int)Register_Timeout);
    }
    if (MAA_flag == STATUS_OBTAINED_KEY    || MAA_flag == STATUS_SENT_ACK)
    {
        countdown = 300 - MAANO3count;
        countdown = countdown < 0 ? 0 : countdown;
        if (MAA_flag == STATUS_OBTAINED_KEY)
        {
            log_info("Key obtained but registration not yet complete,\r\nRemaining time until MAA_flag = STATUS_REGISTRATION_COMPLETE               :      %-6d      (s)", (int)countdown);
        }
        else
        {
            log_info("Key received and ACK sent, but registration not yet complete,\r\nRemaining time until MAA_flag = STATUS_REGISTRATION_COMPLETE               :      %-6d      (s)", (int)countdown);
        }

    }

    if (Broadcast_flag)
    {
        log_info("=========== Broadcast Count Starts =================");
        countdown = Broadcast_meterdelay - BroadcastCount;
        countdown = countdown < 0 ? 0 : countdown;
        log_info("Broadcast delay send                            :      %-6d      (s)", Broadcast_meterdelay);
        log_info("BroadCast sending countdown                     :      %-6d      (s)", (int)countdown);
        log_info("===================================================\r\n");
    }
    if (flag_Power_Off)
    {
        countdown = (denominator % 20) - LastpSendCount;
        countdown = countdown < 0 ? 0 : countdown;
        log_info("====================== Lastp ======================");
        log_info("LastpSendFlag                                   :      %-6d ", LastpSendFlag);
        log_info("Lastp sending countdown                         :      %-6d      (s)", (int)countdown);
        if (PowerResetAnomalyCount > 100)
        {
            countdown = 300 - PowerResetAnomalyCount;
            countdown = countdown < 0 ? 0 : countdown;
            log_info("When both flag_Power_Off and NOT_COMPLETELY_POWER_OFF flags are present,\r\nreset countdown                                 :      %-6d      (s)", (int)countdown);
        }
        else if (NOT_COMPLETELY_POWER_OFF)
        {
            countdown = 300 - PowerResetAnomalyCount;
            countdown = countdown < 0 ? 0 : countdown;
            log_info("When both flag_Power_Off and NOT_COMPLETELY_POWER_OFF flags are present,\r\nreset countdown                                 :      %-6d      (s)", (int)countdown);
        }
        log_info("===================================================\r\n");
    }
    if (NOT_COMPLETELY_POWER_OFF)
    {
        log_info("===================================================");
        countdown = 10 - PowerOnCount;
        countdown = countdown < 0 ? 0 : countdown;
        log_info("Flags related to rapid power restoration before the MCU has restarted");
        log_info("GetPowerOnFlag                                  :      %-6d ", GetPowerOnFlag);
        log_info("Get Power on event countdown                    :      %-6d      (s)", (int)countdown);
        log_info("===================================================");
    }
    if (flag_Power_Off)
    {

    }
    if (receivebusyflag)
    {
        countdown = receivebusytime * 10 - receivecount;
        countdown = countdown < 0 ? 0 : countdown;
        log_info("Remaining time until receivebusyflag            :      %-6d      (s)", (int)countdown);
    }

    if (sendbusyflag)
    {
        countdown = sendbusytime * 10 - sendcount;
        countdown = countdown < 0 ? 0 : countdown;
        log_info("Remaining time until sendbusyflag               :      %-6d      (s)", (int)countdown);

    }
    if (On_Demand_Reading_Type)
    {
        countdown = ondemandreadingtime * 10 - OnDemandReadingcount;
        countdown = countdown < 0 ? 0 : countdown;
        log_info("Remaining time until On_Demand_Reading_Type     :      %-6d      (s)", (int)countdown);
    }
    if (CONTINUE_BUSY)
    {
        countdown = continuebusytime * 10 - CointbusyCount;
        log_info("Remaining time until CONTINUE_BUSY              :      %-6d      (s)", (int)countdown);

    }
    if (ResetRF_Flag)
    {
        countdown = 10 - ResetRFcount;
        countdown = countdown < 0 ? 0 : countdown;
        log_info("Remaining time until ResetRF_Flag               :      %-6d      (s)", (int)countdown);
    }
    log_info("Receive Queue                                   :      %d ", receiveIndex);
    log_info("Send Queue                                      :      %d ", sendIndex);
    log_info("Save byte                                       :      %d / %d",  currentQueueBytes, MAX_QUEUE_BYTES);
    if (receivebusyflag || sendbusyflag || On_Demand_Reading_Type || CONTINUE_BUSY || Broadcast_flag || flag_Power_Off || ResetRF_Flag || NOT_COMPLETELY_POWER_OFF || REGISTER_FLAG)
    {
        log_info("=============================================================   Flag   =============================================================");
        if (receivebusyflag)
        {
            log_info("receivebusyflag               :%-3d Timeout        = %-10d receivecount           = %d", receivebusyflag, (receivebusytime * 10), receivecount);
        }
        if (sendbusyflag)
        {
            log_info("sendbusyflag                  :%-3d Timeout        = %-10d sendcount              = %d", sendbusyflag, (sendbusytime * 10), sendcount);
        }
        if (On_Demand_Reading_Type)
        {
            log_info("On_Demand_Reading_Type        :%-3d Timeout        = %-10d OnDemandReadingcount   = %d", On_Demand_Reading_Type, ondemandreadingtime * 10, OnDemandReadingcount);
        }
        if (CONTINUE_BUSY)
        {
            log_info("CONTINUE_BUSY                 :%-3d Timeout        = %-10d CointbusyCount         = %d", CONTINUE_BUSY, continuebusytime * 10, CointbusyCount);
        }
        if (Broadcast_flag)
        {
            log_info("Broadcast_flag                :%-3d Timeout        = %-10d BroadcastCount         = %d", Broadcast_flag, Broadcast_meterdelay, BroadcastCount);
        }
        if (flag_Power_Off)
        {
            log_info("flag_Power_Off                :%-3d Timeout        = %-10d LastpSendCount         = %d", flag_Power_Off, (denominator % 20), LastpSendCount);
        }
        if (ResetRF_Flag)
        {
            log_info("ResetRF_Flag                  :%-3d Timeout        = %-10d LastpSendCount         = %d", flag_Power_Off, 10, ResetRFcount);
        }
        if (NOT_COMPLETELY_POWER_OFF)
        {
            log_info("NOT_COMPLETELY_POWER_OFF      :%-3d Timeout        = %-10d PowerOnCount           = %d", NOT_COMPLETELY_POWER_OFF, 10, PowerOnCount);
        }
        if (REGISTER_FLAG)
        {
            log_info("REGISTER_FLAG                 :%-3d Timeout        = %-10d RegisterCount          = %d", REGISTER_FLAG, 3600, RegisterCount);
        }
        log_info("====================================================================================================================================\r\n");
    }

}

static void ACK_function(uint8_t num)
{
    char ack_buff[1] = {0x00};
    ack_buff[0] = num;
    if (SuccessRole)
    {
        udf_Rafael_data(TASK_ID, 0x11, &ack_buff[0], 3, 1);
    }
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
            printf("%02X ", no_en_aarq[i]);
        }

        printf("\n\r");
        app_uart_data_send(2, no_en_aarq, sizeof(no_en_aarq));
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
            printf("%02X ", cmdbuf[i]);
        }
        printf("\n\r");

        app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));
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
        printf("%02X ", cmdbuf[i]);
    }
    printf("\n\r");

    app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));
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
        printf("%02X ", cmdbuf[i]);
    }
    printf("\n\r");

    app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));
}

static void Decide_Once(unsigned char *pt, unsigned char *meter_data)
{
    int i = 0;
    printf("Decide_Once\n");

    printf("pt[26] = %d\n", pt[26]);
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

/*
static void receive_queue(char *recvbuf, int receive_length)
{

    if (receiveIndex >= QUEUE_SIZE)
    {

        printf("Receive Queue is full or overflow: %d / %d\n", receiveIndex, QUEUE_SIZE);
        return;
    }
    receivebusyflag = 1;

    _ReceiveCommand_data_t ReceiveCommand_data;

    ReceiveCommand_data.pdata =  mem_malloc(receive_length);

    if (ReceiveCommand_data.pdata)
    {
        memcpy(ReceiveCommand_data.pdata, (uint8_t *)recvbuf, receive_length);
        ReceiveCommand_data.dlen = receive_length;
        while (xQueueSend(ReceiveCommand_Queue_handle, (void *)&ReceiveCommand_data, portMAX_DELAY) != pdPASS);

        APP_EVENT_NOTIFY(EVENT_ELS61_BLOCK_QUEUE);
        receiveIndex ++;
    }

    receivebusyflag = 0;

}
*/

static void receive_queue(char *recvbuf, int receive_length)
{

    if (currentQueueBytes + receive_length > MAX_QUEUE_BYTES)
    {
        printf("Receive Queue is full or overflow: %d bytes needed, %d bytes available\n", receive_length, MAX_QUEUE_BYTES - currentQueueBytes);
        APP_EVENT_NOTIFY(EVENT_ELS61_BLOCK_QUEUE);
        return;
    }
    receivebusyflag = 1;

    _ReceiveCommand_data_t ReceiveCommand_data;

    ReceiveCommand_data.pdata = mem_malloc(receive_length);
    if (ReceiveCommand_data.pdata)
    {
        memcpy(ReceiveCommand_data.pdata, (uint8_t *)recvbuf, receive_length);
        ReceiveCommand_data.dlen = receive_length;

        while (xQueueSend(ReceiveCommand_Queue_handle, (void *)&ReceiveCommand_data, portMAX_DELAY) != pdPASS);
        receiveIndex ++;
        currentQueueBytes += receive_length;  // 更新目前接收佇列使用的字節數
        APP_EVENT_NOTIFY(EVENT_ELS61_BLOCK_QUEUE);
    }

    receivebusyflag = 0;
}


static void udf_SMIP_Res(uint8_t *meter_data)
{
    char send_buff[128] = {0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0D,
                           0xC4, 0x01, 0x43, 0x00, 0x0A, 0x08, 0x31, 0x36,
                           0x36, 0x39, 0x33, 0x37, 0x35, 0x31
                          };
    uint16_t i = 0;

    send_buff[10] = riva_fix_cnt;

    for (i = 0; i < 8; i++)
    {
        send_buff[i + 14] = meter_data[i + 17];
    }

    printf("Send_buff= ");

    for (i = 0; i < 22; i++)
    {
        printf("%02X ", send_buff[i]);// test only
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
    int i = 0, data_len, j = 0; //,check_sum = 0
    unsigned char obis_data[10] = {0x00, 0x07, 0x01, 0x00, 0x63,
                                   0x01, 0x00, 0xFF, 0x02, 0x00
                                  };
    data_len = ct_len;
    for (i = 0; i < data_len; i++)
    {
        auto_send_data[i] = pt_confirm[i];
    }

    printf("pt[1] = %d , pt[3] = %d\n\r", pt_confirm[1],
           pt_confirm[3]);
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
                printf("%02X ", On_Demand_Task_Buff[j]);
            }
            On_Demand_length = On_Demand_length + data_len;
            printf("On_Demand_length = %d \n", On_Demand_length);
        }
        else
        {
            for (i = 0, j = On_Demand_length; i < data_len; i++, j++)
            {
                On_Demand_Task_Buff[j] = pt_confirm[i];
                printf("%02X ", On_Demand_Task_Buff[j]);
            }
            On_Demand_length = On_Demand_length + data_len;
            printf("On_Demand_length = %d \n", On_Demand_length);
        }

        Task_count--;

        if (Task_count > 0)
        {
            printf("Task_count = %d FIRST = %02X  END = %02X \n",
                   Task_count, (Task_count_all - Task_count) * 9 + 1,
                   (Task_count_all - Task_count) * 9 + 9);

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
                printf("%02X ", On_Demand_Task_Buff[i]);
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
                                    printf("%02X ",
                                           Power_On_data_Buff[k]);
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
                                    printf("%02X ",
                                           Power_On_data_Buff[k]);
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
            printf("%02X ", Power_On_Event_Buff[i]);
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
            printf("%02X ", tag[i]);
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
            printf("%02X ", Power_On_Event_Buff[i]);
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
        printf("%02X ", cmdTXT[i]);
    }
    printf("\n\r");*/
    for (i = 0; i < frameLen + 2; i++)
    {
        printf("%02X ", cmdTXT[i]);
    }
    printf("\n\r");

    app_uart_data_send(2, cmdTXT, (frameLen + 2));
}

static void Set_TOU_C2(uint8_t start)
{
    unsigned char obis_tou[9] = {20, 0, 0, 13, 0, 0, 255, 0, 0};
    uint16_t dataLen = 0; //,ip_len=0;
    uint8_t error = 0;
    /*05                                        saveSETdata[0]
    6(Calendar name)    00 08 saveSETdata[1~3]
    7(Season profile)   01 23 saveSETdata[4~6]
    8(Week profile)     01 23 saveSETdata[7~9]
    9(Day profile)      01 23 saveSETdata[10~12]
    10(Active time)     01 23 saveSETdata[13~15]  */
    printf("\n\r---Set_TOU_C2---\n\r");

    dataLen = (saveSETdata[2 + (start - 1) * 3] << 8) + saveSETdata[3 + (start - 1) * 3];
    if (dataLen == 0 || dataLen > 779)
    {
        error = 1;
    }
    if (error == 0 )
    {
        SET_TOU_C2_Start++;
        //SET_TOU_C2_Start = (SET_TOU_C2_Start == 5) ? 0:SET_TOU_C2_Start;//5Âk¹s

        obis_tou[7] = saveSETdata[1 + (start - 1) * 3]; //attribute_id
        udf_Set_Transmit(obis_tou, &saveSETdata[16 + SETdata_index], dataLen, 0, 0);

        SETdata_index += dataLen;
    }
    else
    {
        printf("\nC2 command error!\n");
        udf_Rafael_data(TASK_ID, 0xB2, 0/*error*/, 3, 1); //command error 0226
    }
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

void set_Test(void)
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



static void Set_Command_Select(uint8_t type_Cx, char *command_pk)
{
    uint16_t rf_Len = 0, ip_len = 0;
    uint16_t dataLen = 0;
    uint16_t i = 0;
    unsigned char obis[9];
    //  14 00 00 0D 00 00 FF 09 00
    printf("\n\rIn Set_Command_Select\n\r");
    rf_Len = (command_pk[0] << 8) + command_pk[1];
    ip_len = command_pk[3];

    if (type_Cx == 0xC1) //general set
    {
        dataLen = (rf_Len - 15 - 12 - ip_len); //6-12-9( header/ tag/ obis)
        SET_TOU_C1_INDEX = 1;
        for (i = 0; i < 9; i++)
        {
            obis[i] = command_pk[i + 7 + ip_len];
        }
        udf_Set_Transmit(obis, &command_pk[16 + ip_len], dataLen,   0, 0);
    }
    else if (type_Cx == 0xC2) //combine tou set
    {
        if (command_pk[ip_len + 7] == 0) //1 pk set
        {
            //[9],³]©w´Xµ§ [10][11,12]

            dataLen = rf_Len - 8 - 12 - ip_len;
            memcpy( saveSETdata, (void *)&command_pk[ip_len + 9], dataLen);
            SETdata_index = 0;
            Set_TOU_C2(SET_TOU_C2_Start = 1); //start=1
        }
        else if (command_pk[ip_len + 7] == 1) //transmit
        {
            if (command_pk[ip_len + 8] == 1) //Äò¶Ç²Ä¤@¥]
            {
                //
                SETdata_RECEIVE_Tatol_LEN = 0;
                memset(saveSETdata, 0, sizeof(saveSETdata));
            }

            dataLen = rf_Len - 8 - 12 - ip_len;
            memcpy( &saveSETdata[SETdata_RECEIVE_Tatol_LEN], (void *)&command_pk[ip_len + 9], dataLen);

            SETdata_RECEIVE_Tatol_LEN += dataLen;
            printf("\n\rC2 Transmit ACK!!\n\r");

            udf_Rafael_data(TASK_ID, 0xB2, &command_pk[ip_len + 8], 3, 1); //B2¦^À³
        }
        else if (command_pk[ip_len + 7] == 2) //transmit last 1
        {
            dataLen = rf_Len - 8 - 12 - ip_len;
            memcpy( &saveSETdata[SETdata_RECEIVE_Tatol_LEN], (void *)&command_pk[ip_len + 9], dataLen);

            SETdata_RECEIVE_Tatol_LEN += dataLen;
            //send 2 meter set data
            printf("\n\rC2 Transmit Set!!\n\r");
            SETdata_index = 0;
            Set_TOU_C2(SET_TOU_C2_Start = 1);
        }

    }
    else if (type_Cx == 0xC3) //0626
    {
        if (command_pk[ip_len + 7] == 1) //transmit
        {
            if (command_pk[ip_len + 8] == 1)
            {
                //
                SETdata_RECEIVE_Tatol_LEN = 0;
                memset(saveSETdata, 0, sizeof(saveSETdata));
            }

            dataLen = rf_Len - 8 - 12 - ip_len;
            memcpy( &saveSETdata[SETdata_RECEIVE_Tatol_LEN], (void *)&command_pk[ip_len + 9], dataLen);

            SETdata_RECEIVE_Tatol_LEN += dataLen;
            printf("\n\rC3 Transmit ACK!!\n\r");
            udf_Rafael_data(TASK_ID, 0xB3, &command_pk[ip_len + 8], 3, 1); //B2¦^À³
        }
        else    if (command_pk[ip_len + 7] == 2) //last transmit
        {
            dataLen = rf_Len - 8 - 12 - ip_len;
            memcpy( &saveSETdata[SETdata_RECEIVE_Tatol_LEN], (void *)&command_pk[ip_len + 9], dataLen);
            SETdata_RECEIVE_Tatol_LEN += dataLen;
            printf("\n\rC3 Transmit Set in flash!!!\n\r");

            //WriteFlashData(SETDATA_C3_PAGE, saveSETdata ,SETdata_RECEIVE_Tatol_LEN);

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

        printf("%02X ", ucMeterPacket[i]);
    }

    Event_Notification = 0;
    printf("\n\r");
#endif
}

void udf_Send_SNRM(uint8_t type)
{
    unsigned char cmdbuf[] = {0x7E, 0xA0, 0x07, 0x03, 0x23,
                              0x93, 0xBF, 0x32, 0x7E
                             };
    int i;
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
        printf("%02X ", cmdbuf[i]);
    }
    printf("\n");

    app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));
}

static void Broadcast_delay(void)
{
    char hex[5];
    int i = 0, num;
    int size = sizeof(hex) / sizeof(hex[0]);
    char str[30];

    for (i = 19; i < 24; i++)
    {
        hex[i - 19] = Meter_ALL_ID[i];
    }



    //將字節數組轉換為一個字符串
    for (i = 0; i < size; i++)
    {
        str[i] = hex[i];
    }
    str[size] = '\0'; //添加結束字符
    //將字符串轉換為整數
    sscanf(str, "%d", &num);
    printf("The decimal number is: %d\r\n", num);
    denominator = num;
    if ((denominator % Broadcastmeterdelay) == 0)
    {
        Broadcast_meterdelay = (denominator % Broadcastmeterdelay) + 1;
    }
    else
    {
        Broadcast_meterdelay = (denominator % Broadcastmeterdelay);
    }
    Register_Timeout = (Broadcast_meterdelay + 64);
}

static void udf_get_noEn(unsigned char *obis_data)
{
    uint16_t CRC_check;
    unsigned char cmdbuf[] =
    {
        0x7E, 0xA0, 0x19, 0x03, 0x21, 0x13, 0xF1, 0xF9, 0xE6, 0xE6, //[0-9]
        0x00, 0xC0, 0x01, 0x42,
        0x00, 0x07, 0x01, 0x00, 0x63, 0x01, 0x00, 0xFF, 0x02, 0x01,
        0x01, 0x02,
        0x7E, /*
         0x02, 0x04, //[34-35]
         0x12, 0x00, 0x08, //[36-38]
         0x09, 0x06, 0x00, 0x00, 0x01, 0x00, 0x00, 0xFF, 0x0F, 0x02, 0x12, 0x00, 0x00, //[39-51]
         0x09, 0x0C, 0x07, 0xE1, 0x07, 0x15, 0x05, 0x07, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00, //[52-65]
         0x09, 0x0C, 0x07, 0xE1, 0x07, 0x15, 0x05, 0x08, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00, //[66-79]
         0x01, 0x00, //[80-81]
         0xEA, 0xDB, 0x61, 0x8B, 0x34, 0x98, 0x1A, 0xBE, 0x16, 0x66, 0xF2, 0x12,
         0xE3, 0x1E, 0x7E*/
    };
    int i;

    //      log_info_hexdump("\robis", obis_data, sizeof(obis_data));

    //      for (i=0; i<10; i++)
    //      {
    //          printf("%02X ", *(obis_data+i));
    //      }
    //      printf("\n");


    CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 5);
    cmdbuf [6] = CRC_check & 0xff;
    cmdbuf [7] = (CRC_check >> 8) & 0xff;

    for (i = 0; i < 10; i++)
    {
        cmdbuf[14 + i] = *(obis_data + i);
    }

    CRC_check = CRC_COMPUTE((char *)&cmdbuf[1], 23);
    cmdbuf [24] = CRC_check & 0xff;
    cmdbuf [25] = (CRC_check >> 8) & 0xff;

    for (i = 0; i < 27; i++)
    {
        printf("%02X ", cmdbuf[i]);
    }
    printf("\n");


    app_uart_data_send(2, cmdbuf, 27);
}


static void meter_Boot_Step(void)
{
    unsigned char OBIS_Meter_ID[10] = {0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02, 0xff, 0x02, 0x00};
    unsigned char OBIS_Customer_ID[10] = {0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0xff, 0x02, 0x00};
    unsigned char OBIS_District_ID[10] = {0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0xff, 0x02, 0x00};
    unsigned char OBIS_Meter_Type[10] = {0x00, 0x01, 0x00, 0x00, 0x60, 0x01, 0x00, 0xff, 0x02, 0x00};
    unsigned char OBIS_TIME[10] = {0x00, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0xff, 0x02, 0x00};
    int i = 0;

    switch (meterBootStep)
    {
    case DISC:
        if ((dlms_rx_buf[3] == 0x21 && dlms_rx_buf[5] == 0x1F) || (dlms_rx_buf[3] == 0x21 && dlms_rx_buf[5] == 0x73))
        {
            udf_Send_SNRM(1);
        }
        meterBootStep = SNRM;

        break;
    case SNRM:
        printf("Meter_UA\r");
        if (dlms_rx_buf[3] == 0x21)
        {
            udf_Send_AARQ(1);
        }
        meterBootStep = AARQ;
        break;

    case AARQ://RES-UA
        printf("Meter_AARE_V\r");
        if (dlms_rx_buf[5] == 0x13 && dlms_rx_buf[11] == 0x61)
        {
            printf("District_ID\r");
            vTaskDelay(200);
            udf_get_noEn(&OBIS_District_ID[0]);
            meterBootStep = District_ID;
        }
        break;

    case District_ID:   //Read_RES
        if (dlms_rx_buf[9] == 0xE7 && dlms_rx_buf[11] == 0xC4)
        {
            for (i = 0; i < 8; i++)
            {
                Meter_ALL_ID[i] = dlms_rx_buf[17 + i];
            }

            printf("Customer_ID\r");
            udf_get_noEn(&OBIS_Customer_ID[0]);
            meterBootStep = Customer_ID;
        }
        break;
    case Customer_ID:   //Read_RES
        if (dlms_rx_buf[9] == 0xE7 && dlms_rx_buf[11] == 0xC4)
        {
            for (i = 0; i < 8; i++)
            {
                Meter_ALL_ID[8 + i] = dlms_rx_buf[17 + i];
            }
            printf("Meter_ID\r");
            udf_get_noEn(&OBIS_Meter_ID[0]);
            meterBootStep = Meter_ID;
            //IWDG_ReloadCounter();
        }
        break;
    case Meter_ID:  //Read_RES
        if (dlms_rx_buf[9] == 0xE7 && dlms_rx_buf[11] == 0xC4)
        {
            for (i = 0; i < 8; i++)
            {
                Meter_ALL_ID[16 + i] = dlms_rx_buf[17 + i];
            }
            printf("Service_Code\r");
            udf_get_noEn(&OBIS_Meter_Type[0]);
            meterBootStep = Server_CODE;
        }
        break;
    case Server_CODE:
        if (dlms_rx_buf[9] == 0xE7 && dlms_rx_buf[11] == 0xC4)
        {
            for (i = 0; i < 6; i++)
            {
                Meter_ALL_ID[24 + i] = dlms_rx_buf[17 + i];
            }
            printf("get time\r");
            udf_get_noEn(&OBIS_TIME[0]);
            meterBootStep = Get_time;
        }
        break;
    case Get_time:
        if (dlms_rx_buf[9] == 0xE7 && dlms_rx_buf[11] == 0xC4)
        {
            begin.tm_year = (dlms_rx_buf[16] * 256 + dlms_rx_buf[16 + 1] - 1900); /*= {recvbuf[19],recvbuf[18],recvbuf[17],recvbuf[14],recvbuf[14], recvbuf[12]};(recvbuf[12]*256 + recvbuf[13] -1900)};//12~19*/
            begin.tm_mon = dlms_rx_buf[16 + 2] - 1;
            begin.tm_mday = dlms_rx_buf[16 + 3];
            begin.tm_hour = dlms_rx_buf[16 + 5];
            begin.tm_min = dlms_rx_buf[16 + 6];
            begin.tm_sec = dlms_rx_buf[16 + 7];
            now_time = mktime(&begin);
            RTC_CNT = 0;
            Broadcast_delay();
            meterBootStep = Preliminary_Work_Completed;

            APP_EVENT_NOTIFY(EVENT_METER_BOOT_STEP);
        }
        break;
    }

}


static void udf_Get_continue(void)
{
    int i;
    char strbuf[128];
    unsigned char cipher[128], tag[16];
    unsigned short crc16;
    unsigned char ch;
    uint8_t pt_len;

    unsigned char cmdbuf[] =
    {
        0x7E, 0xA0, 0x26, 0x03, 0x23, 0x13, 0x5F, 0x25, 0xE6, 0xE6, //7E A0 26 03 23 13 5F 25 E6 E6 00 D0 18
        0x00, 0xD0, 0x18, 0x30,
        0x00, 0x00, 0x00, 0x05,
        0xC0, 0x02, 0x44,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0xEA, 0xDB, 0x61, 0x8B, 0x34, 0x98, 0x1A, 0xBE, 0x16, 0x66, 0xF2, //tag
        0xE3, 0x1E, 0x7E
    }; // 46B
    log_info("\n======== udf_Get_continue ========\n");
    cmdbuf[23] = (Resume_index >> 8) & 0xFF;
    cmdbuf[24] = Resume_index & 0xFF;

    // update buffer
    cmdbuf[16] = (ic_count >> 8) & 0xFF;
    cmdbuf[17] = ic_count & 0xFF;

    cmdbuf[20] = ((ic_count - 3) % 0x40) + 0x40;

    //  for (i=0; i<10; i++)
    //      cmdbuf[21+i] = obis_data[i];

    // data encrypt
    pt_len = cmdbuf[12] - 17;

    for (i = 0; i < 4; i++)
    {
        tpc_c_iv[8 + i] = cmdbuf[14 + i];
    }

    udf_AES_GCM_Encrypt(tpc_dk, tpc_gcm_ak, tpc_c_iv, &cmdbuf[18], &cipher[0], &tag[0], pt_len, 17);

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
        printf("%02X ", cmdbuf[i]);

    }
    printf("\n\r");

    app_uart_data_send(2, cmdbuf, sizeof(cmdbuf));

    Resume_index++;
}


static void udf_Meter_Process(uint8_t *meter_data, uint16_t data_len)
{
    int i = 0;
    uint8_t tag[12], pt[1024] = {0};
    uint8_t ex_byte = 0, Res_type;
    uint16_t ct_len = 0;
    uint8_t EN_done = 0;

    unsigned char obis_notification_data[] = { 0x07, 0x00, 0x00, 0x63, 0x62, 0x00, 0xFF, 0x02};
    unsigned char obis_data[10] = {   0x00, 0x07, 0x01, 0x00,  0x63, 0x01, 0x00, 0xFF, 0x02, 0x00}, ack_buff[1] = {0x01};

    if (meter_data[3] == 0x27)  //PASS HAN // frank,201810
    {
        uart_stdio_write(meter_data, data_len);
        return;
    }

    ///////////////////////////Event Notification///////////////////////////////////////////////
    if (MC_done == 1 && data_len == 60 && meter_data[11] == 0xCA) // Event Notification
    {
        printf("send notification\n");
        EN_done = 1;
#if TEST_NOTIFICATION == 1
        if (flag_Power_Off == 0)
        {
            memcmp(event_notification_data, meter_data, 60);
            udf_Rafael_data(1, 8, (char *)&meter_data[0], 3, 60);
            //Notification_Buff(&event_notification_data[0]);
        }
        if (flag_Power_Off == 1) //Power off
        {
        }
        if (MAA_flag == STATUS_NOT_OBTAINED_KEY)
        {
            AAcheck = 1;//start AA
            flag_MAA = 0;
            MAAFIRST = 1;
        }
#elif  TEST_NOTIFICATION == 0 //Only generated once per day.
        // Replace IV
        memcmp(&tpc_s_iv[8], &meter_data[14], 4);

        // Replace Tag
        memcmp(tag, &meter_data[45], 12);
        if (udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv, (uint8_t *)&meter_data[18], tag, &pt[0], 27, 17))
        {
            printf("AES_GCM_Decrypt fail \r\n");
        }
        if (flag_Power_Off == 0)
        {
            Decide_Once(&pt[0], &meter_data[0]);
        }
        if (flag_Power_Off == 1) //Power off
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
            //printf("ic count = %d\r\n", ic_count);
        }

        Res_type = Check_Meter_Res((char *)&meter_data[0], data_len);
        printf("Res Type: %d\n\r", Res_type);
        if (meterBootStep == Preliminary_Work_Completed)
        {
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

            case 61://RES-UA
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

                    udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv, &meter_data[84], tag, &pt[0], 14, 17);

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
                        tpc_s_iv_h[8 + i] = meter_data[80 + i];    //80
                    }

                    // Replace Tag
                    for (i = 0; i < 12; i++)
                    {
                        tag[i] = meter_data[98 + i];    //98
                    }

                    udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv_h, &meter_data[84], tag, &pt[0], 14, 17);
                    udf_Han_Client();
                }
                else if (meter_data[3] == 0x21)
                {
                    printf("Meter_AARE_V\n\r");
                }

                break;

            case 11://RES-Client AA & Change Key RES
                if ((unsigned char)meter_data[12] > 128)
                {
                    ex_byte = meter_data[12] & 0x0F;
                }
                else
                {
                    ex_byte = 0;
                }

                if (ex_byte == 2)
                {
                    ct_len = (meter_data[11 + ex_byte] * 256) + meter_data[12 + ex_byte] - 17;
                }
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

                    udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv, &meter_data[18], tag, &pt[0], ct_len, 17);
                    if (RE_AA == 1)
                    {
                        MAA_flag = STATUS_REGISTRATION_COMPLETE;
                        RE_AA = 0;
                    }
                    if (MAAFIRST == 1)
                    {
                        MAA_flag = STATUS_OBTAINED_KEY;
                        sst_flag = 1;
                        if (memcmp(tpc_guk, &Configuration_Page[48], 16) == 0 &&
                                memcmp(&tpc_gcm_ak[1], &Configuration_Page[64], 16) == 0 &&
                                memcmp(tpc_mkey, &Configuration_Page[80], 16) == 0)
                        {
                            printf("No need to write the same key into flash if it is already the same.\r\n");
                        }
                        else
                        {
                            SaveKey();
                        }
                        KeyErrorflag = 0;//key correct
                        register_steps = 2;
                        MAAFIRST = 2;
                    }
                    if (ACTION_CHANGE_KEY == 1)
                    {
                        if ( pt[0] == 0xC7 && pt[3] == 0x00 && pt[4] == 0x00) // right key and change
                        {
                            if (TPC_AK_FLAG == 1)
                            {
                                //                              for(i=0;i<17;i++)
                                //                              {
                                //                                  tpc_gcm_ak[i] = new_tpc_gcm_ak[i];
                                //                                  tpc_gmac_ak[i] = new_tpc_gmac_ak[i];
                                //                              }
                                TPC_AK_FLAG = 2;
                            }
                            if (TPC_GUK_FLAG == 1)
                            {
                                //                              for(i = 0;i < 16; i++)
                                //                                  tpc_guk[i] = new_tpc_guk[i];
                                TPC_GUK_FLAG = 2;
                            }
                            if (TPC_MK_FLAG == 1)
                            {
                                //                              for(i=0 ;i<16 ;i++)
                                //                                  tpc_mkey[i] = new_tpc_mkey[i];
                                TPC_MK_FLAG = 2;
                            }
                            for (i = 0 ; i < meter_data[2] + 2 ; i++) //copy response
                            {
                                actionkey_response[i] = meter_data[i];
                            }
                            KEYresponse_len = meter_data[2] + 2;
                            udf_Rafael_data(TASK_ID, 0x1A, (char *)&meter_data[0], 3, data_len);
                            //SQBrespone(0x1A, &meter_data[0]);
                            RE_AA = 1;
                            //udf_Send_SNRM(2);
                        }
                        else
                        {
                            for (i = 0 ; i < meter_data[2] + 2 ; i++) //copy response
                            {
                                actionkey_response[i] = meter_data[i];
                            }
                            TPC_GUK_FLAG = 0;
                            TPC_AK_FLAG = 0;
                            TPC_MK_FLAG = 0;
                            udf_Rafael_data(TASK_ID, 0x1A, (char *)&meter_data[0], 3, data_len);
                        }
                        ACTION_CHANGE_KEY = 0;
                        MAA_flag = STATUS_REGISTRATION_COMPLETE;
                    }
                    else if (ON_DEMAND_AA_FLAG == 1)
                    {
                        On_Demand_Reading_Type = 1;

                        for (i = 14 ; i <= 22; i++)
                        {
                            obis_data[i - 13] = On_Demand_Task_Command[i - 14];
                        }
                        //Task_count --;
                        udf_GetData_Request(obis_data, ic_count, 1);
                        MAA_flag = STATUS_REGISTRATION_COMPLETE;
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
                        MAA_flag = STATUS_REGISTRATION_COMPLETE;
                    }


                    if (NOT_COMPLETELY_POWER_OFF == 1)
                    {
                        printf("NOT_COMPLETELY_POWER_OFF = 1\n");
                        udf_Get_New_Number_of_pens(ic_count, &obis_notification_data[0], 1);
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

                    udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv_h, &meter_data[18], tag, &pt[0], ct_len, 17);
                }
                MC_done = 1;
                break;

            case 1:
                if (get_id_done == 0) //Reserve, Unable to obtaon V AA connection ID
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

            case 2: //Read_RES
                if ((unsigned char)meter_data[12] > 128)
                {
                    ex_byte = meter_data[12] & 0x0F;
                }
                else
                {
                    ex_byte = 0;
                }

                if (ex_byte == 2)
                {
                    ct_len = (meter_data[11 + ex_byte] * 256) + meter_data[12 + ex_byte] - 17;
                }
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

                    udf_AES_GCM_Decrypt(tpc_dk, tpc_gcm_ak, tpc_s_iv, &meter_data[18 + ex_byte], tag, &pt[0], ct_len, 17);
                    if (pt[1] == 0x02  && MAA_flag == STATUS_REGISTRATION_COMPLETE && flag_Power_Off == 0) //Continue transmission
                    {
                        Refuse_Command = 0;
                        log_info("====== continuable transmission ======");
                        CONTINUE_BUSY = (pt[3] == 0x00) ? 1 : 0;
                        if (pt[6] == 0 && pt[7] == 0x01)
                        {
                            continuebusyflagstart = 1;
                            Resume_index = 1;
                            Continue_lastindex = 0;
                            Continue_TASK_ID = TASK_ID;
                        }
                        Continue_index = pt[6] * 256 + pt[7];
                        log_info("continue transfer to: %d", Continue_index);

                        /////////////////////////////
                        if (CONTINUE_BUSY == 1)
                        {

                            ELS61_MESSAGE_LOCK = 1;             //LOCK 4G QUEUE
                            if (On_Demand_Reading_Type == 1)
                            {
                                on_Demand_Task_Assembly(&pt[0], ct_len, &pt[0]);
                            }
                            else if (Auto_Reading == 1 && On_Demand_Reading_Type == 0)
                            {

                                udf_Rafael_data(Continue_TASK_ID, On_Demand_Type, (char *)&meter_data[0], 3, data_len);
                                //auto_send_type_buff(On_Demand_Type,&meter_data[0]);
                            }
                            else if (On_Demand_ID == 1 && On_Demand_Reading_Type == 0)
                            {
                                udf_Rafael_data(Continue_TASK_ID, On_Demand_Type, (char *)&meter_data[0], 3, data_len);
                                //On_Demand_type_buff(On_Demand_Type,&meter_data[0]);
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
                            else if (Auto_Reading == 1 && On_Demand_Reading_Type == 0)
                            {

                                udf_Rafael_data(Continue_TASK_ID, On_Demand_Type, (char *)&meter_data[0], 3, data_len);
                                //auto_send_type_buff(On_Demand_Type,&meter_data[0]);
                                Auto_Reading = 0;
                            }
                            else if (On_Demand_ID == 1 && On_Demand_Reading_Type == 0)
                            {
                                udf_Rafael_data(Continue_TASK_ID, On_Demand_Type, (char *)&meter_data[0], 3, data_len);
                                //On_Demand_type_buff(On_Demand_Type,&meter_data[0]);
                                On_Demand_ID = 0;
                            }
                            else
                            {
                                printf("\nERROR COMMAND\n");
                            }
                            CONTINUE_BUSY = 0;
                        }
                        if (Continue_index == (Continue_lastindex + 1))
                        {
                            Continue_lastindex = Continue_index;
                            STEP_CONTINUE_FLAG = 0;
                            if ((target_pos == 2 || target_pos  == 3) && CONTINUE_BUSY == 1 && STEP_CONTINUE_FLAG == 0 && flag_Power_Off == 0 )
                            {
                                STEP_CONTINUE_FLAG = 1;
                                udf_Get_continue();
                            }
                        }
                        else
                        {

                            log_info("packet resumption error");
                            CONTINUE_BUSY = 0;
                            continuebusyflagstart = 0;
                            Resume_index = 1;
                        }

                        if (CONTINUE_BUSY == 0)
                        {
                            Resume_index = 1;
                            ELS61_MESSAGE_LOCK = 0;             //UNLOCK 4G QUEUE
                        }
                    }
                    else if (MAA_flag == STATUS_REGISTRATION_COMPLETE && flag_Power_Off == 0 && NOT_COMPLETELY_POWER_OFF == 0)
                    {
                        Refuse_Command = 0;
                        if (On_Demand_Reading_Type == 1)
                        {
                            on_Demand_Task_Assembly(&pt[0], ct_len, &pt[0]);
                        }
                        else if (Auto_Reading == 1 && On_Demand_Reading_Type == 0)
                        {
                            udf_Rafael_data(TASK_ID, On_Demand_Type, (char *)&meter_data[0], 3, data_len);
                            //auto_send_type_buff(On_Demand_Type,&meter_data[0]);
                            Auto_Reading = 0;
                        }
                        else if (On_Demand_ID == 1 && On_Demand_Reading_Type == 0)
                        {
                            //log_info("task id = %d On_Demand_Type = %d data_len = %d", TASK_ID, On_Demand_Type, data_len);
                            //log_info_hexdump("meter_data", meter_data, data_len);
                            udf_Rafael_data(TASK_ID, On_Demand_Type, (char *)&meter_data[0], 3, data_len);
                            //On_Demand_type_buff(On_Demand_Type,&meter_data[0]);
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
                    if (MAA_flag == STATUS_SENT_ACK || NOT_COMPLETELY_POWER_OFF == 1)
                    {
                        MAA_flag = STATUS_REGISTRATION_COMPLETE;
                        Power_On_Event_Log_Return(&pt[0]); //Return Negative charge data
                        NOT_COMPLETELY_POWER_OFF = 0;
                        GetPowerOnFlag = 0;
                        Refuse_Command = 0;
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

                    udf_AES_GCM_Decrypt(tpc_dk, tpc_gcm_ak, tpc_s_iv_h, &meter_data[18 + ex_byte], tag, &pt[0], ct_len, 17);
                }
                break;

            case 21://SET_RES
                if ((unsigned char)meter_data[12] > 128)
                {
                    ex_byte = meter_data[12] & 0x0F;
                }
                else
                {
                    ex_byte = 0;
                }

                if (ex_byte == 2)
                {
                    ct_len = (meter_data[11 + ex_byte] * 256) + meter_data[12 + ex_byte] - 17;
                }
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

                udf_AES_GCM_Decrypt(tpc_dk, tpc_gcm_ak, tpc_s_iv, &meter_data[18 + ex_byte], tag, &pt[0], ct_len, 17);
                SETACTION++;
                //0615//bob
                if (pt[0] == 0xC5 && pt[3] == 0 && SET_TOU_C2_Start != 0 && SET_TOU_C2_Start <= saveSETdata[0]/*0620*/ ) //tou_c2 ok
                {
                    Refuse_Command = 0;
                    //0617
                    printf("\n\ragain\n\r");
                    Set_TOU_C2(SET_TOU_C2_Start);
                }
                else if (pt[0] == 0xC5 && pt[3] == 0 && SET_TOU_C2_Start > saveSETdata[0]/*0620*/) //tou_c2 last, ok sent ack
                {
                    Refuse_Command = 0;
                    printf("\n\rSET_TOU_C2 OK!!!!\n\r");
                    SET_TOU_C2_Start = 0;
                    SETdata_index = 0;
                    memset(saveSETdata, 0, sizeof(saveSETdata));
                    udf_Rafael_data(TASK_ID, 0xB2, (char *)&meter_data[0], 3, data_len); //���]��4g
                }
                else if (pt[0] == 0xC5 && pt[3] != 0 && SET_TOU_C2_Start != 0) //tou_c2 ok
                {
                    Refuse_Command = 0;
                    printf("\n\rSET_TOU_C2 errrrrrr!!!!\n\r");
                    udf_Rafael_data(TASK_ID, 0xB2, (char *)&meter_data[0], 3, data_len); //0717

                    SET_TOU_C2_Start = 0;
                    SETdata_index = 0;
                    memset(saveSETdata, 0, sizeof(saveSETdata));
                }
                //0615//bob
                //0617//bob
                if (pt[0] == 0xC5 && pt[3] == 0 && SET_TOU_C1_INDEX == 1) //tou_c1 last, ok sent ack
                {
                    Refuse_Command = 0;
                    //SET_TOU_C1_INDEX
                    printf("\n\rSET_TOU_C1 OK!!!!\n\r");
                    SET_TOU_C1_INDEX = 0;
                    udf_Rafael_data(TASK_ID, 0xB1, (char *)&meter_data[0], 3, meter_data[2] + 2); // Packaged for 4G
                }
                else    if (pt[0] == 0xC5 && pt[3] != 0 && SET_TOU_C1_INDEX == 1) //tou_c1 error
                {
                    Refuse_Command = 0;
                    printf("\n\rSET_TOU_C1 error!!!!\n\r");
                    SET_TOU_C1_INDEX = 0;
                    udf_Rafael_data(TASK_ID, 0xB1, (char *)&meter_data[0], 3, meter_data[2] + 2); // Packaged for 4G
                }
                //0617//bob
                //0621
                if (pt[0] == 0xC5 && pt[3] == 0 && SET_TIME_C0_INDEX == 1) //set time
                {
                    Refuse_Command = 0;
                    //SET_TOU_C1_INDEX
                    printf("\n\rSET_TIME_C0_INDEX OK!!!!\n\r");
                    SET_TIME_C0_INDEX = 0;
                    udf_Rafael_data(TASK_ID, 0xB0, (char *)&meter_data[0], 3, data_len); // Packaged for 4G
                }
                else    if (pt[0] == 0xC5 && pt[3] != 0 && SET_TIME_C0_INDEX == 1)
                {
                    Refuse_Command = 0;
                    printf("\n\rSET_TIME_C0_INDEX error!!!!\n\r");
                    SET_TIME_C0_INDEX = 0;
                    udf_Rafael_data(TASK_ID, 0xB0, (char *)&meter_data[0], 3, data_len); // Packaged for 4G
                }
                //0621
                if ( pt[3] != 0  && pt[0] == 0xC5 && SQBsetStart == 1) //error
                {
                    Refuse_Command = 0;
                    SQBrespone(0x1B, &meter_data[0] );
                    SQBsettingNUM = 0xFF;//break
                    SQBsetStart = 0;

                    SETQBtmp = 1;
                    HANDSQB = 0;
                    memset( saveSETdata, 0, sizeof(saveSETdata));
                    //      memset( SaveSETheader ,0,sizeof(SaveSETheader));//bob
                }

                if (SETACTION != 0 && hitHANDguN == 1)
                {
                    Refuse_Command = 0;
                    set_Test();
                }
                break;

            case 3:
                printf("GET Alarm\n\r");
                udf_Alarm_Register1();
                break;
            case 99://ACTION_RES
                printf("GET Action_res\n\r");
                if (Auto_Reading == 1)
                {
                    auto_read_loadprofile = 0;
                    auto_read_Midnight = 0;
                    auto_read_ALT  = 0;
                    Auto_Reading = 0;
                }
                if (POWERON_Restart == 1)   //0205 matt
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
                {
                    ct_len = (meter_data[11 + ex_byte] * 256) + meter_data[12 + ex_byte] - 17;
                }
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

                udf_AES_GCM_Decrypt(tpc_dk, tpc_gcm_ak, tpc_s_iv, &meter_data[18 + ex_byte], tag, &pt[0], ct_len, 17);
                udf_Rafael_data(TASK_ID, 19, (char *)&meter_data[0], 3, data_len);
                //On_Demand_type_buff(0x19,&meter_data[0]);//20191104
                break;
            case 33://Error
                if (MAA_flag == STATUS_NOT_OBTAINED_KEY && sst_flag == 1)
                {
                    //NVIC_SystemReset(); // 0211 matt
                    printf("Key_Error or SST Fail\n\r");
                    //udf_Rafael_data(1,1,(char *)&ack_buff[0],3,1);

                    RESTART_FLAG = 1;       // key error 1hr restart 0211 matt
                }
                if (TPC_GUK_FLAG == 1 || TPC_AK_FLAG == 1 || TPC_MK_FLAG == 1)
                {
                    TPC_GUK_FLAG = 0;
                    TPC_AK_FLAG = 0;
                    TPC_MK_FLAG = 0;
                }
                if (POWERON_Restart == 1)   //0205 matt
                {
                    printf("NVIC_SystemReset:4\n\r");
                    NVIC_SystemReset();
                }

                if (MAA_flag == STATUS_REGISTRATION_COMPLETE)
                {
                    Refuse_Command ++;
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
            case 12://EVENT NOTIFICATION
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
                    udf_AES_GCM_Decrypt(tpc_guk, tpc_gcm_ak, tpc_s_iv_h, &meter_data[18], tag, &pt[0], 27, 17);
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
        else //Read Meter Data on Boot
        {
            meter_Boot_Step();
        }

    }
}
static void flash_Information(void)
{
    static uint8_t read_buf[0x100];
    int i = 0;
    printf("\n\rflash_get_Configuration_Page(200) = ");
    flash_read_page((uint32_t)(read_buf), ConfigurationPage);
    memcpy(Configuration_Page, read_buf, 200);
    printf("\nFan ID                            :  ");
    for (i = 2; i <= 11; i++)
    {
        printf("%c ", Configuration_Page[i]);
    }
    printf("\n");

#if 0 //use ot dataset command directily
    printf("Leader Weight: %02X\n", Configuration_Page[12]);
    printf("Networkid: ");
    for (i = 13; i <= 14; i++)
    {
        printf("%02X ", Configuration_Page[i]);
    }
    printf("\n");
    printf("PANID: ");
    for (i = 15; i <= 16; i++)
    {
        printf("%02X ", Configuration_Page[i]);
    }
    printf("\n");
    printf("Channel: %02X\n", Configuration_Page[17]);
#endif
    printf("send els61 timeout                :  ");
    for (i = 18; i <= 19; i++)
    {
        printf("%02X ", Configuration_Page[i]);
    }
    printf("   (10 sec)");
    printf("\n");
    //    printf("send rf timeout                   :  ");
    //    for (i = 20; i <= 21; i++)
    //    {
    //        printf("%02X ", Configuration_Page[i]);
    //    }
    //    printf("   (10 sec)");
    printf("receive timeout                   :  ");
    for (i = 22; i <= 23; i++)
    {
        printf("%02X ", Configuration_Page[i]);
    }
    printf("   (10 sec)");
    printf("\n");
    printf("continue timeout                  :  ");
    for (i = 24; i <= 25; i++)
    {
        printf("%02X ", Configuration_Page[i]);
    }
    printf("   (10 sec)");
    printf("\n");
    //    printf("A                                : %02X\n", Configuration_Page[26]);
    //    printf("Server IP: ");
    //    for (i = 27; i <= 30; i++)
    //    {
    //        printf("%02X ", Configuration_Page[i]);
    //    }
    //    printf("\n");
    //    printf("Socket port: ");
    //    for (i = 31; i <= 32; i++)
    //    {
    //        printf("%02X ", Configuration_Page[i]);
    //    }
    //    printf("\n");
    //    printf("APN: ");
    //    for (i = 33; i <= 47; i++)
    //    {
    //        printf("%02X ", Configuration_Page[i]);
    //    }
    //    printf("\n");
    printf("tpc guk                           :  ");
    for (i = 48; i <= 63; i++)
    {
        printf("%02X ", Configuration_Page[i]);
    }
    printf("\n");
    printf("tpc gcm ak                        :  ");
    for (i = 64; i <= 79; i++)
    {
        printf("%02X ", Configuration_Page[i]);
    }
    printf("\n");
    printf("tpc mkey                          :  ");
    for (i = 80; i <= 95; i++)
    {
        printf("%02X ", Configuration_Page[i]);
    }
    printf("\n");
    printf("on demandreading timeout          :  ");

    for (i = 96; i <= 97; i++)
    {
        printf("%02X ", Configuration_Page[i]);
    }
    printf("   (10 sec)");
    printf("\n");

    printf("keyObtainedStatusIncorrecttimeout :  %-3d      (min)\n", Configuration_Page[138]);
    printf("registerRetrytime                 :  %-3d      (min)\n", Configuration_Page[139]);
    printf("Rafael_reset                      :  %-3d      (min)\n", Configuration_Page[140]);
    printf("Broadcastmeterdelay               :  %-3d      (MOD)\n", Configuration_Page[141]);

}
void udf_Meter_received_task(const uint8_t *aBuf, uint16_t aBufLength)
{
    const uint8_t    *end;
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

    if (cmd_length >= 1200)
    {
        /*dlms_rx_buf clean*/
        memset(dlms_rx_buf, 0x0, dlms_rx_length);
        dlms_rx_length = 0;
    }
    else
    {
        if (dlms_rx_length == cmd_length)
        {
            log_info_hexdump("Meter data", dlms_rx_buf, dlms_rx_length);
            udf_Meter_Process(dlms_rx_buf, dlms_rx_length);
            memset(dlms_rx_buf, 0x0, dlms_rx_length);
            dlms_rx_length = 0;
        }
        else if (dlms_rx_length > cmd_length)
        {
            log_info_hexdump("ERROR Command", dlms_rx_buf, dlms_rx_length);
            /*dlms_rx_buf clean*/
            memset(dlms_rx_buf, 0x0, dlms_rx_length);
            dlms_rx_length = 0;
        }
    }
}


static void _cli_cmd_get_flash(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    flash_Information();
}
static void _cli_cmd_setflash(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    //// unsigned char obis_data[19] = { 0x07, 0x01, 0x00, 0x63, 0x01, 0x00, 0xFF, 0x02, 0x02, 0x06, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x00, 0x00, 0x14};
    ////        MAA_flag = STATUS_REGISTRATION_COMPLETE;
    ////     On_Demand_ID = 1;
    ////     target_pos = 0;
    ////     On_Demand_Reading_Type = 0;
    ////     AC_Command(obis_data);
    //  if(CONTINUE_BUSY== 0)
    //   CONTINUE_BUSY = 1;
    //  else
    //      CONTINUE_BUSY = 0;
    ////    register_command();
    //    //  //continuetest();
    static uint8_t read_buf[0x100];
    int i;
    uint8_t factory_id_null[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
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
static void _cli_cmd_maa(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    log_info("MAA_flag 0  --->  STATUS_NOT_OBTAINED_KEY");
    log_info("Key not yet obtained\n");
    log_info("MAA_flag 1  --->  STATUS_OBTAINED_KEY");
    log_info("Key obtained but ACK not yet sent to the system\n");
    log_info("MAA_flag 2  --->  STATUS_SENT_ACK");
    log_info("ACK for key receipt sent to the system, retrieving power-on data status\n");
    log_info("MAA_flag 3  --->  STATUS_REGISTRATION_COMPLETE");
    log_info("Registration Complete\n");
}
static void _cli_cmd_RESTART(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    NVIC_SystemReset();
}
static void _cli_cmd_log_level(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    if (LOGLEVEL2 == LOGLEVEL2CLOSE)
    {
        log_info("log level 1");
    }
    else
    {
        log_info("log level 2");
    }
}
static void _cli_cmd_log_level1(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    LOGLEVEL2 = LOGLEVEL2CLOSE;
    log_info("disable encryption and decryption, leaving only plaintext");
}
static void _cli_cmd_log_level2(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    LOGLEVEL2 = LOGLEVEL2OPEN;
    log_info("display all encrypted and decrypted content");
}
const sh_cmd_t g_cli_cmd_setflash STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "flash_set",
    .pDescription = "set_flash",
    .cmd_exec     = _cli_cmd_setflash,
};
const sh_cmd_t g_cli_cmd_log_level1 STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "log_level_1",
    .pDescription = "disable encryption and decryption, leaving only plaintext",
    .cmd_exec     = _cli_cmd_log_level1,
};
const sh_cmd_t g_cli_cmd_log_level STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "loglevel?",
    .pDescription = "Check the current log level",
    .cmd_exec     = _cli_cmd_log_level,
};
const sh_cmd_t g_cli_cmd_log_level2 STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "log_level_2",
    .pDescription = "display all encrypted and decrypted content",
    .cmd_exec     = _cli_cmd_log_level2,
};
const sh_cmd_t g_cli_cmd_maa STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "MAA_flag?",
    .pDescription = "MAA flag status",
    .cmd_exec     = _cli_cmd_maa,
};
const sh_cmd_t g_cli_cmd_factoryid STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "factoryid",
    .pDescription = "factory id setting",
    .cmd_exec     = _cli_cmd_factory_id_set,

    //      .pCmd_name    = "flag status",
    //    .pDescription = "get_fan_status",
    //    .cmd_exec     = _cli_cmd_get_fan_status,
};
const sh_cmd_t g_cli_cmd_get_fanstatus STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "flagstatus",
    .pDescription = "flagstatus represents the status of all flags",
    .cmd_exec     = _cli_cmd_get_fan_status,
};
const sh_cmd_t g_cli_cmd_get_queue STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "flash_get",
    .pDescription = "Flash configuration status",
    .cmd_exec     = _cli_cmd_get_flash,
};
const sh_cmd_t g_cli_cmd_RESTART STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "FAN_SYS_RESTART",
    .pDescription = "Restart module",
    .cmd_exec     = _cli_cmd_RESTART,
};
static void udf_Action_Transmit(unsigned char obisTXT[], unsigned char plainTXT[], uint16_t plainTXT_Len, uint32_t frameNum, uint8_t lastNum)
{
    unsigned char cmdTXT[779] =
    {
        0x7E, 0xA0, 0x2C, 0x03, 0x23, 0x13, 0x6C, 0x8D, 0xE6, 0xE6,
        0x00, 0xD3, 0x1E, 0x30, 0x00, 0x00, 0x00, 0x01, 0xC3, 0x01,
        0x49, 0x00, 0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //50
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //100
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //150
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //200
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //250
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //300
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //350
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //400
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //450
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //500
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //550
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //600
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //650
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //700
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, //750
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x7E
    }; //779B
    uint8_t key_update_obis[9] = {0x40, 0x00, 0x00, 0x2B, 0x00, 0x01, 0xFF, 0x02, 0x01};
    uint8_t shift = 0, shiftTrans = 0, shiftOBIS = 0, check_key_update_obis = 0;
    uint16_t i = 0, frameLen = 0, cipherLen = 0, plainLen = 0;
    unsigned short crc16;
    unsigned char ch;
    unsigned char cipher[744], tag[16];
    //int riva_cnt = 0x40;
    printf("in Action Transmit\n\r");

    plainLen = plainTXT_Len;
    cipherLen = plainLen + 21; //5 + 4 + 12
    frameLen = cipherLen + 14; //12 + 2


    //Total frame length modification
    if (frameLen >= 256)
    {
        if (cipherLen >= 256) //encrypt plain text length modification
        {
            shift = 2;
            cmdTXT[12] = 0x82; //extra
            cmdTXT[15] = 0x30;
        }
        else if (cipherLen >= 128)
        {
            shift = 1;
            cmdTXT[12] = 0x81; //extra
        }
    }
    else if (frameLen >= 128)
    {
        if (cipherLen >= 128)
        {
            shift = 1;
            cmdTXT[12] = 0x81; //extra
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
    cmdTXT[18 + shift] = 0xC3;
    cmdTXT[19 + shift] = 01;
    cmdTXT[20 + shift] = ((ic_count - 3) % 0x40) + 0x40;; //40¶}©l
    cmdTXT[21 + shift] = (lastNum > 0) ? 01 : 00;

    for (i = 0; i < 9; i++)
    {
        cmdTXT[22 + shift + i] = obisTXT[i]; //OBIS insertion
        if (key_update_obis[i] == obisTXT[i])
        {
            check_key_update_obis ++;
        }
    }
    shiftOBIS = 9;
    if (check_key_update_obis == 9)
    {
        cmdTXT[11] = 0xCB;
    }

    //Continuing transmit set up
    if (frameNum != 0)
    {
        if (frameNum == 1)
        {
            cmdTXT[19 + shift] = 0x02; //transmit command
            cmdTXT[31 + shift] = 0x00; //extra "1"
            shiftOBIS++;
        }
        else
        {
            cmdTXT[19 + shift] = 0x03; //transmit command
            shiftOBIS -= 9;
        }
        cmdTXT[22 + shift + shiftOBIS] = (frameNum >> 24) & 0xFF; //extra
        cmdTXT[23 + shift + shiftOBIS] = (frameNum >> 16) & 0xFF; //extra
        cmdTXT[24 + shift + shiftOBIS] = (frameNum >> 8) & 0xFF; //extra
        cmdTXT[25 + shift + shiftOBIS] = frameNum & 0xFF; //extra
        if (plainLen >= 256)
        {
            cmdTXT[26 + shift + shiftOBIS] = 0x82; //extra
            cmdTXT[27 + shift + shiftOBIS] = (plainLen >> 8) & 0xFF; //extra
            cmdTXT[28 + shift + shiftOBIS] = plainLen & 0xFF; //extra
            shiftTrans = 7; //4 + 3
        }
        else if (plainLen >= 128)
        {
            cmdTXT[26 + shift + shiftOBIS] = 0x81;  //extra
            cmdTXT[27 + shift + shiftOBIS] = plainLen & 0xFF; //extra
            shiftTrans = 6; //4 + 2
        }
        else
        {
            cmdTXT[26 + shift + shiftOBIS] = plainLen & 0xFF; //extra
            shiftTrans = 5; //4 + 1
        }
    }

    //Length adjustment
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
        cmdTXT[13] = ((cipherLen) >> 8) & 0xFF; //extra
        cmdTXT[14] = (cipherLen) & 0xFF;
        break;
    default:
        break;
    }


    //Byte1~5 CRC
    crc16 = 0xFFFF;
    for (i = 1; i < 6; i++)
    {
        ch = cmdTXT[i];
        crc16 = UpdateCRC16(crc16, ch);
    }

    //Byte1~5 CRC   insertion
    crc16 = ~crc16;
    cmdTXT[6] = crc16 & 0xFF;
    cmdTXT[7] = (crc16 >> 8) & 0xFF;

    //plainTXT insertion
    for (i = 0; i < plainLen; i++)
    {
        cmdTXT[22 + shift + shiftTrans + shiftOBIS + i] = plainTXT[i];
    }

    //Data encrypt
    for (i = 0; i < 4; i++)
    {
        tpc_c_iv[8 + i] = cmdTXT[14 + shift + i]; //¥Îcount±a¤J¨ìiv¤º
    }
    if (check_key_update_obis == 9)
    {
        udf_AES_GCM_Encrypt(tpc_guk, tpc_gcm_ak, tpc_c_iv, &cmdTXT[18 + shift], &cipher[0], &tag[0], plainLen + shiftTrans + shiftOBIS + 4, 17);
    }
    else
    {
        udf_AES_GCM_Encrypt(tpc_dk, tpc_gcm_ak, tpc_c_iv, &cmdTXT[18 + shift], &cipher[0], &tag[0], plainLen + shiftTrans + shiftOBIS + 4, 17);    //©ñ¤Jcmdbuf,²£¥Í§¹¦¨«á¥Ñcipher±µ¦¬
    }

    //Cipher insertion
    for (i = 0; i < 4 + shiftOBIS + shiftTrans + plainLen; i++)
    {
        cmdTXT[18 + shift + i] = cipher[i];
    }

    //Tag insertion
    for (i = 0; i < 12; i++)
    {
        cmdTXT[22 + shift + shiftTrans + shiftOBIS + plainLen + i] = tag[i];
    }

    //Total CRC
    crc16 = 0xFFFF;
    for (i = 1; i <= frameLen - 2; i++)
    {
        ch = cmdTXT[i];
        crc16 = UpdateCRC16(crc16, ch);
    }

    //Total CRC insertion
    crc16 = ~crc16;
    cmdTXT[frameLen - 1] = crc16 & 0xFF;
    cmdTXT[frameLen] = (crc16 >> 8) & 0xFF;
    cmdTXT[frameLen + 1] = 0x7E;  //end flag

    /*for (i = 0; i < frameLen + 2 ; i++)
    {
      printf("%02X ", cmdTXT[i]);
    }
    printf("\n\r");*/
    for (i = 0; i < frameLen + 2; i++)
    {
        printf("%02X ", cmdTXT[i]);
    }
    printf("\n\r");
    app_uart_data_send(2, cmdTXT, frameLen + 2);


}

static void DASprocessCommand(uint8_t *data, uint16_t data_lens)
{
    uint8_t checkSum = 0, actionkey_resend_FLAG = 0, ipv6check = 0, plaintext_location = 0;
    uint16_t i = 0, dataLen = 0, j = 0, command_datalocation = 0; //, k = 0
    unsigned char obis[8] = {0}, timeobis[9] = {8, 0, 0, 1, 0, 0, 255, 2, 0};
    //IWDG_ReloadCounter();
    char *recvbuf = NULL;
    recvbuf = mem_malloc(1500 * sizeof(char));
    if (NULL == recvbuf)
    {
        return;
    }
    memset(recvbuf, 0x0, (1500 * sizeof(char)));
    memcpy(recvbuf, (char *)data, data_lens);

    if (256 * recvbuf[1] + recvbuf[2] + 4 <= 1500)
    {
        dataLen = 256 * recvbuf[1] + recvbuf[2] + 4;
        if (target_pos == 2 || target_pos == 3)
        {
            dataLen = 256 * recvbuf[8] + recvbuf[9] + 4 + 8;
        }
        dataLen = (dataLen > 0x8000) ? dataLen - 0x8000 : dataLen ; //bob
    }
    log_info("dataLen = %d / 1500", dataLen);
    //    log_info_hexdump("DASprocessCommand", recvbuf, dataLen);

    log_info("================================ Receive Queue Command ================================");
    for (i = recvbuf[11] + 12; i < (data_lens + 7) - 12/*tag*/; i++ )
    {
        printf("%02X ", recvbuf[i]);

    }
    printf("\n");
    log_info("=======================================================================================");


    if (recvbuf[recvbuf[11] + 13] * 256 + recvbuf[recvbuf[11] + 14] < 65535)
    {
        TASK_ID = recvbuf[recvbuf[11] + 13] * 256 + recvbuf[recvbuf[11] + 14];
    }
    //printf("TASK_ID = %d ", TASK_ID);
    if (recvbuf[recvbuf[11] + 12] == 0xA1)
    {
        HESKEY = 1;
        if (A2_TIMESynchronize == 0 && MAA_flag == STATUS_NOT_OBTAINED_KEY)
        {
            plaintext_location = recvbuf[11] + 15;
            sst_flag = 1;//start sst
            RE_REG = 0;//RE_REGISTER_END
            begin.tm_year = (recvbuf[plaintext_location] * 256 + recvbuf[plaintext_location + 1] - 1900); /*= {recvbuf[19],recvbuf[18],recvbuf[17],recvbuf[14],recvbuf[14], recvbuf[12]};(recvbuf[12]*256 + recvbuf[13] -1900)};//12~19*/
            begin.tm_mon = recvbuf[plaintext_location + 2] - 1;
            begin.tm_mday = recvbuf[plaintext_location + 3];
            begin.tm_hour = recvbuf[plaintext_location + 5];
            begin.tm_min = recvbuf[plaintext_location + 6];
            begin.tm_sec = recvbuf[plaintext_location + 7];
            for (i = 0 ; i < 16 ; i++) //update key
            {
                tpc_guk[i] = recvbuf[plaintext_location + 8 + i];
                tpc_gcm_ak[i + 1] = recvbuf[plaintext_location + 24 + i];
                tpc_gmac_ak[i + 1] = recvbuf[plaintext_location + 24 + i];
                tpc_mkey[i] = recvbuf[plaintext_location + 40 + i];
            }
            now_time = mktime(&begin);
            RTC_CNT = 0;
            AAcheck = 1;//start AA
            flag_MAA = 0;
            MAAFIRST = 1;
        }
        else if (KeyErrorflag == 1 && A2_TIMESynchronize == 1 && MAA_flag == STATUS_NOT_OBTAINED_KEY) //­Y¬Okey¿ù»~ «h¯à¦A®³key
        {
            plaintext_location = recvbuf[11] + 15;
            sst_flag = 1;//start sst
            RE_REG = 0;//RE_REGISTER_END
            begin.tm_year = (recvbuf[plaintext_location] * 256 + recvbuf[plaintext_location + 1] - 1900); /*= {recvbuf[19],recvbuf[18],recvbuf[17],recvbuf[14],recvbuf[14], recvbuf[12]};(recvbuf[12]*256 + recvbuf[13] -1900)};//12~19*/
            begin.tm_mon = recvbuf[plaintext_location + 2] - 1;
            begin.tm_mday = recvbuf[plaintext_location + 3];
            begin.tm_hour = recvbuf[plaintext_location + 5];
            begin.tm_min = recvbuf[plaintext_location + 6];
            begin.tm_sec = recvbuf[plaintext_location + 7];
            for (i = 0 ; i < 16 ; i++) //update key
            {
                tpc_guk[i] = recvbuf[plaintext_location + 8 + i];
                tpc_gcm_ak[i + 1] = recvbuf[plaintext_location + 24 + i];
                tpc_gmac_ak[i + 1] = recvbuf[plaintext_location + 24 + i];
                tpc_mkey[i] = recvbuf[plaintext_location + 40 + i];
            }
            now_time = mktime(&begin);
            RTC_CNT = 0;
            AAcheck = 1;//start AA
            flag_MAA = 0;
            MAAFIRST = 1;
            registerRetrytime = 60;//
        }
        else
        {
            if (MAA_flag == STATUS_REGISTRATION_COMPLETE)
            {
                MAA_flag = STATUS_REGISTRATION_COMPLETE;
            }
            if (REGISTER_FLAG)
            {
                REGISTER_FLAG = 0;
            }
        }
        A2_TIMESynchronize = 1;

    }

    else if (recvbuf[recvbuf[11] + 12] == 0xC1) //0614
    {

        Set_Command_Select(recvbuf[recvbuf[11] + 12], &recvbuf[8]/*RF_len in*/);
    }
    else if (recvbuf[recvbuf[11] + 12] == 0xC2) //0615
    {
        printf("set\n");
        Set_Command_Select(recvbuf[recvbuf[11] + 12], &recvbuf[8]/*RF_len in*/);
    }
    else if (recvbuf[recvbuf[11] + 12] == 0xC3) //0626
    {
        Set_Command_Select(recvbuf[recvbuf[11] + 12], &recvbuf[8]);
    }
    else if (recvbuf[recvbuf[11] + 12] == 0xC4) //0626
    {
        Set_Command_Select(recvbuf[recvbuf[11] + 12], &recvbuf[8]);
    }

    else if (recvbuf[recvbuf[11] + 12] == 0x96) //Reset
    {
        printf("reset ok!\n\r");
        //Reset_FAN();
    }
    else if (recvbuf[recvbuf[11] + 12] == 0xA4)
    {
        //setContinue = ((ELSData[9]>>7) ==1) ? 1:0;
        //setLast = (setContinue == 1) ? 1 :0;
        dataLen = (recvbuf[9] << 1) * 128 + recvbuf[10];
        for (i = 0; i < 9; i++)
        {
            obis[i] = recvbuf[i + 13];
        }

        udf_Set_Transmit(obis, (unsigned char *)&recvbuf[22], dataLen - 11, 0, 0);
    }
    else if (recvbuf[recvbuf[11] + 12] == 0xA3) //On Demand Reading
    {
        //On_Demand_Reading_Type = 1;
        //Tesk_ID = ELSData[12];
        //On_Demand_ID = 1;
        Task_count = recvbuf[recvbuf[11] + 15];
        Task_count_all = Task_count;
        ON_DEMAND_AA_FLAG = 1;
        printf("On_Demand_Task_Command = ");
        for (i = recvbuf[11] + 16; i < recvbuf[11] + 16 + Task_count * 9; i++)
        {
            On_Demand_Task_Command[i - recvbuf[11] - 16] = recvbuf[i];
            printf("%02X ", On_Demand_Task_Command[i - recvbuf[11] - 16]);
        }
        printf("\n");
        udf_Send_DISC(2);
    }
    else if (recvbuf[recvbuf[11] + 12] == 0x09) //ACTION,20191104
    {
        printf("ACTION\n\r");
        udf_Action_Transmit((unsigned char *)&recvbuf[recvbuf[11] + 15], (unsigned char *)&recvbuf[recvbuf[11] + 24], dataLen - 12 - recvbuf[11] - 14 - 4 - 9, 0, 0);
    }
    else if (recvbuf[recvbuf[11] + 12] == 0xAC && MAA_flag == STATUS_REGISTRATION_COMPLETE) //
    {
        AC_Command(&recvbuf[recvbuf[11] + 15]);
    }
    else if (recvbuf[recvbuf[11] + 12] == 0x93) //³]©wFirewall¥\¯à && ¬y¶q­­¨î
    {
        //firewall³]©w­È
        FirewallSetData[0] = recvbuf[12];
        FirewallSetData[1] = recvbuf[13];


        if (recvbuf[12] == FirewallSetData[0] && recvbuf[13] ==  FirewallSetData[1])
        {
            CheckFirewall = 0;
        }

        //¬y¶q­­¨î³]©w­È
        FlowSetData[0] = recvbuf[14];
        FlowSetData[1] = recvbuf[15];

        //Change A,B type
        Initial_Value[6] = recvbuf[16];
        LoadprofileAB = Initial_Value[6];

        //write flash
        WriteFlashData(ConfigurationPage, Initial_Value, 256);

        //Firewall_Set_Check();
    }
    else if (recvbuf[recvbuf[11] + 12] == 0x92) //FAN LOG
    {
        //                      RF_CommandLog(0xff);
    }
    else if (recvbuf[recvbuf[11] + 12] == 0x91) //FAN¦Û§ÚÀË´ú
    {
        //FAN_Check();
    }
    else if (recvbuf[recvbuf[11] + 12] == 0x94) //Ping FAN
    {
        udf_Rafael_data(1, 0x24, 0x00, 3, 1);
    }
    else if (recvbuf[recvbuf[11] + 12] == 0x95) //Ping meter
    {
        FLAG_PING_METER = 1;
        PingMeterSec = RTC_CNT;
        udf_Send_SNRM(1);

    }
    else if (recvbuf[recvbuf[11] + 12] == 0xAA)
    {
        ACTION_CHANGE_KEY = 1;
        for (i = 0; i < recvbuf[recvbuf[11] + 12 + 13]; i++) // total no. key
        {
            if (recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x00 || recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x02 || recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x62 || recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x63 || recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x64)
            {
                for (j = 0 ; j < 24 ; j++)
                {
                    kek_chipher[j] = recvbuf[recvbuf[11] + 12 + 17 + 3 + j + i * 30];
                }
                //                              udf_AES_KEK_Decrypt();

                if (recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x00) //FAN_guk
                {
                    printf("tpc_guk_update\n\r");
                    //                                  for(k=0 ;k<16 ;k++)
                    //                                      new_tpc_guk[k] = kek_plaint[k];
                    if (TPC_GUK_FLAG == 0)
                    {
                        TPC_GUK_FLAG = 1;
                    }
                    else
                    {
                        actionkey_resend_FLAG = 1;
                    }
                }
                else if (recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x02) //FAN_ak
                {
                    printf("tpc_ak_update\n\r");
                    //                                  for(k=0 ;k<16 ;k++)
                    //                                  {
                    //                                      new_tpc_gcm_ak[k+1] = kek_plaint[k];
                    //                                      new_tpc_gmac_ak[k+1] = kek_plaint[k];
                    //                                  }
                    if (TPC_AK_FLAG == 0)
                    {
                        TPC_AK_FLAG = 1;
                    }
                    else
                    {
                        actionkey_resend_FLAG = 1;
                    }
                }
                else if (recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x062) //HAN_guk
                {
                    printf("HAN_guk_update\n\r");

                }
                else if (recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x063) //HAN_ak
                {
                    printf("HAN_ak_update\n\r");

                }
                else if (recvbuf[recvbuf[11] + 12 + 17 + i * 30] == 0x064) //MK
                {
                    printf("tpc_MK_update\n\r");
                    //                                  for(k=0 ;k<16 ;k++)
                    //                                      new_tpc_mkey[k] = kek_plaint[k];
                    if (TPC_MK_FLAG == 0)
                    {
                        TPC_MK_FLAG = 1;
                    }
                    else
                    {
                        actionkey_resend_FLAG = 1;
                    }
                }

            }
        }
        if (actionkey_resend_FLAG == 1)
        {
            printf("Resend_actionkey_response\n\r");
            //Key_Restart = 1;
            KeyResCount = 0;
            udf_Rafael_data(TASK_ID, 0x1A, (char *)&actionkey_response[0], 3, KEYresponse_len);
            //SQBrespone(0x1A, &actionkey_response[0]);
        }
        else
        {
            Key_Restart = 1;
            KeyResCount = 0;
            printf("len = %d \n", (recvbuf[8] * 256) + recvbuf[9] - 18 - recvbuf[11]);
            udf_Action_Transmit((unsigned char *)&recvbuf[recvbuf[11] + 15], (unsigned char *)&recvbuf[recvbuf[11] + 24], (recvbuf[8] * 256) + recvbuf[9] - 27 - recvbuf[11], 0, 0);
        }
    }






    if (recvbuf)
    {
        mem_free(recvbuf);
    }
}

void ReceiveCommand_Queue_processing(void)
{
    _ReceiveCommand_data_t ReceiveCommand_data;
    if ( receivebusyflag == 0 && CONTINUE_BUSY == 0 && On_Demand_Reading_Type == 0)
    {
        printf("\nReceive Queue : %d \r\n", receiveIndex);
        if (xQueueReceive(ReceiveCommand_Queue_handle, (void *)&ReceiveCommand_data, 0) == pdPASS)
        {
            DASprocessCommand(ReceiveCommand_data.pdata, ReceiveCommand_data.dlen);
            if (receiveIndex > 0)
            {
                receiveIndex--;
            }
            // 更新發送佇列的字節計數
            if (currentQueueBytes >= ReceiveCommand_data.dlen)
            {
                currentQueueBytes -= ReceiveCommand_data.dlen;
            }
            if (ReceiveCommand_data.pdata)
            {
                mem_free(ReceiveCommand_data.pdata);
            }
        }
    }
}
void Rafael_printFunction(uint8_t *payload, uint16_t meter_len)
{
    int i = 0;
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    // 計算 meter_data 在 payload 中的起始位置
    uint8_t *meter_data_in_payload = payload + 1 + strlen(string) + 1 + 2;

    // 打印 meter_data_in_payload 的內容
    log_info("\n=============================== Data Sent to Leader =====================================");
    printf("66 64 30 ");
    for (i = 0; i < meter_len - 4; i++)
    {
        printf("%02X ", meter_data_in_payload[i]);
    }
    printf("\n");
    log_info("===========================================================================================");

    TASKCOMMAND = TASK_FINISHED_FLAG;




}
void Send_Queue_processing(void)
{
    _SendCommand_data_t SendCommand_data;

    if (sendIndex > 0 && sendbusyflag == 0 && CONTINUE_BUSY == 0 && On_Demand_Reading_Type == 0 && (target_pos == 3 || target_pos == 2))
    {
        while (xQueuePeek(SendCommand_Queue_handle, (void *)&SendCommand_data, 0) == pdTRUE)
        {
            if (xQueueReceive(SendCommand_Queue_handle, (void *)&SendCommand_data, 0) == pdPASS)
            {
                log_info("\nSend Queue : %d ", sendIndex);
                Rafael_printFunction(SendCommand_data.pdata, SendCommand_data.dlen);
                if (app_udpSend(SendCommand_data.PeerPort, SendCommand_data.PeerAddr, SendCommand_data.pdata, SendCommand_data.dlen))
                {
                    printf("app_udpSend error\r\n");
                }
                if (sendIndex > 0)
                {
                    sendIndex --;
                }
                // 更新發送佇列的字節計數
                if (currentQueueBytes >= SendCommand_data.dlen)
                {
                    currentQueueBytes -= SendCommand_data.dlen;
                }
                if (SendCommand_data.pdata)
                {
                    mem_free(SendCommand_data.pdata);
                }
            }
        }
    }
    else
    {
        if (sendIndex > 0)
        {
            log_info("sendIndex = %d  sendbusyflag = %d  CONTINUE_BUSY = %d  On_Demand_Reading_Type = %d target_pos = %d", sendIndex, sendbusyflag, CONTINUE_BUSY, On_Demand_Reading_Type, target_pos);
            printf("Command Flag Error\r\n");
        }
    }
}

void AAFIRST(void)
{
    int i = 0;

    if (MAA_flag == STATUS_NOT_OBTAINED_KEY)
    {
        for (i = 0; i < 16; i++)
        {
            tpc_guk[i] = Configuration_Page[i + 48];
            tpc_gcm_ak[i + 1] = Configuration_Page[i + 64];
            tpc_gmac_ak[i + 1] = Configuration_Page[i + 64];
            tpc_mkey[i] = Configuration_Page[i + 80];
        }
        MAAFIRST = 1;
        Auto_MAA();


    }
}


static void Retry_Register_task(void)
{
    if (MAA_flag)
    {
        Register_Timeout = 0;
    }
    else
    {
        register_command();
        Register_Timeout = (registerRetrytime * 60);
    }
}
void Rafael_Register(void)
{

    int i = 0;
    unsigned char obis_notification_data[] = { 0x07, 0x00, 0x00, 0x63, 0x62, 0x00, 0xFF, 0x02};
    char ack_buff[1] = {0x00};

    //    if ((MAA_flag == STATUS_NOT_OBTAINED_KEY && register_steps == 0) || (MAA_flag == STATUS_OBTAINED_KEY && register_steps == 2 && ipv6_first == 0))
    //    {
    //        log_info("MAA_flag = %d ", MAA_flag);
    //        log_info("register_steps = %d ", register_steps);

    //        fan_number();
    //        ipv6_first = 1;
    //    }
    if (meterBootStep == Preliminary_Work_Completed && (target_pos == 2 || target_pos == 3) && register_steps == 0 && MAA_flag == STATUS_NOT_OBTAINED_KEY && SuccessRole == 1)
    {
        register_command();
        register_steps = 2;
    }

    if (sst_flag == 1 && MAA_flag == STATUS_OBTAINED_KEY && register_steps == 2 && SuccessRole == 1)
    {

        printf("ack Corrent\n\r");//®³¨ìkey«ásend ack
        ACK_function(0);
        RE_REG = 1;
        sst_flag = 0;
        MAA_flag = STATUS_SENT_ACK;
        if (target_pos == 2 || target_pos == 3)
        {
            ack_flag = 1;    // green light on flag
        }
    }
    ///////////////////////////////////////////////////////////////
    if ((MAA_flag == STATUS_SENT_ACK && FIRST_POWER_ON_FLAG == 1 ) || (EVENT_LOG_POWER_ON == 1 && NOT_COMPLETELY_POWER_OFF == 1 ))
    {
        if (target_pos == 2 || target_pos == 3 )
        {
            FIRST_POWER_ON_FLAG = 0;
            EVENT_LOG_POWER_ON = 0;
            //log_info("NOT_COMPLETELY_POWER_OFF = %d , FIRST_NOT_COMPLETELY_POWER_ON_FLAG = %d\n", NOT_COMPLETELY_POWER_OFF, FIRST_NOT_COMPLETELY_POWER_ON_FLAG);
            if (NOT_COMPLETELY_POWER_OFF == 1 && FIRST_NOT_COMPLETELY_POWER_ON_FLAG == 1)
            {
                //                printf("NVIC_SystemReset:1\n\r");
                //                NVIC_SystemReset();
            }
            else
            {
                udf_Get_New_Number_of_pens(ic_count, &obis_notification_data[0], 1);
            }
        }
    }
    if (AAcheck == 1 && flag_MAA == 0)
    {
        Auto_MAA();
    }


    if (MAA_flag >= STATUS_SENT_ACK && (target_pos == 2 || target_pos == 3) && ack_flag == 0 && SuccessRole == 1)
    {
        log_info( "ack flag = %d", ack_flag);
        ack_flag = 1; // green light on flag
    }

    //}
}
void vTimerCallback(TimerHandle_t xTimer)
{
    unsigned char obis_notification_data[] = { 0x07, 0x00, 0x00, 0x63, 0x62, 0x00, 0xFF, 0x02};
    uint8_t RetryRegisterCommand = 0, RegisterFlag = 1, BroadcastFlag = 0, Continue_Flag = 0, ResetFAN_Flag = 0;
    RTC_CNT++;
    now_timer = now_time + RTC_CNT;
    if (meterBootStep != Preliminary_Work_Completed)
    {
        meterBootStepCount++;
        if (meterBootStepCount > 10)
        {
            log_info("meterBootStep fails to reach the 'Preliminary_Work_Completed' status within ten seconds ");
            NVIC_SystemReset();
        }
    }
    if (SuccessRole)
    {
        Rafael_Register();
    }
    if (MAA_flag == STATUS_NOT_OBTAINED_KEY && register_steps == 2)
    {
        if (Register_Timeout > 0 && --Register_Timeout == 0)
        {
            APP_EVENT_NOTIFY(EVENT_REGISTER);
        }
    }
    //rececive
    HandleBusyFlagAndCount(&receivebusyflag, &receivecount, receivebusytime * 10, 1); //*(10sec)

    //send
    HandleBusyFlagAndCount(&sendbusyflag, &sendcount, sendbusytime * 10, 2); //*(10sec)

    //On_Demand_Reading_Type
    HandleBusyFlagAndCount(&On_Demand_Reading_Type, &OnDemandReadingcount, ondemandreadingtime * 10, 3); //*(10sec)

    //CONTINUE_BUSY
    Continue_Flag = HandleBusyFlagAndCount(&CONTINUE_BUSY, &CointbusyCount, continuebusytime * 10, 4); //*(10sec)
    //    if ((target_pos == 2 || target_pos  == 3) && CONTINUE_BUSY == 1 && STEP_CONTINUE_FLAG == 0 && flag_Power_Off == 0 )
    //      {
    //              STEP_CONTINUE_FLAG = 1;
    //              vTaskDelay(100);
    //        udf_Get_continue();
    //    }
    if (Continue_Flag)
    {
        Continue_function_timeout_handlr();
    }
    //ResetRF_Flag
    ResetFAN_Flag = HandleBusyFlagAndCount(&ResetRF_Flag, &ResetRFcount, 10, 5); //*(10sec)

    if ((target_pos != 4 && target_pos != 2 && target_pos != 3) )
    {
        RafaelRoleCount ++;
        if (RafaelRoleCount >= (Rafael_reset * 60.0))
        {
            log_info("Rafael did not obtain the role within the specified time.");
            NVIC_SystemReset();
        }
    }
    else
    {
        RafaelRoleCount = 0;
    }
    if (ResetFAN_Flag)
    {
        log_info("ResetFAN_Flag == 1 Restart.");
        NVIC_SystemReset();
    }
    if (MAA_flag == STATUS_OBTAINED_KEY || MAA_flag == STATUS_SENT_ACK) //300 sec
    {
        MAANO3count ++;
        if (MAANO3count >= (keyObtainedStatusIncorrecttimeout * 60))
        {
            sst_flag = 0;
            MAA_flag = STATUS_REGISTRATION_COMPLETE;
            MAANO3count = 0;
        }
    }
    Light_controller_function();

    //BroadcastsendFuntion
    BroadcastFlag = HandleBusyFlagAndCount(&Broadcast_flag, &BroadcastCount, Broadcast_meterdelay, 6);

    if (BroadcastFlag)
    {
        Broadcast_function_timeout_handlr();
    }
    if (flag_Power_Off && LastpSendFlag == 0)
    {
        if (LastpSendCount == 0)
        {
            log_info("flag_Power_Off = %d LastpSendFlag = %d LastpCount = %d", flag_Power_Off, LastpSendFlag, LastpSendCount);
            printf("GPIO29 Interrupt Triggered\r\nPower OFF\r\n");
        }
        LastpSendCount ++;
        gpio_pin_write(14, 0);
        gpio_pin_write(15, 0);
        Power_Off_Function();
    }
    else if (flag_Power_Off == 0 && NOT_COMPLETELY_POWER_OFF && GetPowerOnFlag == 0)
    {
        if (PowerOnCount == 0)
        {
            printf("GPIO29 Interrupt Triggered\r\nPower ON\r\n");
        }
        PowerOnCount++;
        gpio_pin_write(14, 1);
        gpio_pin_write(15, 1);
        if (PowerOnCount > 10)
        {
            printf("After a power outage, and before the MCU has restarted, upon power restoration, start querying for power restoration messages.\r\n");
            GetPowerOnFlag = 1;
            udf_Send_DISC(2);
            PowerOnCount = 0;
        }
    }

    if (flag_Power_Off)
    {

        PowerResetAnomalyCount++;
        if (PowerResetAnomalyCount >= 300)
        {
            printf("Rapid power cycling causing abnormal state\r\n");
            gpio_pin_write(14, 1);
            gpio_pin_write(15, 1);
            flag_Power_Off = 0;
            LastpSendFlag = 0;
            LastpSendCount = 0;
            NOT_COMPLETELY_POWER_OFF = 0;
            GetPowerOnFlag = 0;
            PowerResetAnomalyCount = 0;
            PowerOnCount = 0;
        }
    }
    else if (NOT_COMPLETELY_POWER_OFF)
    {

        PowerResetAnomalyCount++;
        gpio_pin_write(14, 1);
        gpio_pin_write(15, 1);
        if (PowerResetAnomalyCount >= 300)
        {
            printf("Rapid power cycling causing abnormal state\r\n");
            gpio_pin_write(14, 1);
            gpio_pin_write(15, 1);
            LastpSendFlag = 0;
            LastpSendCount = 0;
            NOT_COMPLETELY_POWER_OFF = 0;
            GetPowerOnFlag = 0;
            PowerResetAnomalyCount = 0;
            PowerOnCount = 0;
        }
    }


    if (REGISTER_FLAG && MAA_flag == STATUS_REGISTRATION_COMPLETE)
    {
        RegisterCount++;
        if (First_COUNT == 0)
        {
            RegisterCount = 0;
            First_COUNT = 1;
            register_command();
        }
        if (RegisterCount >= 3600)
        {
            register_command();
            RegisterCount = 0;
        }
    }

    if (sendIndex > 0 && sendbusyflag == 0 && CONTINUE_BUSY == 0 && On_Demand_Reading_Type == 0 && (target_pos == 3 || target_pos == 2))
    {
        APP_EVENT_NOTIFY(EVENT_SEND_QUEUE);
    }
    else if (receiveIndex > 0  && receivebusyflag == 0 && CONTINUE_BUSY == 0 && On_Demand_Reading_Type == 0 && TASKCOMMAND == TASK_FINISHED_FLAG )
    {
        TASKCOMMAND = TASK_UNFINISHED_FLAG;
        APP_EVENT_NOTIFY(EVENT_ELS61_BLOCK_QUEUE);
    }

}

void udf_Meter_init(otInstance *instance)
{
    gpio_cfg_output(14);
    gpio_cfg_output(15);
    gpio_pin_write(14, 0); //green,close
    gpio_pin_write(15, 1); //red ,open
    Configuration_Page_init(instance);
    flash_Information();
    udf_Send_DISC(1);

    //    ELS61_Block_Queue_handle = xQueueCreate(QUEUE_SIZE, sizeof(_ELS61_Block_data_t));
    ReceiveCommand_Queue_handle = xQueueCreate(QUEUE_SIZE, sizeof(_ReceiveCommand_data_t));
    SendCommand_Queue_handle        = xQueueCreate(QUEUE_SIZE, sizeof(_SendCommand_data_t));

    das_dlms_timer = xTimerCreate("das_dlms_timer",
                                  (1000),
                                  true,
                                  ( void * ) 0,
                                  vTimerCallback);

    xTimerStart(das_dlms_timer, 0 );
    setup_gpio29();


}
void evaluate_commandAM1(char *recvbuf, uint16_t len_test)
{
    unsigned char timeobis[9] = {8, 0, 0, 1, 0, 0, 255, 2, 0}, ack_buff[1] = {0x00};
    //size_t len_test=0;
    int i = 0, j;
    uint8_t default_tag[12], ipv6check = 0;
    uint16_t ip6_len;
    uint8_t *default_pt = NULL;
    char *am1buffer = NULL;
    char flashbuffer[150] = {0};
    int8_t Rssi = 0;
    int8_t Set_address = 0;

    for (i = 1, j = 0 ; i < 1 + recvbuf[0]; i++, j++)
    {
        if (recvbuf[i] == Broadcast_ip[j])
        {
            ipv6check++;
        }
    }
    am1buffer = mem_malloc(1500 * sizeof(char));
    if (NULL == am1buffer)
    {
        return;
    }
    default_pt = mem_malloc(1500 * sizeof(uint8_t));
    if (NULL == default_pt)
    {
        return;
    }

    if (ipv6check == 7)
    {
        ipv6check = 0;
        //        log_info_hexdump("check Broadcast ipv6", recvbuf, recvbuf[0] + 1);
        //        for (i = 1, j = 0 ; i < 1 + recvbuf[0]; i++, j++)
        //        {
        //            if (recvbuf[i] == Broadcast_ip[j])
        //            {
        //                ipv6check++;
        //            }
        //        }
        memcpy(&am1buffer[11], recvbuf, len_test + 11);
        am1buffer[1] = (len_test + 11) / 256;
        am1buffer[2] = (len_test + 11) % 256;
        log_info("len  = %d", am1buffer[1] * 256 + am1buffer[2]);
        //        log_info_hexdump("Am1buffer", am1buffer, len_test + 11);
        log_info("\n===================================== Broadcast Command =====================================");
        for (i = am1buffer[11] + 12; i < len_test + 11; i++ )
        {
            printf("%02X ", am1buffer[i]);

        }
        printf("\n");
        log_info("=========================================================================================");
        if (recvbuf[am1buffer[11] + 13] * 256 + am1buffer[am1buffer[11] + 14] < 65535)
        {
            TASK_ID = am1buffer[am1buffer[11] + 13] * 256 + am1buffer[am1buffer[11] + 14];
        }
        printf("TASK_ID = %d ", TASK_ID);
        if (am1buffer[am1buffer[11] + 12] == 0x03 && MAA_flag == STATUS_REGISTRATION_COMPLETE)
        {
            Broadcast_savebuff_flag = 1;
            AC_Command(&am1buffer[am1buffer[11] + 15]);

        }
        else if (am1buffer[am1buffer[11] + 12] == 0x45 ) //Flash initialization.
        {
            printf("\n\rflash_set_test\n\r");
            WriteFlashData(ConfigurationPage, flash_test, 200);
            //NVIC_SystemReset();
        }
        else if (am1buffer[am1buffer[11] + 12] == 0x17 && MAA_flag == STATUS_REGISTRATION_COMPLETE)
        {
            Broadcast_savebuff_flag = 1;
            Rssi = app_get_parent_rssi();
            printf("Rssi = %d ", Rssi);
            udf_Rafael_data(TASK_ID, 0x17, Rssi, 3, 1);
        }
    }
    else
    {
        am1buffer[1] = (len_test + 11) / 256;
        am1buffer[2] = (len_test + 11) % 256;
        log_info("len  = %d", am1buffer[1] * 256 + am1buffer[2]);
        memcpy(&am1buffer[11], recvbuf, len_test);
        am1buffer[8] = ((len_test + 2) >> 8) & 0xff;
        am1buffer[9] = (len_test + 2) & 0xff;
        log_info("\n===================================== HES Command =====================================");
        for (i = am1buffer[11] + 12; i < len_test + 11; i++ )
        {
            printf("%02X ", am1buffer[i]);

        }
        printf("\n");
        log_info("=======================================================================================");

        if (recvbuf[am1buffer[11] + 13] * 256 + am1buffer[am1buffer[11] + 14] < 65535)
        {
            TASK_ID = am1buffer[am1buffer[11] + 13] * 256 + am1buffer[am1buffer[11] + 14];
        }
        ip6_len = am1buffer[11];
        //        log_info("TASK_ID = %d", TASK_ID);
        //        log_info("ip6_len = %d", ip6_len);
        //        log_info("typecheck = %d", am1buffer[am1buffer[11] + 12]);
        if (am1buffer[am1buffer[11] + 12] == 0x91 || am1buffer[am1buffer[11] + 12] == 0x92 || am1buffer[am1buffer[11] + 12] == 0x94 || am1buffer[am1buffer[11] + 12] == 0x95 || am1buffer[am1buffer[11] + 12] == 0x96) // not Decrypt command
        {
            receive_queue(&am1buffer[0], len_test + 4);
        }
        else if (am1buffer[am1buffer[11] + 12] == 0x17 && MAA_flag == STATUS_REGISTRATION_COMPLETE)
        {
            Rssi = app_get_parent_rssi();
            printf("Rssi = %d ", Rssi);
            udf_Rafael_data(TASK_ID, 0x17, Rssi, 3, 1);
        }

        else if (am1buffer[am1buffer[11] + 12] == 0xC0 && am1buffer[am1buffer[11] + 15] == 0x19) //set time without encrypt
        {
            Set_address = am1buffer[11] + 12;
            if (am1buffer[Set_address + 1] * 256 + am1buffer[Set_address + 2] < 65535)
            {
                TASK_ID = am1buffer[Set_address + 1] * 256 + am1buffer[Set_address + 2];
            }
            begin.tm_year = (am1buffer[Set_address + 4] * 256 + am1buffer[Set_address + 5] - 1900);
            begin.tm_mon  = am1buffer[Set_address + 6] - 1;
            begin.tm_mday = am1buffer[Set_address + 7];
            begin.tm_hour = am1buffer[Set_address + 9];
            begin.tm_min  = am1buffer[Set_address + 10];
            begin.tm_sec  = am1buffer[Set_address + 11]; //bob//0614
            RTC_CNT = 0;
            now_time = mktime(&begin);//19
            log_info("\n\rSet Time_C0");
            SET_TIME_C0_INDEX = 1;//0714
            udf_Set_Transmit(timeobis, &am1buffer[Set_address + 3], 13, 0, 0);  //0714
        }

        else if (am1buffer[am1buffer[11] + 12] == 0x03 && MAA_flag == STATUS_REGISTRATION_COMPLETE) //¸ÉÅª
        {
            printf("\n\rAC buffer:\n\r");
            for (i = 0; i < len_test + 4 + 6; i++)
            {
                printf("%02X ", am1buffer[i]);
            }
            printf("\n\r");
            AC_Command(&am1buffer[am1buffer[11] + 15]);

        }
        else if (am1buffer[am1buffer[11] + 12]  == 0xDC ) //set flash
        {
            flashsetfuntion(&am1buffer[am1buffer[11] + 15]);
        }
        else if (am1buffer[am1buffer[11] + 12]  == 0xDD ) //get flash
        {
            ReadFlashData_Normal(ConfigurationPage, Configuration_Page, 150);//0615
            flashbuffer[0] = 0x00;
            for (i = 0; i < 150; i++)
            {
                flashbuffer[i + 1] = Configuration_Page[i];
            }
            udf_Rafael_data(TASK_ID, 0xDD, &flashbuffer[0], 3, 143);

        }
        else if (am1buffer[am1buffer[11] + 12]  == 0xDF ) //Reset
        {
            CONTINUE_BUSY   =   1;
            udf_Rafael_data(TASK_ID, 0xF2, &ack_buff[0], 3, 1);
            ResetRF_Flag = 1;
        }
        else if (am1buffer[am1buffer[11] + 12] == 0x97 ) //set time without encrypt
        {
            begin.tm_year = (am1buffer[42] * 256 + am1buffer[43] - 1900);
            begin.tm_mon  = am1buffer[44] - 1;
            begin.tm_mday = am1buffer[45];
            begin.tm_hour = am1buffer[47];
            begin.tm_min  = am1buffer[48];
            begin.tm_sec  = am1buffer[49];
            RTC_CNT = 0;
            now_time = mktime(&begin);//19
            //SET_FAN_TIME();
        }
        else if (am1buffer[am1buffer[11] + 12] == 0xA8 ) //set time without encrypt
        {
            begin.tm_year = (am1buffer[43] * 256 + am1buffer[44] - 1900);
            begin.tm_mon  = am1buffer[45] - 1;
            begin.tm_mday = am1buffer[46];
            begin.tm_hour = am1buffer[48];
            begin.tm_min  = am1buffer[49];
            begin.tm_sec  = am1buffer[50];
            RTC_CNT = 0;
            now_time = mktime(&begin);//19

            udf_Set_Transmit(timeobis, &recvbuf[42], 13, 0, 0);
        }
        else
        {
            ///update command///////////////////////////////////
            for (i = 0; i < 12; i++)                                                        //
            {
                default_tag[i] = am1buffer[i + len_test - 1];    //
            }
            //log_info("\nrecvbuf[%02x] len = %d \n\r", am1buffer[11] + 15, len_test - 3 - am1buffer[11] - 13);
            udf_AES_GCM_Decrypt(system_ekey, system_add, system_ic, &am1buffer[am1buffer[11] + 15], default_tag, &default_pt[0], len_test - 3 - am1buffer[11] - 13, 17);                                      //

            for (i = 0; i < len_test - 14 - am1buffer[11]; i++)                                         // decrypt
            {
                am1buffer[am1buffer[11] + 15 + i] = default_pt[i];    //
            }
            //
            ///update over//////////////////////////////////////
            if (MAA_flag != 3 && (am1buffer[am1buffer[11] + 12] == 0xAC || am1buffer[am1buffer[11] + 12] == 0xA3))
            {
                log_info("***Initialization*** ,%02X\n ", recvbuf[11]);

            }
            else
            {
                receive_queue(&am1buffer[0], len_test + 4);
            }
        }


    }

    if (am1buffer)
    {
        mem_free(am1buffer);
    }
    if (default_pt)
    {
        mem_free(default_pt);
    }
}

void __das_dlms_task(app_task_event_t sevent)
{
    if (sevent & EVENT_METER_BOOT_STEP)
    {
        if (meterBootStep == Preliminary_Work_Completed)
        {
            AAFIRST();//register to server
        }
    }

    if (sevent & EVENT_ELS61_BLOCK_QUEUE)
    {
        ReceiveCommand_Queue_processing();
    }
    if (sevent & EVENT_SEND_QUEUE)
    {
        Send_Queue_processing();
    }
    if (sevent & EVENT_REGISTER)
    {
        Retry_Register_task();
    }
}
