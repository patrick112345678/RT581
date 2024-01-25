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


#define ZB_TEST_NAME FB_DAP_TC_01B_NOCERT_TABLE_FULL_THZR1
#define ZB_TRACE_FILE_ID 41189
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

static zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

#define DUT_ENDPOINT 8
#define HA_DOOR_LOCK_ENDPOINT DUT_ENDPOINT

/********************* Declare attributes  **************************/

/* Door Lock cluster attributes data */
/* Set attributes values as Initial Conditions on the test */
static zb_uint8_t lock_state       = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED;
static zb_uint8_t lock_type        = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_TYPE_DEADBOLT;
static zb_uint8_t actuator_enabled = ZB_ZCL_ATTR_DOOR_LOCK_ACTUATOR_ENABLED_ENABLED;


ZB_ZCL_DECLARE_DOOR_LOCK_ATTRIB_LIST(door_lock_attr_list,
    &lock_state,
    &lock_type,
    &actuator_enabled);

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
ZB_HA_DECLARE_DOOR_LOCK_CLUSTER_LIST(door_lock_cluster,
                                     door_lock_attr_list,
                                     basic_attr_list,
                                     identify_attr_list,
                                     groups_attr_list);

ZB_HA_DECLARE_DOOR_LOCK_EP(door_lock_ep,
                           HA_DOOR_LOCK_ENDPOINT,
                           door_lock_cluster);

ZB_HA_DECLARE_DOOR_LOCK_CTX(door_lock_cluster_ctx, door_lock_ep);


static zb_uint16_t s_peer_addr;
static void mgmt_bind_resp_cb(zb_uint8_t param);
static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_1_zr");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr);
  ZB_BDB().bdb_primary_channel_set = (1 << 14);
  ZB_BDB().bdb_secondary_channel_set = 0;
  /* Assignment required to force Distributed formation */
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
  /* Not mandatory, but possible: set address */
  ZB_PIBCACHE_NETWORK_ADDRESS() = 0x1aaa;
  ZB_BDB().bdb_mode = 1;
  ZB_AF_REGISTER_DEVICE_CTX(&door_lock_cluster_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(DUT_ENDPOINT, zcl_specific_cluster_cmd_handler);

  /* Decrease from 180s for debug */
  ZB_BDB().bdb_commissioning_time = 20;

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


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "Successfull steering", (FMT__0));
        zb_bdb_finding_binding_target(DUT_ENDPOINT);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APS1, "Successfull F&B", (FMT__0));
        {
          zb_uint8_t i;
          zb_buf_t *buf;
          zb_zdo_mgmt_bind_param_t *req_params;

          TRACE_MSG(TRACE_ZDO3, ">>send_mgmt_bind_req: param = %i", (FMT__D, param));

          buf = ZB_BUF_FROM_REF(param);
          req_params = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_bind_param_t);
          req_params->start_index = 0;
          /* We need an address of DUT. For sure DUT is the only our neighbor. */

          i = zb_nwk_neighbor_next_rx_on_i(0);
          if (i == 0xff)
          {
            TRACE_MSG(TRACE_ERROR, "Empty neighbor!", (FMT__0));
          }
          else
          {
            zb_neighbor_tbl_ent_t *ent = ZB_NWK_NEIGHBOR_BY_I(i);
            zb_address_short_by_ref(&req_params->dst_addr, ent->u.base.addr_ref);
            s_peer_addr = req_params->dst_addr;
            zb_zdo_mgmt_bind_req(param, mgmt_bind_resp_cb);
            param = 0;
          }
        }
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
  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
}


static void mgmt_bind_resp_cb(zb_uint8_t param)
{
  zb_zdo_mgmt_bind_resp_t resp;

  TRACE_MSG(TRACE_ZDO1, ">>mgmt_bind_resp_cb: param = %i", (FMT__D, param));

  resp = *(zb_zdo_mgmt_bind_resp_t*) ZB_BUF_BEGIN(ZB_BUF_FROM_REF(param));

  TRACE_MSG(TRACE_ZDO1, "mgmt_bind_resp_cb: status = %i", (FMT__D, resp.status));

  if (resp.status == ZB_ZDP_STATUS_SUCCESS
      && resp.start_index + resp.binding_table_list_count < resp.binding_table_entries)
  {
    zb_zdo_mgmt_bind_param_t *req_params;

    TRACE_MSG(TRACE_ZDO3, ">>send_mgmt_bind_req: param = %i", (FMT__D, param));

    req_params = ZB_GET_BUF_PARAM(ZB_BUF_FROM_REF(param), zb_zdo_mgmt_bind_param_t);
    req_params->start_index = resp.start_index + resp.binding_table_list_count;
    req_params->dst_addr = s_peer_addr;
    zb_zdo_mgmt_bind_req(param, mgmt_bind_resp_cb);
    param = 0;
  }
  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
  TRACE_MSG(TRACE_ZDO1, "<<mgmt_bind_resp_cb", (FMT__0));
}


static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t cmd_info;
  zb_bool_t cmd_processed = ZB_FALSE;
  zb_zcl_attr_t*        attr_desc;

  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(zcl_cmd_buf, &cmd_info);

  if (cmd_info.cmd_direction != ZB_ZCL_FRAME_DIRECTION_TO_SRV)
  {
    /* report attr */
    cmd_processed = ZB_TRUE;
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
  else
  {
    /* Command from server to client */
    switch (cmd_info.cluster_id)
    {
      case ZB_ZCL_CLUSTER_ID_DOOR_LOCK:
      {
        if (!cmd_info.is_common_command)
        {
          attr_desc = zb_zcl_get_attr_desc_a(ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint,
                                             ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                             ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID);
          TRACE_MSG(TRACE_ZCL3, "Process specific command %hd", (FMT__H, cmd_info.cmd_id));
          switch (cmd_info.cmd_id)
          {
            case ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR:
              TRACE_MSG(TRACE_ZCL1, "Cmd: ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR", (FMT__0));
              ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED);
              ZB_ZCL_DOOR_LOCK_SEND_LOCK_DOOR_RES(
                zcl_cmd_buf,
                ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint,
                ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint,
                cmd_info.profile_id,
                cmd_info.seq_number,
                ZB_ZCL_STATUS_SUCCESS);
              cmd_processed = ZB_TRUE;
              break;

            case ZB_ZCL_CMD_DOOR_LOCK_UNLOCK_DOOR:
              TRACE_MSG(TRACE_ZCL1, "Cmd: ZB_ZCL_CMD_DOOR_LOCK_UNLOCK_DOOR", (FMT__0));
              {
                zb_uint8_t value = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_UNLOCKED;
                ZB_ZCL_SET_ATTRIBUTE(DUT_ENDPOINT, ZB_ZCL_CLUSTER_ID_DOOR_LOCK, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID, &value, ZB_FALSE);
              }

              ZB_ZCL_DOOR_LOCK_SEND_UNLOCK_DOOR_RES(
                zcl_cmd_buf,
                ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint,
                ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint,
                cmd_info.profile_id,
                cmd_info.seq_number,
                ZB_ZCL_STATUS_SUCCESS);
              cmd_processed = ZB_TRUE;
              break;

            default:
              TRACE_MSG(TRACE_ERROR, "ERROR cluster 0x%x is not supported in the test",
                        (FMT__D, cmd_info.cluster_id));
              break;
          }
        } /* if */
      }   /* switch cluster id */
    }
  }

  TRACE_MSG(
    TRACE_ZCL1,
    "< zcl_specific_cluster_cmd_handler cmd_processed %hd",
    (FMT__H, cmd_processed));
  return cmd_processed;
}


/*! @} */
