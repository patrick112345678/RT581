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
/* PURPOSE:
*/


#define ZB_TEST_NAME TP_R22_CCB_2462_GZR
#define ZB_TRACE_FILE_ID 63740

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

enum test_step_e
{
    TS_TEST_START,
    TS_NWK_DUTZR_NO_IEEE,
    TS_NWK_GZR_NO_IEEE,
    TS_NWK_DUTZR_IEEE_DUTZR,
    TS_NWK_DUTZR_IEEE_GZR,
    TEST_STEPS_COUNT
};

static const zb_ieee_addr_t g_ieee_addr_gzr = IEEE_ADDR_gZR;
static const zb_ieee_addr_t g_ieee_addr_dutzr = IEEE_ADDR_DUT_ZR;

static zb_uint8_t g_is_first_start = ZB_TRUE;
static zb_zdo_device_annce_t s_gzed_info;
static zb_uint8_t g_test_step;

static void test_enable_dev_annce_callback(zb_uint8_t unused);
static void test_device_annce_cb(zb_zdo_device_annce_t *da);
static void test_logic_iteration(zb_uint8_t unused);
static void test_send_device_annce(zb_uint8_t param, zb_uint16_t nwk_src_short);
static void test_build_and_send_device_annce(zb_uint8_t param, zb_uint16_t nwk_src_short);

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_gzr");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_gzr);

    /* join as a router */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_set_nvram_erase_at_start(ZB_TRUE);

    zb_set_max_children(0);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            if (g_is_first_start)
            {
                g_is_first_start = ZB_FALSE;

                ZB_SCHEDULE_ALARM(test_enable_dev_annce_callback, 0, TEST_GZR_STARTUP_DELAY);
            }
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void test_enable_dev_annce_callback(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_zdo_register_device_annce_cb(test_device_annce_cb);
}

static void test_device_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_APS2, ">> test_device_annce_cb, da %p", (FMT__P, da));

    if (ZB_IEEE_ADDR_CMP(da->ieee_addr, g_ieee_addr_dutzr))
    {
        ZB_IEEE_ADDR_COPY(&s_gzed_info.ieee_addr, da->ieee_addr);
        s_gzed_info.nwk_addr = da->nwk_addr;
        s_gzed_info.capability = da->capability;

        ZB_SCHEDULE_ALARM(test_logic_iteration, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_GZR_SPOOF_DEV_ANNCE_DELAY_MS));
    }

    TRACE_MSG(TRACE_APS2, "<< test_device_annce_cb", (FMT__0));
}

static void test_logic_iteration(zb_uint8_t unused)
{
    zb_callback2_t call_cb;
    zb_uint16_t nwk_src_short;

    ZVUNUSED(unused);

    g_test_step++;

    TRACE_MSG(TRACE_ZDO1, ">>test_logic_iteration: step = %d", (FMT__D, g_test_step));

    switch (g_test_step)
    {
    case TS_NWK_DUTZR_NO_IEEE:
        nwk_src_short = s_gzed_info.nwk_addr;
        call_cb = test_send_device_annce;
        break;
    case TS_NWK_GZR_NO_IEEE:
        nwk_src_short = zb_cert_test_get_network_addr();
        call_cb = test_send_device_annce;
        break;
    case TS_NWK_DUTZR_IEEE_DUTZR:
    case TS_NWK_DUTZR_IEEE_GZR:
        nwk_src_short = s_gzed_info.nwk_addr;
        call_cb = test_build_and_send_device_annce;
        break;
    default:
        if (g_test_step == TEST_STEPS_COUNT)
        {
            TRACE_MSG(TRACE_ZDO1, "test_logic_iteration: TEST FINISHED", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ZDO1, "test_logic_iteration: TEST ERROR - illegal state", (FMT__0));
        }
        break;
    }

    if (call_cb != NULL)
    {
        if (zb_buf_get_out_delayed_ext(call_cb, nwk_src_short, 0) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "test_logic_iteration: zb_buf_get_out_delayed_ext failed", (FMT__0));
        }
    }

    TRACE_MSG(TRACE_ZDO1, "<<test_logic_iteration", (FMT__0));
}

