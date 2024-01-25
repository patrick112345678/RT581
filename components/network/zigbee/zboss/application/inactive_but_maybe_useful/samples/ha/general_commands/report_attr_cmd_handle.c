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

#define ZB_TRACE_FILE_ID 40116
/* TODO: Include in a build and debug */

/*! @brief Sample function to parse report attributes command
    @param cmd_buf buffer with payload
*/
static void report_attr_req_handler_sample(zb_buf_t *cmd_buf)
{
    zb_zcl_report_attr_req_t *rep_attr_req;

    TRACE_MSG(TRACE_ZCL1, ">>sample report attr command", (FMT__0));

    do
    {
        ZB_ZCL_GET_NEXT_REPORT_ATTR_REQ(cmd_buf, rep_attr_req);
        TRACE_MSG(TRACE_ZCL3, "rep_attr_req %p", (FMT__P, rep_attr_req));
        if (rep_attr_req)
        {
            TRACE_MSG(TRACE_ZCL3, "attr dump: attr id %hd",
                      (FMT__H, rep_attr_req->attr_id));

            TRACE_MSG(TRACE_ZCL3, "attr type %hd", (FMT__H, rep_attr_req->attr_type));
            switch ( rep_attr_req->attr_type )
            {
            case ZB_ZCL_ATTR_TYPE_8BIT:
            case ZB_ZCL_ATTR_TYPE_U8:
            case ZB_ZCL_ATTR_TYPE_BOOL:
            case ZB_ZCL_ATTR_TYPE_8BITMAP:
                TRACE_MSG(TRACE_ZCL3, "value %hd",
                          (FMT__H, *(zb_uint8_t *)rep_attr_req->attr_value));
                break;

            case ZB_ZCL_ATTR_TYPE_16BIT:
            case ZB_ZCL_ATTR_TYPE_U16:
            case ZB_ZCL_ATTR_TYPE_16BITMAP:
                TRACE_MSG(TRACE_ZCL3, "value %d",
                          (FMT__D, *(zb_uint16_t *)rep_attr_req->attr_value));
                break;

            case ZB_ZCL_ATTR_TYPE_32BIT:
            case ZB_ZCL_ATTR_TYPE_U32:
            case ZB_ZCL_ATTR_TYPE_32BITMAP:
                TRACE_MSG(
                    TRACE_ZCL3, "value %d %d",
                    ( FMT__D_D, *(zb_uint16_t *)rep_attr_req->attr_value,
                      *(((zb_uint16_t *)rep_attr_req->attr_value) + 1)));
                break;

            case ZB_ZCL_ATTR_TYPE_IEEE_ADDR:
                TRACE_MSG(TRACE_ZCL3, "value " TRACE_FORMAT_64,
                          (FMT__A, TRACE_ARG_64(rep_attr_req->attr_value)));
                break;

            default:
                break;
            }
        }
    } while (rep_attr_req);

    zb_free_buf(cmd_buf);
    TRACE_MSG(TRACE_ZCL1, "<<sample report attr command", (FMT__0));
}
