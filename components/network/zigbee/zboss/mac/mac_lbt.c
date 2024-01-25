/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * www.dsr-zboss.com
 * www.dsr-corporation.com
 * All rights reserved.
 *
 * This is unpublished proprietary source code of DSR Corporation
 * The copyright notice does not evidence any actual or intended
 * publication of such source code.
 *
 * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
 * Corporation
 *
 * Commercial Usage
 * Licensees holding valid DSR Commercial licenses may use
 * this file in accordance with the DSR Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement between you and
 * DSR.
 */
/*  PURPOSE: Routines specific to LBT
*/

#define ZB_TRACE_FILE_ID 55
#include "zb_common.h"
#include "mac_internal.h"

/*
 * LBT is not implemented exactly as in specs.
 * The resulting behaviour was not changed.
 */

#if defined ZB_SUB_GHZ_LBT && !defined ZB_MACSPLIT_HOST

#include "zb_mac_globals.h"

static zb_bool_t mac_lbt_cca(zb_uint32_t scan_duration_usec);

static zb_bool_t mac_lbt_is_channel_busy(void);

static zb_mac_lbt_ret_t zb_mac_lbt_lbt(void);

static void mac_lbt_tx_off_alarm(zb_uint8_t unused);


static zb_bool_t mac_lbt_cca(zb_uint32_t scan_duration_usec)
{
    zb_bool_t ret = ZB_TRUE;
    zb_uint8_t i;

    TRACE_MSG(TRACE_MAC1, ">> mac_lbt_cca scan_duration %ld",
              (FMT__L, scan_duration_usec));

    for (i = 0; i < scan_duration_usec / ZB_MAC_LBT_GRANULARITY_USEC; i++)
    {
        /* MM: TODO: Bug here:
         * Definetely, this should be GET_RSSI function and scan the whole time
         * period.
         * I'll return to this later on a real HW
         */

        if (mac_lbt_is_channel_busy())
        {
            ret = ZB_FALSE;
            break;
        }

        osif_sleep_using_transc_timer(ZB_MAC_LBT_GRANULARITY_USEC);
    }

    TRACE_MSG(TRACE_MAC1, "<< mac_lbt_cca ret %hd",
              (FMT__H, ret));

    return ret;
}


static zb_bool_t mac_lbt_is_channel_busy(void)
{
    zb_bool_t channel_busy = ZB_FALSE;
    zb_int8_t rssi;

    /*
     * TODO: Use here ZB_TRANSCEIVER_PERFORM_CCA(rssi) function instead of direct rssi measurement.
     * After this will be done, need to check LBT mechanism using
     * TP/154/MAC/CHANNEL-ACCESS-03 test on the all Sub-GHz pages (GB, EU and NA PHY).
     */
#if defined ZB_NSNG
    rssi = -100;
#else
    rssi = ZB_TRANSCEIVER_GET_SYNC_RSSI();
#endif

    if (rssi > ZB_MAC_LBT_RSSI_THRESHOLD_BY_PAGE(MAC_PIB().phy_current_page))
    {
        channel_busy = ZB_TRUE;
    }

    return channel_busy;
}


static zb_mac_lbt_ret_t zb_mac_lbt_lbt(void)
{
    zb_mac_lbt_ret_t ret = ZB_MAC_LBT_RET_BUSY;
    zb_time_t        tmo = ZB_TIME_ADD(osif_transceiver_time_get(), ZB_MAC_LBT_TIMEOUT_USEC);
    zb_uint8_t       count;
    zb_bool_t        channel_busy;
    zb_bool_t        rx_on_off;
    zb_time_t        time_now;

    TRACE_MSG(TRACE_MAC1, ">> zb_mac_lbt_lbt", (FMT__0));

    rx_on_off = ZB_TRANSCEIVER_GET_RX_ON_OFF();
    ZB_TRANSCEIVER_SET_RX_ON_OFF(1);

    count = 0;
    time_now = osif_transceiver_time_get();
    while (count < ZB_MAC_LBT_MAX_TX_RETRIES &&
            ZB_TIME_GE(tmo, time_now))
    {
        zb_bool_t lbt_cca_min = mac_lbt_cca(ZB_MAC_LBT_MIN_FREE_USEC);
        channel_busy = mac_lbt_is_channel_busy();

        if (!channel_busy && lbt_cca_min)
        {
            zb_bool_t lbt_cca_max = mac_lbt_cca(ZB_RANDOM_JTR(ZB_MAC_LBT_MAX_RANDOM_USEC));
            if (lbt_cca_max)
            {
                ret = ZB_MAC_LBT_RET_OK;
                break;
            }
            count++;
        }

        osif_sleep_using_transc_timer(ZB_MAC_LBT_GRANULARITY_USEC);
    }

    ZB_TRANSCEIVER_SET_RX_ON_OFF(rx_on_off);

    TRACE_MSG(TRACE_MAC1, "<< zb_mac_lbt_lbt ret %hd", (FMT__H, ret));

    return ret;
}


