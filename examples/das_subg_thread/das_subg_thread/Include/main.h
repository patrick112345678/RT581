#ifndef  __MAIN_H
#define  __MAIN_H

#include <openthread/thread.h>
#include <openthread/thread_ftd.h>
#include <openthread/icmp6.h>
#include <openthread/cli.h>
#include <openthread/ncp.h>
#include <openthread/coap.h>
#include <openthread_port.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "mem_mgmt.h"
#include <time.h>
#define NETWORKKEY_USE 0//0: NO networkkey 1:use networkkey
#define STATUS_NOT_OBTAINED_KEY 0 // Status indicating the key has not been obtained
#define STATUS_OBTAINED_KEY     1 // Status indicating the key has been obtained
#define STATUS_SENT_ACK         2 // Status indicating the ACK has been sent after obtaining the key
#define STATUS_REGISTRATION_COMPLETE 3 // Status indicating the registration process is complete

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define THREAD_CHANNEL      4
#else
#define THREAD_CHANNEL      11
#endif
#define THREAD_PANID        0xff01
#define THREAD_UDP_PORT     5678
#define THREAD_COAP_PORT    (THREAD_UDP_PORT + 2)

#define DEMO_UDP            1
#define DEMO_COAP           2

#define EVK_READ_BOARDCAST_START 1
#define EVK_READ_START      1
#define EVK_NOT_READ        0

#define APP_EVENT_NOTIFY_ISR(ebit)                 (g_app_task_evt_var |= ebit); __app_task_signal()
#define APP_EVENT_NOTIFY(ebit)                     enter_critical_section(); g_app_task_evt_var |= ebit; leave_critical_section(); __app_task_signal()
#define APP_EVENT_GET_NOTIFY(ebit)                 enter_critical_section(); ebit = g_app_task_evt_var; g_app_task_evt_var = EVENT_NONE; ; leave_critical_section()

#define OPEN 1
#define CLOSE 0
typedef enum
{
    EVENT_NONE                       = 0,

    EVENT_UART1_UART_IN                    = 0x00000002,
    EVENT_UART2_UART_IN                    = 0x00000004,
    EVENT_UART1_UART_OUT_DONE              = 0x00000008,
    EVENT_UART2_UART_OUT_DONE              = 0x00000010,

    EVENT_UDP_RECEIVED                     = 0x00000020,

    EVENT_METER_BOOT_STEP                  = 0x00000040,

    EVENT_ELS61_BLOCK_QUEUE                = 0x00000080,

    EVENT_SEND_QUEUE                       = 0x00000100,

    EVENT_REGISTER                         = 0x00000200,
    EVENT_ALL                              = 0xffffffff,
} app_task_event_t;

extern app_task_event_t g_app_task_evt_var;
void __app_task_signal(void);
void app_task (void) ;
int8_t app_get_parent_rssi();
extern uint8_t UART2DEBUG;
extern uint8_t EVK_READ_FLAG;
extern uint8_t EVK_BOARDCAST_READ_FLAG;
/*network_management*/
otError nwk_mgm_init(otInstance *aInstance);
void nwk_mgm_neighbor_Change_Callback(otNeighborTableEvent aEvent, const otNeighborTableEntryInfo *aEntryInfo);

/*das_hex_cmd*/
bool das_hex_cmd_status_check();
void das_get_meter_id_init();
void das_hex_cmd_received_task(const uint8_t *aBuf, uint16_t aBufLength);

void das_hex_command_response(bool state, uint8_t cmd_id);
void das_hex_cmd_get_meter_id();
void das_hex_cmd_command_bootup();
void das_hex_cmd_state_change(uint8_t state);
void das_hex_cmd_udp_ack();
void das_hex_cmd_get_udp_ip(uint8_t *Param, uint16_t lens);
void das_hex_cmd_get_udp_received_data(uint8_t *Param, uint16_t lens);

/*das_dlms_cmd*/
void udf_Meter_init(otInstance *instance);
void udf_Meter_received_task(const uint8_t *aBuf, uint16_t aBufLength);
void evaluate_commandAM1(char *recvbuf, uint16_t len_test);
void __das_dlms_task(app_task_event_t sevent);
extern uint8_t target_pos;
extern uint8_t SuccessRole, MAA_flag, GreenLight_Flag, flag_Power_Off;
extern time_t now_time;
extern uint32_t now_timer, RTC_CNT;
extern struct tm begin;
extern uint16_t Register_Timeout;
/*app uart*/
void app_uart_init(void);
int app_uart_data_send(uint8_t u_port, uint8_t *p_data, uint16_t data_len);
void __uart_task(app_task_event_t sevent);

/*app udp*/
uint8_t app_sockInit(otInstance *instance, void (*handler)(otMessage *, const otMessageInfo *), uint16_t udp_port);
otError app_udpSend(uint16_t PeerPort, otIp6Address PeerAddr, uint8_t *data, uint16_t data_lens);
void Rafael_printFunction(uint8_t *payload, uint16_t meter_len);
void app_udp_received_queue_push(uint8_t *data, uint16_t data_lens);
void __udp_task(app_task_event_t sevent);

#endif // __DEMO_GPIO_H
