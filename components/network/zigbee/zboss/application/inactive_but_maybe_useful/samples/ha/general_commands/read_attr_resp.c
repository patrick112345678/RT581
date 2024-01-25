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

#define ZB_TRACE_FILE_ID 40118
/* TODO: Include in a build and debug */

/*! @brief Sample function to parse read attributes response command
    @param cmd_buf buffer with payload
*/
static void th_read_attr_resp_handler_example(zb_buf_t *cmd_buf)
{
  zb_zcl_read_attr_res_t *read_attr_resp;

  TRACE_MSG(TRACE_ZCL1, ">>th example read attr resp", (FMT__0));

  do
  {
    ZB_ZCL_GET_NEXT_READ_ATTR_RESP(cmd_buf, read_attr_resp);
    TRACE_MSG(TRACE_ZCL3, "read_attr_resp %p", (FMT__P, read_attr_resp));
    if (read_attr_resp)
    {
      TRACE_MSG(
          TRACE_ZCL3,
          "attr dump: attr id %hd, status %hd",
          (FMT__H_H, read_attr_resp->attr_id, read_attr_resp->status));
      if (read_attr_resp->status == ZB_ZCL_STATUS_SUCCESS)
      {
        TRACE_MSG(TRACE_ZCL3, "attr type %hd", (FMT__H, read_attr_resp->attr_type));
        switch ( read_attr_resp->attr_type )
        {
          case ZB_ZCL_ATTR_TYPE_8BIT:
          case ZB_ZCL_ATTR_TYPE_U8:
          case ZB_ZCL_ATTR_TYPE_BOOL:
          case ZB_ZCL_ATTR_TYPE_8BITMAP:
            TRACE_MSG(
                TRACE_ZCL3,
                "value %hd",
                (FMT__H, *(zb_uint8_t*)read_attr_resp->attr_value));
            break;

          case ZB_ZCL_ATTR_TYPE_16BIT:
          case ZB_ZCL_ATTR_TYPE_U16:
          case ZB_ZCL_ATTR_TYPE_16BITMAP:
            TRACE_MSG(
                TRACE_ZCL3,
                "value %d",
                (FMT__D, *(zb_uint16_t*)read_attr_resp->attr_value));
            break;

          case ZB_ZCL_ATTR_TYPE_32BIT:
          case ZB_ZCL_ATTR_TYPE_U32:
          case ZB_ZCL_ATTR_TYPE_32BITMAP:
            TRACE_MSG(
                TRACE_ZCL3,
                "value %d %d",
                ( FMT__D_D,
                  *(zb_uint16_t*)read_attr_resp->attr_value,
                  *(((zb_uint16_t*)read_attr_resp->attr_value) + 1)));
            break;

          case ZB_ZCL_ATTR_TYPE_IEEE_ADDR:
            TRACE_MSG(
                TRACE_ZCL3,
                "value " TRACE_FORMAT_64,
                (FMT__A, TRACE_ARG_64(read_attr_resp->attr_value)));
            break;

          default:
            break;
        }
      }
    }
  }
  while(read_attr_resp);

  zb_free_buf(cmd_buf);
  TRACE_MSG(TRACE_ZCL1, "<<th example read attr resp", (FMT__0));
}
