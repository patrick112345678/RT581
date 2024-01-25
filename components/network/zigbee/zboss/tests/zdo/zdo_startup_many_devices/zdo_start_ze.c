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
/* PURPOSE: test for ZED application written using ZDO.
*/

#define ZB_TRACE_FILE_ID 40038
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_startup.h"
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_min.h"
#endif

//#define SUBGIG

#ifndef ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif
/*! \addtogroup ZB_TESTS */
/*! @{ */

zb_ieee_addr_t g_ze_addr = TEST_ZE_ADDR;
static zb_uint_t     g_packets_sent;
static zb_uint_t     g_packets_rcvd;

#define SUBGHZ_PAGE 1
#define SUBGHZ_CHANNEL 25

MAIN()
{
    ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR )
#ifdef ZB_NS_BUILD
    if ( argc < 3 )
    {
        printf("%s <read pipe path> <write pipe path>\n", argv[0]);
        return 0;
    }
#endif
#endif
    ZB_SET_TRAF_DUMP_ON();
    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_ze");



#ifdef ZB_SECURITY
    ZB_SET_NIB_SECURITY_LEVEL(5);
#endif

    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), g_ze_addr);

    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_FALSE);

#ifndef SUBGIG
    /* ZB_AIB().aps_channel_mask = (1L << CHANNEL); */
    zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL);
#else

    {
        zb_channel_list_t channel_list;
        zb_channel_list_init(channel_list);
        zb_channel_page_list_set_mask(channel_list, SUBGHZ_PAGE, 1 << SUBGHZ_CHANNEL);
        zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
        //zb_se_set_network_ed_role_select_device(channel_list);
    }
#endif

    /* Set to 1 to force entire nvram erase. */
    ZB_AIB().aps_nvram_erase_at_start = 1;
    /* set to 1 to enable nvram usage. */
    ZB_AIB().aps_use_nvram = 1;
    ZB_SET_TRAF_DUMP_ON();

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


static void test_exit(zb_uint8_t param)
{
    ZB_ASSERT(param == 0);
}


static zb_bool_t exchange_finished()
{
    return (zb_bool_t)(g_packets_sent >= PACKETS_FROM_ED_NR &&
                       g_packets_rcvd >= PACKETS_FROM_ZC_NR);
}


static void send_data(zb_bufid_t buf)
{
    zb_apsde_data_req_t *req = ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t);
    zb_uint8_t *ptr = NULL;
    zb_short_t i;

    req->dst_addr.addr_short = 0; /* send to ZC */
    req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    req->radius = 1;
    req->profileid = 2;
    req->src_endpoint = 10;
    req->dst_endpoint = 10;
    req->clusterid = 0xf00d;

    ptr = zb_buf_initial_alloc(buf, ZB_TEST_DATA_SIZE);

    for (i = 0 ; i < ZB_TEST_DATA_SIZE ; ++i)
    {
        ptr[i] = i % 32 + '0';
    }
    TRACE_MSG(TRACE_APS2, "Sending apsde_data.request", (FMT__0));

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, buf);
    /* Poll for incoming data */
    TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
    zb_zdo_pim_start_turbo_poll_packets(1);
}


void send_data_loop(zb_uint8_t param)
{
    zb_bufid_t   buf;

    ZB_ASSERT(param == 0);
    buf = zb_buf_get_out();
    if (buf == 0)
    {
        TRACE_MSG(TRACE_ERROR, "Can't allocate buffer!", (FMT__0));
        ZB_SCHEDULE_ALARM(send_data_loop, 0, 1 * ZB_TIME_ONE_SECOND );
        return;
    }
    g_packets_sent++;
    TRACE_MSG(TRACE_ERROR, "Sending apsde_data.request ED", (FMT__0));
    send_data(buf);
    if (g_packets_sent < PACKETS_FROM_ED_NR)
    {
        ZB_SCHEDULE_ALARM(send_data_loop, 0, 1 * ZB_TIME_ONE_SECOND ); //
    }
    else
    {
        if (exchange_finished())
        {
            ZB_SCHEDULE_ALARM(test_exit, 0, 1 * ZB_TIME_ONE_SECOND);
        }
    }
}


zb_uint8_t data_indication(zb_uint8_t param)
{
    zb_ushort_t i;
    zb_uint8_t *ptr;
    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

    if (ind->profileid == 0x0002)
    {
        /* Remove APS header from the packet */
        ZB_APS_HDR_CUT_P(param, ptr);

        TRACE_MSG(TRACE_APS2, "data_indication: packet %hd len %d handle 0x%x",
                  (FMT__H_D_D,
                   param, (int)zb_buf_len(param), zb_buf_get_status(param)));
        for (i = 0; i < zb_buf_len(param); ++i)
        {
            TRACE_MSG(TRACE_APS3, "%x %c", (FMT__D_C, (int)ptr[i], ptr[i]));
            if (ptr[i] != i % 32 + '0')
            {
                TRACE_MSG(TRACE_ERROR, "Bad data %hx %c wants %x %c", (FMT__H_C_D_C, ptr[i], ptr[i],
                          (int)(i % 32 + '0'), (char)i % 32 + '0'));
            }
        }
        g_packets_rcvd++;
        if (exchange_finished())
        {
            ZB_SCHEDULE_ALARM(test_exit, 0, 1 * ZB_TIME_ONE_SECOND);
        }
        zb_buf_free(param);
        return ZB_TRUE;               /* processed */
    }
    return ZB_FALSE;
}


void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        //! [signal_skip_startup]
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            zb_af_set_data_indication(data_indication);

            if (PACKETS_FROM_ED_NR != 0)
            {
                ZB_SCHEDULE_CALLBACK(send_data_loop, 0);
            }
            break;
        //! [signal_sleep]
        case ZB_COMMON_SIGNAL_CAN_SLEEP:
        {
            zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_can_sleep_params_t);
            /* TODO: check if app is ready for sleep and sleep_tmo is ok, disable peripherals if needed */
            TRACE_MSG(TRACE_ERROR, "Can sleep for %ld ms", (FMT__L, can_sleep_params->sleep_tmo));
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif
            /* TODO: enable peripherals if needed */
        }
        break;
        //! [signal_sleep]
        default:
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Device START failed", (FMT__0));
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    zb_buf_free(param);
}


/*! @} */
