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

#define ZB_TRACE_FILE_ID 40121
/* TODO: Debug it! */

/*! Sample fumction to parse write attribute response command
  @param cmd_buf - pointer to a buffer with Write attribute response payload
  @note ZCL header should be cut before calling this function
*/
void write_attr_resp_handler_sample(zb_buf_t *cmd_buf)
{
  zb_zcl_write_attr_res_t *write_attr_resp;

  TRACE_MSG(TRACE_ZCL1, ">> example write attr resp %p", (FMT__P, cmd_buf));

  do
  {
    ZB_ZCL_GET_NEXT_WRITE_ATTR_RES(cmd_buf, write_attr_resp);
    TRACE_MSG(TRACE_ZCL3, "write_attr_resp %p", (FMT__P, write_attr_resp));
    if (write_attr_resp)
    {
      if (write_attr_resp->status == ZB_ZCL_STATUS_SUCCESS)
      {
        TRACE_MSG(TRACE_ZCL2, "all attributes were written OK", (FMT__0));
        break;
      }
      else
      {
        TRACE_MSG(TRACE_ZCL2, "Error while writing attr: status %hd, attr id %d",
                  (FMT__H_D, write_attr_resp->status, write_attr_resp->attr_id));
      }
    }
  }
  while (write_attr_resp);

  TRACE_MSG(TRACE_ZCL1, "<< example write attr resp", (FMT__0));
}
