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
/* PURPOSE: Simple coordinator for GP Combo device
*/

#define ZB_TRACE_FILE_ID 41542
#include "zb_common.h"

#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK

#include "../include/simple_combo_match.h"
#include "zb_mac_globals.h"
#ifndef ZB_NSNG
#include "zb_led_button.h"
#endif
#include "zb_ha.h"
#include "zb_secur_api.h"


#include "../include/simple_combo_zcl.h"

//#if ! defined ZB_COORDINATOR_ROLE
//#error define ZB_COORDINATOR_ROLE to compile zc tests
//#endif

zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xac};
#define ENDPOINT  10
#define RESTART_COMM_TIMEOUT 3 * ZB_TIME_ONE_SECOND
#define MAX_COMM_TRY 2

static zb_uint8_t cur_commissioning_count = 0;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/********************* Declare device **************************/

ZB_HA_DECLARE_COMBO_CLUSTER_LIST( sample_clusters,
                                  basic_attr_list, identify_attr_list);

ZB_HA_DECLARE_COMBO_EP(sample_ep, ENDPOINT, sample_clusters);

ZB_HA_DECLARE_COMBO_CTX(sample_ctx, sample_ep);

/******************* Declare server parameters *****************/

/******************* Declare test data & constants *************/

zb_uint8_t test_specific_cluster_cmd_handler(zb_uint8_t param);

void start_comm(zb_uint8_t param);
void stop_comm(zb_uint8_t param);


MAIN()
{
    ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

    /* Init device, load IB values from nvram or set it to default */


    ZB_INIT("zdo_zr");


#if 0
    ZB_SET_TRACE_LEVEL(1);
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(3);
    ZB_SET_TRACE_MASK(
        TRACE_SUBSYSTEM_MAC |
        TRACE_SUBSYSTEM_MACLL |
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
    ZB_ZGP_SET_MATCH_INFO(&g_zgps_match_info);

    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&sample_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, test_specific_cluster_cmd_handler);

    /* Need to block GPDF recv if want to work thu the Proxy */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 1;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_USE_BUTTONS
    /* Left - start comm. mode */
    zb_button_register_handler(0, 0, start_comm);
    /* Right - stop comm. mode */
    zb_button_register_handler(1, 0, stop_comm);
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


zb_uint8_t test_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t processed = ZB_FALSE;
    static int state = 0;

    TRACE_MSG(TRACE_ZCL1, "test_specific_cluster_cmd_handler %i", (FMT__H, param));
    ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
    TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

    switch (cmd_info->cluster_id)
    {
    case ZB_ZCL_CLUSTER_ID_ON_OFF:
        if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
        {
            switch (cmd_info->cmd_id)
            {
            case ZB_ZCL_CMD_ON_OFF_ON_ID:
                TRACE_MSG(TRACE_ZCL1, "ON", (FMT__0));
#ifndef ZB_NSNG
                zb_osif_led_on(0);
#endif
                state = 1;
                processed = ZB_TRUE;
                break;
            case ZB_ZCL_CMD_ON_OFF_OFF_ID:
                TRACE_MSG(TRACE_ZCL1, "OFF", (FMT__0));
#ifndef ZB_NSNG
                zb_osif_led_off(0);
#endif
                processed = ZB_TRUE;
                state = 1;
                break;
            case ZB_ZCL_CMD_ON_OFF_TOGGLE_ID:
                TRACE_MSG(TRACE_ZCL1, "TOGGLE", (FMT__0));
                state = !state;
#ifndef ZB_NSNG
                if (state)
                {
                    zb_osif_led_on(0);
                }
                else
                {
                    zb_osif_led_off(0);
                }
#endif
                processed = ZB_TRUE;
                break;
            }
        }
        break;
    case ZB_ZCL_CLUSTER_ID_SCENES:
        if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
        {
            if (cmd_info->cmd_id == ZB_ZCL_CMD_SCENES_RECALL_SCENE)
            {
                zb_zcl_scenes_recall_scene_req_t *req;
                req = (zb_zcl_scenes_recall_scene_req_t *)ZB_BUF_BEGIN(zcl_cmd_buf);
                TRACE_MSG(TRACE_ZCL1, "SCENES_RECALL_SCENE scene_id %hd", (FMT__H, req->scene_id));
#ifndef ZB_NSNG
                if (req->scene_id <= 3)
                {
                    zb_int_t i;
                    for (i = 0 ; i < 2 ; ++i)
                    {
                        if (req->scene_id & (1 << i))
                        {
                            zb_osif_led_on(1 + i);
                        }
                        else
                        {
                            zb_osif_led_off(1 + i);
                        }
                    }
                }
#endif
            }
            else
            {
                TRACE_MSG(TRACE_ZCL1, "Scene cmd %hd", (FMT__H, cmd_info->cmd_id));
            }
            processed = ZB_TRUE;
        }
        break;
    case ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
        if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
        {
            TRACE_MSG(TRACE_ZCL1, "Level control cmd %hd", (FMT__H, cmd_info->cmd_id));
            processed = ZB_TRUE;
        }
        break;
    }
    if (processed)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

    return processed;
}


void comm_done_cb(zb_zgpd_id_t *zgpd_id,
                  zb_zgp_comm_status_t result)
{
    ZVUNUSED(zgpd_id);
    ZVUNUSED(result);
    TRACE_MSG(TRACE_APP1, "commissioning stopped", (FMT__0));
#ifdef ZB_USE_BUTTONS
    zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif

    if (cur_commissioning_count < MAX_COMM_TRY)
    {
        ZB_SCHEDULE_ALARM(start_comm, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));
    }
}


void start_comm(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, "start commissioning", (FMT__0));
    ZB_ZGP_REGISTER_COMM_COMPLETED_CB(comm_done_cb);

    cur_commissioning_count++;

    zb_zgps_start_commissioning(0);
#ifdef ZB_USE_BUTTONS
    zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
}


void stop_comm(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, "stop commissioning", (FMT__0));
    zb_zgps_stop_commissioning();
#ifdef ZB_USE_BUTTONS
    zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
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
        ZB_SCHEDULE_ALARM(start_comm, 0,
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
