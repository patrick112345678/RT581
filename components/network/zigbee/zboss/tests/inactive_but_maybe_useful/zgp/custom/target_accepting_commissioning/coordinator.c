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
/* PURPOSE: Simple coordinator for GP device
*/

#define ZB_TRACE_FILE_ID 41568
#include "zb_common.h"

#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK

#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zboss_api_zgp.h"
#include "zb_zcl.h"
#include "match_info.h"

#include "zb_ha.h"

#include "sample_controller.h"

#define COMMISSIONING_TIMEOUT  0
#define DECOMMISSIONING_TIMEOUT 0

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif


zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
#define ENDPOINT  10

#define ZGP_TRANSL_TBL_ADD_ENTRY(ent, id, cmd_id, ep, profile, clstr_id, zb_cmd_id, pld) \
{ \
  ent->options = id.app_id; \
  ent->zgpd_id = id.addr; \
  ent->zgpd_cmd_id = cmd_id; \
  ent->endpoint = ep; \
  ent->Zigbee_profile_id = profile; \
  ent->cluster_id = clstr_id; \
  ent->Zigbee_cmd_id = zb_cmd_id; \
  ent->payload = pld; \
}
/* Handler for specific zcl commands */
zb_uint8_t sample_specific_cluster_cmd_handler(zb_uint8_t param);

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/********************* Declare device **************************/

ZB_HA_DECLARE_SAMPLE_CLUSTER_LIST( sample_clusters,
                                   basic_attr_list, identify_attr_list);

ZB_HA_DECLARE_SAMPLE_EP(sample_ep, ENDPOINT, sample_clusters);

ZB_HA_DECLARE_SAMPLE_CTX(sample_ctx, sample_ep);

void start_commissioning(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_zgps_start_commissioning(COMMISSIONING_TIMEOUT);
}


void commissioning_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
    TRACE_MSG(TRACE_SPECIAL2, ">> commissioning_cb result %d", (FMT__D, result));

    if (result == ZB_ZGP_ZGPD_DECOMMISSIONED)
    {
        TRACE_MSG(TRACE_APP1, "Decommissioning completed. ZGPD Src ID: %d",
                  (FMT__D, zgpd_id->addr.src_id));
    }
    else if (result == ZB_ZGP_COMMISSIONING_COMPLETED)
    {
        TRACE_MSG(TRACE_APP1, "Commissioning completed. ZGPD Src ID: %d",
                  (FMT__D, zgpd_id->addr.src_id));
    }

    ZB_SCHEDULE_ALARM(start_commissioning, 0, 1 * ZB_TIME_ONE_SECOND);

    TRACE_MSG(TRACE_APP1, "<< commissioning_cb ", (FMT__0));
}


void comm_req_cb(
    zb_zgpd_id_t  *zgpd_id,
    zb_uint8_t    device_id,
    zb_uint16_t   manuf_id,
    zb_uint16_t   manuf_model_id)
{
    TRACE_MSG(TRACE_APP1, ">> comm_req_cb zgpd_id %p, dev_id 0x%hx, manuf_id %d, manuf_model_id 0x%x",
              (FMT__P_H_D_H, zgpd_id, device_id, manuf_id, manuf_model_id));

    ZVUNUSED(manuf_id);
    ZVUNUSED(manuf_model_id);

    zb_zgps_accept_commissioning(ZB_TRUE);

    TRACE_MSG(TRACE_APP1, "<< comm_req_cb ", (FMT__0));
}


void report_attribute_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                         zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value);

