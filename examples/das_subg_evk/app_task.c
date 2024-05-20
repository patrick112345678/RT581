/**
 * @file app_task.c
 * @author jiemin Cao (jiemin.cao@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2024-05-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "main.h"
#include "log.h"
#include "timers.h"
#include <time.h>
#include "lmac15p4.h"
#include "subg_ctrl.h"
#include "mac_frame_gen.h"
#include "shell.h"
#include "cli.h"

#define PHY_PIB_TURNAROUND_TIMER 1000
#define PHY_PIB_CCA_DETECTED_TIME 640 // 8 symbols for 50 kbps-data rate
#define PHY_PIB_CCA_DETECT_MODE 2
#define PHY_PIB_CCA_THRESHOLD 65
#define MAC_PIB_UNIT_BACKOFF_PERIOD 1640   // PHY_PIB_TURNAROUND_TIMER + PHY_PIB_CCA_DETECTED_TIME
#define MAC_PIB_MAC_ACK_WAIT_DURATION 2000 // oqpsk neee more then 2000
#define MAC_PIB_MAC_MAX_BE 5
#define MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME 82080 // for 50 kbps-data rate
#define MAC_PIB_MAC_MAX_FRAME_RETRIES 4
#define MAC_PIB_MAC_MAX_CSMACA_BACKOFFS 4
#define MAC_PIB_MAC_MIN_BE 2

#define FREQ (920000) // 915 MHZ range is (902~928), but Taiwan range is (920~925)
#define CHANNEL_SPACING 500

#define MAC_HEADER_CHAR "rafael"
#define MAC_HEADER_CHAR_LENS 6

static SemaphoreHandle_t    appSemHandle          = NULL;
app_task_event_t g_app_task_evt_var = EVENT_NONE;

static uint16_t SPanId = SUBG_PANID;
static uint32_t SCurrentChannel = SUBG_CHANNEL;
static uint8_t SMinChannel = 1;
static uint8_t SMacAddr[8] = {0xFF, 0xFF, 0xEE, 0xEE, 0xAA, 0xBB, 0xCC, 0xDD};
static uint8_t SMacTxState = 0;
static uint8_t SMacIsTxDone = 1;

static const uint8_t channel_min = 1;
static const uint8_t channel_max = 10;

typedef struct
{
    uint16_t lens;
    uint8_t data[FSK_MAX_DATA_SIZE];
    uint8_t rssi;
    uint8_t snr;
} rx_done_pkt_t;

static rx_done_pkt_t rx_done_pkt;

static uint8_t mac_tx_dsn = 0;

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

void __mac_broadcast_send(uint8_t *data, uint16_t lens)
{
    char mac_data_header[6] = MAC_HEADER_CHAR;
    uint8_t *mac_tx_data = NULL;
    uint16_t mac_tx_lens = lens + 6;

    static MacBuffer_t MacHdrPtr;
    static MacHdr_t MacHdr;

    uint8_t tx_control;

    if (SMacIsTxDone == 1)
    {
        tx_control = 0x0;

        MacHdr.macFrmCtrl.frameType        = MAC_DATA;
        MacHdr.macFrmCtrl.secEnab          = false;
        MacHdr.macFrmCtrl.framePending     = false;
        MacHdr.macFrmCtrl.ackReq           = false;

        MacHdr.macSecCtrl.secLevel         = SEC_LEVEL_NONE;
        MacHdr.macSecCtrl.keyIdMode        = KEY_ID_MODE_IMPLICITY;
        MacHdr.macFrmCtrl.panidCompr       = true;

        MacHdr.dsn                         = mac_tx_dsn;

        MacHdr.destPanid                   = 0xFFFF;

        MacHdr.destAddr.mode               = SHORT_ADDR;
        MacHdr.destAddr.shortAddr          = 0xFFFF;
        // MacHdr.destAddr.longAddr[0]        = 0xFFFFFFFF;
        // MacHdr.destAddr.longAddr[1]        = 0xFFFFFFFF;

        MacHdr.srcPanid                    = SPanId;

        MacHdr.srcAddr.mode                = LONG_ADDR;
        MacHdr.srcAddr.longAddr[0] =
            SMacAddr[4] << 24 | SMacAddr[5] << 16 | SMacAddr[6] << 8 | SMacAddr[7];
        MacHdr.srcAddr.longAddr[1] =
            SMacAddr[0] << 24 | SMacAddr[1] << 16 | SMacAddr[2] << 8 | SMacAddr[3];


        memset(&MacHdrPtr.buf[0], 0x0, MAX_DATA_SIZE);
        MacHdrPtr.dptr = &MacHdrPtr.buf[0];
        MacHdrPtr.len = 0;
        mac_genHeader(&MacHdrPtr, &MacHdr);

        mac_tx_lens += MacHdrPtr.len;

        mac_tx_data = mem_malloc(mac_tx_lens);

        if (mac_tx_data)
        {
            mem_memcpy(mac_tx_data, MacHdrPtr.dptr, MacHdrPtr.len);
            mem_memcpy(&mac_tx_data[MacHdrPtr.len], mac_data_header, MAC_HEADER_CHAR_LENS);
            mem_memcpy(&mac_tx_data[MacHdrPtr.len + MAC_HEADER_CHAR_LENS], data, lens);
            log_debug_hexdump("br mac send ", mac_tx_data, mac_tx_lens);
            lmac15p4_tx_data_send(0, mac_tx_data, mac_tx_lens, tx_control, mac_tx_dsn);
            SMacIsTxDone = 0;
            mem_free(mac_tx_data);
            ++mac_tx_dsn;
        }
    }
}

int __mac_unicast_send(uint8_t *dst_mac_addr, uint8_t *data, uint16_t lens)
{
    char mac_data_header[6] = MAC_HEADER_CHAR;
    uint8_t *mac_tx_data = NULL;
    uint16_t mac_tx_lens = lens + 6;

    static MacBuffer_t MacHdrPtr;
    static MacHdr_t MacHdr;

    uint8_t tx_control;

    if (SMacIsTxDone == 1)
    {
        tx_control = 0x0;

        MacHdr.macFrmCtrl.frameType        = MAC_DATA;
        MacHdr.macFrmCtrl.secEnab          = false;
        MacHdr.macFrmCtrl.framePending     = false;
        MacHdr.macFrmCtrl.ackReq           = true;

        MacHdr.macSecCtrl.secLevel         = SEC_LEVEL_NONE;
        MacHdr.macSecCtrl.keyIdMode        = KEY_ID_MODE_IMPLICITY;
        MacHdr.macFrmCtrl.panidCompr       = true;

        MacHdr.dsn                         = mac_tx_dsn;

        MacHdr.destPanid                   = SPanId;

        MacHdr.destAddr.mode               = LONG_ADDR;
        MacHdr.destAddr.longAddr[0] =
            dst_mac_addr[4] << 24 | dst_mac_addr[5] << 16 | dst_mac_addr[6] << 8 | dst_mac_addr[7];
        MacHdr.destAddr.longAddr[1] =
            dst_mac_addr[0] << 24 | dst_mac_addr[1] << 16 | dst_mac_addr[2] << 8 | dst_mac_addr[3];

        MacHdr.srcPanid                    = SPanId;

        MacHdr.srcAddr.mode                = LONG_ADDR;
        MacHdr.srcAddr.longAddr[0] =
            SMacAddr[4] << 24 | SMacAddr[5] << 16 | SMacAddr[6] << 8 | SMacAddr[7];
        MacHdr.srcAddr.longAddr[1] =
            SMacAddr[0] << 24 | SMacAddr[1] << 16 | SMacAddr[2] << 8 | SMacAddr[3];


        memset(&MacHdrPtr.buf[0], 0x0, MAX_DATA_SIZE);
        MacHdrPtr.dptr = &MacHdrPtr.buf[0];
        MacHdrPtr.len = 0;
        mac_genHeader(&MacHdrPtr, &MacHdr);

        mac_tx_lens += MacHdrPtr.len;

        mac_tx_data = mem_malloc(mac_tx_lens);

        if (mac_tx_data)
        {
            mem_memcpy(mac_tx_data, MacHdrPtr.dptr, MacHdrPtr.len);
            mem_memcpy(&mac_tx_data[MacHdrPtr.len], mac_data_header, MAC_HEADER_CHAR_LENS);
            mem_memcpy(&mac_tx_data[MacHdrPtr.len + MAC_HEADER_CHAR_LENS], data, lens);
            log_debug_hexdump("mac send ", mac_tx_data, mac_tx_lens);
            lmac15p4_tx_data_send(0, mac_tx_data, mac_tx_lens, tx_control, mac_tx_dsn);
            SMacIsTxDone = 0;
            SMacTxState = 0;
            while (SMacIsTxDone != 1)
            {
                vTaskDelay(1);
            }
            mem_free(mac_tx_data);
            ++mac_tx_dsn;
        }
    }
    return SMacTxState;
}

void __mac_received_cb(uint8_t *data, uint16_t lens, int8_t rssi, uint8_t *src_addr)
{
    log_info("rssi %d \r\n", rssi);
    log_debug_hexdump("Src addr : ", src_addr, 8);
    log_debug_hexdump("MacR : ", data, lens);
}

void __scan_received_cb(uint16_t panid, uint8_t *src_addr)
{
   
    log_debug_hexdump("Src addr ", src_addr, 8);
		log_info("panid %04x \r\n", panid);
}

void __scan_start()
{
    uint8_t tmp_ch = 0;
    uint8_t data[8] = {0x03, 0x08, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07};
    for (tmp_ch = channel_min; tmp_ch <= channel_max ; tmp_ch++)
    {

        subg_ctrl_frequency_set(FREQ + (CHANNEL_SPACING * (tmp_ch - SMinChannel)));

        lmac15p4_tx_data_send(0, data, 8, 0, mac_tx_dsn);
        mac_tx_dsn++;
        vTaskDelay(200);
    }

    /*set channel back*/
    subg_ctrl_frequency_set(FREQ + (CHANNEL_SPACING * (SCurrentChannel - SMinChannel)));
}

