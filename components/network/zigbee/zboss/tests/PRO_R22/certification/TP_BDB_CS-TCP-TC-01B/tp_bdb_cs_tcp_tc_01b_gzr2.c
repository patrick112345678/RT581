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
/* PURPOSE: TP_BDB_CS_TCP_TC_01B_GZR2 ZigBee Router (gZR2)
*/

#define ZB_TEST_NAME TP_BDB_CS_TCP_TC_01B_GZR2
#define ZB_TRACE_FILE_ID 40044

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"
#include "../nwk/nwk_internal.h"
#include "zb_console_monitor.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_gzr2 = IEEE_ADDR_gZR2;
static zb_uint8_t     s_step_idx;

enum test_steps_e
{
    START,
    FIRST_LEAVE,
    SECOND_LEAVE,
    LAST_LEAVE
};

typedef ZB_PACKED_PRE struct test_device_nvram_dataset_s
{
    zb_uint8_t step_idx;
    zb_uint8_t align[3];
} ZB_PACKED_STRUCT test_device_nvram_dataset_t;

#ifdef ZB_USE_NVRAM
static zb_uint16_t test_get_nvram_data_size();
static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);
#endif

static void test_send_leave_delayed(zb_uint8_t unused);
static void test_send_leave(zb_uint8_t param);
static void test_nlme_leave_request(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    char command_buffer[100], *command_ptr;
    char next_cmd[40];
    zb_bool_t res;

    ZB_INIT("zdo_3_gzr");
#if UART_CONTROL
    test_control_init();
#endif

#ifdef ZB_USE_NVRAM
    zb_nvram_register_app1_read_cb(test_nvram_read_app_data);
    zb_nvram_register_app1_write_cb(test_nvram_write_app_data, test_get_nvram_data_size);
#endif

    zb_set_long_address(g_ieee_addr_gzr2);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

    TRACE_MSG(TRACE_APP1, "Send 'erase' for flash erase or just press enter to be continued \n", (FMT__0));
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);
    zb_console_monitor_get_cmd((zb_uint8_t *)command_buffer, sizeof(command_buffer));
    command_ptr = (char *)(&command_buffer);
    res = parse_command_token(&command_ptr, next_cmd, sizeof(next_cmd));
    if (strcmp(next_cmd, "erase") == 0)
    {
        zb_set_nvram_erase_at_start(ZB_TRUE);
    }
    else
    {
        zb_set_nvram_erase_at_start(ZB_FALSE);
    }
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_ALARM(test_send_leave_delayed, 0, TEST_GZR2_LEAVE_DELAY);
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

#ifdef ZB_USE_NVRAM
static zb_uint16_t test_get_nvram_data_size()
{
    TRACE_MSG(TRACE_ERROR, "test_get_nvram_data_size, ret %hd", (FMT__H, sizeof(test_device_nvram_dataset_t)));
    return sizeof(test_device_nvram_dataset_t);
}

static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    test_device_nvram_dataset_t ds;
    zb_ret_t ret;

    TRACE_MSG(TRACE_ERROR, ">> test_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(ds));

    ret = zb_osif_nvram_read(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    if (ret == RET_OK)
    {
        s_step_idx = ds.step_idx;
    }

    TRACE_MSG(TRACE_ERROR, "<< test_nvram_read_app_data ret %d", (FMT__D, ret));
}

static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;
    test_device_nvram_dataset_t ds;

    TRACE_MSG(TRACE_ERROR, ">> test_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

    ds.step_idx = s_step_idx;

    ret = zb_osif_nvram_write(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    TRACE_MSG(TRACE_ERROR, "<< test_nvram_write_app_data, ret %d", (FMT__D, ret));

    return ret;
}
#endif /* ZB_USE_NVRAM */

static void test_send_leave_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    zb_buf_get_out_delayed(test_send_leave);
}

static void test_send_leave(zb_uint8_t param)
{
    s_step_idx++;
    zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
    zb_zdo_mgmt_leave_param_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
    zb_nlme_leave_request_t *req2 = ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t);
    TRACE_MSG(TRACE_ZDO1, ">>send_leave: buf_param = %d", (FMT__D, param));
    switch (s_step_idx)
    {
    case FIRST_LEAVE:
        TRACE_MSG(TRACE_APP1, "FIRST_LEAVE, s_step_idx = %d", (FMT__D, s_step_idx));
        req->rejoin = 1;
        req->dst_addr = zb_cert_test_get_network_addr();
        ZB_IEEE_ADDR_COPY(req->device_address, g_ieee_addr_gzr2);
        req->remove_children = 0;
        zdo_mgmt_leave_req(param, NULL);
        break;
    case SECOND_LEAVE:
        TRACE_MSG(TRACE_APP1, "SECOND_LEAVE, s_step_idx = %d", (FMT__D, s_step_idx));

        req2->remove_children = 0;
        req2->rejoin = 0;
        ZB_IEEE_ADDR_COPY(req2->device_address, g_ieee_addr_gzr2);
        test_nlme_leave_request(param);
        //construct_and_send_leave(param);
        //zb_buf_get_out_delayed_ext(construct_and_send_leave, 0, 0);
        break;
    case LAST_LEAVE:
        TRACE_MSG(TRACE_APP1, "LAST_LEAVE, s_step_idx = %d", (FMT__D, s_step_idx));
        req->rejoin = 0;
        req->dst_addr = zb_cert_test_get_network_addr();
        ZB_IEEE_ADDR_COPY(req->device_address, g_ieee_addr_gzr2);
        req->remove_children = 0;
        zdo_mgmt_leave_req(param, NULL);
        break;
    default:
        TRACE_MSG(TRACE_APP1, "Unknown state, stop test, s_step_idx = %d", (FMT__D, s_step_idx));
        break;
    }


    TRACE_MSG(TRACE_ZDO1, "<<send_leave", (FMT__0));
}

