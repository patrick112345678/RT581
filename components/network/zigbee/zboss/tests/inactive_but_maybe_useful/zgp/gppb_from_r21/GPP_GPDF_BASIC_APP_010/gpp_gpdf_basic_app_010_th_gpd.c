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


#define ZB_TEST_NAME GPP_GPDF_BASIC_APP_010_TH_GPD
#define ZB_TRACE_FILE_ID 41512
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_uint8_t   g_zgpd_key[] = TEST_SEC_KEY;

static zb_ieee_addr_t g_zgpd_addr = TH_GPD_IEEE_ADDR;


/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_SET_MAC_DSN_INIT,
    TEST_STATE_COMMISSIONING1,
    TEST_STATE_SET_MAC_DSN,      /*Set DSN 0xC3*/
    TEST_STATE_SEND_DATA_GPDF1,  /*DSN = 0xC3*/
    TEST_STATE_SEND_DATA_GPDF2,  /*DSN = 0xC4*/
    TEST_STATE_SEND_DATA_GPDF3,  /*DSN = 0xC5 (1)*/
    TEST_STATE_SET_MAC_DSN1,     /*Set DSN 0xC5*/
    TEST_STATE_SEND_DATA_GPDF4,  /*DSN = 0xC5 (2)*/
    TEST_STATE_SET_MAC_DSN2,     /*Set DSN 0xC5*/
    TEST_STATE_SEND_DATA_GPDF5,  /*DSN = 0xC5 (3)*/
    TEST_STATE_SEND_DATA_GPDF6,  /*DSN = 0xC6*/
    TEST_STATE_SEND_DATA_GPDF7,  /*DSN = 0xC7*/
    TEST_STATE_SEND_DATA_GPDF8,  /*DSN = 0xC8 (1)*/
    TEST_STATE_SET_MAC_DSN3,     /*Set DSN 0xC8*/
    TEST_STATE_SEND_DATA_GPDF9,  /*DSN = 0xC8 (2)*/
    TEST_STATE_SEND_DATA_GPDF10,  /*DSN = 0xC9*/
    TEST_STATE_SEND_DATA_GPDF11, /*DSN = 0xCA*/
    TEST_STATE_SEND_DATA_GPDF12, /*DSN = 0xCB*/
    TEST_STATE_SEND_DATA_GPDF13, /*DSN = 0xCC*/
    TEST_STATE_SEND_DATA_GPDF14, /*DSN = 0xCD*/
    TEST_STATE_SEND_DATA_GPDF15, /*DSN = 0xCE*/
    TEST_STATE_SEND_DATA_GPDF16, /*DSN = 0xCF*/
    TEST_STATE_SEND_DATA_GPDF17, /*DSN = 0xD0*/
    TEST_STATE_SEND_DATA_GPDF18, /*DSN = 0xD1*/
    TEST_STATE_SEND_DATA_GPDF19, /*DSN = 0xD2*/
    TEST_STATE_SEND_DATA_GPDF20, /*DSN = 0xD3*/
    TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 2000)

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
    ZVUNUSED(buf);
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_DATA_GPDF1: /*Step 1*/
    {
        ZGPD->ext_nwk_present = 1;
        //      ZGPD->security_key_type = 1;
        ZGPD->ch_replace_key_type = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_KEY_TYPE);  /*TODO: This hack does
                                        * not works, need debugging*/
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF2: /*Step 2*/
    {
        ZGPD->ext_nwk_present = 1;
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_MISSING_IEEE);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF3: /*Step 3A*/
    {
        ZGPD->ext_nwk_present = 1;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
        ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_RESERVED2;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF4: /*Step 3B*/
    {
        ZGPD->ext_nwk_present = 1;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
        ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_RESERVED1;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF5: /*Step 3C*/
    {
        ZGPD->ext_nwk_present = 1;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
        ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_MAINTENANCE;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF6: /*Step 4*/
    {
        //      ZGPD->ext_nwk_present = 1;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_PROTO_VERSION);
        ZGPD->ch_replace_proto_version = 2;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF7: /*Step 5A*/
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
        ZGPD->ch_replace_extnwk_flag = 0;
        ZGPD->ch_insert_extnwk_data = 1;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF8: /*Step 5B*/
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
        ZGPD->ch_replace_extnwk_flag = 1;
        ZGPD->ch_insert_extnwk_data = 0;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF9: /*Step 5C*/
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
        ZGPD->ch_replace_extnwk_flag = 0;
        ZGPD->ch_insert_extnwk_data = 0;
    }
    case TEST_STATE_SEND_DATA_GPDF10: /*Step 6*/
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
        ZGPD->ch_replace_app_id = 1;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF11: /*Step 7*/
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
        ZGPD->ch_replace_app_id = 3;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF12: /*Step 8*/
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SEC_LEVEL);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_KEY_TYPE);
        ZGPD->ch_replace_sec_level = 2;
        ZGPD->ch_replace_key_type = ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF13: /*Step 9*/
    {
        ZGPD->ext_nwk_present = 1;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_DIRECTION);
        ZGPD->ch_replace_direction = 1;
        ZB_ZGPD_SET_PAUSE(4);
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF14: /*Step 10*/
    {
        zb_uint8_t i;

        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_ATTR_REPORT);
        for (i = 0; i < 54; i++)
        {
            ZB_GPDF_PUT_UINT8(*ptr, i);
        }
        ZB_ZGPD_CHACK_RESET_ALL();
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF15: /*Step 11*/
    {
        ZGPD->ext_nwk_present = 1;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 1;
        ZGPD->ch_replace_autocomm = 1;
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF16: /*Step 12A*/
    {
        ZGPD->ext_nwk_present = 1;
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_CORRUPT_IEEE_ADDR);
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF17: /*Step 12B*/
    {
        ZGPD->ext_nwk_present = 1;
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_BZERO_IEEE_ADDR);
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF18: /*Step 13*/
    {
        ZGPD->ext_nwk_present = 1;
        ZB_ZGPD_CHACK_RESET_ALL();
        g_zgpd_ctx.id.endpoint = 5;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF19: /*Step 14*/
    {
        ZGPD->ext_nwk_present = 1;
        g_zgpd_ctx.id.endpoint = 0;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF20: /*Step 15*/
    {
        ZGPD->ext_nwk_present = 1;
        g_zgpd_ctx.id.endpoint = 0xff;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    }
    };
}

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void zgpd_set_dsn(zb_uint8_t param, zb_callback_t cb)
{
    zgpd_set_dsn_and_call(param, cb);
}

static void perform_next_state(zb_uint8_t param)
{
    TEST_DEVICE_CTX.test_state++;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_COMMISSIONING1:
        zb_zgpd_start_commissioning(comm_cb);
        ZB_ZGPD_SET_PAUSE(2);
        break;
    case TEST_STATE_SET_MAC_DSN_INIT:
    {
        if (param == 0)
        {
            ZB_GET_OUT_BUF_DELAYED(perform_next_state);
            TEST_DEVICE_CTX.test_state--;
            break;
        }
        ZGPD->mac_dsn = 0x81;
        zgpd_set_dsn(param, perform_next_state);
        break;
    }
    case TEST_STATE_SET_MAC_DSN:
    {
        if (param == 0)
        {
            ZB_GET_OUT_BUF_DELAYED(perform_next_state);
            TEST_DEVICE_CTX.test_state--;
            break;
        }
        ZGPD->mac_dsn = 0xc3;
        zgpd_set_dsn(param, perform_next_state);
        break;
    }
    case TEST_STATE_SET_MAC_DSN1:
    case TEST_STATE_SET_MAC_DSN2:
    {
        if (param == 0)
        {
            ZB_GET_OUT_BUF_DELAYED(perform_next_state);
            TEST_DEVICE_CTX.test_state--;
            break;
        }
        ZGPD->mac_dsn = 0xc5;
        zgpd_set_dsn(param, perform_next_state);
        break;
    }
    case TEST_STATE_SET_MAC_DSN3:
    {
        if (param == 0)
        {
            ZB_GET_OUT_BUF_DELAYED(perform_next_state);
            TEST_DEVICE_CTX.test_state--;
            break;
        }
        ZGPD->mac_dsn = 0xc8;
        zgpd_set_dsn(param, perform_next_state);
        break;
    }
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
    };
}


static void zgp_custom_startup()
{
#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("th_gpd");


    ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0010, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr);
    ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
    g_zgpd_ctx.id.endpoint = 1;
    ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_NO_SECURITY);
    ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
    ZB_ZGPD_SET_SECURITY_KEY(g_zgpd_key);
    ZB_ZGPD_SET_OOB_KEY(g_zgpd_key);

    ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
