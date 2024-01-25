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
/* PURPOSE: Test for testing ReadAttr from GreenPower cluster
*/

#define ZB_TRACE_FILE_ID 41554
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "../zgp/zgp_cluster.h"
#include "../zgp/zgp_helper.h"

#include "zb_ha.h"

#include "test_config.h"

void next_step(zb_buf_t * zcl_cmd_buf);

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);


#if defined ZB_ENABLE_HA

//zb_uint16_t g_zc_addr = 0;
zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READ_MAX_PROXY_TABLE_ENTRIES1,
  TEST_STATE_WRITE_MAX_PROXY_TABLE_ENTRIES,
  TEST_STATE_READ_MAX_PROXY_TABLE_ENTRIES2,
  TEST_STATE_READ_PROXY_TABLE1,
  TEST_STATE_WRITE_PROXY_TABLE,
  TEST_STATE_READ_PROXY_TABLE2,
  TEST_STATE_READ_FUNCTIONALITY1,
  TEST_STATE_WRITE_FUNCTIONALITY,
  TEST_STATE_READ_FUNCTIONALITY2,
  TEST_STATE_READ_ACTIVE_FUNCTIONALITY1,
  TEST_STATE_WRITE_ACTIVE_FUNCTIONALITY,
  TEST_STATE_READ_ACTIVE_FUNCTIONALITY2,
  TEST_STATE_FINISHED
};

/*! @brief Test harness state
    Takes values of @ref test_states_e
*/
//warning: first state TEST_STATE_INITIAL cause assertion failed error!
zb_uint8_t g_test_state = TEST_STATE_READ_MAX_PROXY_TABLE_ENTRIES1;
zb_short_t g_error_cnt = 0;

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("th_tool");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);


  zb_set_default_ed_descriptor_values();

  zgp_cluster_set_app_zcl_cmd_handler(zcl_specific_cluster_cmd_handler);

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

