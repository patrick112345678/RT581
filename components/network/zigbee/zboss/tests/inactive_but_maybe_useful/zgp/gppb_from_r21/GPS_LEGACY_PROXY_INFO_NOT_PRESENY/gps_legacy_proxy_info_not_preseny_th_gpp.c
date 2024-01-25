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
/* PURPOSE: TH-GPP/TH-TOOL
*/

#define ZB_TEST_NAME GPS_LEGACY_PROXY_INFO_NOT_PRESENY_TH_GPP

#define ZB_TRACE_FILE_ID 63460
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zb_secur_api.h"
#include "zb_ha.h"
#include "test_config.h"
#include "zgp/zgp_internal.h"

#ifdef ZB_ENABLE_HA
#include "../include/zgp_test_templates.h"

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_th_gpp_addr = TH_GPP_IEEE_ADDR;

static void fill_gpdf_info(zb_gpdf_info_t *gpdf_info);
static void send_gp_notify(zb_uint8_t param);
static void save_sec_frame_counter(zb_gpdf_info_t *gpdf_info);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_WAIT_PAIRING,
    TEST_STATE_SEND_GP_NOTIFICATION1,
    TEST_STATE_SEND_GP_NOTIFICATION2,
    /* FINISH */
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    ZVUNUSED(cb);

    TRACE_MSG(TRACE_APP1, ">send_zcl: test_state = %d", (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_GP_NOTIFICATION1:
    case TEST_STATE_SEND_GP_NOTIFICATION2:
        ZB_ZGPC_SET_PAUSE(5);
        send_gp_notify(buf_ref);
        break;
    }

    TRACE_MSG(TRACE_APP1, "<send_zcl", (FMT__0));
}