static void mac_lbt_tx_off_alarm(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    TRACE_MSG(TRACE_MAC3, "mac_lbt_tx_off_alarm, let lbt_radio_busy = 0", (FMT__0));

    MAC_CTX().flags.lbt_radio_busy = ZB_FALSE;

    if (!ZB_MAC_GET_ACK_NEEDED())
    {
        MAC_CTX().flags.tx_q_busy = ZB_FALSE;
    }
}


static void mac_set_trans_tx_status(zb_uint_t tx_status)
{
#ifdef ZB_NSNG
    ZVUNUSED(tx_status);
#else
    ZB_RADIO_INT_DISABLE();
    TRANS_CTX().tx_status = tx_status;
    ZB_MAC_SET_TX_INT_STATUS_BIT();
    ZB_MAC_SET_TRANS_INT();
    ZB_RADIO_INT_ENABLE();
#endif
}


void zb_mac_lbt_tx(zb_uint8_t mhr_len, zb_bufid_t buf, zb_uint8_t wait_type)
{
    zb_mac_lbt_ret_t lbt_status;
    zb_uint32_t frame_duration;
    zb_uint32_t tx_symbols;

    ZVUNUSED(wait_type);

    TRACE_MSG(TRACE_MAC1, ">> zb_mac_lbt_tx buf %p len %hd",
              (FMT__P_H, buf, zb_buf_len(buf)));

    /* Check if the frame can be transmitted */
    if (MAC_CTX().flags.lbt_radio_busy)
    {
        mac_set_trans_tx_status(ZB_TRANS_CHANNEL_BUSY_ERROR);
        TRACE_MSG(TRACE_MAC1, "<< zb_mac_lbt_tx TX not ready", (FMT__0));
        return;
    }

#if defined ZB_MAC_TESTING_MODE
    if (!MAC_CTX().cert_hacks.lbt_radio_busy_disabled)
#endif
    {
        zb_time_t mac_lbt_min_off_ms = ZB_MAC_LBT_TX_MIN_OFF_MS;
        ZB_SCHEDULE_ALARM(mac_lbt_tx_off_alarm, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(mac_lbt_min_off_ms) + 1U);
    }

    tx_symbols =  ZB_MAC_DUTY_CYCLE_RAMP_UP_SYMBOLS;
    tx_symbols += ZB_MAC_EU_FSK_SHR_LEN_SYMBOLS;
    tx_symbols += ZB_MAC_EU_FSK_PHR_LEN_SYMBOLS;
    tx_symbols += (zb_buf_len(buf) + 2U /* MFR */) * ZB_SUB_GHZ_PHY_SYMBOLS_PER_OCTET;
    tx_symbols += ZB_MAC_DUTY_CYCLE_RAMP_DOWN_SYMBOLS;

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
    if (zb_mac_duty_cycle_is_limit_exceeded(tx_symbols))
    {
        /* Force status update, if any status occasionaly hangs due to ack sending */
        zb_mac_duty_cycle_check_mode();
        ZB_SCHEDULE_ALARM_CANCEL(mac_lbt_tx_off_alarm, ZB_ALARM_ANY_PARAM);
        mac_set_trans_tx_status(ZB_TRANS_TX_LBT_TO);
        TRACE_MSG(TRACE_MAC1, "<< zb_mac_lbt_tx DC limit exceeded", (FMT__0));
        return;
    }
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

    lbt_status = zb_mac_lbt_lbt();
    if (lbt_status == ZB_MAC_LBT_RET_BUSY)
    {
        ZB_SCHEDULE_ALARM_CANCEL(mac_lbt_tx_off_alarm, ZB_ALARM_ANY_PARAM);
        mac_set_trans_tx_status(ZB_TRANS_CHANNEL_BUSY_ERROR);
        TRACE_MSG(TRACE_MAC1, "<< zb_mac_lbt_tx channel busy", (FMT__0));
        return;
    }

#if defined ZB_MAC_TESTING_MODE
    if (!MAC_CTX().cert_hacks.lbt_radio_busy_disabled)
#endif
    {
        ZB_SCHEDULE_ALARM_CANCEL(mac_lbt_tx_off_alarm, ZB_ALARM_ANY_PARAM);
        frame_duration = ZB_USECS_TO_MILLISECONDS(tx_symbols * ZB_SUB_GHZ_SYMBOL_DURATION_USEC);
        ZB_SCHEDULE_ALARM(mac_lbt_tx_off_alarm, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_MAC_LBT_TX_MIN_OFF_MS + frame_duration) + 1U);
    }

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
    /* Recheck by spec */
    if (zb_mac_duty_cycle_is_limit_exceeded(tx_symbols))
    {
        mac_set_trans_tx_status(ZB_TRANS_TX_LBT_TO);
        ZB_SCHEDULE_ALARM_CANCEL(mac_lbt_tx_off_alarm, ZB_ALARM_ANY_PARAM);
        TRACE_MSG(TRACE_MAC1, "<< zb_mac_lbt_tx DC limit exceeded", (FMT__0));
        return;
    }
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