void __mac_task(app_task_event_t sevent)
{
    if (sevent & EVENT_MAC_RX_DONE)
    {
        uint8_t src_mac_addr[8];
        for (int i = 0 ; i < (rx_done_pkt.lens - MAC_HEADER_CHAR_LENS); i++)
        {
            if (memcmp(&rx_done_pkt.data[i], MAC_HEADER_CHAR, MAC_HEADER_CHAR_LENS) == 0)
            {
                for (uint8_t k = 0 ; k < 8 ; k++)
                {
                    src_mac_addr[7 - k] = rx_done_pkt.data[i - 8 + k];
                }
                __mac_received_cb(&rx_done_pkt.data[i + MAC_HEADER_CHAR_LENS],
                                  (rx_done_pkt.lens - i - MAC_HEADER_CHAR_LENS - 2),
                                  (-rx_done_pkt.rssi),
                                  (uint8_t *)&src_mac_addr);
                break;
            }
        }
        if (rx_done_pkt.data[0] == 0x0)
        {
            uint16_t id = rx_done_pkt.data[4] << 8 | rx_done_pkt.data[3];
            for (uint8_t k = 0 ; k < 8 ; k++)
            {
                src_mac_addr[7 - k] = rx_done_pkt.data[5 + k];
            }
            __scan_received_cb(id, src_mac_addr);
        }
    }

    if (sevent & EVENT_MAC_TX_DONE)
    {
        if (SMacIsTxDone == 0)
        {
            SMacIsTxDone = 1;
        }
    }
}