/*! Test step */
void next_step(zb_buf_t * zcl_cmd_buf)
{
  zb_uint32_t val;
  zb_uint8_t *attr_val;

  TRACE_MSG(TRACE_ZCL1, "> next_step: zcl_cmd_buf: %p, state %d",
            (FMT__P_D, zcl_cmd_buf, g_test_state));

  if (g_test_state == TEST_STATE_READ_MAX_PROXY_TABLE_ENTRIES1 ||
      g_test_state == TEST_STATE_READ_MAX_PROXY_TABLE_ENTRIES2)
  {
    zgp_cluster_read_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), DUT_GPP_ADDR, DUT_GPP_ADDR_MODE,
                          ZB_ZCL_ATTR_GPP_MAX_PROXY_TABLE_ENTRIES_ID,
                          ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
  }
  else
  if (g_test_state == TEST_STATE_WRITE_MAX_PROXY_TABLE_ENTRIES)
  {
    val = 0x0a;
    attr_val = (zb_uint8_t*)&val;
    zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), DUT_GPP_ADDR, DUT_GPP_ADDR_MODE,
                           ZB_ZCL_ATTR_GPP_MAX_PROXY_TABLE_ENTRIES_ID, ZB_ZCL_ATTR_TYPE_U8, attr_val,
                           ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
  }
  else
  if (g_test_state == TEST_STATE_READ_PROXY_TABLE1 ||
      g_test_state == TEST_STATE_READ_PROXY_TABLE2)
  {
    zgp_cluster_read_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), DUT_GPP_ADDR, DUT_GPP_ADDR_MODE,
                          ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                          ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
  }
  else
  if (g_test_state == TEST_STATE_WRITE_PROXY_TABLE)
  {
    val = 0x0000;
    attr_val = (zb_uint8_t*)&val;
    zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), DUT_GPP_ADDR, DUT_GPP_ADDR_MODE,
                           ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID, ZB_ZCL_ATTR_TYPE_LONG_OCTET_STRING, attr_val,
                           ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
  }
  if (g_test_state == TEST_STATE_READ_FUNCTIONALITY1 ||
      g_test_state == TEST_STATE_READ_FUNCTIONALITY2)
  {
    zgp_cluster_read_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), DUT_GPP_ADDR, DUT_GPP_ADDR_MODE,
                          ZB_ZCL_ATTR_GPP_FUNCTIONALITY_ID,
                          ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
  }
  else
  if (g_test_state == TEST_STATE_WRITE_FUNCTIONALITY)
  {
    val = 0xffff;
    attr_val = (zb_uint8_t*)&val;
    zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), DUT_GPP_ADDR, DUT_GPP_ADDR_MODE,
                           ZB_ZCL_ATTR_GPP_FUNCTIONALITY_ID, ZB_ZCL_ATTR_TYPE_24BITMAP, attr_val,
                           ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
  }
  else
  if (g_test_state == TEST_STATE_READ_ACTIVE_FUNCTIONALITY1 ||
      g_test_state == TEST_STATE_READ_ACTIVE_FUNCTIONALITY2)
  {
    zgp_cluster_read_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), DUT_GPP_ADDR, DUT_GPP_ADDR_MODE,
                          ZB_ZCL_ATTR_GPP_ACTIVE_FUNCTIONALITY_ID,
                          ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
  }
  else
  if (g_test_state == TEST_STATE_WRITE_ACTIVE_FUNCTIONALITY)
  {
    val = 0x0000;
    attr_val = (zb_uint8_t*)&val;
    zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), DUT_GPP_ADDR, DUT_GPP_ADDR_MODE,
                           ZB_ZCL_ATTR_GPP_ACTIVE_FUNCTIONALITY_ID, ZB_ZCL_ATTR_TYPE_24BITMAP, attr_val,
                           ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
  }
  if (g_test_state == TEST_STATE_FINISHED)
  {
    zb_free_buf(zcl_cmd_buf);
    if (!g_error_cnt)
    {
      TRACE_MSG(TRACE_ZCL1, "Test finished. Status: OK", (FMT__0));
    }
    else
    {
      TRACE_MSG(TRACE_ZCL1, "Test finished. Status: FAILED, error number %hd", (FMT__H, g_error_cnt));
    }
  }

  TRACE_MSG(TRACE_ZCL3, "< next_step: g_test_state: %hd", (FMT__H, g_test_state));
}

void read_attr_handler(zb_buf_t *zcl_cmd_buf)
{
  zb_bool_t test_error = ZB_FALSE;

  switch (g_test_state)
  {
    case TEST_STATE_READ_MAX_PROXY_TABLE_ENTRIES1:
    case TEST_STATE_READ_MAX_PROXY_TABLE_ENTRIES2:
    {
      zb_zcl_read_attr_res_t *read_attr_resp;

      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(zcl_cmd_buf, read_attr_resp);
      TRACE_MSG(TRACE_ZCL2, "ZB_ZCL_CMD_READ_ATTRIB_RESP getted: attr_id 0x%hx, status: 0x%hx, value 0x%hd",
                (FMT__D_H_D, read_attr_resp->attr_id, read_attr_resp->status, *read_attr_resp->attr_value));
      if (read_attr_resp->status != ZB_ZCL_STATUS_SUCCESS/* ||
          *read_attr_resp->attr_value != DUT_MAX_PROXY_TABLE_ENTRIES*/)
      {
        test_error = ZB_TRUE;
      }
      TRACE_MSG(TRACE_ZCL1, "finished", (FMT__0));
    }
    break;
    case TEST_STATE_READ_FUNCTIONALITY1:
    case TEST_STATE_READ_FUNCTIONALITY2:
    case TEST_STATE_READ_ACTIVE_FUNCTIONALITY1:
    case TEST_STATE_READ_ACTIVE_FUNCTIONALITY2:
    {
      zb_zcl_read_attr_res_t *read_attr_resp;

      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(zcl_cmd_buf, read_attr_resp);
      TRACE_MSG(TRACE_ZCL2, "ZB_ZCL_CMD_READ_ATTRIB_RESP getted: attr_id 0x%hx, status: 0x%hx, value 0x%hd",
                (FMT__D_H_D, read_attr_resp->attr_id, read_attr_resp->status, *read_attr_resp->attr_value));
      if (read_attr_resp->status != ZB_ZCL_STATUS_SUCCESS/* ||
          *read_attr_resp->attr_value != DUT_FUNCTIONALITY*/)
      {
        test_error = ZB_TRUE;
      }
      TRACE_MSG(TRACE_ZCL1, "finished", (FMT__0));
    }
    break;
  }

  if (test_error)
  {
    ++g_error_cnt;
  }
  ++g_test_state;
}

