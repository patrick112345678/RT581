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

#define ZB_TRACE_FILE_ID 40120
/* TODO: Debug it! */

/*! Sample fumction to parse Configure reporting response command
  @param cmd_buf - pointer to a buffer with Configure reporting response payload
  @note ZCL header should be cut before calling this function
*/
void configure_reporting_resp_handler_sample(zb_buf_t *cmd_buf)
{
  zb_zcl_configure_reporting_res_t *config_rep_resp;

  TRACE_MSG(TRACE_ZCL1, ">> sample handling configure reporting resp %p", (FMT__P, cmd_buf));

  do
  {
    ZB_ZCL_GET_NEXT_CONFIGURE_REPORTING_RES(cmd_buf, config_rep_resp);
    TRACE_MSG(TRACE_ZCL3, "config_rep_resp %p", (FMT__P, config_rep_resp));
    if (config_rep_resp)
    {
      if (config_rep_resp->status == ZB_ZCL_STATUS_SUCCESS)
      {
        TRACE_MSG(TRACE_ZCL2, "all configure reporting records were applied OK", (FMT__0));
        break;
      }
      else
      {
        TRACE_MSG(TRACE_ZCL2, "Error while setting configure reporting: status %hd, direction %hd, attr id %d",
                  (FMT__H_H_D, config_rep_resp->status, config_rep_resp->direction, config_rep_resp->attr_id));
      }
    }
  }
  while (config_rep_resp);

  TRACE_MSG(TRACE_ZCL1, "<< sample handling configure reporting resp", (FMT__0));
}