#if defined ZB_MAC_TESTING_MODE
    if (!MAC_CTX().cert_hacks.lbt_radio_busy_disabled)
#endif
    {
        TRACE_MSG(TRACE_MAC3, "let lbt_radio_busy = 1", (FMT__0));
        MAC_CTX().flags.lbt_radio_busy = ZB_TRUE;
    }

    ZB_TRANS_SEND_FRAME_SUB_GHZ(mhr_len, buf, ZB_MAC_TX_WAIT_NONE);

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
    zb_mac_duty_cycle_accumulate_time(tx_symbols, ZB_TRUE);
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

    TRACE_MSG(TRACE_MAC1, "<< zb_mac_lbt_tx frame sent", (FMT__0));
}


zb_mac_lbt_ret_t zb_mac_lbt_send_ack(zb_uint8_t ack_dsn)
{
    zb_mac_lbt_ret_t ret = ZB_MAC_LBT_RET_OK;

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
    zb_uint16_t tx_symbols;
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

#if defined ZB_NSNG
    TRACE_MSG(TRACE_MAC1, ">> zb_mac_lbt_send_ack ack_dsn %hd", (FMT__H, ack_dsn));
#endif

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
    tx_symbols =  ZB_MAC_DUTY_CYCLE_RAMP_UP_SYMBOLS;
    tx_symbols += ZB_MAC_EU_FSK_SHR_LEN_SYMBOLS;
    tx_symbols += ZB_MAC_EU_FSK_PHR_LEN_SYMBOLS;
    tx_symbols += ZB_MAC_LBT_SUB_GHZ_ACK_LEN_SYMBOLS;
    tx_symbols += ZB_MAC_DUTY_CYCLE_RAMP_DOWN_SYMBOLS;

    if (zb_mac_duty_cycle_is_limit_exceeded(tx_symbols))
    {
        ret = ZB_MAC_LBT_RET_TX_TO_MAX;
    }
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

    if (ret == ZB_MAC_LBT_RET_OK)
    {
        //osif_sleep_using_transc_timer(ZB_MAC_LBT_ACK_WINDOW_START_USEC);

        ZB_TRANS_SEND_ACK_SUB_GHZ(ack_dsn);

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
        zb_mac_duty_cycle_accumulate_time(tx_symbols, ZB_FALSE);
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */
    }

#if defined ZB_NSNG
    TRACE_MSG(TRACE_MAC1, "<< zb_mac_lbt_send_ack ret %hd", (FMT__H, ret));
#endif

    return ret;
}

zb_int8_t zb_mac_lbt_rssi_threshold_by_page(zb_uint8_t logical_page)
{
    zb_int8_t rssi_threshold;

    switch (logical_page)
    {
    case ZB_CHANNEL_PAGE30_SUB_GHZ:
        rssi_threshold = ZB_MAC_LBT_GB_THRESHOLD_LEVEL_HP;
        break;

    case ZB_CHANNEL_PAGE23_SUB_GHZ:
        rssi_threshold = ZB_MAC_LBT_NA_THRESHOLD_LEVEL_LP;
        break;

    default:
        rssi_threshold = ZB_MAC_LBT_GB_THRESHOLD_LEVEL_LP;
        break;
    }

    return rssi_threshold;
}

#endif  /* ZB_SUB_GHZ_LBT && !defined ZB_ZGPD_ROLE && !defined ZB_MACSPLIT_HOST*/
