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
/* PURPOSE: TH-GPP/TH-TOOL
*/

#define ZB_TEST_NAME GPS_DTCLK_JOINING_INV_TC_SET_BY_ATTR_TH_GPP

#define ZB_TRACE_FILE_ID 63480
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zb_secur_api.h"
#include "zb_ha.h"
#include "test_config.h"
#include "zgp/zgp_internal.h"

#ifdef ZB_ENABLE_HA
#include "../include/zgp_test_templates.h"

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_th_gpp_addr = TH_GPP_IEEE_ADDR;
static zb_uint16_t g_dut_addr;

static void dev_annce_cb(zb_zdo_device_annce_t *da);
static void zgpc_send_pairing_config_cb(zb_uint8_t buf_ref);
static void zgpc_send_pairing_config(zb_uint8_t buf_ref);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_WAIT_DUT_JOINING,
    TEST_STATE_READ_OUT_DUT_GPS_SECUR_LVL_1,
    TEST_STATE_WRITE_DUT_GPS_SECUR_LVL,
    TEST_STATE_READ_OUT_DUT_GPS_SECUR_LVL_2,
    TEST_STATE_WAIT_2,
    TEST_STATE_READ_DUT_SINK_TABLE1,
    TEST_STATE_UNICAST_SINK_COMM_MODE_TO_DUT,
    TEST_STATE_WAIT_3,
    TEST_STATE_READ_DUT_SINK_TABLE2,
    TEST_STATE_UNICAST_GP_PAIRING_CONFIG,
    TEST_STATE_READ_DUT_SINK_TABLE3,
    /* FINISH */
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    TRACE_MSG(TRACE_APP1, ">send_zcl: test_state = %d", (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_OUT_DUT_GPS_SECUR_LVL_1:
    case TEST_STATE_READ_OUT_DUT_GPS_SECUR_LVL_2:
    {
        zgp_cluster_read_attr(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                              ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    }

    case TEST_STATE_WRITE_DUT_GPS_SECUR_LVL:
    {
        zb_uint8_t new_secur_lvl = 0x0E;
        zgp_cluster_write_attr(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                               ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID,
                               ZB_ZCL_ATTR_TYPE_8BITMAP,
                               &new_secur_lvl,
                               ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    }

    case TEST_STATE_READ_DUT_SINK_TABLE1:
    case TEST_STATE_READ_DUT_SINK_TABLE2:
    case TEST_STATE_READ_DUT_SINK_TABLE3:
    {
        zgp_cluster_read_attr(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                              ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;
    }
    case TEST_STATE_UNICAST_SINK_COMM_MODE_TO_DUT:
    {
        zb_uint8_t options = 0x09; /* Action = Enter; Involve Proxies */
        zgp_cluster_send_gp_sink_commissioning_mode(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                options, DUT_ENDPOINT, cb);
        break;
    }
    case TEST_STATE_UNICAST_GP_PAIRING_CONFIG:
    {
        zgpc_send_pairing_config(buf_ref);
        break;
    }
    }

    TRACE_MSG(TRACE_APP1, "<send_zcl", (FMT__0));
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

    TRACE_MSG(TRACE_APP1, ">perform_next_state: test_state = %d",
              (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAIT_DUT_JOINING:
        TRACE_MSG(TRACE_APP1, "Wait for DUT joining.", (FMT__0));
        break;

    case TEST_STATE_WAIT_2:
        ZB_ZGPC_SET_PAUSE(TEST_STEP1_TH_DELAY);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    case TEST_STATE_WAIT_3:
        ZB_ZGPC_SET_PAUSE(TEST_STEP2_TH_DELAY);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
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

    TRACE_MSG(TRACE_APP1, "<perform_next_state", (FMT__0));
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
    zb_ieee_addr_t th_ieee = DUT_GPS_IEEE_ADDR;
    TRACE_MSG(TRACE_APP1, "dev_annce_cb: ieee = " TRACE_FORMAT_64 " NWK = 0x%x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

    if (ZB_IEEE_ADDR_CMP(th_ieee, da->ieee_addr) == ZB_TRUE)
    {
        if (TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_DUT_JOINING)
        {
            g_dut_addr = da->nwk_addr;
            PERFORM_NEXT_STATE(0);
        }
    }
}

static void zgpc_send_pairing_config_cb(zb_uint8_t buf_ref)
{
    zb_free_buf(ZB_BUF_FROM_REF(buf_ref));
    ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, 2 * ZB_TIME_ONE_SECOND);
}

static void zgpc_send_pairing_config(zb_uint8_t buf_ref)
{
    zb_uint8_t actions = 0x09; /* Send GP Pairing + Extend Sink Table */
    zb_zgp_sink_tbl_ent_t ent;
    zb_uint8_t key[] = TEST_OOB_KEY;

    ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

    ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                      ZB_ZGP_APP_ID_0000,
                      ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED,
                      ZB_TRUE, /* Seq Num capabilities */
                      ZB_FALSE, /* Rx On Capabilities */
                      ZB_TRUE, /* Fixed location */
                      ZB_FALSE, /* Assigned Alias */
                      ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);

    ent.sec_options = ZB_ZGP_SEC_LEVEL_FULL_NO_ENC | (ZB_ZGP_SEC_KEY_TYPE_NWK << 2);

    ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
    ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID;
    ent.security_counter = 1;
    ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    ent.u.sink.match_dev_tbl_idx = 0xff;
    ent.groupcast_radius = 5;
    ZB_MEMCPY(ent.zgpd_key, key, ZB_CCM_KEY_SIZE);

    zgp_cluster_send_pairing_configuration(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                           actions, &ent, 0xFE /* number of paired endpoints */, NULL,
                                           0, 0, 0, 0, NULL, NULL,
                                           zgpc_send_pairing_config_cb);
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_gpp");

    zb_set_long_address(g_th_gpp_addr);
    zb_set_network_router_role(1l << TEST_CHANNEL);
    ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
    zb_set_max_children(0);

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

    ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
    ZGP_CTX().device_role = ZGP_DEVICE_PROXY;
    //TEST_DEVICE_CTX.gp_pairing_hndlr_cb = gp_pairing_handler_cb;
    zb_zdo_register_device_annce_cb(dev_annce_cb);
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()

#else // defined ZB_ENABLE_HA

#include <stdio.h>
MAIN()
{
    ARGV_UNUSED;

    printf("HA profile should be enabled in zb_config.h\n");

    MAIN_RETURN(1);
}

#endif
