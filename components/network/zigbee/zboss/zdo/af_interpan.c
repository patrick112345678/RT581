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
/* PURPOSE: AF: inter-PAN path.
*/

#define ZB_TRACE_FILE_ID 2088
#include "zb_common.h"
#include "zb_aps_interpan.h"

#if defined ZB_ENABLE_INTER_PAN_EXCHANGE

void zb_intrp_data_confirm(zb_uint8_t param)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZDO1, "> zb_intrp_data_confirm param %hd status %d", (FMT__H_D, param, zb_buf_get_status(param)));

#if defined ZB_ENABLE_ZLL && !defined ZB_BDB_TOUCHLINK
  ZLL_TRAN_CTX().send_confirmed = (zb_buf_get_status(param) == MAC_SUCCESS);
#endif /* ZB_ENABLE_ZLL */

#if defined ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
  if (ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().intrp_mchan_data_req_confirmed != NULL)
  {
    ret = ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().intrp_mchan_data_req_confirmed(param);
  }
#endif /* defined ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */
#if defined (ZB_ENABLE_ZCL)
  if (ret != RET_OK)
  {
    ret = zb_zcl_ack_callback(param);
  }
#endif
  if (ret != RET_OK)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "< zb_intrp_data_confirm", (FMT__0));
}/* void zb_intrp_data_confirm(zb_uint8_t param) */

void zb_intrp_data_indication(zb_uint8_t param)
{
  zb_intrp_data_ind_t* indication = ZB_BUF_GET_PARAM(param, zb_intrp_data_ind_t);
  zb_uint8_t header_size;
  zb_bool_t is_process = ZB_FALSE;

  TRACE_MSG(TRACE_ZDO1, "> zb_intrp_data_indication param %hd", (FMT__H, param));

  header_size = ZB_INTRP_HEADER_SIZE(indication->dst_addr_mode == ZB_INTRP_ADDR_GROUP);
  (void)zb_buf_cut_left(param, header_size);

#if defined ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
  if (ZDO_CTX().af_inter_pan_data_cb != NULL)
  {
    is_process = (*ZDO_CTX().af_inter_pan_data_cb)(param, ZB_PIBCACHE_CURRENT_PAGE(), ZB_PIBCACHE_CURRENT_CHANNEL());

    if (is_process)
    {
      return;
    }
  }
#endif

  if (indication->profile_id == ZB_AF_ZLL_PROFILE_ID)
  {
    TRACE_MSG(TRACE_APS3, "check Inter-PAN packet prof id 0x%hx", (FMT__H, indication->profile_id));

    TRACE_MSG(TRACE_APS3, "found device, call its handler", (FMT__0));
#if defined ZB_ENABLE_ZLL
    if (indication->cluster_id == ZB_ZCL_CLUSTER_ID_TOUCHLINK_COMMISSIONING)
    {
      /* If set, schedule processing device command */
      ZB_SCHEDULE_CALLBACK(zll_process_device_command, param);

      is_process=ZB_TRUE;
    }
#endif
  }

  if(!is_process)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "< zb_intrp_data_indication", (FMT__0));
}/* void zb_intrp_data_indication(zb_uint8_t param) */

#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */
