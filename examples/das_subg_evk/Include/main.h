#ifndef  __MAIN_H
#define  __MAIN_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "mem_mgmt.h"
#include <time.h>

#define SUBG_CHANNEL      4
#define SUBG_PANID        0xff01

#define APP_EVENT_NOTIFY_ISR(ebit)                 (g_app_task_evt_var |= ebit); __app_task_signal()
#define APP_EVENT_NOTIFY(ebit)                     enter_critical_section(); g_app_task_evt_var |= ebit; leave_critical_section(); __app_task_signal()
#define APP_EVENT_GET_NOTIFY(ebit)                 enter_critical_section(); ebit = g_app_task_evt_var; g_app_task_evt_var = EVENT_NONE; ; leave_critical_section()

typedef enum
{
    EVENT_NONE                       = 0,

    EVENT_UART1_UART_IN                    = 0x00000002,
    EVENT_UART2_UART_IN                    = 0x00000004,
    EVENT_UART1_UART_OUT_DONE              = 0x00000008,
    EVENT_UART2_UART_OUT_DONE              = 0x00000010,

    EVENT_MAC_RX_DONE                      = 0x00000020,
    EVENT_MAC_TX_DONE                      = 0x00000040,

    EVENT_ALL                              = 0xffffffff,
} app_task_event_t;

extern app_task_event_t g_app_task_evt_var;
void __app_task_signal(void);
void app_task (void) ;

/*app uart*/
void app_uart_init(void);
int app_uart_data_send(uint8_t u_port, uint8_t *p_data, uint16_t data_len);
void __uart_task(app_task_event_t sevent);

#endif // __DEMO_GPIO_H