static void __mac_tx_done(uint32_t tx_status)
{

    SMacTxState = tx_status;
    APP_EVENT_NOTIFY_ISR(EVENT_MAC_TX_DONE);

}

static void __mac_rx_done(uint16_t packet_length, uint8_t *rx_data_address,
                          uint8_t crc_status, uint8_t rssi, uint8_t snr)
{
    static uint8_t rx_done_cnt = 0;

    if (crc_status == 0)
    {
        rx_done_pkt.lens = packet_length - 14;
        memcpy(&rx_done_pkt.data, rx_data_address + 9, rx_done_pkt.lens);
        rx_done_pkt.rssi = rssi;
        rx_done_pkt.snr = snr;
        APP_EVENT_NOTIFY_ISR(EVENT_MAC_RX_DONE);
    }

    ++rx_done_cnt > 4 ? rx_done_cnt = 0 : rx_done_cnt;
}

void __mac_init()
{
    lmac15p4_callback_t mac_cb;
    mac_cb.rx_cb = __mac_rx_done;
    mac_cb.tx_cb = __mac_tx_done;
    lmac15p4_cb_set(0, &mac_cb);

    subg_ctrl_sleep_set(false);

    subg_ctrl_idle_set();

    subg_ctrl_modem_config_set(SUBG_CTRL_MODU_FSK, SUBG_CTRL_DATA_RATE_50K, SUBG_CTRL_FSK_MOD_1);

    subg_ctrl_mac_set(SUBG_CTRL_MODU_FSK, SUBG_CTRL_CRC_TYPE_16, SUBG_CTRL_WHITEN_DISABLE);

    subg_ctrl_preamble_set(SUBG_CTRL_MODU_FSK, 8);

    subg_ctrl_sfd_set(SUBG_CTRL_MODU_FSK, 0x00007209);

    subg_ctrl_filter_set(SUBG_CTRL_MODU_FSK, SUBG_CTRL_FILTER_TYPE_GFSK);

    /* PHY PIB */
    lmac15p4_phy_pib_set(PHY_PIB_TURNAROUND_TIMER,
                         PHY_PIB_CCA_DETECT_MODE,
                         PHY_PIB_CCA_THRESHOLD,
                         PHY_PIB_CCA_DETECTED_TIME);
    /* MAC PIB */
    lmac15p4_mac_pib_set(MAC_PIB_UNIT_BACKOFF_PERIOD, MAC_PIB_MAC_ACK_WAIT_DURATION,
                         MAC_PIB_MAC_MAX_BE,
                         MAC_PIB_MAC_MAX_CSMACA_BACKOFFS,
                         MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME, MAC_PIB_MAC_MAX_FRAME_RETRIES,
                         MAC_PIB_MAC_MIN_BE);

    subg_ctrl_frequency_set(FREQ + (CHANNEL_SPACING * (SCurrentChannel - SMinChannel)));

    uint32_t sExtendAddr_0 =
        SMacAddr[4] << 24 | SMacAddr[5] << 16 | SMacAddr[6] << 8 | SMacAddr[7];

    uint32_t sExtendAddr_1 =
        SMacAddr[0] << 24 | SMacAddr[1] << 16 | SMacAddr[2] << 8 | SMacAddr[3];

    lmac15p4_address_filter_set(0, false, 0xFFFF, sExtendAddr_0, sExtendAddr_1, SPanId, 0);

    /* Auto ACK */
    lmac15p4_auto_ack_set(true);
    /* Auto State */
    lmac15p4_auto_state_set(true);

    lmac15p4_src_match_ctrl(0, true);

}

