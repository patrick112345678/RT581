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
/* PURPOSE: Commissioning tool for GP Combo device
*/

#define ZB_TRACE_FILE_ID 41558
#include "zb_common.h"

#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK

#include "../include/simple_combo_match.h"
#include "zb_mac_globals.h"
#ifndef ZB_NSNG
#include "zb_led_button.h"
#endif
#include "zb_ha.h"
#include "zb_secur_api.h"
#include "zgp/zgp_internal.h"

#include "test_config.h"

#include "../include/simple_combo_zcl.h"

//#if ! defined ZB_COORDINATOR_ROLE
//#error define ZB_COORDINATOR_ROLE to compile zc tests
//#endif

zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xac};
#define ENDPOINT  10

zb_uint8_t   g_zgpd_key[] = TEST_OOB_KEY;

MAIN()
{
    ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

    /* Init device, load IB values from nvram or set it to default */


    ZB_INIT("ct");


#if 1
    ZB_SET_TRACE_LEVEL(3);
    ZB_SET_TRACE_MASK(
        //                    TRACE_SUBSYSTEM_MAC|
        //                    TRACE_SUBSYSTEM_MACLL|
        //                    TRACE_SUBSYSTEM_NWK|
        //                    TRACE_SUBSYSTEM_APS|
        //                    TRACE_SUBSYSTEM_ZDO|
        TRACE_SUBSYSTEM_APP |
        TRACE_SUBSYSTEM_ZCL |
        TRACE_SUBSYSTEM_ZGP);
#endif

    //  zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);
    zb_set_default_ffd_descriptor_values(ZB_ROUTER);

    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);
    ZB_PIBCACHE_PAN_ID() = 0x1aab;

    ZB_AIB().aps_designated_coordinator = 0;
    ZB_AIB().aps_channel_mask = (1 << 11);
    /* Must use NVRAM for ZGP */
    ZB_AIB().aps_use_nvram = 1;
    /* use well-known key to simplify decrypt in Wireshark */
    zb_secur_setup_nwk_key(g_key_nwk, 0);

    ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;

    /* Need to block GPDF recv if want to work thu the Proxy */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 1;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_USE_BUTTONS
    /* Left - start comm. mode */
    //  zb_button_register_handler(0, 0, start_comm);
    /* Right - stop comm. mode */
    //  zb_button_register_handler(1, 0, stop_comm);
#endif
#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zcl_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

void commissioning_sink(zb_uint8_t param)
{
    if (param == 0)
    {
        ZB_GET_OUT_BUF_DELAYED(commissioning_sink);
    }
    else
    {
        zgp_cluster_send_gp_sink_commissioning_mode(param,
                0x0000,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_ZGP_GP_SINK_COMM_MODE_FILL_OPT(
                    1, 0, 0, 1),
                0xff,
                NULL);
    }
}

void wait_for_comm(zb_uint8_t param)
{
    zb_free_buf(ZB_BUF_FROM_REF(param));
    ZB_SCHEDULE_ALARM(commissioning_sink, 0,
                      ZB_MILLISECONDS_TO_BEACON_INTERVAL(2000));
}

void configure_sink(zb_uint8_t param)
{
    if (param == 0)
    {
        ZB_GET_OUT_BUF_DELAYED(configure_sink);
    }
    else
    {
        zb_zgp_sink_tbl_ent_t ent;
        zb_zgp_cluster_list_t cll;
        zb_uint8_t            actions;
        zb_uint8_t            app_info;
        zb_uint8_t            num_paired_endpoints;
        zb_uint8_t            num_gpd_commands;

        actions = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_REPLACE,
                  ZGP_PAIRING_CONF_SEND_PAIRING);

        app_info = ZB_ZGP_FILL_GP_PAIRING_CONF_APP_INFO(ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MANUF_ID_NO_PRESENT,
                   ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MODEL_ID_NO_PRESENT,
                   ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GPD_CMDS_NO_PRESENT,
                   ZB_ZGP_GP_PAIRING_CONF_APP_INFO_CLSTS_NO_PRESENT);

        ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

#define ZGPD_SEQ_NUM_CAP 1
#define ZGPD_RX_ON_CAP 0
#define ZGPD_FIX_LOC 1
#define ZGPD_USE_ASSIGNED_ALIAS 0
#define ZGPD_USE_SECURITY 1

        ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                          ZB_ZGP_APP_ID_0000,
                          //ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST,
                          //ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED,
                          ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED,
                          ZGPD_SEQ_NUM_CAP,
                          ZGPD_RX_ON_CAP,
                          ZGPD_FIX_LOC,
                          ZGPD_USE_ASSIGNED_ALIAS,
                          ZGPD_USE_SECURITY);
        ent.sec_options = ZGP_TBL_FILL_SEC_OPTIONS(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                          ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);

        ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
        ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID;
        ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
        ent.u.sink.match_dev_tbl_idx = 0xff;
        ent.zgpd_assigned_alias = 0x5678;

        ent.u.sink.sgrp[0].sink_group = 0x5678;
        ent.u.sink.sgrp[0].alias = 0x5678;

        num_paired_endpoints = 0;

        num_gpd_commands = 0;

        cll.server_cl_num = 0;
        cll.client_cl_num = 0;

        ZB_MEMCPY(ent.zgpd_key, g_zgpd_key, ZB_CCM_KEY_SIZE);

        zgp_cluster_send_pairing_configuration(param,
                                               0x0000,
                                               ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                               actions,
                                               &ent,
                                               num_paired_endpoints,
                                               NULL,
                                               app_info,
                                               0,
                                               0,
                                               num_gpd_commands,
                                               NULL,
                                               &cll,
                                               wait_for_comm);
    }
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_APP1, "> zb_zdo_startup_complete %h", (FMT__H, param));

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        zb_free_buf(buf);
#ifdef ZB_NSNG
        ZB_SCHEDULE_ALARM(configure_sink, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(2000));
#endif
    }
    else
    {
        TRACE_MSG(
            TRACE_ERROR,
            "Device started FAILED status %d",
            (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
    }
    TRACE_MSG(TRACE_APP1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK

#include <stdio.h>
MAIN()
{
    ARGV_UNUSED;

    printf("HA profile and ZGP sink role should be enabled in zb_config.h\n");

    MAIN_RETURN(1);
}

#endif // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK
