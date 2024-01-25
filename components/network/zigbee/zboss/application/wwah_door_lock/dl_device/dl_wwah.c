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

#define ZB_TRACE_FILE_ID 40134
#include "zboss_api.h"
#include "zb_wwah_door_lock.h"
#include "zcl/zb_zcl_wwah.h"


static zb_zcl_wwah_debug_report_t debug_report_table[DEBUG_REPORT_TABLE_SIZE];

/*Fill Debug Report Table with default values
  Fill #4 Debug Report for testing puprose */
void setup_debug_report(void)
{
  zb_uindex_t i;
  zb_char_t *debug_report_message = "Issue #4";
  zb_uint8_t report_id = 4;

  ZB_ASSERT(DEBUG_REPORT_TABLE_SIZE > 0 && DEBUG_REPORT_TABLE_SIZE <= 0xFE);

  ZB_ZCL_SET_ATTRIBUTE(WWAH_DOOR_LOCK_EP, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_WWAH_CURRENT_DEBUG_REPORT_ID_ID, &report_id, ZB_FALSE);
  debug_report_table[0] = (zb_zcl_wwah_debug_report_t){report_id, strlen(debug_report_message), debug_report_message};
  for(i = 1; i < DEBUG_REPORT_TABLE_SIZE; ++i)
  {
    debug_report_table[i] = ZB_ZCL_WWAH_DEBUG_REPORT_FREE_RECORD;
  }
}

void dl_process_wwah_debug_report_query_cb(zb_uint8_t param)
{
  /* zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t); */
  const zb_uint8_t *report_id = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_uint8_t);
  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_NOT_FOUND;
  for (zb_uint8_t i = 0; i <= DEBUG_REPORT_TABLE_SIZE; ++i)
  {
    if (debug_report_table[i].report_id == *report_id)
    {
      ZB_ZCL_DEVICE_CMD_PARAM_OUT_SET(param, &debug_report_table[i]);
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
      TRACE_MSG(TRACE_APP1, "debug report %d found", (FMT__D, *report_id));
      break;
    }
  }
  TRACE_MSG(TRACE_APP1, "<< dl_process_wwah_debug_report_query_cb", (FMT__0));
}

zb_bool_t dl_wwah_app_event_retry_should_discard_event(zb_uint8_t failed_attempts)
{
  return (zb_bool_t)(0xFF != MAX_REDELIVERY_ATTEMPTS() && failed_attempts > MAX_REDELIVERY_ATTEMPTS());
}

/**
 * Returns a timeout (in seconds) to wait the next re-delivery attempt
 * if delivery failed last 'failed_attempts' times
 */
zb_uint_t dl_wwah_app_event_retry_get_next_timeout(zb_uint8_t failed_attempts)
{
  zb_uint_t timeout;
  zb_uindex_t i;

  timeout = FIRST_BACKOFF_TIME();

  for (i = 0; i < failed_attempts-1 && timeout < MAX_BACKOFF_TIME(); i++)
  {
    timeout *= COMMON_RATIO();
  }

  return timeout;
}
