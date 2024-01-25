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

#define ZB_TEST_NAME GPS_COMMISSIONING_BIDIR_OOB_KEY_APP_000_TH_TOOL
#define ZB_TRACE_FILE_ID 41507

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_SET_SEC_KEY_TYPE,
    TEST_STATE_SET_SEC_KEY,
    TEST_STATE_READ_GPS_SINK_TABLE,
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    zb_uint8_t new_shared_key_type = ZB_ZGP_SEC_KEY_TYPE_NO_KEY;
    zb_uint8_t new_key[16];

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SET_SEC_KEY_TYPE:
        zgp_cluster_write_attr(buf_ref, TH_GPS_ADDR, TH_GPS_ADDR_MODE,
                               ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_TYPE_ID,
                               ZB_ZCL_ATTR_TYPE_8BITMAP,
                               &new_shared_key_type,
                               ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        ZB_ZGPC_SET_PAUSE(2);
        break;
    case TEST_STATE_SET_SEC_KEY:
        ZB_MEMSET(new_key, 0, 16);
        zgp_cluster_write_attr(buf_ref, TH_GPS_ADDR, TH_GPS_ADDR_MODE,
                               ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_ID,
                               ZB_ZCL_ATTR_TYPE_128_BIT_KEY,
                               new_key,
                               ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        ZB_ZGPC_SET_PAUSE(20);
        break;
    case TEST_STATE_READ_GPS_SINK_TABLE:
        zgp_cluster_read_attr(buf_ref, TH_GPS_ADDR, TH_GPS_ADDR_MODE,
                              ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        ZB_ZGPC_SET_PAUSE(2);
        break;
    };
}

static void handle_gp_sink_table_response(zb_uint8_t buf_ref)
{
    zb_bool_t   test_error = ZB_FALSE;
    zb_buf_t   *buf = ZB_BUF_FROM_REF(buf_ref);
    zb_uint8_t *ptr = ZB_BUF_BEGIN(buf);

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_GPS_SINK_TABLE:
    {
        zb_uint8_t status;

        ZB_ZCL_PACKET_GET_DATA8(&status, ptr);

        if (status != ZB_ZCL_STATUS_SUCCESS)
        {
            test_error = ZB_TRUE;
        }
    }
    break;
    }

    if (test_error)
    {
        TEST_DEVICE_CTX.err_cnt++;
        TRACE_MSG(TRACE_APP1, "Error at state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));
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
        if (TEST_DEVICE_CTX.err_cnt)
        {
            TRACE_MSG(TRACE_APP1, "Test finished. Status: ERROR[%hd]", (FMT__H, TEST_DEVICE_CTX.err_cnt));
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        }
        break;
    default:
    {
        if (param)
        {
            zb_free_buf(ZB_BUF_FROM_REF(param));
        }
        ZB_SCHEDULE_ALARM(test_send_command, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
    }
    }
}

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_tool");

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_set_default_ed_descriptor_values();
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 1;
#endif
    ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;

    TEST_DEVICE_CTX.gp_sink_tbl_req_cb = handle_gp_sink_table_response;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