static void test_nlme_leave_request(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_neighbor_tbl_ent_t *nbt = NULL;
    zb_nwk_status_t status = ZB_NWK_STATUS_SUCCESS;
    zb_nlme_leave_request_t r;

    ZB_MEMCPY(&r, ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t), sizeof(r));
    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_leave_request %hd", (FMT__H, param));
    if (!ZB_JOINED())
    {
        TRACE_MSG(TRACE_ERROR, "got leave.request when not joined", (FMT__0));
        ret = RET_ERROR;
        status = ZB_NWK_STATUS_INVALID_REQUEST;
    }
    else if (!ZB_IEEE_ADDR_IS_ZERO(r.device_address)
             && !ZB_64BIT_ADDR_CMP(r.device_address, ZB_PIBCACHE_EXTENDED_ADDRESS()))
    {
#ifdef ZB_ROUTER_ROLE
        if (ZB_IS_DEVICE_ZC_OR_ZR())
        {
            if (zb_nwk_neighbor_get_by_ieee(r.device_address, &nbt) != RET_OK)
            {
                ret = RET_ERROR;
                status = ZB_NWK_STATUS_UNKNOWN_DEVICE;
                TRACE_MSG(TRACE_NWK2, "device is not in the neighbor", (FMT__0));
            }
            else if (nbt->device_type != ZB_NWK_DEVICE_TYPE_ED
#ifdef ZB_CERTIFICATION_HACKS
                     && !ZB_CERT_HACKS().enable_leave_to_router_hack
#endif
                    )
            {
                /* 3.2.2.16.3
                 *
                 * On receipt of this primitive by a Zigbee coordinator or
                 * Zigbee router and with the DeviceAddress parameter not
                 * equal to NULL and not equal to the local device's IEEE
                 * address, the NLME determines wether the specified device
                 * is in the Neighbor Table and the device type is 0x02
                 * (Zigbee End device). If the requested device doesn't exist
                 * or the device type is not 0x02, the NLME issues the
                 * NLME-LEAVE.confirm primitive with a status of
                 * UNKNOWN_DEVICE.
                 */
                ret = RET_ERROR;
                TRACE_MSG(TRACE_NWK2, "device is not ED", (FMT__0));
                status = ZB_NWK_STATUS_UNKNOWN_DEVICE;
            }
            else if (nbt->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
            {
                /*
                 * see 3.6.1.10.2 Method for a Device to Remove Its Child
                 * from the Network
                 *
                 * If the relationship field of the neighbor table entry
                 * corresponding to the device being removed has a value of
                 * 0x05, indicating that it is an unauthenticated child, the
                 * device shall not send a network leave command frame.
                 */

                /* Trigger NLME-LEAVE.confirm with status ZB_NWK_STATUS_UNKNOWN_DEVICE */
                TRACE_MSG(TRACE_NWK2, "device is not authenticated - don't send LEAVE to it (status ZB_NWK_STATUS_UNKNOWN_DEVICE)", (FMT__0));
                status = ZB_NWK_STATUS_UNKNOWN_DEVICE;
                ret = RET_ERROR;
            }
            else
            {
                ret = RET_OK;
            }
        }
        else
#endif  /* ZB_ROUTER_ROLE */
        {
            /* If not NULL or device itself and an ED, return an error */
            ret = RET_ERROR;
            TRACE_MSG(TRACE_NWK2, "device is unknown", (FMT__0));
            status = ZB_NWK_STATUS_UNKNOWN_DEVICE;
        }
    }
    else
    {
        ret = RET_OK;
    }

#ifdef ZB_COORDINATOR_ROLE
    if (ZB_IS_DEVICE_ZC() && ret == RET_OK && !nbt)
    {
        TRACE_MSG(TRACE_ERROR, "invalid param for coord", (FMT__0));
        ret = RET_ERROR;
        status = ZB_NWK_STATUS_INVALID_REQUEST;
    }
#endif

    if (ret == RET_OK)
    {
        /*
          a) local leave (device_address is empty/self-address)
          b) force remote device leave

          Anyway, send LEAVE_REQUEST
        */
        zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());
        zb_nwk_hdr_t *nwhdr;
        zb_uint8_t *lp;

        TRACE_MSG(TRACE_NWK3, "secure %hd", (FMT__H, secure));
        /* NOTE: ret is RET_ERROR if we are router and nbt is NULL. */
        if (nbt == NULL)
        {
            /* if no neighbor table entry, this is leave for us */
            ZG->nwk.leave_context.rejoin_after_leave = r.rejoin;
            nwhdr = nwk_alloc_and_fill_hdr(param,
                                           ZB_PIBCACHE_NETWORK_ADDRESS(),
                                           ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE, /* see 3.4.4.2  */
                                           ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);
        }
#ifdef ZB_ROUTER_ROLE
        else
        {
            zb_uint16_t child_addr;
            zb_address_short_by_ref(&child_addr, nbt->u.base.addr_ref);
            nwhdr = nwk_alloc_and_fill_hdr(param,
                                           ZB_PIBCACHE_NETWORK_ADDRESS(),
                                           child_addr,
                                           ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);
        }
#endif  /* ZB_ROUTER_ROLE */

#ifdef ZB_CERTIFICATION_HACKS
        if (ZB_CERT_HACKS().nwk_leave_from_unknown_addr == ZB_TRUE)
        {
            ZB_IEEE_ADDR_COPY(&nwhdr->src_ieee_addr, ZB_CERT_HACKS().nwk_leave_from_unknown_ieee_addr);
            nwhdr->src_addr = ZB_CERT_HACKS().nwk_leave_from_unknown_short_addr;
        }
#endif
        if (secure)
        {
            nwk_mark_nwk_encr(param);
        }
        /* Don't want it to be routed - see 3.4.4.2 */
        nwhdr->radius = 1;

        lp = (zb_uint8_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_LEAVE, (zb_uint8_t)sizeof(zb_uint8_t));
        *lp = 0;
        ZB_LEAVE_PL_SET_REJOIN(*lp, r.rejoin);
        ZB_LEAVE_PL_SET_REQUEST(*lp);
#ifdef ZB_ROUTER_ROLE
        // Set "remove_children" if device not ZED only
        ZB_LEAVE_PL_SET_REMOVE_CHILDREN(*lp, ZB_B2U(ZB_U2B(r.remove_children) && (nbt != NULL ? nbt->device_type != ZB_NWK_DEVICE_TYPE_ED : ZB_TRUE)));
        if (nbt != NULL)
        {
            //ZB_LEAVE_PL_SET_REQUEST(*lp);
            TRACE_MSG(TRACE_NWK3, "send leave.request request 1 rejoin %hd remove_children %hd",
                      (FMT__H_H, r.rejoin, r.remove_children));
        }
        else
#endif  /* router */
        {
            TRACE_MSG(TRACE_NWK3, "send leave.request request 0 (I am leaving) rejoin %hd", (FMT__H, r.rejoin));

            if (secure && !ZG->aps.authenticated)
            {
                /* See spec 4.6.3.6.3. A device that wishes to leave the
                 * network and does not have the active network key shall
                 * quietly leave the network without sending a NWK leave announcement.  */
                TRACE_MSG(TRACE_NWK3, "Skip sending, silent leave", (FMT__0));
                ret = RET_ERROR;
                status = ZB_NWK_STATUS_NO_KEY;
            }
        }
    }

    if (ret == RET_OK)
    {
        (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_LEAVE_CONFIRM_AT_DATA_CONFIRM_HANDLE);
#if (defined ZB_ROUTER_ROLE && defined ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN)
        /* NK: with stochastic addr assign is used, this check will never be ok */
        if (nbt)
        {
            zb_uint16_t child_addr;
            zb_address_short_by_ref(&child_addr, nbt->addr_ref);

            /* if we removed last joined device, we could decrease number of child to */
            /* to save some address space */

            /* TODO: check for child - ZR? */
            if (child_addr == ZB_NWK_ED_ADDRESS_ASSIGN() - 1)
            {
                ZB_NIB().ed_child_num--;
            }
        }
#endif
        ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
    }
    else
    {
        zb_nlme_leave_confirm_t *lc;

        TRACE_MSG(TRACE_NWK1, "leave.request failed %hd", (FMT__H, status));
        (void)zb_buf_reuse(param);
        lc = zb_buf_alloc_tail(param, sizeof(*lc));
        lc->status = status;
        ZB_IEEE_ADDR_COPY(lc->device_address, r.device_address);
        ZB_SCHEDULE_CALLBACK(zb_nlme_leave_confirm, param);
    }
    TRACE_MSG(TRACE_NWK1, "<<zb_nlme_leave_request %d", (FMT__D, ret));
}

/*! @} */