void app_task (void)
{
    appSemHandle = xSemaphoreCreateBinary();
    app_task_event_t event = EVENT_NONE;

    __mac_init();
    app_uart_init();

    log_info("DAS SubG EVK Init \n");

    log_info("PAN ID              : %x", SPanId);

    if (SCurrentChannel < channel_min || SCurrentChannel > channel_max)
    {
        log_info("error channel             : %d", SCurrentChannel);
    }
    else
    {
        log_info("channel             : %d", SCurrentChannel);
    }

    log_info("mac addr            : %02X%02X%02X%02X%02X%02X%02X%02X \r\n",
             SMacAddr[0], SMacAddr[1], SMacAddr[2], SMacAddr[3], SMacAddr[4], SMacAddr[5],
             SMacAddr[6], SMacAddr[7]);

    while (true)
    {
        if (xSemaphoreTake(appSemHandle, portMAX_DELAY))
        {
            APP_EVENT_GET_NOTIFY(event);

            /*app uart use*/
            __uart_task(event);

            /*app mac use*/
            __mac_task(event);
        }
    }
}

static int _cli_cmd_panid(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    if(argc > 1)
    {
        uint16_t apanid = utility_strtox(argv[1], 0, 4) ;
        if(SPanId != apanid)
        {
            SPanId = apanid;
            uint32_t sExtendAddr_0 =
                SMacAddr[4] << 24 | SMacAddr[5] << 16 | SMacAddr[6] << 8 | SMacAddr[7];

            uint32_t sExtendAddr_1 =
                SMacAddr[0] << 24 | SMacAddr[1] << 16 | SMacAddr[2] << 8 | SMacAddr[3];

            lmac15p4_address_filter_set(0, false, 0xFFFF, sExtendAddr_0, sExtendAddr_1, SPanId, 0);
        }
    }
    else
    {
        log_info("%02x",SPanId);
    }

}

