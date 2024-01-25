/**
 * @file lmac15p4.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-07-26
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <stdio.h>
#include "lmac15p4.h"
#include "hosal_rf.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "log.h"

#ifndef __weak
#define __weak                      __attribute__ ((weak))
#endif


typedef struct
{
    uint8_t mac_promiscuous_mode;
    uint16_t short_source_address;
    uint32_t long_source_address_0;
    uint32_t long_source_address_1;
    uint16_t pan_id;
    uint8_t is_coordinator;

    uint8_t sr_addr_ctrl_type;
    uint8_t key[16];
    uint8_t channel;

    lmac15p4_callback_t callback_set;
} _mac15p4_pan_info_t;

static _mac15p4_pan_info_t glist_pan_info[2];

static uint8_t g_now_pan_idx = 0xFF;
static uint8_t g_now_channel = 0;

static SemaphoreHandle_t xSemaphore = NULL;
static StaticSemaphore_t xSemaphoreBuffer;

void lmac15p4_RxDoneEvent(uint16_t packet_length, uint8_t *pdata,
                          uint8_t crc_status, uint8_t rssi, uint8_t snr)
{
    uint16_t pan_id = 0;
    pf_rx_done_cb *rxcb;

    pan_id = pdata[12] | (pdata[13] << 8);

    //log_info("pan_id 0x%02X", pan_id);


    //if(pan_id == glist_pan_info[0].pan_id || pan_id == 0xFFFF)
    {
        rxcb = glist_pan_info[0].callback_set.rx_cb;

        if (rxcb != NULL)
        {
            rxcb(packet_length, pdata, crc_status, rssi, snr);
        }
    }
    //if(pan_id == glist_pan_info[1].pan_id || pan_id == 0xFFFF)
    {
        rxcb = glist_pan_info[1].callback_set.rx_cb;

        if (rxcb != NULL)
        {
            rxcb(packet_length, pdata, crc_status, rssi, snr);
        }
    }
}

void lmac15p4_TxDoneEvent(uint32_t tx_status)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    pf_tx_done_cb *txcb;
    if (glist_pan_info[g_now_pan_idx].callback_set.tx_cb != NULL)
    {
        txcb = glist_pan_info[g_now_pan_idx].callback_set.tx_cb;
        txcb(tx_status);
    }

    xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

static int __txdone_cb(void *p_arg)
{
    uint32_t status = (uint32_t)p_arg;

    lmac15p4_TxDoneEvent(status);

    return 0;
}

static int __rxdone_cb(void *p_arg)
{
    uint8_t *pdata = p_arg;
    uint32_t data_len = 0;

    data_len = (pdata[2] | (pdata[3] << 8)) + 4;

    lmac15p4_RxDoneEvent(data_len, pdata, pdata[4], pdata[5], pdata[6]);

    return 0;
}

int lmac15p4_init(lmac15p4_modem_t modem)
{
    memset(&glist_pan_info, 0x00, sizeof(glist_pan_info));
    g_now_pan_idx = 0;

    hosal_rf_ioctl(HOSAL_RF_IOCTL_MODEM_SET, (void *)modem);
    hosal_rf_callback_set(HOSAL_RF_PCI_RX_CALLBACK, __rxdone_cb, NULL);
    hosal_rf_callback_set(HOSAL_RF_PCI_TX_CALLBACK, __txdone_cb, NULL);


    xSemaphore = xSemaphoreCreateBinaryStatic( &xSemaphoreBuffer );

    xSemaphoreGive(xSemaphore);

    return 0;
}

void lmac15p4_cb_set(uint32_t pan_idx, lmac15p4_callback_t *callback_set)
{
    glist_pan_info[pan_idx].callback_set.rx_cb = callback_set->rx_cb;
    glist_pan_info[pan_idx].callback_set.tx_cb = callback_set->tx_cb;
}

int lmac15p4_read_rssi(void)
{
    int rssi;
    hosal_rf_ioctl(HOSAL_RF_IOCTL_RSSI_GET, (void *)&rssi);

    return rssi;
}

char *lmac15p4_get_version(void)
{
    return 0;
}

void lmac15p4_address_filter_set(uint32_t pan_idx, uint8_t mac_promiscuous_mode,
                                 uint16_t short_source_address,
                                 uint32_t long_source_address_0, uint32_t long_source_address_1,
                                 uint16_t pan_id, uint8_t is_coordinator)
{
    hosal_rf_15p4_address_filter_t filter = {0};

    filter.is_coordinator = is_coordinator;
    filter.long_address_0 = long_source_address_0;
    filter.long_address_1 = long_source_address_1;
    filter.panid = pan_id;
    filter.promiscuous = mac_promiscuous_mode;
    filter.short_address = short_source_address;


    glist_pan_info[pan_idx].is_coordinator = is_coordinator;
    glist_pan_info[pan_idx].long_source_address_0 = long_source_address_0;
    glist_pan_info[pan_idx].long_source_address_1 = long_source_address_1;
    glist_pan_info[pan_idx].pan_id = pan_id;
    glist_pan_info[pan_idx].mac_promiscuous_mode = mac_promiscuous_mode;
    glist_pan_info[pan_idx].short_source_address = short_source_address;

    if (g_now_pan_idx != pan_idx)
    {
        g_now_pan_idx = pan_idx;

        hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_OPERATION_PAN_IDX_SET, (void *)pan_idx);
    }

    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_ADDRESS_FILTER_SET, (void *)&filter);
}

void lmac15p4_mac_pib_set(uint32_t a_unit_backoff_period,
                          uint32_t mac_ack_wait_duration, uint8_t mac_max_BE,
                          uint8_t mac_max_CSMA_backoffs, uint32_t mac_max_frame_total_wait_time,
                          uint8_t mac_max_frame_retries, uint8_t mac_min_BE)
{
    hosal_rf_15p4_mac_pib_t mac_pib = {0};

    mac_pib.backoff_period = a_unit_backoff_period;
    mac_pib.ack_wait_duration = mac_ack_wait_duration;
    mac_pib.max_BE = mac_max_BE;
    mac_pib.max_CSMA_backoffs = mac_max_CSMA_backoffs;
    mac_pib.max_frame_total_wait_time = mac_max_frame_total_wait_time;
    mac_pib.max_frame_retries = mac_max_frame_retries;
    mac_pib.min_BE = mac_min_BE;

    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_MAC_PIB_SET, (void *)&mac_pib);
}

void lmac15p4_phy_pib_set(uint16_t a_turnaround_time, uint8_t phy_cca_mode,
                          uint8_t phy_cca_threshold, uint16_t phy_cca_duration)
{
    hosal_rf_15p4_phy_pib_t phy_pib = {0};

    phy_pib.turnaround_time = a_turnaround_time;
    phy_pib.cca_duration = phy_cca_duration;
    phy_pib.cca_mode = phy_cca_mode;
    phy_pib.cca_threshold = phy_cca_threshold;

    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_PHY_PIB_SET, (void *)&phy_pib);
}

void lmac15p4_channel_set(lmac154_channel_t ch)
{
    uint32_t chammel_freq;

    if (g_now_channel != ch)
    {
        g_now_channel = ch;

        chammel_freq = 2405 + (5 * ch);
        hosal_rf_ioctl(HOSAL_RF_IOCTL_FREQUENCY_SET, (void *)chammel_freq);
    }
}

void lmac15p4_auto_ack_set(uint32_t auto_ack_enable)
{
    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_AUTO_ACK_SET, (void *) auto_ack_enable);
}

void lmac15p4_auto_state_set(uint32_t auto_state_enable)
{
    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_AUTO_STATE_SET, (void *) auto_state_enable);
}


void lmac15p4_ack_pending_bit_set(uint32_t pan_idx, uint32_t pending_bit_enable)
{

    if (g_now_pan_idx != pan_idx)
    {
        g_now_pan_idx = pan_idx;

        hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_OPERATION_PAN_IDX_SET, (void *)pan_idx);
    }
    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_ACK_PENDING_BIT_SET, (void *) pending_bit_enable);
}

int lmac15p4_tx_data_send(uint32_t pan_idx, uint8_t *tx_data_address, uint16_t packet_length, uint8_t mac_control, uint8_t mac_dsn)
{
    int ret = 0;
    hosal_rf_tx_data_t tx_data = {0};

    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    tx_data.data_len = packet_length;
    tx_data.control = mac_control;
    tx_data.dsn = mac_dsn;
    tx_data.pData = tx_data_address;

    if (g_now_pan_idx != pan_idx)
    {
        g_now_pan_idx = pan_idx;

        hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_OPERATION_PAN_IDX_SET, (void *)pan_idx);
    }

    ret = hosal_rf_ioctl(HOSAL_RF_IOCTL_TX_START_SET, (void *) &tx_data);

    if (ret != 0)
    {
        xSemaphoreGive(xSemaphore);
    }

    return ret;
}

void lmac15p4_src_match_ctrl(uint32_t pan_idx, uint32_t enable)
{

    if (g_now_pan_idx != pan_idx)
    {
        g_now_pan_idx = pan_idx;

        hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_OPERATION_PAN_IDX_SET, (void *)pan_idx);
    }
    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_SOURCE_ADDRESS_MATCH_SET, (void *) enable);
}
void lmac15p4_src_match_short_entry(uint8_t control_type, uint8_t *short_addr)
{
    hosal_rf_15p4_src_match_t match = {0};

    match.control_type = control_type;
    match.addr = short_addr;

    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_SOURCE_ADDRESS_SHORT_CONTROL_SET, (void *) &match);
}
void lmac15p4_src_match_extended_entry(uint8_t control_type, uint8_t *extended_addr)
{
    hosal_rf_15p4_src_match_t match = {0};

    match.control_type = control_type;
    match.addr = extended_addr;

    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_SOURCE_ADDRESS_EXTEND_CONTROL_SET, (void *) &match);
}
void lmac15p4_csl_receiver_ctrl(uint8_t csl_receiver_ctrl, uint16_t csl_period)
{

}
void lmac15p4_csl_accuracy_get(uint8_t *csl_accuracy)
{

}
void lmac15p4_csl_uncertainty_get(uint8_t *csl_uncertainty)
{

}
void lmac15p4_csl_sample_time_update(uint32_t csl_sample_time)
{

}

void lmac15p4_read_ack(uint8_t *ack_data, uint8_t *ack_time)
{
    hosal_rf_15p4_ack_packet_t ack;
    ack.pdata = ack_data;
    ack.ptime = ack_time;

    hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_ACK_PACKET_GET, (void *) &ack);
}

void lmac15p4_key_set(uint32_t pan_idx, uint8_t *key)
{
    if (g_now_pan_idx != pan_idx)
    {
        g_now_pan_idx = pan_idx;

        hosal_rf_ioctl(HOSAL_RF_IOCTL_15P4_OPERATION_PAN_IDX_SET, (void *) pan_idx);
    }

    hosal_rf_ioctl(HOSAL_RF_IOCTL_KEY_SET, (void *) key);
}