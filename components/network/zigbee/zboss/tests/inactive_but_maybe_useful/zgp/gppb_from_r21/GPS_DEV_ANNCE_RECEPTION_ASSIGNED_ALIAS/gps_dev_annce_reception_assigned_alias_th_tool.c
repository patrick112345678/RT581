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
/* PURPOSE: TH TOOL
*/

#define ZB_TEST_NAME GPS_DEV_ANNCE_RECEPTION_ASSIGNED_ALIAS_TH_TOOL
#define ZB_TRACE_FILE_ID 41502

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

#ifdef ZB_ENABLE_HA
#include "../include/zgp_test_templates.h"

#ifndef ZB_NSNG
static void left_btn_hndlr(zb_uint8_t param)
{
    ZVUNUSED(param);
}
#endif

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

static void send_pairing_config_delayed(zb_uint8_t param);
static void send_pairing_config(zb_uint8_t param);

static void send_dev_annce_delayed(zb_uint8_t param);
static void send_dev_annce(zb_uint8_t param);


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_tool");

    zb_set_default_ffd_descriptor_values(ZB_ROUTER);

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
    ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_secur_setup_nwk_key(g_key_nwk, 0);

    /* Must use NVRAM for ZGP */
    ZB_AIB().aps_use_nvram = 1;
    ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;

    HW_INIT();

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

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_APP1, "> zb_zdo_startup_complete %hd", (FMT__H, param));

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_ALARM(send_pairing_config_delayed, 0, TH_TOOL_PAIR_CFG_SEND_DELAY);
        ZB_SCHEDULE_ALARM(send_dev_annce_delayed, 0, TH_TOOL_DEV_ANNCE_SEND_DELAY);
    }
    else
    {
        TRACE_MSG(
            TRACE_ERROR,
            "Device started FAILED status %d",
            (FMT__D, (int)buf->u.hdr.status));
    }
    zb_free_buf(buf);
    TRACE_MSG(TRACE_APP1, "< zb_zdo_startup_complete", (FMT__0));
}

/*============================================================================*/

static void send_pairing_config_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_GET_OUT_BUF_DELAYED(send_pairing_config);
}

static void send_pairing_config(zb_uint8_t param)
{
    zb_zgp_sink_tbl_ent_t        ent;
    zb_zgp_cluster_list_t        cll;
    zb_uint8_t                   actions;
    zb_uint8_t                   app_info;
    zb_uint8_t                   num_paired_endpoints;
    zb_uint8_t                   num_gpd_commands;
    zb_uint16_t                  short_address;
    zb_uint8_t key[16] = TEST_OOB_KEY;

    TRACE_MSG(TRACE_APP1, ">send_pairing_config %hd", (FMT__H, param));

    ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

    ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                      ZB_ZGP_APP_ID_0000,
                      ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED,
                      1, /* ZGPD_SEQ_NUM_CAP */
                      0, /* ZGPD_RX_ON_CAP */
                      0, /* ZGPD_FIX_LOC */
                      1, /* ZGPD_USE_ASSIGNED_ALIAS */
                      1); /* ZGPD_USE_SECURITY */

    ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
    ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID;
    ent.security_counter = TEST_ZGPD_MAC_DSN_VALUE - 1;
    ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    ent.u.sink.match_dev_tbl_idx = 0xff;
    ent.zgpd_assigned_alias = TEST_ZGPD_ASN_ALIAS;
    ent.groupcast_radius = 0;
    ent.sec_options = ZGP_TBL_FILL_SEC_OPTIONS(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                      ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
    ZB_MEMCPY(ent.zgpd_key, key, 16);
    ent.security_counter = TEST_ZGPD_SECUR_CNT_VALUE - 1;

    actions = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_REPLACE,
              ZGP_PAIRING_CONF_SEND_PAIRING);
    app_info = ZB_ZGP_FILL_GP_PAIRING_CONF_APP_INFO(ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MANUF_ID_NO_PRESENT,
               ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MODEL_ID_NO_PRESENT,
               ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GPD_CMDS_NO_PRESENT,
               ZB_ZGP_GP_PAIRING_CONF_APP_INFO_CLSTS_NO_PRESENT);

    num_paired_endpoints = 0xfe;
    num_gpd_commands = 0;
    cll.server_cl_num = 0;
    cll.client_cl_num = 0;
    short_address = 0x0000;

    zgp_cluster_send_pairing_configuration(param,
                                           short_address,
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
                                           NULL);

    TRACE_MSG(TRACE_APP1, "<send_pairing_config", (FMT__0));
}

static void send_dev_annce_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_GET_OUT_BUF_DELAYED(send_dev_annce);
}

static void send_dev_annce(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr;
    zb_apsde_data_req_t *dreq;

    TRACE_MSG(TRACE_APP1, ">send_dev_annce %hd", (FMT__H, param));

    ZB_BUF_INITIAL_ALLOC(buf, 12, ptr);
    /* tsn */
    ZDO_CTX().tsn++;
    *ptr++ = ZDO_CTX().tsn;
    /* nwk_addr */
    zb_put_next_htole16(&ptr, TEST_ZGPD_ASN_ALIAS);
    /* ieee_addr */
    ZB_MEMCPY(ptr, g_th_tool_addr, 8);
    ptr += 8;
    /* Capability */
    *ptr = 0; /* Set capabilities to 0 */

    dreq = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsde_data_req_t));
    ZB_BZERO(dreq, sizeof(*dreq));
    dreq->dst_addr.addr_short = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    dreq->clusterid = ZDO_DEVICE_ANNCE_CLID;
    dreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    dreq->profileid = ZB_AF_ZDO_PROFILE_ID;
    zb_apsde_data_request(param);

    TRACE_MSG(TRACE_APP1, "<send_dev_annce", (FMT__0));
}

#else // defined ZB_ENABLE_HA

#include <stdio.h>
MAIN()
{
    ARGV_UNUSED;

    printf("HA profile should be enabled in zb_config.h\n");

    MAIN_RETURN(1);
}

#endif
