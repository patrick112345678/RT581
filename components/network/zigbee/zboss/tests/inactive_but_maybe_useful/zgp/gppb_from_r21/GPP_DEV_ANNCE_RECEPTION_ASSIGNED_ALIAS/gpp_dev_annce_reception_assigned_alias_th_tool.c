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
/* PURPOSE: TH gps
*/

#define ZB_TEST_NAME GPP_DEV_ANNCE_RECEPTION_ASSIGNED_ALIAS_TH_TOOL
#define ZB_TRACE_FILE_ID 41429

#include "../include/zgp_test_templates.h"

#include "test_config.h"

#define ENDPOINT 10

#define ZGPD_SEQ_NUM_CAP 1
#define ZGPD_RX_ON_CAP 0
#define ZGPD_FIX_LOC 0
#define ZGPD_USE_ASSIGNED_ALIAS 1
#define ZGPD_USE_SECURITY 0

static zb_ieee_addr_t g_th_gps_addr = TH_TOOL_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint16_t g_dut_addr;

static void dev_annce_cb(zb_zdo_device_annce_t *da);
static void zgpc_send_pairing(zb_uint8_t param, zb_bool_t add_sink);

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_WAIT_FOR_DUT_STARTUP,
    TEST_STATE_SEND_PAIRING,
    TEST_READ_OUT_DUT_PROXY_TABLE,
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 4000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    ZVUNUSED(cb);

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_PAIRING:
        zgpc_send_pairing(buf_ref, ZB_TRUE);
        ZB_ZGPC_SET_PAUSE(5);
        break;
    case TEST_READ_OUT_DUT_PROXY_TABLE:
        zgp_cluster_read_attr(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                              ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        ZB_ZGPC_SET_PAUSE(2);
        break;
    }
}

static void zgpc_send_pairing(zb_uint8_t param, zb_bool_t add_sink)
{
    zb_zgp_sink_tbl_ent_t ent;

    ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

    ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                      ZB_ZGP_APP_ID_0000,
                      ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED,
                      ZGPD_SEQ_NUM_CAP,
                      ZGPD_RX_ON_CAP,
                      ZGPD_FIX_LOC,
                      ZGPD_USE_ASSIGNED_ALIAS,
                      ZGPD_USE_SECURITY);

    ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
    ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID;
    ent.security_counter = TEST_MAC_DSN_VALUE - 1;
    ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    ent.u.sink.match_dev_tbl_idx = 0xff;
    ent.zgpd_assigned_alias = TEST_ZGPD_ASN_ALIAS;
    ent.groupcast_radius = 0;

    if (add_sink)
    {
        zb_zgp_gp_pairing_send_req_t *req;

        ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(ZB_BUF_FROM_REF(param), req, &ent, NULL);
        ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 1, 0, 1);
        zgp_cluster_send_gp_pairing(param);
    }
    else
    {
        zb_zgp_gp_pairing_send_req_t *req;

        ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(ZB_BUF_FROM_REF(param), req, &ent, NULL);
        ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 0, 0, 1);
        zgp_cluster_send_gp_pairing(param);
    }
}

static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
    zb_ieee_addr_t dut_ieee = DUT_GPP_IEEE_ADDR;
    TRACE_MSG(TRACE_APP1, "dev_annce_cb: ieee = " TRACE_FORMAT_64 " NWK = 0x%x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

    if (ZB_IEEE_ADDR_CMP(dut_ieee, da->ieee_addr) == ZB_TRUE)
    {
        g_dut_addr = da->nwk_addr;
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND);
    }
}

static void perform_next_state(zb_uint8_t param)
{
    if (TEST_DEVICE_CTX.pause)
    {
        ZB_SCHEDULE_ALARM(perform_next_state, 0,
                          ZB_TIME_ONE_SECOND * TEST_DEVICE_CTX.pause);
        TEST_DEVICE_CTX.pause = 0;
        return;
    }

    TEST_DEVICE_CTX.test_state++;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAIT_FOR_DUT_STARTUP:
        /* Transition to next state should be performed in dev_annce_cb */
        if (param)
        {
            zb_free_buf(ZB_BUF_FROM_REF(param));
        }
        break;
    case TEST_STATE_FINISHED:
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    default:
    {
        if (param)
        {
            zb_free_buf(ZB_BUF_FROM_REF(param));
        }
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
    }
}

static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
    zb_uint16_t short_addr = zb_random();
    zb_ieee_addr_t th_zr_ieee = TH_ZR_IEEE_ADDR;

    TRACE_MSG(TRACE_APP1, "ADDR_CB: ieee_addr = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ieee_addr)));
    TRACE_MSG(TRACE_APP1, "ADDR_CB: th_ieee = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(th_zr_ieee)));

    if (ZB_IEEE_ADDR_CMP(ieee_addr, th_zr_ieee))
    {
        short_addr = TH_ZR_NWK_ADDR;
    }
    else
    {
        short_addr = zb_random();
    }
    /* fixed address for TH ZR, Random - for other devices */
    return short_addr;
}

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_tool");

    /* let's always be coordinator */
    ZB_AIB().aps_designated_coordinator = 1;
    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);

    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gps_addr);
    ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_secur_setup_nwk_key(g_key_nwk, 0);

    ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);

    /* Must use NVRAM for ZGP */
    ZB_AIB().aps_use_nvram = 1;

    /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 0;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

    ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;

    ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;

    ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                                 ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

    ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
    ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;

    zb_nwk_set_address_assignment_cb(addr_ass_cb);
    zb_zdo_register_device_annce_cb(dev_annce_cb);
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
