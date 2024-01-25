/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2021 DSR Corporation, Denver CO, USA.
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
/*  PURPOSE: MAC split transport dual echo test
*/

#define ZB_TRACE_FILE_ID 111
#include "zb_common.h"

#ifdef ZB_MACSPLIT_HOST
zb_uint8_t str_send[] = "host_to_device_test_string";
zb_uint8_t str_recv[] = "device_to_host_test_stirng";
zb_uint8_t str_init[] = "macsplit_dual_echo_test_host";
#endif

#ifdef ZB_MACSPLIT_DEVICE
zb_uint8_t str_send[] = "device_to_host_test_stirng";
zb_uint8_t str_recv[] = "host_to_device_test_string";
zb_uint8_t str_init[] = "macsplit_device";
#endif

#if (defined ZB_MACSPLIT_HOST && defined ZB_MACSPLIT_DEVICE) || (!defined ZB_MACSPLIT_HOST && !defined ZB_MACSPLIT_DEVICE)
#error Only host or only device should be defined
#endif

static void send_data(zb_bufid_t param);

#ifdef ZB_MACSPLIT_DEVICE
void zb_ib_set_defaults(zb_char_t *rx_pipe)
{
  ZVUNUSED(rx_pipe);
}

void zb_nwk_init()
{
}

void zb_check_oom_status(zb_uint8_t param)
{
  ZVUNUSED(param);
}

void zb_zcl_diagnostics_inc(zb_uint16_t attr_id, zb_uint8_t value)
{
  ZVUNUSED(attr_id);
  ZVUNUSED(value);
}


void zb_aps_init()
{
}

void zb_zdo_init()
{
}

void zb_zcl_init()
{
}

void zll_init()
{
}

void zb_nwk_unlock_in(zb_uint8_t param)
{
  ZVUNUSED(param);
}

void zb_sleep_can_sleep(zb_uint32_t sleep_tmo)
{
  ZVUNUSED(sleep_tmo);
}

void zb_sleep_init()
{
}

zb_uint32_t zb_sleep_calc_sleep_tmo()
{
  return 0;
}
#endif /* ZB_MACSPLIT_DEVICE */

void zboss_signal_handler(zb_bufid_t param)
{
#ifdef ZB_MACSPLIT_DEVICE
  ZVUNUSED(param);
#endif
#ifdef ZB_MACSPLIT_HOST
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_MACSPLIT_DEVICE_BOOT:
        send_data(0);
        TRACE_MSG(TRACE_APP1, "Macsplit device and echo test are started", (FMT__0));
        break;
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR,
              "Device started FAILED status %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }
  if (param)
  {
    zb_buf_free(param);
  }
#endif /* ZB_MACSPLIT_HOST */
}

static void send_echo_data(zb_bufid_t param)
{
  zb_macsplit_transport_send_data(param);
}

static void send_data(zb_bufid_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_uint8_t *ptr;

  ZVUNUSED(param);

  zb_buf_initial_alloc(buf, sizeof(str_send));
  ptr = zb_buf_begin(buf);
  ZB_MEMCPY(ptr, str_send, sizeof(str_send));

  zb_macsplit_transport_send_data(buf);
}

static void recv_data(zb_bufid_t ref)
{
  zb_uint8_t *ptr;

  ptr = zb_buf_begin(ref);

  if (ZB_MEMCMP(ptr, str_recv, sizeof(str_recv)) == 0U)
  {
    send_echo_data(ref);
  }
  else if (ZB_MEMCMP(ptr, str_send, sizeof(str_send)) == 0U)
  {
    zb_buf_free(ref);
    send_data(0);
  }
  else
  {
    zb_buf_free(ref);
  }
}

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();

  ZB_INIT(str_init);

  zb_macsplit_set_cb_recv_data(recv_data);

#ifdef ZB_MACSPLIT_DEVICE
  ZB_SCHEDULE_APP_ALARM(send_data, 0, ZB_TIME_ONE_SECOND);

  while (!ZB_SCHEDULER_IS_STOP())
  {
    zb_sched_loop_iteration();
  }
#endif

#ifdef ZB_MACSPLIT_HOST
  /* Initiate the stack start with starting the commissioning */
  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    /* Call the main loop */
    zboss_main_loop();
  }
#endif

  TRACE_DEINIT();

  MAIN_RETURN(0);
}
