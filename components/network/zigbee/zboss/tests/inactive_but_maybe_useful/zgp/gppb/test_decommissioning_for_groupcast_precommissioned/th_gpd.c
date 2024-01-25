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
/* PURPOSE: Simple ZGPD for send GPDF as described
in 2.3.4 test specification.
*/

#define ZB_TRACE_FILE_ID 41560
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"

#if defined ZB_ZGPD_ROLE

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;
static zb_uint8_t   g_oob_key[] = TEST_OOB_KEY;
static zb_uint8_t   g_cmd_num = 0;
zb_uint8_t g_key_nwk[] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

static void send_cmd(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> send_cmd %hd", (FMT__H, param));

  if (param == 0)
  {
    ZB_GET_OUT_BUF_DELAYED(send_cmd);
  }
  else
  {
    if (g_cmd_num < TEST_CMD_NUM)
    {
      zb_buf_t *buf = ZB_BUF_FROM_REF(param);
      zb_uint8_t* ptr;
      static zb_uint8_t cmds[] = {ZB_GPDF_CMD_ON, ZB_GPDF_CMD_OFF, ZB_GPDF_CMD_TOGGLE};

      ptr = ZB_START_GPDF_PACKET(buf);
      ZB_GPDF_PUT_UINT8(ptr, cmds[g_cmd_num % 3]);
      ZB_FINISH_GPDF_PACKET(buf, ptr);

      ZB_SEND_DATA_GPDF_CMD(param);

      ++g_cmd_num;
      TRACE_MSG(TRACE_APP1, "g_cmd_num %hd", (FMT__H, g_cmd_num));

      if (g_cmd_num == TEST_CMD_NUM)
      {
        ZB_SCHEDULE_ALARM(send_cmd, 0, 2 * ZB_TIME_ONE_SECOND);
      }
      else
      {
        ZB_SCHEDULE_ALARM(send_cmd, 0, ZB_TIME_ONE_SECOND);
      }
    }
    else
    {
      zb_buf_t *buf = ZB_BUF_FROM_REF(param);
      zb_uint8_t* ptr;

      ptr = ZB_START_GPDF_PACKET(buf);
      ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_CMD_DECOMMISSIONING);
      ZB_FINISH_GPDF_PACKET(buf, ptr);

      ZB_SEND_DATA_GPDF_CMD(param);
    }
  }

  TRACE_MSG(TRACE_APP1, "<< send_cmd", (FMT__0));
}

static void zgpd_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APP2, ">> zgpd_startup_complete status %d", (FMT__D, (int)buf->u.hdr.status));

  if (buf->u.hdr.status == RET_OK)
  {
    TRACE_MSG(TRACE_APP2, "DUT-GPD Device STARTED OK", (FMT__0));
    ZGPD->channel = TEST_CHANNEL;
    ZB_SCHEDULE_ALARM(send_cmd, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED", (FMT__0));
    zb_free_buf(buf);
  }
}

MAIN()
{
  ARGV_UNUSED;

#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("th_gpd");


#if 0
  ZB_SET_TRACE_LEVEL(0);
  ZB_SET_TRACE_MASK(0);
#endif
  /*******************************/

  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

  /*ZB_ZGPD_SEND_IEEE_SRC_ADDR_IN_COMM_REQ();*/
  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);

  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
  ZB_ZGPD_SET_OOB_KEY(g_oob_key);
//  ZB_ZGPD_REQUEST_SECURITY_KEY();


  if (zb_zgpd_dev_start(zgpd_startup_complete) != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "DUT-GPD Device start FAILED", (FMT__0));
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
