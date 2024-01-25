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
/* PURPOSE: On/off cluster implementation in ZGPD, unidirectional commissioning,
full security.
*/

#define ZB_TRACE_FILE_ID 41550
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"

#if defined ZB_ZGPD_ROLE

zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;
zb_uint8_t   g_zgpd_key[] = TEST_SEC_KEY;
zb_bool_t    g_commissioned = ZB_FALSE;
zb_uint8_t   g_notificaions_num = 0;

zb_ieee_addr_t g_zgpd_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xba};

void zgpd_set_ieee_and_call(zb_uint8_t param, zb_callback_t func);

void send_cmd(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> battery_notification_loop %hd", (FMT__H, param));

    if (param == 0)
    {
        ZB_GET_OUT_BUF_DELAYED(send_cmd);
    }
    else
    {
        if (g_notificaions_num < TEST_NOTIFICATIONS_NUM)
        {
            zb_buf_t *buf = ZB_BUF_FROM_REF(param);
            zb_uint8_t *ptr;
            static zb_uint8_t cmds[] = {ZB_GPDF_CMD_ON, ZB_GPDF_CMD_OFF, ZB_GPDF_CMD_TOGGLE};

            ptr = ZB_START_GPDF_PACKET(buf);
            ZB_GPDF_PUT_UINT8(ptr, cmds[g_notificaions_num % 3]);
            ZB_FINISH_GPDF_PACKET(buf, ptr);

            ZB_SEND_DATA_GPDF_CMD(param);

            ++g_notificaions_num;
            TRACE_MSG(TRACE_APP1, "g_notificaions_num %hd", (FMT__H, g_notificaions_num));
            ZB_SCHEDULE_ALARM(send_cmd, 0, ZB_TIME_ONE_SECOND);
        }
        else
        {
            TRACE_MSG(TRACE_APP2, "notifications sent, now send req attributes", (FMT__0));
            /* TODO: implement Request Attribute command and send */
            TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        }
    }

    TRACE_MSG(TRACE_APP1, "<< battery_notification_loop", (FMT__0));
}

void prepare_send_cmd(zb_uint8_t param)
{
    ZB_SCHEDULE_ALARM(send_cmd, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000));
}

void set_ieee_cb(zb_uint8_t param)
{
    zb_free_buf(ZB_BUF_FROM_REF(param));
    ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
    ZB_SCHEDULE_ALARM(prepare_send_cmd, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));
}

void zgpd_startup_complete(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_APP2, ">> zgpd_startup_complete status %d", (FMT__D, (int)buf->u.hdr.status));

    if (buf->u.hdr.status == RET_OK)
    {
        TRACE_MSG(TRACE_APP2, "ZGPD Device STARTED OK", (FMT__0));
        ZGPD->channel = TEST_CHANNEL;
        zgpd_set_ieee_and_call(param, set_ieee_cb);
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

    ZB_INIT("zgpd");


#if 0
    ZB_SET_TRACE_LEVEL(0);
    ZB_SET_TRACE_MASK(0);
#endif
    /*******************************/

    ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

    /*ZB_ZGPD_SEND_IEEE_SRC_ADDR_IN_COMM_REQ();*/
    ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
    ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
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
