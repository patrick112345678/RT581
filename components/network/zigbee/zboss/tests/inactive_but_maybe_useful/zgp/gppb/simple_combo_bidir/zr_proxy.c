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

#define ZB_TRACE_FILE_ID 41540
#include "zb_common.h"

#include "zb_ha.h"

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr tests
#endif


zb_ieee_addr_t g_zr_addr = {0xa1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xab};

#define ENDPOINT  10
#define RESTART_COMM_TIMEOUT 3 * ZB_TIME_ONE_SECOND

/******************* Declare server parameters *****************/

/******************* Declare test data & constants *************/

zb_uint8_t test_specific_cluster_cmd_handler(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

/* Init device, load IB values from nvram or set it to default */

  
  ZB_INIT("zdo_zr");


#if 0
  ZB_SET_TRACE_LEVEL(3);
  ZB_SET_TRACE_MASK(
                    TRACE_SUBSYSTEM_MAC|
                    TRACE_SUBSYSTEM_MACLL|
//                    TRACE_SUBSYSTEM_NWK|
//                    TRACE_SUBSYSTEM_APS|
//                    TRACE_SUBSYSTEM_ZDO|
                    TRACE_SUBSYSTEM_APP|
                    TRACE_SUBSYSTEM_ZCL|
                    TRACE_SUBSYSTEM_ZGP);
#endif
  zb_set_default_ffd_descriptor_values(ZB_ROUTER);

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zr_addr);
  /* rejoin to be able to debug at channel 11 */
  ZB_IEEE_ADDR_COPY(ZB_AIB().aps_use_extended_pan_id, &g_zc_addr);
  ZB_PIBCACHE_NETWORK_ADDRESS() = 0x1001;

  zb_aib_channel_page_list_set_2_4GHz_mask(1<<11);
  ZB_AIB().aps_use_nvram = 1;

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
  zb_bufid_t zcl_cmd_buf = param;
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t processed = ZB_FALSE;

  TRACE_MSG(TRACE_ZCL1, "test_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, zb_buf_len(zcl_cmd_buf)));

  switch (cmd_info->cluster_id)
  {
    case ZB_ZCL_CLUSTER_ID_ON_OFF:
      if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
      {
        switch (cmd_info->cmd_id)
        {
          case ZB_ZCL_CMD_ON_OFF_ON_ID:
            TRACE_MSG(TRACE_ZCL1, "ON", (FMT__0));
            processed = ZB_TRUE;
            break;
          case ZB_ZCL_CMD_ON_OFF_OFF_ID:
            TRACE_MSG(TRACE_ZCL1, "OFF", (FMT__0));
            processed = ZB_TRUE;
            break;
          case ZB_ZCL_CMD_ON_OFF_TOGGLE_ID:
            TRACE_MSG(TRACE_ZCL1, "TOGGLE", (FMT__0));
            processed = ZB_TRUE;
            break;
        }
      }
      break;
    case ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
      if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
      {
        TRACE_MSG(TRACE_ZCL1, "Level control cmd %hd", (FMT__H, cmd_info->cmd_id));
      }
      break;
  }
  if (processed)
  {
    zb_buf_free(param);
  }

  return processed;
}


void start_comm(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, "start commissioning again", (FMT__0));
//  zb_zgps_start_commissioning(0);
//  ZB_SCHEDULE_ALARM(start_comm, 0, RESTART_COMM_TIMEOUT);
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_bufid_t buf = param;

  TRACE_MSG(TRACE_APP1, "> zb_zdo_startup_complete %h", (FMT__H, param));

  if (zb_buf_get_status(buf) == 0)
  {
    TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
    start_comm(0);
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "Device started FAILED status %d",
        (FMT__D, (int)zb_buf_get_status(buf)));
  }
  zb_buf_free(buf);
  TRACE_MSG(TRACE_APP1, "< zb_zdo_startup_complete", (FMT__0));
}
