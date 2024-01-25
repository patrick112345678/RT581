#ifndef  __MAIN_H
#define  __MAIN_H

#include <openthread/thread.h>
#include <openthread/thread_ftd.h>
#include <openthread/icmp6.h>
#include <openthread/cli.h>
#include <openthread/ncp.h>
#include <openthread/coap.h>
#include <openthread_port.h>

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define THREAD_CHANNEL      3
#else
#define THREAD_CHANNEL      11
#endif
#define THREAD_PANID        0x6767
#define THREAD_UDP_PORT     5678
#define THREAD_COAP_PORT    (THREAD_UDP_PORT + 2)

#define DEMO_UDP            1
#define DEMO_COAP           2

void app_task (void) ;

/*network_management*/
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
void udf_Meter_received_task(const uint8_t *aBuf, uint16_t aBufLength);

/*app uart*/
void app_uart_init(void);
int app_uart_data_send(uint8_t u_port, uint8_t *p_data, uint16_t data_len);

/*app udp*/
uint8_t app_sockInit(otInstance *instance, void (*handler)(otMessage *, const otMessageInfo *), uint16_t udp_port);
otError app_udpSend(uint16_t PeerPort, otIp6Address PeerAddr, uint8_t *data, uint8_t data_lens);

#endif // __DEMO_GPIO_H