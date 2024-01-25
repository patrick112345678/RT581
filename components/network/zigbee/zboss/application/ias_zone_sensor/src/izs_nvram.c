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
/* PURPOSE: Implementation of callbacks for reading/writing stored into NVRAM data
*/

#define ZB_TRACE_FILE_ID 40148
#include "izs_device.h"
#include "zboss_api.h"

void izs_write_app_data(zb_uint8_t param)
{
  ZVUNUSED(param);
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
}

zb_uint16_t izs_get_nvram_data_size(void)
{
  TRACE_MSG(TRACE_APP1, "dl_get_nvram_data_size, ret %hd", (FMT__H, sizeof(izs_device_nvram_dataset_t)));
  return sizeof(izs_device_nvram_dataset_t);
}

void izs_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  izs_device_nvram_dataset_t ds;
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> izs_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

  ZB_ASSERT(payload_length == sizeof(ds));

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  if (ret == RET_OK)
  {
    g_device_ctx.enrollment_method = ds.enrollment_method;
    g_device_ctx.zone_attr.zone_status = ds.zone_status;
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "nvram read error %d", (FMT__D, ret));
  }

  TRACE_MSG(TRACE_APP1, "<< izs_nvram_read_app_data", (FMT__0));
}


zb_ret_t izs_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  izs_device_nvram_dataset_t ds;

  ds.enrollment_method = g_device_ctx.enrollment_method;
  ds.zone_status = g_device_ctx.zone_attr.zone_status;

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  TRACE_MSG(TRACE_APP1, "<< izs_nvram_write_app_data, ret %d", (FMT__D, ret));

  return ret;
}


