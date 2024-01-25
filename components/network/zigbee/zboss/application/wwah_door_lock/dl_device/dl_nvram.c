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

#define ZB_TRACE_FILE_ID 40132
#include "zboss_api.h"
#include "zb_wwah_door_lock.h"
#include "zcl/zb_zcl_wwah.h"

void dl_write_app_data(zb_uint8_t param)
{
  ZVUNUSED(param);
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
}

zb_uint16_t dl_get_nvram_data_size(void)
{
  TRACE_MSG(TRACE_APP1, "dl_get_nvram_data_size, ret %hd", (FMT__H, sizeof(dl_device_nvram_dataset_t)));
  return sizeof(dl_device_nvram_dataset_t);
}

void dl_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  dl_device_nvram_dataset_t ds;
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> dl_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

  ZB_ASSERT(payload_length == sizeof(ds));

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  if (ret == RET_OK)
  {
    FIRST_BACKOFF_TIME() = ds.app_event_retry.first_backoff_time_in_seconds;
    COMMON_RATIO() = ds.app_event_retry.backoff_sequence_common_ratio;
    MAX_BACKOFF_TIME() = ds.app_event_retry.max_backoff_time_in_seconds;
    MAX_REDELIVERY_ATTEMPTS() = ds.app_event_retry.max_re_delivery_attempts;
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "nvram read error %d", (FMT__D, ret));
  }

  TRACE_MSG(TRACE_APP1, "<< dl_nvram_read_app_data", (FMT__0));
}


zb_ret_t dl_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  dl_device_nvram_dataset_t ds;

  ds.app_event_retry.first_backoff_time_in_seconds = FIRST_BACKOFF_TIME();
  ds.app_event_retry.backoff_sequence_common_ratio = COMMON_RATIO();
  ds.app_event_retry.max_backoff_time_in_seconds = MAX_BACKOFF_TIME();
  ds.app_event_retry.max_re_delivery_attempts = MAX_REDELIVERY_ATTEMPTS();

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  TRACE_MSG(TRACE_APP1, "<< dl_nvram_write_app_data, ret %d", (FMT__D, ret));

  return ret;
}


