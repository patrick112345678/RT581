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
/*  PURPOSE:
*/

#define ZB_TRACE_FILE_ID 51
#include "zb_common.h"
#include "zb_macsplit_internal.h"
#include "zb_stack_info.h"

#ifdef ZB_MACSPLIT_HOST
#define DEVICE_STACK_INFO_TIMEOUT 500

static void no_info_received_alarm(zb_bufid_t param);

static zb_device_stack_information_callback s_stack_info_cb;

void zb_macsplit_get_device_stack_information(zb_device_stack_information_callback cb)
{
  s_stack_info_cb = cb;

  ZB_SCHEDULE_ALARM(no_info_received_alarm, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(DEVICE_STACK_INFO_TIMEOUT));

  zb_buf_get_out_delayed(zb_macsplit_get_device_version_request);
}

void zb_macsplit_get_device_version_confirm(zb_bufid_t param)
{
  zb_stack_information_t *stack_info = (zb_stack_information_t *)zb_buf_begin(param);
  ZB_SCHEDULE_ALARM_CANCEL(no_info_received_alarm, ZB_ALARM_ANY_PARAM);

  if(s_stack_info_cb)
  {
    s_stack_info_cb(stack_info);
    s_stack_info_cb = NULL;
  }
}

static void no_info_received_alarm(zb_bufid_t param)
{
  ZVUNUSED(param);
  if(s_stack_info_cb)
  {
    s_stack_info_cb(NULL);
    s_stack_info_cb = NULL;
  }
}

#endif /* ZB_MACSPLIT_HOST */

#ifdef ZB_MACSPLIT_DEVICE
void zb_macsplit_send_device_stack_information(zb_bufid_t param)
{
  zb_stack_information_t *stack_info;

  stack_info = zb_buf_initial_alloc(param, sizeof(zb_stack_information_t));

  zb_get_stack_information(stack_info);

  zb_macsplit_get_device_version_confirm(param);
}

void zb_get_stack_information(zb_stack_information_t *stack_info)
{
  //TODO: replace ZB_VERSION
  ZB_MEMCPY(stack_info->version, ZB_VERSION, sizeof(ZB_VERSION));
  /*
     ZBOSS_SDK_REVISION is deprecated. MACSPLIT_APP_REVISION is used instead.
     Note that MACSPLIT_APP_REVISION should be defined in the Options file.
  */
  stack_info->revision = MACSPLIT_APP_REVISION;
}

#endif /* MACSPLIT_DEVICE */