static int _cli_cmd_channel(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    if(argc > 1)
    {
        uint8_t achannel = utility_strtox(argv[1], 0, 2) ;
        if(SCurrentChannel != achannel)
        {
            SCurrentChannel = achannel;
            subg_ctrl_frequency_set(FREQ + (CHANNEL_SPACING * (SCurrentChannel - SMinChannel)));
        }
    }
    else
    {
        log_info("%u",SCurrentChannel);
    }
}

static int _cli_cmd_scan(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    __scan_start();
}

static int _cli_cmd_macsend(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
		log_info("send buffer");
    if (argc < 4) {
        log_info("Usage: macsend <dst_addr> <payload_len> <payload_hex>");
        return -1;
    }

    uint8_t dst_addr[8];
    uint8_t br_addr[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    
    for(uint8_t i = 0 ; i < 8; i++)
    {
        dst_addr[i] = (uint8_t)utility_strtox(&argv[1][i*2], 0, 2);
    }

    uint16_t payload_len = (uint16_t)utility_strtox(argv[2], 0, 4);
		if (payload_len * 2 != strlen(argv[3])) 
		{
			log_info("payload_len = %d datalen = %d",payload_len*2,strlen(argv[3]) );
			log_info("Error: payload length does not match length of provided hex data");
			return -1;
		}

    uint8_t* payload = (uint8_t*)malloc(payload_len);
    if (!payload) 
		{
        log_info("Error: Memory allocation failed");
        return -1;
    }

    for (uint16_t i = 0; i < payload_len; i++) 
		{
        payload[i] = (uint8_t)utility_strtox(&argv[3][i*2], 0, 2);
    }

    log_info("send mac addr            : %02X%02X%02X%02X%02X%02X%02X%02X \r\n",
             dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3], dst_addr[4], dst_addr[5], dst_addr[6], dst_addr[7]);


    if(memcmp(&br_addr, &dst_addr, 8) == 0)
    {
        __mac_broadcast_send(payload, payload_len);
    }
    else
    {
        __mac_unicast_send(&dst_addr, payload, payload_len);
    }

    free(payload);

    return 0;
}

const sh_cmd_t g_cli_cmd_macsend STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "macsend",
    .pDescription = "mac send ",
    .cmd_exec     = _cli_cmd_macsend,
};

const sh_cmd_t g_cli_cmd_scan STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "scan",
    .pDescription = "scan",
    .cmd_exec     = _cli_cmd_scan,
};

const sh_cmd_t g_cli_cmd_panid STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "panid",
    .pDescription = "panid",
    .cmd_exec     = _cli_cmd_panid,
};

const sh_cmd_t g_cli_cmd_channel STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "channel",
    .pDescription = "channel",
    .cmd_exec     = _cli_cmd_channel,
};