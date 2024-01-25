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
/*  PURPOSE: Routines specific for Duty Cycle Monitoring
*/

#define ZB_TRACE_FILE_ID 57

#include "zb_common.h"

#if defined ZB_MAC_DUTY_CYCLE_MONITORING && !defined ZB_MACSPLIT_HOST

#include "zb_mac_globals.h"
#include "mac_internal.h"

void zb_mac_duty_cycle_mib_init(void)
{
    TRACE_MSG(TRACE_MAC3, ">> zb_mac_duty_cycle_mib_init", (FMT__0));

    MAC_PIB().mib_duty_cycle_limited_threshold = ZB_MAC_DUTY_CYCLE_LIMITED_THRESHOLD_SYMBOLS;
    MAC_PIB().mib_duty_cycle_critical_threshold = ZB_MAC_DUTY_CYCLE_CRITICAL_THRESHOLD_SYMBOLS;
    MAC_PIB().mib_duty_cycle_regulated = 0U;
    MAC_PIB().mib_duty_cycle_used = 0U;
    MAC_PIB().mib_duty_cycle_status = ZB_MAC_DUTY_CYCLE_STATUS_NORMAL;

    ZB_BZERO(&MAC_PIB().mib_duty_cycle_bucket, sizeof(zb_mac_duty_cycle_bucket_t));

    TRACE_MSG(TRACE_MAC3, "<< zb_mac_duty_cycle_mib_init", (FMT__0));
}


void zb_mac_duty_cycle_update_regulated(zb_uint8_t page)
{
    TRACE_MSG(TRACE_MAC3, ">> zb_mac_duty_cycle_update_regulated page %hd", (FMT__H, page));

    if (page == ZB_CHANNEL_PAGE28_SUB_GHZ || page == ZB_CHANNEL_PAGE29_SUB_GHZ)
    {
        MAC_PIB().mib_duty_cycle_regulated = ZB_MAC_DUTY_CYCLE_REGULATED_SYMBOLS_PAGES_28_29;
    }
    else if (page == ZB_CHANNEL_PAGE30_SUB_GHZ || page == ZB_CHANNEL_PAGE31_SUB_GHZ)
    {
        MAC_PIB().mib_duty_cycle_regulated = ZB_MAC_DUTY_CYCLE_REGULATED_SYMBOLS_PAGES_30_31;
    }
    else
    {
        /* TODO: Update for other pages when specification will be ready */
        MAC_PIB().mib_duty_cycle_regulated = ZB_MAC_DUTY_CYCLE_REGULATED_SYMBOLS_PAGES_28_29;
    }

    TRACE_MSG(TRACE_MAC3, "<< zb_mac_duty_cycle_update_regulated regulated %ld",
              (FMT__L, MAC_PIB().mib_duty_cycle_regulated));
}


zb_bool_t zb_mac_duty_cycle_is_limit_exceeded(zb_uint32_t tx_symbols)
{
    zb_bool_t ret = ZB_FALSE;

    /* Duty cycle monitoring necessary only for EU and GB Sub-GHz PHY */
    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ_GB_FSK(MAC_PIB().phy_current_page) ||
            ZB_LOGICAL_PAGE_IS_SUB_GHZ_EU_FSK(MAC_PIB().phy_current_page))
    {
        if (MAC_PIB().mib_duty_cycle_used + tx_symbols >= MAC_PIB().mib_duty_cycle_regulated)
        {
            ret = ZB_TRUE;
        }
    }

    return ret;
}


static void mac_duty_cycle_indication_sender(zb_uint8_t param)
{
    zb_mlme_duty_cycle_mode_indication_t *ind;

    TRACE_MSG(TRACE_MAC1, ">> mac_duty_cycle_indication_sender param %hd status %hd",
              (FMT__H_H, param, MAC_PIB().mib_duty_cycle_status));

    ind = ZB_BUF_GET_PARAM(param, zb_mlme_duty_cycle_mode_indication_t);

    ind->status = MAC_PIB().mib_duty_cycle_status;

    ZB_SCHEDULE_CALLBACK(zb_mlme_duty_cycle_mode_indication, param);

    TRACE_MSG(TRACE_MAC1, "<< mac_duty_cycle_indication_sender", (FMT__0));
}


