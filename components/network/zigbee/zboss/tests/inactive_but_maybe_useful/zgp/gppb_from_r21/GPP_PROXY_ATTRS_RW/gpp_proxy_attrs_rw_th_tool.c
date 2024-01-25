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
/* PURPOSE: TH tool
*/

#define ZB_TEST_NAME GPP_PROXY_ATTRS_RW_TH_TOOL
#define ZB_TRACE_FILE_ID 41418

#include "../include/zgp_test_templates.h"

#include "test_config.h"

#ifdef ZB_CERTIFICATION_HACKS

#define ZGPD_SEQ_NUM_CAP 1
#define ZGPD_RX_ON_CAP 0
#define ZGPD_FIX_LOC 0
#define ZGPD_USE_SECURITY 0

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
static zb_ieee_addr_t g_zgpd_addr = TEST_ZGPD_IEEE_ADDR;

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_READ_GPP_MAX_PROXY_TABLE_ENTRIES1,
    TEST_STATE_WRITE_GPP_MAX_PROXY_TABLE_ENTRIES,
    TEST_STATE_READ_GPP_MAX_PROXY_TABLE_ENTRIES2,
    TEST_STATE_READ_PROXY_TABLE1,
    TEST_STATE_WRITE_PROXY_TABLE,
    TEST_STATE_READ_PROXY_TABLE2,
    TEST_STATE_READ_GPP_FUNCTIONALITY1,
    TEST_STATE_WRITE_GPP_FUNCTIONALITY,
    TEST_STATE_READ_GPP_FUNCTIONALITY2,
    TEST_STATE_READ_GPP_ACTIVE_FUNCTIONALITY1,
    TEST_STATE_WRITE_GPP_ACTIVE_FUNCTIONALITY,
    TEST_STATE_READ_GPP_ACTIVE_FUNCTIONALITY2,
    TEST_STATE_PROXY_TABLE_REQUEST1_IDX_0,
    TEST_STATE_SEND_PAIRING1,
    TEST_STATE_READ_PROXY_TABLE3,
    TEST_STATE_SEND_PAIRING2,
    TEST_STATE_READ_PROXY_TABLE4,
    TEST_STATE_PROXY_TABLE_REQUEST2_IDX_0,
    TEST_STATE_PROXY_TABLE_REQUEST3_IDX_1,
    TEST_STATE_SEND_PAIRING3,
    TEST_STATE_PROXY_TABLE_REQUEST4_IDX_2,
    TEST_STATE_PROXY_TABLE_REQUEST5,
    TEST_STATE_PROXY_TABLE_REQUEST6,
    TEST_STATE_PROXY_TABLE_REQUEST7_IDX_3,
    TEST_STATE_PROXY_TABLE_REQUEST8,
    TEST_STATE_PROXY_TABLE_REQUEST9,
    TEST_STATE_SINK_TABLE_REQUEST,
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_GPP_MAX_PROXY_TABLE_ENTRIES1:
    case TEST_STATE_READ_GPP_MAX_PROXY_TABLE_ENTRIES2:
        zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_MAX_PROXY_TABLE_ENTRIES_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    case TEST_STATE_WRITE_GPP_MAX_PROXY_TABLE_ENTRIES:
    {
        zb_uint8_t val = 0x0a;

        zgp_cluster_write_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                               ZB_ZCL_ATTR_GPP_MAX_PROXY_TABLE_ENTRIES_ID,
                               ZB_ZCL_ATTR_TYPE_U8, &val,
                               ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    }
    case TEST_STATE_READ_PROXY_TABLE1:
    case TEST_STATE_READ_PROXY_TABLE2:
    case TEST_STATE_READ_PROXY_TABLE3:
    case TEST_STATE_READ_PROXY_TABLE4:
        zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    case TEST_STATE_WRITE_PROXY_TABLE:
    {
        zb_uint16_t val = 0;

        zgp_cluster_write_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                               ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                               ZB_ZCL_ATTR_TYPE_LONG_OCTET_STRING, (zb_uint8_t *)&val,
                               ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    }
    case TEST_STATE_READ_GPP_FUNCTIONALITY1:
    case TEST_STATE_READ_GPP_FUNCTIONALITY2:
        zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_FUNCTIONALITY_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    case TEST_STATE_WRITE_GPP_FUNCTIONALITY:
    {
        zb_uint32_t val = 0x0000ffff;

        zgp_cluster_write_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                               ZB_ZCL_ATTR_GPP_FUNCTIONALITY_ID,
                               ZB_ZCL_ATTR_TYPE_24BITMAP, (zb_uint8_t *)&val,
                               ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    }
    case TEST_STATE_READ_GPP_ACTIVE_FUNCTIONALITY1:
    case TEST_STATE_READ_GPP_ACTIVE_FUNCTIONALITY2:
        zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_ACTIVE_FUNCTIONALITY_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    case TEST_STATE_WRITE_GPP_ACTIVE_FUNCTIONALITY:
    {
        zb_uint32_t val = 0;

        zgp_cluster_write_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                               ZB_ZCL_ATTR_GPP_ACTIVE_FUNCTIONALITY_ID,
                               ZB_ZCL_ATTR_TYPE_24BITMAP, (zb_uint8_t *)&val,
                               ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    }
    case TEST_STATE_PROXY_TABLE_REQUEST1_IDX_0:
    case TEST_STATE_PROXY_TABLE_REQUEST2_IDX_0:
    {
        zgp_cluster_send_gp_proxy_table_request(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                                                ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                        ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX),
                                                NULL, 0, NULL);
        break;
    }
    case TEST_STATE_SEND_PAIRING1:
    {
        zb_zgp_sink_tbl_ent_t ent;

        ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

        ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                          ZB_ZGP_APP_ID_0000,
                          ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST,
                          ZGPD_SEQ_NUM_CAP,
                          ZGPD_RX_ON_CAP,
                          ZGPD_FIX_LOC,
                          0, /* ZGPD_USE_ASSIGNED_ALIAS */
                          ZGPD_USE_SECURITY);

        ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
        ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID;
        ent.security_counter = ZGPD_INITIAL_FRAME_COUNTER;
        ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
        ent.u.sink.match_dev_tbl_idx = 0xff;

        {
            zb_zgp_gp_pairing_send_req_t *req;

            ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(ZB_BUF_FROM_REF(buf_ref), req, &ent, NULL);
            ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 1, 0, 1);
            zgp_cluster_send_gp_pairing(buf_ref);
        }
        break;
    }
    case TEST_STATE_SEND_PAIRING2:
    {
        zb_zgp_sink_tbl_ent_t ent;

        ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

        ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                          ZB_ZGP_APP_ID_0010,
                          ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST,
                          ZGPD_SEQ_NUM_CAP,
                          ZGPD_RX_ON_CAP,
                          ZGPD_FIX_LOC,
                          0, /* ZGPD_USE_ASSIGNED_ALIAS */
                          ZGPD_USE_SECURITY);

        ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
        ZB_IEEE_ADDR_COPY(&ent.zgpd_id.ieee_addr, &g_zgpd_addr);
        ent.endpoint = 0x01;
        ent.security_counter = ZGPD_INITIAL_FRAME_COUNTER;
        ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
        ent.u.sink.match_dev_tbl_idx = 0xff;

        {
            zb_zgp_gp_pairing_send_req_t *req;

            ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(ZB_BUF_FROM_REF(buf_ref), req, &ent, NULL);
            ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 1, 0, 1);
            zgp_cluster_send_gp_pairing(buf_ref);
        }
        break;
    }
    case TEST_STATE_PROXY_TABLE_REQUEST3_IDX_1:
    {
        zgp_cluster_send_gp_proxy_table_request(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                                                ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                        ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX),
                                                NULL, 1, NULL);
        break;
    }
    case TEST_STATE_SEND_PAIRING3:
    {
        zb_zgp_sink_tbl_ent_t ent;

        ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

        ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                          ZB_ZGP_APP_ID_0000,
                          ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST,
                          ZGPD_SEQ_NUM_CAP,
                          ZGPD_RX_ON_CAP,
                          ZGPD_FIX_LOC,
                          0, /* ZGPD_USE_ASSIGNED_ALIAS */
                          ZGPD_USE_SECURITY);

        ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
        ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID2;
        ent.security_counter = ZGPD_INITIAL_FRAME_COUNTER;
        ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
        ent.u.sink.match_dev_tbl_idx = 0xff;

        {
            zb_zgp_gp_pairing_send_req_t *req;

            ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(ZB_BUF_FROM_REF(buf_ref), req, &ent, NULL);
            ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 1, 0, 1);
            zgp_cluster_send_gp_pairing(buf_ref);
        }
        break;
    }
    case TEST_STATE_PROXY_TABLE_REQUEST4_IDX_2:
    {
        zgp_cluster_send_gp_proxy_table_request(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                                                ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                        ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX),
                                                NULL, 2, NULL);
        break;
    }
    case TEST_STATE_PROXY_TABLE_REQUEST5:
    {
        zb_zgpd_id_t zgpd_id;

        zgpd_id.app_id = ZB_ZGP_APP_ID_0000;
        zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID;
        zgpd_id.endpoint = 0;
        zgp_cluster_send_gp_proxy_table_request(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                                                ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                        ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID),
                                                &zgpd_id, 0, NULL);
        break;
    }
    case TEST_STATE_PROXY_TABLE_REQUEST6:
    {
        zb_zgpd_id_t zgpd_id;

        zgpd_id.app_id = ZB_ZGP_APP_ID_0010;
        ZB_IEEE_ADDR_COPY(&zgpd_id.addr.ieee_addr, &g_zgpd_addr);
        zgpd_id.endpoint = 0x01;
        zgp_cluster_send_gp_proxy_table_request(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                                                ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0010,
                                                        ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID),
                                                &zgpd_id, 0, NULL);
        break;
    }
    case TEST_STATE_PROXY_TABLE_REQUEST7_IDX_3:
    {
        zgp_cluster_send_gp_proxy_table_request(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                                                ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                        ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX),
                                                NULL, 3, NULL);
        break;
    }
    case TEST_STATE_PROXY_TABLE_REQUEST8:
    {
        zb_zgpd_id_t zgpd_id;

        zgpd_id.app_id = ZB_ZGP_APP_ID_0000;
        zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID3;
        zgpd_id.endpoint = 0;
        zgp_cluster_send_gp_proxy_table_request(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                                                ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                        ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID),
                                                &zgpd_id, 0, NULL);
        break;
    }
    case TEST_STATE_PROXY_TABLE_REQUEST9:
    {
        zb_zgpd_id_t zgpd_id;

        zgpd_id.app_id = ZB_ZGP_APP_ID_0010;
        ZB_IEEE_ADDR_COPY(&zgpd_id.addr.ieee_addr, &g_zgpd_addr);
        zgpd_id.endpoint = 0x02;
        zgp_cluster_send_gp_proxy_table_request(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                                                ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0010,
                                                        ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID),
                                                &zgpd_id, 0, NULL);
        break;
    }
    case TEST_STATE_SINK_TABLE_REQUEST:
    {
        zgp_cluster_send_gp_sink_table_request(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                                               ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0010,
                                                       ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX),
                                               NULL, 0, NULL);
        break;
    }
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
    case TEST_STATE_FINISHED:
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    default:
        ZB_SCHEDULE_ALARM(test_send_command, param, ZB_TIME_ONE_SECOND);
    }
}

static void zgpc_custom_startup()
{
    ZB_INIT("th_tool");

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_set_default_ed_descriptor_values();

    /* Must use NVRAM for ZGP */
    ZB_AIB().aps_use_nvram = 1;

    /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 1;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

    ZB_NIB().nwk_use_multicast = ZB_FALSE;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
