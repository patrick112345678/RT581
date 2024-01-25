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

#define ZB_TRACE_FILE_ID 41569
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"

#if defined ZB_ZGPD_ROLE

zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;
zb_uint8_t   g_zgpd_key[] = TEST_SEC_KEY;
zb_bool_t    g_commissioned = ZB_FALSE;

void user_action(zb_uint8_t param);

zb_uint8_t g_reporting_index = 0;

void user_action(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> user_action %hd", (FMT__H, param));

  if(param==0)
  {
    if (g_reporting_index == REPORT_COUNT)
    {
      zb_zgpd_decommission();
      g_commissioned = ZB_FALSE;
    }
    else
    {
      ZB_GET_OUT_BUF_DELAYED(user_action);
    }
  }
  else
  {
    zb_int16_t cluster_id = ZB_ZCL_CLUSTER_ID_IAS_ZONE;

    zb_int16_t val2 = (g_reporting_index % 2 ? ZB_ZCL_IAS_ZONE_ZONE_STATUS_ALARM1 : 0);
    zb_gpdf_attr_report_fld_t report2 = {ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID,
                                      ZB_ZCL_ATTR_TYPE_16BIT,
                                      &val2};

    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t* ptr;

    ZB_ZGPD_ATTR_REPORTING_CMD_INIT(buf, cluster_id, ptr);
    ZB_ZGPD_ATTR_REPORTING_CMD_NEXT(ptr, report2);
    ZB_ZGPD_ATTR_REPORTING_CMD_FINISH(buf, ptr);

    ZB_SEND_DATA_GPDF_CMD(param);

    ZB_SCHEDULE_ALARM(user_action, 0, 5 * ZB_TIME_ONE_SECOND);

    g_reporting_index++;
  }

  TRACE_MSG(TRACE_APP1, "<< user_action", (FMT__0));
}


void battery_notification_loop(zb_uint8_t param)
{
  zb_uint8_t volt_diff = 10;
  TRACE_MSG(TRACE_APP1, ">> battery_notification_loop %hd", (FMT__H, param));

  if(param==0)
  {
    ZB_GET_OUT_BUF_DELAYED(battery_notification_loop);
  }
  else if (g_commissioned)
  {
    zb_int16_t cluster_id = ZB_ZCL_CLUSTER_ID_POWER_CONFIG;
    zb_uint8_t val = 100 - volt_diff * g_reporting_index;
    zb_gpdf_attr_report_fld_t report = {ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
                                      ZB_ZCL_ATTR_TYPE_U8,
                                      &val};

    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t* ptr;

    ZB_ZGPD_ATTR_REPORTING_CMD_INIT(buf, cluster_id, ptr);
    ZB_ZGPD_ATTR_REPORTING_CMD_NEXT(ptr, report);
    ZB_ZGPD_ATTR_REPORTING_CMD_FINISH(buf, ptr);

    ZB_SEND_DATA_GPDF_CMD(param);

    ZB_SCHEDULE_ALARM(battery_notification_loop, 0, 7 * ZB_TIME_ONE_SECOND);
  }

  TRACE_MSG(TRACE_APP1, "<< battery_notification_loop", (FMT__0));
}


void comm_cb(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t comm_result = buf->u.hdr.status;

  if (comm_result == ZB_ZGPD_COMM_SUCCESS)
  {
    g_commissioned = ZB_TRUE;
    ZB_SCHEDULE_CALLBACK(user_action, param);
#ifdef TEST_ZGPD_SEND_BATTERY_LEVEL_INFO
    ZB_SCHEDULE_CALLBACK(battery_notification_loop, 0);
#endif
  }
}


void zgpd_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APP2, ">> zgpd_startup_complete status %d", (FMT__D, (int)buf->u.hdr.status));

  if (buf->u.hdr.status == RET_OK)
  {
    TRACE_MSG(TRACE_APP2, "ZGPD Device STARTED OK", (FMT__0));

    zb_zgpd_start_commissioning(&comm_cb);
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
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zgpd");


  /*******************************/
  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_MANUF_SPECIFIC_DEV_ID);
  ZB_ZGPD_SET_MANUF_SPECIFIC_DEV(ZB_ZGPD_DEF_MANUFACTURER_ID, ZB_ZGP_MS_DOOR_SENSOR_DEV_ID);

  ZB_ZGPD_USE_MAINTENANCE_FRAME_FOR_CHANNEL_REQ();
  /*ZB_ZGPD_SEND_IEEE_SRC_ADDR_IN_COMM_REQ();*/
  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
  ZB_ZGPD_SET_SECURITY(TEST_SECURITY_LEVEL, ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL, g_zgpd_key);

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