void zb_mac_duty_cycle_check_mode(void)
{
    zb_mac_duty_cycle_status_t status;

    /* Duty cycle monitoring necessary only for EU and GB Sub-GHz PHY */
    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ_GB_FSK(MAC_PIB().phy_current_page) ||
            ZB_LOGICAL_PAGE_IS_SUB_GHZ_EU_FSK(MAC_PIB().phy_current_page))
    {
        if (MAC_PIB().mib_duty_cycle_used >= MAC_PIB().mib_duty_cycle_regulated)
        {
            status = ZB_MAC_DUTY_CYCLE_STATUS_SUSPENDED;
        }
        else if (MAC_PIB().mib_duty_cycle_used >= MAC_PIB().mib_duty_cycle_critical_threshold)
        {
            status = ZB_MAC_DUTY_CYCLE_STATUS_CRITICAL;
        }
        else if (MAC_PIB().mib_duty_cycle_used >= MAC_PIB().mib_duty_cycle_limited_threshold)
        {
            status = ZB_MAC_DUTY_CYCLE_STATUS_LIMITED;
        }
        else
        {
            status = ZB_MAC_DUTY_CYCLE_STATUS_NORMAL;
        }
    }
    else
    {
        status = ZB_MAC_DUTY_CYCLE_STATUS_NORMAL;
    }

    TRACE_MSG(TRACE_MAC1, "zb_mac_duty_cycle_check_mode old_status %hd new_status %hd",
              (FMT__H_H, MAC_PIB().mib_duty_cycle_status, status));

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().zcl_subghz_cluster_test)
    {
        static zb_uint_t cntr = 0U;
        static zb_bool_t go_up = ZB_TRUE;
        static zb_mac_duty_cycle_status_t subst_status = ZB_MAC_DUTY_CYCLE_STATUS_NORMAL;

        if (++cntr % 55 == 0U)
        {
            if (go_up)
            {
                if (subst_status == ZB_MAC_DUTY_CYCLE_STATUS_NORMAL)
                {
                    subst_status = ZB_MAC_DUTY_CYCLE_STATUS_LIMITED;
                }
                else if (subst_status == ZB_MAC_DUTY_CYCLE_STATUS_LIMITED)
                {
                    subst_status = ZB_MAC_DUTY_CYCLE_STATUS_CRITICAL;
                    go_up = ZB_FALSE;
                }
            }
            else
            {
                if (subst_status == ZB_MAC_DUTY_CYCLE_STATUS_CRITICAL)
                {
                    subst_status = ZB_MAC_DUTY_CYCLE_STATUS_LIMITED;
                }
                else if (subst_status == ZB_MAC_DUTY_CYCLE_STATUS_LIMITED)
                {
                    subst_status = ZB_MAC_DUTY_CYCLE_STATUS_NORMAL;
                    go_up = ZB_TRUE;
                }
            }
        }
        status = subst_status;
        TRACE_MSG(TRACE_MAC4, "zb_mac_duty_cycle_check_mode: subst_status %d", (FMT__D, status));
    }
#endif

    if (status != MAC_PIB().mib_duty_cycle_status)
    {
        MAC_PIB().mib_duty_cycle_status = status;
        (void)zb_buf_get_out_delayed(mac_duty_cycle_indication_sender);
    }
}


void zb_mac_duty_cycle_bump_buckets(void)
{
    zb_uint32_t old_bucket;
    zb_uint32_t *new_bucket;

    TRACE_MSG(TRACE_MAC1, ">> zb_mac_duty_cycle_bump_buckets buckets in use %hd, current bucket idx %hd, last bucket idx %hd duty_cycle_used %ld",
              (FMT__H_H_H_L, MAC_PIB().mib_duty_cycle_bucket.written,
               MAC_PIB().mib_duty_cycle_bucket.write_i,
               MAC_PIB().mib_duty_cycle_bucket.read_i,
               MAC_PIB().mib_duty_cycle_used));

    /* Circular buffer is utilized in non-standard way:
     * All its entries work as intergrator - periodically delete old and allocate new entry.
     * Accumulating is being performed into the current bucket - i.e., write_idx which is not yet
     * commited. Bucket is commited when periodic function is being called - bump_buckets(). Old entry
     * is being retrieved and substracted from MAC_PIB().mib_duty_cycle_used
     */

    /* Trick with write_idx; Write_idx points to the bucket already in use, but
     * not commited into the circular buffer */
    if (ZB_RING_BUFFER_FREE_SPACE(&MAC_PIB().mib_duty_cycle_bucket) > 1U)
    {
        ZB_RADIO_INT_DISABLE();
        /* If the buffer is not full yet, do nothing - only allocate new bucket */
        ZB_RING_BUFFER_FLUSH_PUT(&MAC_PIB().mib_duty_cycle_bucket);
        new_bucket = (zb_uint32_t *)ZB_RING_BUFFER_GETW(&MAC_PIB().mib_duty_cycle_bucket);
        *new_bucket = 0U;

        ZB_RADIO_INT_ENABLE();
    }
    else
    {
        ZB_RADIO_INT_DISABLE();

        old_bucket = *(zb_uint32_t *)ZB_RING_BUFFER_GET(&MAC_PIB().mib_duty_cycle_bucket);
        ZB_RING_BUFFER_FLUSH_GET(&MAC_PIB().mib_duty_cycle_bucket);

        MAC_PIB().mib_duty_cycle_used -= old_bucket;

        /* Move to the next bucket and clean it */
        ZB_RING_BUFFER_FLUSH_PUT(&MAC_PIB().mib_duty_cycle_bucket);
        new_bucket = (zb_uint32_t *)ZB_RING_BUFFER_GETW(&MAC_PIB().mib_duty_cycle_bucket);
        *new_bucket = 0U;

        ZB_RADIO_INT_ENABLE();

        TRACE_MSG(TRACE_MAC3, "subst old_bucket %lu", (FMT__L, old_bucket));

        zb_mac_duty_cycle_check_mode();
    }

    TRACE_MSG(TRACE_MAC1, "<< zb_mac_duty_cycle_bump_buckets buckets in use %hd, current bucket %hd, last bucket %hd duty_cycle_used %ld",
              (FMT__H_H_H_L, MAC_PIB().mib_duty_cycle_bucket.written,
               MAC_PIB().mib_duty_cycle_bucket.write_i,
               MAC_PIB().mib_duty_cycle_bucket.read_i,
               MAC_PIB().mib_duty_cycle_used));
}