MAIN()
{
    ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zc");



    zb_set_default_ed_descriptor_values();

    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);
    ZB_PIBCACHE_PAN_ID() = 0x1aaa;

    ZB_AIB().aps_designated_coordinator = 1;

    ZB_ZGP_SET_MATCH_INFO(&g_zgps_match_info);

    ZB_ZGP_REGISTER_COMM_REQ_CB(comm_req_cb);
    ZB_ZGP_REGISTER_COMM_COMPLETED_CB(commissioning_cb);
    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&sample_ctx);

    ZB_ZCL_SET_REPORT_ATTR_CB(&report_attribute_cb);

    zb_zgp_transl_tbl_ent_t *ent;
    zb_zgpd_id_t zgpd_id = {ZB_ZGP_APP_ID_0000, ZB_ZGP_SRC_ID_ALL};
    transl_tbl_cmd_pld_t pld = { 0, {0} };

    ent = zb_zgp_grab_transl_tbl_entry(&ZGP_CTX().transl_tbl);
    ZGP_TRANSL_TBL_ADD_ENTRY(
        ent,
        zgpd_id,
        ZB_GPDF_CMD_TOGGLE,
        ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_CLUSTER_ID_ON_OFF,
        ZB_ZCL_CMD_ON_OFF_TOGGLE_ID,
        pld);

    ent = zb_zgp_grab_transl_tbl_entry(&ZGP_CTX().transl_tbl);
    ZGP_TRANSL_TBL_ADD_ENTRY(
        ent,
        zgpd_id,
        ZB_GPDF_CMD_MOVE_STEP_ON,
        ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
        ZB_ZCL_LEVEL_CONTROL_MOVE_MODE_UP,
        pld);

    ent = zb_zgp_grab_transl_tbl_entry(&ZGP_CTX().transl_tbl);
    ZGP_TRANSL_TBL_ADD_ENTRY(
        ent,
        zgpd_id,
        ZB_GPDF_CMD_MOVE_STEP_OFF,
        ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
        ZB_ZCL_LEVEL_CONTROL_MOVE_MODE_DOWN,
        pld);

    ent = zb_zgp_grab_transl_tbl_entry(&ZGP_CTX().transl_tbl);
    ZGP_TRANSL_TBL_ADD_ENTRY(
        ent,
        zgpd_id,
        ZB_GPDF_CMD_MULTI_CLUSTER_ATTR_REPORT,
        ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
        ZB_ZCL_CMD_REPORT_ATTRIB,
        pld);

    ent = zb_zgp_grab_transl_tbl_entry(&ZGP_CTX().transl_tbl);
    ZGP_TRANSL_TBL_ADD_ENTRY(
        ent,
        zgpd_id,
        ZB_GPDF_CMD_MULTI_CLUSTER_ATTR_REPORT,
        ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
        ZB_ZCL_CMD_REPORT_ATTRIB,
        pld);


    ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, sample_specific_cluster_cmd_handler);

    ZB_SET_NIB_SECURITY_LEVEL(0);

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

void report_attribute_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                         zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value)
{
    ZVUNUSED(value);
    ZVUNUSED(addr);
    ZVUNUSED(attr_type);
    TRACE_MSG(TRACE_APP1, ">> report_attribute_cb ep %hd, cluster %d, attr %d",
              (FMT__H_D_D, ep, cluster_id, attr_id));

    TRACE_MSG(TRACE_APP1, "<< report_attribute_cb", (FMT__0));
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_APP1, "> zb_zdo_startup_complete %h", (FMT__H, param));

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        zb_zgps_start_commissioning(COMMISSIONING_TIMEOUT);
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

zb_uint8_t sample_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t ret = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> sample_specific_cluster_cmd_handler %i", (FMT__H, param));
    ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
    TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

    if ( cmd_info -> cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI )
    {
        switch ( cmd_info -> cluster_id )
        {
        case ZB_ZCL_CLUSTER_ID_ON_OFF:
            if ( !cmd_info -> is_common_command )
            {
                switch ( cmd_info -> cmd_id )
                {
                case ZB_ZCL_CMD_ON_OFF_TOGGLE_ID:
                    TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_CMD_IAS_ZONE_ZONE_STATUS_CHANGE_NOT_ID", (FMT__0));
                    break;

                default:
                    break;
                }
            }
            break;

        case ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
            if ( !cmd_info -> is_common_command )
            {
                switch ( cmd_info -> cmd_id )
                {
                case ZB_ZCL_LEVEL_CONTROL_MOVE_MODE_UP:
                    TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_CMD_ON_OFF_ON_ID", (FMT__0));
                    break;

                case ZB_ZCL_LEVEL_CONTROL_MOVE_MODE_DOWN:
                    TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_CMD_ON_OFF_OFF_ID", (FMT__0));
                    break;

                default:
                    break;
                }
            }
            break;

        case ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT:
        case ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT:
        {
            zb_zcl_report_attr_req_t *rep_attr_req;

            ZB_ZCL_GENERAL_GET_NEXT_REPORT_ATTR_REQ(zcl_cmd_buf, rep_attr_req);
        }
        break;

        default:
            break;
        }
    }

    TRACE_MSG(TRACE_ZCL1, "<< sample_specific_cluster_cmd_handler", (FMT__0));
    return ret;
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