static void perform_next_state(zb_uint8_t param)
{
    if (TEST_DEVICE_CTX.pause)
    {
        ZB_SCHEDULE_ALARM(perform_next_state, 0,
                          ZB_TIME_ONE_SECOND * TEST_DEVICE_CTX.pause);
        TEST_DEVICE_CTX.pause = 0;
        return;
    }

    TEST_DEVICE_CTX.test_state++;

    TRACE_MSG(TRACE_APP1, ">perform_next_state: test_state = %d",
              (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAIT_PAIRING:
        TRACE_MSG(TRACE_APP1, "Wait pairing with ZGPD.", (FMT__0));
        break;

    case TEST_STATE_FINISHED:
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;

    default:
    {
        if (param)
        {
            zb_free_buf(ZB_BUF_FROM_REF(param));
        }
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
    }

    TRACE_MSG(TRACE_APP1, "<perform_next_state", (FMT__0));
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void gp_pairing_handler_cb(zb_uint8_t buf_ref)
{
    TRACE_MSG(TRACE_APP1, ">gp_pairing_handler_cb: buf_ref = %d, test_state = %d",
              (FMT__D_D, buf_ref, TEST_DEVICE_CTX.test_state));

    ZVUNUSED(buf_ref);
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAIT_PAIRING:
        ZB_ZGPC_SET_PAUSE(5);
        PERFORM_NEXT_STATE(0);
        break;
    }

    TRACE_MSG(TRACE_APP1, "<gp_pairing_handler_cb", (FMT__0));
}

static void fill_gpdf_info(zb_gpdf_info_t *gpdf_info)
{
    TRACE_MSG(TRACE_APP1, ">fill_gpd_info: gpdf_info = %p", (FMT__P, gpdf_info));

    ZB_BZERO(gpdf_info, sizeof(*gpdf_info));
    gpdf_info->zgpd_id.app_id = ZB_ZGP_APP_ID_0000;
    gpdf_info->zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_GP_NOTIFICATION1:
    {
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_ON;
        break;
    }
    case TEST_STATE_SEND_GP_NOTIFICATION2:
    {
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_OFF;
        /* Always excellent */
        gpdf_info->rssi = 0x3b;
        gpdf_info->lqi = 0x3;
        break;
    }
    }

    gpdf_info->sec_frame_counter = zgp_proxy_table_get_security_counter(&gpdf_info->zgpd_id) + 1;

    TRACE_MSG(TRACE_APP1, "<fill_gpd_info", (FMT__0));
}

static void send_gp_notify(zb_uint8_t param)
{
    zb_buf_t       *buf = ZB_BUF_FROM_REF(param);
    zb_uint16_t    options = 0;
    zb_gpdf_info_t gpdf_info;
    zb_uint8_t     gpp_gpd_link;
    zb_uint8_t     *ptr = ZB_BUF_BEGIN(buf);
    zb_uint16_t    gpp_short_addr = ZB_PIBCACHE_NETWORK_ADDRESS();

    TRACE_MSG(TRACE_APP1, ">send_gp_notify: param = %d, test_state = %d",
              (FMT__D_D, param, TEST_DEVICE_CTX.test_state));

    /* APP ID */
    options |= ZB_ZGP_APP_ID_0000;
    /* Security level */
    options |= ZB_ZGP_SEC_LEVEL_FULL_NO_ENC << 6;
    /* Security Key type */
    options |= ZB_ZGP_SEC_KEY_TYPE_NWK << 8;
    /* Proxy info not present */

    fill_gpdf_info(&gpdf_info);
    save_sec_frame_counter(&gpdf_info);

    gpp_gpd_link = zb_zgp_cluster_encode_link_quality(gpdf_info.rssi, gpdf_info.lqi);

    ptr = ZB_ZCL_START_PACKET_REQ(buf)
          ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL(ptr, ZB_ZCL_DISABLE_DEFAULT_RESPONSE)
          ZB_ZCL_CONSTRUCT_COMMAND_HEADER_REQ(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_SERVER_CMD_GP_NOTIFICATION);
    ZB_ZCL_PACKET_PUT_DATA16(ptr, &options);
    ZB_ZCL_PACKET_PUT_DATA32(ptr, &gpdf_info.zgpd_id.addr.src_id);
    ZB_ZCL_PACKET_PUT_DATA32(ptr, &gpdf_info.sec_frame_counter);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, gpdf_info.zgpd_cmd_id);
    /* No payload */
    ZB_ZCL_PACKET_PUT_DATA8(ptr, 0);

    if (TEST_DEVICE_CTX.test_state == TEST_STATE_SEND_GP_NOTIFICATION2)
    {
        ZB_ZCL_PACKET_PUT_DATA16(ptr, &gpp_short_addr);
        ZB_ZCL_PACKET_PUT_DATA8(ptr, gpp_gpd_link);
    }

#ifdef ZB_USEALIAS
    ZB_ZCL_FINISH_PACKET_O(buf, ptr);
    {
        zb_addr_u a;
        a.addr_short = 0x0000; /* dst is DUT ZC */
        ZB_ZCL_SEND_COMMAND_SHORT_ALIAS(buf, &a, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZGP_ENDPOINT,
                                        ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                        ZB_ZCL_CLUSTER_ID_GREEN_POWER, 0 /* groupcast_radius */,
                                        NULL, ZB_FALSE, 0x0000, 0);
    }
#else
    ZB_ZCL_FINISH_PACKET(buf, ptr);
    ZB_ZCL_SEND_COMMAND_SHORT(buf, 0x0000, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZGP_ENDPOINT,
                              ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                              ZB_ZCL_CLUSTER_ID_GREEN_POWER, NULL);
#endif

    TRACE_MSG(TRACE_APP1, "<send_gp_notify", (FMT__0));
}

static void save_sec_frame_counter(zb_gpdf_info_t *gpdf_info)
{
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_GP_NOTIFICATION1:
    case TEST_STATE_SEND_GP_NOTIFICATION2:
        zgp_proxy_table_set_security_counter(&gpdf_info->zgpd_id, gpdf_info->sec_frame_counter);
        break;
    }
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_gpp");

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    zb_set_default_ffd_descriptor_values(ZB_ROUTER);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gpp_addr);
    ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
    /* Must use NVRAM for ZGP */

    /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 0;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

    ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
    ZGP_CTX().device_role = ZGP_DEVICE_PROXY;
    TEST_DEVICE_CTX.gp_pairing_hndlr_cb = gp_pairing_handler_cb;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()

#else // defined ZB_ENABLE_HA

#include <stdio.h>
MAIN()
{
    ARGV_UNUSED;

    printf("HA profile should be enabled in zb_config.h\n");

    MAIN_RETURN(1);
}

#endif