static void test_send_device_annce(zb_uint8_t param, zb_uint16_t nwk_src_short)
{
#ifndef NCP_MODE_HOST
    zb_zdo_device_annce_t *da;
    zb_apsde_data_req_t   *dreq = zb_buf_get_tail(param,
                                  sizeof(zb_apsde_data_req_t));

    TRACE_MSG(TRACE_ZDO1, ">>test_send_device_annce %hd", (FMT__H, param));

    da = zb_buf_initial_alloc(param, sizeof(*da));
    ZB_BZERO(dreq, sizeof(*dreq));

    ZDO_TSN_INC();
    da->tsn = zb_cert_test_get_zdo_tsn();;
    ZB_HTOLE16(&da->nwk_addr, &ZB_PIBCACHE_NETWORK_ADDRESS());
    zb_get_long_address(da->ieee_addr);
    da->capability = 0;
    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(da->capability, ZG->nwk.handle.rejoin_capability_alloc_address);

    ZB_MAC_CAP_SET_ROUTER_CAPS(da->capability);

    ZG->zdo.handle.dev_annce_param = param;

    dreq->dst_addr.addr_short = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    dreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    dreq->clusterid = ZDO_DEVICE_ANNCE_CLID;

    dreq->use_alias = ZB_TRUE;
    dreq->alias_src_addr = nwk_src_short;

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);

    TRACE_MSG(TRACE_ZDO1, "<<test_send_device_annce", (FMT__0));
#else
    ZVUNUSED(param);
    ZVUNUSED(nwk_src_short);
    ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}

static void test_build_and_send_device_annce(zb_bufid_t buf, zb_uint16_t nwk_src_short)
{
    zb_uint8_t *byte_p;
    zb_uint8_t aps_fc = 0;
    zb_uint8_t mac_tx_opt;

    TRACE_MSG(TRACE_APS1, ">>test_build_and_send_dev_annce: param = %d", (FMT__D, buf));

    byte_p = zb_buf_initial_alloc(buf, sizeof(zb_zdo_device_annce_t));

    /* ZDO Device Annce: tsn | device nwk_addr | device ieee_addr| device capabilities */
    zb_cert_test_inc_zdo_tsn();
    *byte_p++ = zb_cert_test_get_zdo_tsn();;
    byte_p = zb_put_next_htole16(byte_p, ZB_PIBCACHE_NETWORK_ADDRESS());
    ZB_IEEE_ADDR_COPY(byte_p, g_ieee_addr_gzr);
    byte_p += sizeof(zb_ieee_addr_t);
    *byte_p = TEST_GZR_CAP_INFO;

    /* creating APS header */
    ZB_APS_FC_SET_FRAME_TYPE(aps_fc, ZB_APS_FRAME_DATA);
    ZB_APS_FC_SET_DELIVERY_MODE(aps_fc, ZB_APS_DELIVERY_BROADCAST);

    /* APS Header: fc | dest_ep | cluster_id | profile_id | src_ep | aps_counter - 8 bytes */
    byte_p = zb_buf_alloc_left(buf, sizeof(zb_uint8_t) * 8);
    *byte_p++ = aps_fc;
    *byte_p++ = 0; /* dest endpoint */
    byte_p = zb_put_next_htole16(byte_p, ZDO_DEVICE_ANNCE_CLID);
    byte_p = zb_put_next_htole16(byte_p, ZB_AF_ZDO_PROFILE_ID);
    *byte_p++ = 0; /* src endpoint */
    *byte_p++ = zb_cert_test_get_aps_counter();
    zb_cert_test_inc_aps_counter();

    /* NWK Header: fc | dest_addr | src_addr | radius | seq_number | ext_src - 16 bytes + security header */
    byte_p = zb_buf_alloc_left(buf, sizeof(zb_uint8_t) * (16 + sizeof(zb_nwk_aux_frame_hdr_t)));
    ZB_NWK_FRAMECTL_SET_DISCOVER_ROUTE(byte_p, 0);
    ZB_NWK_FRAMECTL_SET_SECURITY(byte_p, ZB_TRUE);
    ZB_NWK_FRAMECTL_SET_SOURCE_IEEE(byte_p, ZB_TRUE);
    ZB_NWK_FRAMECTL_SET_PROTOCOL_VERSION(byte_p, 2);
    byte_p += 2;
    byte_p = zb_put_next_htole16(byte_p, 0xffff);
    byte_p = zb_put_next_htole16(byte_p, nwk_src_short);
    *byte_p++ = 30;
    *byte_p++ = zb_cert_test_get_nib_seq_number();
    zb_cert_test_inc_nib_seq_number();
    if (g_test_step == TS_NWK_DUTZR_IEEE_DUTZR)
    {
        ZB_IEEE_ADDR_COPY(byte_p, s_gzed_info.ieee_addr);
    }
    else
    {
        ZB_IEEE_ADDR_COPY(byte_p, g_ieee_addr_gzr);
    }

    /* building MCPS data request */
    zb_buf_flags_or(buf, ZB_BUF_SECUR_NWK_ENCR);
    zb_buf_set_handle(buf, buf);
    mac_tx_opt = 0; /* no ack needed, no gts/cap,  direct transmission */
#if 1
    zb_mcps_build_data_request(buf, ZB_PIBCACHE_NETWORK_ADDRESS(),
                               0xffff, mac_tx_opt, buf);
#endif

    zb_nwk_unlock_in(buf);
    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, buf);

    TRACE_MSG(TRACE_APS1, ">>test_build_and_send_dev_annce", (FMT__0));
}
