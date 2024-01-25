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


#define ZB_TEST_NAME FB_DAP_TC_01A_THZR
#define ZB_TRACE_FILE_ID 41194
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

#define TH_ENDPOINT 8
#define HA_DOOR_LOCK_CONTROLLER_ENDPOINT TH_ENDPOINT

static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);


/** [COMMON_DECLARATION] */
/********************* Declare attributes  **************************/
/* Basic cluster attributes data */
static zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
static zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t g_attr_identify_time = 0;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Groups cluster attributes data */
static zb_uint8_t g_attr_name_support = 0;
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

/********************* Declare device **************************/
/*
ZB_HA_DECLARE_DOOR_LOCK_CONTROLLER_CLUSTER_LIST(door_lock_controller_cluster,
    door_lock_attr_list,
    basic_attr_list,
    identify_attr_list,
    groups_attr_list);
*/
ZB_HA_DECLARE_DOOR_LOCK_CONTROLLER_CLUSTER_LIST(door_lock_controller_cluster,
    basic_attr_list,
    identify_attr_list,
    groups_attr_list);


ZB_HA_DECLARE_DOOR_LOCK_CONTROLLER_EP(door_lock_controller_ep,
                                      HA_DOOR_LOCK_CONTROLLER_ENDPOINT,
                                      door_lock_controller_cluster);


ZB_HA_DECLARE_DOOR_LOCK_CONTROLLER_CTX(door_lock_controller_cluster_ctx,
                                       door_lock_controller_ep);
/** [COMMON_DECLARATION] */


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_2_zr");


  /* Pass verdict is: broadcast Beacon request at all channels */
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr);
  ZB_BDB().bdb_primary_channel_set = (1 << 14);
  ZB_BDB().bdb_secondary_channel_set = 0;
  ZB_BDB().bdb_mode = 1;
  ZB_AF_REGISTER_DEVICE_CTX(&door_lock_controller_cluster_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(TH_ENDPOINT, zcl_specific_cluster_cmd_handler);
  ZB_BDB().bdb_commissioning_group_id = 7;

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


static zb_bool_t finding_binding_cb(zb_int16_t status, zb_ieee_addr_t addr, zb_uint8_t ep, zb_uint16_t cluster)
{
  TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
            (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
  return ZB_TRUE;
}


static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t cmd_processed = ZB_FALSE;


  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %hd", (FMT__H, param));

  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_ZCL1, "payload size: %hd", (FMT__H, ZB_BUF_LEN(zcl_cmd_buf)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
  {
    TRACE_MSG(TRACE_ZCL1, "Skip command, Unsupported direction \"to server\"", (FMT__0));
  }
  else
  {
    /* Command from server to client */
    switch (cmd_info->cluster_id)
    {
      case ZB_ZCL_CLUSTER_ID_GROUPS:
      {
        if (!cmd_info->is_common_command)
        {
          TRACE_MSG(TRACE_ZCL3, "Process specific command %hd", (FMT__H, cmd_info->cmd_id));
          switch (cmd_info->cmd_id)
          {
            case ZB_ZCL_CMD_GROUPS_ADD_GROUP_RES:
            {
              zb_zcl_groups_add_group_res_t *add_group_res;
              zb_uint8_t *cmd_ptr;
              zb_uint16_t dest_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
              zb_uint16_t dest_endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;

              TRACE_MSG(TRACE_ZCL1, "get add group response", (FMT__0));
              ZB_ZCL_GROUPS_GET_ADD_GROUP_RES(zcl_cmd_buf, add_group_res);

              TRACE_MSG(TRACE_ZCL1, "Response status %hd, group_id %d",
                        (FMT__H_D, add_group_res->status, add_group_res->group_id));
              if (add_group_res->status != ZB_ZCL_STATUS_SUCCESS ||
                  add_group_res->group_id != ZB_BDB().bdb_commissioning_group_id)
              {
                TRACE_MSG(TRACE_ERROR, "ERROR incorrect response for ADD_GROUP received", (FMT__0));
                zb_free_buf(ZB_BUF_FROM_REF(param));
              }
              else
              {
                TRACE_MSG(TRACE_ZCL3, "Sending GROUPS_GET_GROUP_MEMBERSHIP_REQ", (FMT__0));

                ZB_ZCL_GROUPS_INIT_GET_GROUP_MEMBERSHIP_REQ(zcl_cmd_buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 0);
                ZB_ZCL_GROUPS_SEND_GET_GROUP_MEMBERSHIP_REQ(zcl_cmd_buf, cmd_ptr, dest_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, dest_endpoint,
                                                            TH_ENDPOINT, ZB_AF_HA_PROFILE_ID, NULL);
              }
              cmd_processed = ZB_TRUE;
              break;
            }

            case ZB_ZCL_CMD_GROUPS_GET_GROUP_MEMBERSHIP_RES:
            {
              zb_zcl_groups_get_group_membership_res_t *group_member_res;
              zb_int_t i;
              /** [Parse Get Group Membership response] */
              ZB_ZCL_GROUPS_GET_GROUP_MEMBERSHIP_RES(ZB_BUF_FROM_REF(param), group_member_res);
              /** [Parse Get Group Membership response] */

              if (group_member_res)
              {
                TRACE_MSG(TRACE_ZCL1, "Got GROUPS_GET_GROUP_MEMBERSHIP_RES: capacity %hd, group_count %hd",
                          (FMT__H_H, group_member_res->capacity, group_member_res->group_count));

                for(i = 0; i < group_member_res->group_count; i++)
                {
                  TRACE_MSG(TRACE_ZCL1, "group id %d", (FMT__D, group_member_res->group_id[i]));
                }
              }
              else
              {
                TRACE_MSG(TRACE_ERROR, "Error, incorrect GROUP_MEMBERSHIP_RES received", (FMT__0));
              }
              cmd_processed = ZB_TRUE;
              zb_free_buf(ZB_BUF_FROM_REF(param));
            }
            break;
            default:
              TRACE_MSG(TRACE_ERROR, "ERROR unknow cmd received", (FMT__0));
              break;

          }
          break;
        } /* if */

        default:
          TRACE_MSG(TRACE_ERROR, "ERROR cluster 0x%x is not supported in the test",
                    (FMT__D, cmd_info->cluster_id));
          break;
      }
    } /* if */
  }   /* switch cluster id */

  TRACE_MSG(
    TRACE_ZCL1,
    "< zcl_specific_cluster_cmd_handler cmd_processed %hd",
    (FMT__H, cmd_processed));
  return cmd_processed;
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        zb_bdb_finding_binding_initiator(TH_ENDPOINT, finding_binding_cb);
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "Successfull steering", (FMT__0));
        zb_bdb_finding_binding_initiator(TH_ENDPOINT, finding_binding_cb);
        break;

      default:
        TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }
  zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