void write_attr_handler(zb_buf_t *zcl_cmd_buf)
{
  zb_bool_t test_error = ZB_FALSE;

  switch (g_test_state)
  {
    case TEST_STATE_WRITE_MAX_PROXY_TABLE_ENTRIES:
    case TEST_STATE_WRITE_FUNCTIONALITY:
    case TEST_STATE_WRITE_ACTIVE_FUNCTIONALITY:
    {
      zb_zcl_write_attr_res_t *res;

      ZB_ZCL_GET_NEXT_WRITE_ATTR_RES(zcl_cmd_buf, res);

      if (res->status != ZB_ZCL_STATUS_READ_ONLY)
      {
        test_error = ZB_TRUE;
      }
    }
    break;
  }

  if (test_error)
  {
    ++g_error_cnt;
  }
  ++g_test_state;
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  /** [VARIABLE] */
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t cmd_processed = ZB_FALSE;
  zb_zcl_default_resp_payload_t* default_res;
  /** [VARIABLE] */


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
    /** [HANDLER] */
    switch (cmd_info->cluster_id)
    {
      case ZB_ZCL_CLUSTER_ID_BASIC:
      case ZB_ZCL_CLUSTER_ID_GREEN_POWER:
      {
        if (cmd_info->is_common_command)
        {
          switch (cmd_info->cmd_id)
          {
            case ZB_ZCL_CMD_DEFAULT_RESP:
              TRACE_MSG(TRACE_ZCL1, "Process general command %hd", (FMT__H, cmd_info->cmd_id));

              default_res = ZB_ZCL_READ_DEFAULT_RESP(zcl_cmd_buf);
              TRACE_MSG(TRACE_ZCL2, "ZB_ZCL_CMD_DEFAULT_RESP getted: cmd_id 0x%hx, status: 0x%hx",
                        (FMT__H_H, default_res->command_id, default_res->status));

//              status = default_res->status;
              ++g_test_state;

              next_step(zcl_cmd_buf);
              break;

            case ZB_ZCL_CMD_READ_ATTRIB_RESP:
              read_attr_handler(zcl_cmd_buf);

              next_step(zcl_cmd_buf);
              break;

            case ZB_ZCL_CMD_WRITE_ATTRIB_RESP:
              write_attr_handler(zcl_cmd_buf);

              next_step(zcl_cmd_buf);
              break;
            default:
              TRACE_MSG(TRACE_ERROR, "ERROR, Unsupported general command", (FMT__0));
            break;
          }
          cmd_processed = ZB_TRUE;
        }
        else
        {
          g_error_cnt++;
          TRACE_MSG(TRACE_ERROR, "Error, unknow cmd received", (FMT__0));
        }
        break;
      }

      default:
        TRACE_MSG(TRACE_ERROR, "Cluster 0x%x is not supported in the test",
                  (FMT__D, cmd_info->cluster_id));
        break;
    }
    /** [HANDLER] */
  }

  TRACE_MSG(TRACE_ZCL1,
            "< zcl_specific_cluster_cmd_handler cmd_processed %hd", (FMT__H, cmd_processed));
  return cmd_processed;
}


void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_ZCL1, "> zb_zdo_startup_complete %hd", (FMT__H, param));

  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));

    next_step(buf);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Error, Device start FAILED status %d",
              (FMT__D, (int)buf->u.hdr.status));
    zb_free_buf(buf);
  }

  TRACE_MSG(TRACE_ZCL1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA

#include <stdio.h>
int main()
{
  printf(" HA is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_HA