void zb_mac_duty_cycle_accumulate_time(zb_uint32_t tx_symbols, zb_bool_t check_mode)
{
    zb_uint32_t *current_bucket;

    /* Duty cycle monitoring necessary only for EU and GB Sub-GHz PHY */
    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ_GB_FSK(MAC_PIB().phy_current_page) ||
            ZB_LOGICAL_PAGE_IS_SUB_GHZ_EU_FSK(MAC_PIB().phy_current_page))
    {
        current_bucket = (zb_uint32_t *)ZB_RING_BUFFER_GETW(&MAC_PIB().mib_duty_cycle_bucket);

#if defined ZB_NSNG
        TRACE_MSG(TRACE_MAC1, ">> zb_mac_duty_cycle_accumulate_time tx_symbols %ld current_bucket %ld, duty_cycle_used %ld",
                  (FMT__L_L_L, tx_symbols, *current_bucket, MAC_PIB().mib_duty_cycle_used));
#endif

        *current_bucket += tx_symbols;
        MAC_PIB().mib_duty_cycle_used += tx_symbols;

        if (check_mode)
        {
            TRACE_MSG(TRACE_MAC1, "zb_mac_duty_cycle_accumulate_time tx_symbols %ld dc_used %ld current_bucket %ld",
                      (FMT__L_L_L, tx_symbols, MAC_PIB().mib_duty_cycle_used, *current_bucket));

            zb_mac_duty_cycle_check_mode();
        }

#if defined ZB_NSNG
        TRACE_MSG(TRACE_MAC1, "<< zb_mac_duty_cycle_accumulate_time current_bucket %ld, duty_cycle_used %ld",
                  (FMT__L_L, *current_bucket, MAC_PIB().mib_duty_cycle_used));
#endif
    }
}


void zb_mac_duty_cycle_periodic(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    TRACE_MSG(TRACE_MAC1, "zb_mac_duty_cycle_periodic", (FMT__0));

    /*
     * Duty cycle monitoring necessary only for EU and GB Sub-GHz PHY
     * Also start duty cycle monitoring after MLME Reset Req
     */
    if (MAC_PIB().phy_current_page == ZB_MAC_INVALID_LOGICAL_PAGE ||
            ZB_LOGICAL_PAGE_IS_SUB_GHZ_GB_FSK(MAC_PIB().phy_current_page) ||
            ZB_LOGICAL_PAGE_IS_SUB_GHZ_EU_FSK(MAC_PIB().phy_current_page))
    {
        zb_mac_duty_cycle_bump_buckets();

        ZB_SCHEDULE_ALARM(zb_mac_duty_cycle_periodic, 0,
                          ZB_MAC_DUTY_CYCLE_TIME_PERIOD_SEC * ZB_TIME_ONE_SECOND);
    }
    else
    {
        ZB_SCHEDULE_ALARM_CANCEL(zb_mac_duty_cycle_periodic, ZB_ALARM_ANY_PARAM);
    }
}

zb_uint_t zb_mac_duty_cycle_get_time_period_sec(void)
{
    return (zb_uint_t)(ZB_MAC_DUTY_CYCLE_TIME_PERIOD_SEC);
}

#ifdef TEST_DUTY_C
void zb_mac_test_dec_th(void)
{
    MAC_PIB().mib_duty_cycle_limited_threshold /= 100U;
    MAC_PIB().mib_duty_cycle_critical_threshold /= 100U;
    MAC_PIB().mib_duty_cycle_regulated /= 100U;
}
#endif /* TEST_DUTY_C */

#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING && !ZB_MACSPLIT_HOST*/
