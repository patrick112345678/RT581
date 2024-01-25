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
/* PURPOSE: On/off cluster implementation in ZGPD
*/

#define ZB_TRACE_FILE_ID 41572
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"

#if defined ZB_ZGPD_ROLE

zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;
zb_uint8_t   g_zgpd_key[] = TEST_SEC_KEY;
zb_bool_t    g_commissioned = ZB_FALSE;

void comm_cb(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t comm_result = buf->u.hdr.status;

  if (comm_result == ZB_ZGPD_COMM_FAILED)
  {

  }
}

void check_commissioning_cb(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, ">> check_commissioning_cb %hd", (FMT__H, param));
  if (!ZGPD_IS_COMMISSIONED())
  {
    TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
  }
  TRACE_MSG(TRACE_APP1, "<< check_commissioning_cb %hd", (FMT__H, param));
}

void zgpd_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APP2, ">> zgpd_startup_complete status %d", (FMT__D, (int)buf->u.hdr.status));

  if (buf->u.hdr.status == RET_OK)
  {
    TRACE_MSG(TRACE_APP2, "ZGPD Device STARTED OK", (FMT__0));
    zb_zgpd_start_commissioning(&comm_cb);
    ZB_SCHEDULE_ALARM(check_commissioning_cb, 0, 10 * ZB_TIME_ONE_SECOND);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED", (FMT__0));
  }

  zb_free_buf(buf);
}


MAIN()
{
  ARGV_UNUSED;

#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zgpd");


  /*******************************/

  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_UNDEFINED_DEV_ID);

  ZB_ZGPD_USE_MAINTENANCE_FRAME_FOR_CHANNEL_REQ();
  /*ZB_ZGPD_SEND_IEEE_SRC_ADDR_IN_COMM_REQ();*/
  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
  ZB_ZGPD_SET_SECURITY(ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC, ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL, g_zgpd_key);

  if (zb_zgpd_dev_start(zgpd_startup_complete) != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED", (FMT__0));
  }
  else
  {
    zgpd_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

#else //defined ZB_ENABLE_ZGP && defined ZB_ZGPD_ROLE

MAIN()
{
  ARGV_UNUSED;

  printf("ZB_ENABLE_ZGP and ZB_ZGPD_ROLE should be defined in zb_config.h");

  MAIN_RETURN(1);
}

#endif //defined ZB_ENABLE_ZGP && defined ZB_ZGPD_ROLE
