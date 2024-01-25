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
/* PURPOSE: APS Unencrypted Transport Key feature coordinator
*/

#define ZB_TRACE_FILE_ID 40139
#include "aps_unencrypted_tkey_tests_zc.h"
#include "aps_unencrypted_tkey_tests_zc_hal.h"

zb_ieee_addr_t g_zc_addr = APS_UNENCRYPTED_TKEY_TESTS_ZC_ADDRESS;
zb_uint16_t g_current_aps_req_dest_short_addr = ZB_NWK_BROADCAST_ALL_DEVICES;

aps_unencrypted_tkey_tests_zc_ctx_t zc_ctx;

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static void init_ctx()
{
    zb_uint8_t i;

    ZB_BZERO(&zc_ctx, sizeof(aps_unencrypted_tkey_tests_zc_ctx_t));

    for (i = 0; i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES; ++i)
    {
        zc_ctx.devices[i].led_num = 0xFF;
    }
}

#ifdef ZB_USE_NVRAM
void nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    application_dataset_t ds;
    zb_uint8_t i;
    ZB_ASSERT(payload_length == sizeof(ds));

    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_read_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    ZB_MEMCPY(&zc_ctx.devices, &ds, sizeof(ds));
    for (i = 0; i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES; i++)
    {
        if (zc_ctx.devices[i].dev_type == SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY)
        {
            aps_unencrypted_tkey_zc_device_joined_indication(zc_ctx.devices[i].led_num);
        }
    }
}

zb_ret_t nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;
    application_dataset_t ds;
    ZB_MEMCPY(&ds, &zc_ctx.devices, sizeof(application_dataset_t));
    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));
    return ret;
}

zb_uint16_t nvram_get_app_data_size(void)
{
    return sizeof(application_dataset_t);
}
#endif

void aps_unencrypted_tkey_tests_zc_send_leave_req_cb(zb_uint8_t param)
{
#ifdef ZB_USE_NVRAM
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
    zb_free_buf(ZB_BUF_FROM_REF(param));
}

void aps_unencrypted_tkey_tests_zc_send_leave_req(zb_uint8_t param, zb_uint16_t short_addr)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_mgmt_leave_param_t *req_param;
    zb_address_ieee_ref_t addr_ref;
    zb_uint8_t i;

    for (i = 0; i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES && zc_ctx.devices[i].nwk_addr != short_addr; i++);
    if (i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES)
    {
        aps_unencrypted_tkey_zc_device_leaved_indication(zc_ctx.devices[i].led_num);
        zc_ctx.devices[i].led_num = 0xFF;
    }

    if (zb_address_by_short(short_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
        req_param = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_leave_param_t);
        ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

        req_param->dst_addr = short_addr;
        req_param->rejoin = 0;
        zdo_mgmt_leave_req(param, aps_unencrypted_tkey_tests_zc_send_leave_req_cb);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "tried to remove 0x%xd, but device is already left", (FMT__D, short_addr));
        zb_free_buf(buf);
    }
}

void aps_unencrypted_tkey_tests_zc_delayed_leave(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_uint8_t i;
    for (i = 0; i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES &&
            zc_ctx.devices[i].dev_type != SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY; i++);
    if (i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES)
    {
        zc_ctx.devices[i].dev_type = SIMPLE_DEV_TYPE_UNUSED;
        ZB_GET_OUT_BUF_DELAYED2(aps_unencrypted_tkey_tests_zc_send_leave_req, zc_ctx.devices[i].nwk_addr);
    }
}

static void aps_unencrypted_tkey_tests_zc_unicast_send_buffer_aps_payload(zb_uint8_t param, zb_uint16_t dst_addr)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_addr_u addr_u = { 0 };

    zb_uint8_t payload_size = 10;
    zb_uint8_t payload_ptr[10] = { 0x01 };
    zb_uint8_t i;

    TRACE_MSG(TRACE_APP1, ">> send_aps_payload, param: %hd", (FMT__H, param));

    for (i = 0; i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES; i++)
    {
        if (zc_ctx.devices[i].dev_type == SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY &&
                dst_addr == zc_ctx.devices[i].nwk_addr)
        {
            addr_u.addr_short = zc_ctx.devices[i].nwk_addr;
            zb_aps_send_user_payload(
                param,
                addr_u,  /* dst_addr */
                0x1111,  /* profile id */
                0x2222,  /* cluster id */
                33,      /* destination endpoint */
                44,      /* source endpoint */
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_TRUE,
                payload_ptr,
                payload_size);
            g_current_aps_req_dest_short_addr = zc_ctx.devices[i].nwk_addr;
        }
    }
    TRACE_MSG(TRACE_APP1, "<< send_aps_payload", (FMT__0));
}

static void aps_unencrypted_tkey_tests_zc_broadcast_send_buffer_aps_payload(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_addr_u addr_u = { 0 };

    zb_uint8_t payload_size = 7;
    zb_uint8_t payload_ptr[7] = { 0x33, 0x31, 0x3d, 0x30, 0x31, 0x45, 0x30 };

    TRACE_MSG(TRACE_APP1, ">> send_aps_payload, param: %hd", (FMT__H, param));

    addr_u.addr_short = ZB_NWK_BROADCAST_ALL_DEVICES;
    zb_aps_send_user_payload(
        param,
        addr_u,  /* dst_addr */
        0xc091,  /* profile id */
        0x000b,  /* cluster id */
        1,      /* destination endpoint */
        1,      /* source endpoint */
        ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        ZB_FALSE,
        payload_ptr,
        payload_size);

    TRACE_MSG(TRACE_APP1, "<< send_aps_payload", (FMT__0));
}

static void aps_unencrypted_tkey_tests_zc_unicast_send_buffer_aps_payload_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_GET_OUT_BUF_DELAYED(aps_unencrypted_tkey_tests_zc_broadcast_send_buffer_aps_payload);
    ZB_SCHEDULE_APP_ALARM(aps_unencrypted_tkey_tests_zc_unicast_send_buffer_aps_payload_delayed, 0, 45 * ZB_TIME_ONE_SECOND);
}

static void aps_unencrypted_tkey_tests_zc_send_buffer_aps_payload_cb(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t aps_payload_size = 0;
    zb_uint8_t *aps_payload_ptr = zb_aps_get_aps_payload(param, &aps_payload_size);
    TRACE_MSG(TRACE_APP1, ">> send_aps_payload_cb, param: %hd, buf_status: 0x%x",
              (FMT__H_D, param, buf->u.hdr.status));

    if (param)
    {
        zb_uint8_t buf_len = ZB_BUF_LEN(buf);

        TRACE_MSG(TRACE_APP1, "buf_len: %hd, aps_payload_size: %hd",
                  (FMT__H_H, buf_len, aps_payload_size));

        switch ((zb_aps_user_payload_cb_status_t)buf->u.hdr.status)
        {
        case ZB_APS_USER_PAYLOAD_CB_STATUS_SUCCESS:
            if (g_current_aps_req_dest_short_addr != ZB_NWK_BROADCAST_ALL_DEVICES)
            {
                zb_uint8_t i;
                for (i = 0; i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES &&
                        zc_ctx.devices[i].nwk_addr != g_current_aps_req_dest_short_addr; i++);
                if (i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES)
                {
                    aps_unencrypted_tkey_zc_device_message_indication(zc_ctx.devices[i].led_num);
                    ZB_SCHEDULE_APP_ALARM(aps_unencrypted_tkey_zc_device_message_indication, zc_ctx.devices[i].led_num, ZB_TIME_ONE_SECOND);
                    g_current_aps_req_dest_short_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
                }
            }
            TRACE_MSG(TRACE_APP1, "Transmission status: SUCCESS", (FMT__0));
            break;

        case ZB_APS_USER_PAYLOAD_CB_STATUS_NO_APS_ACK:
            TRACE_MSG(TRACE_APP1, "Transmission status: NO_APS_ACK", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Transmission status: INVALID", (FMT__0));
            break;
        }

        zb_free_buf(buf);
    }

    TRACE_MSG(TRACE_APP1, "<< send_aps_payload_cb", (FMT__0));
}

void aps_unencrypted_tkey_tests_zc_button_handler(zb_uint8_t button_no)
{
    switch (button_no)
    {
    case APS_TKEY_SECURITY_BUTTON:
        zb_is_transport_key_aps_encryption_enabled() ?
        zb_disable_transport_key_aps_encryption() :
        zb_enable_transport_key_aps_encryption();
        zc_ctx.button_aps_unencrypted_tkey.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE;
        break;
    case ZDO_LEAVE_BUTTON:
        ZB_SCHEDULE_APP_CALLBACK(aps_unencrypted_tkey_tests_zc_delayed_leave, 0);
        zc_ctx.button_zdo_leave.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE;
        break;
    case SEND_TO_FIRST_DEVICE:
        ZB_GET_OUT_BUF_DELAYED2(aps_unencrypted_tkey_tests_zc_unicast_send_buffer_aps_payload, zc_ctx.devices[0].nwk_addr);
        zc_ctx.button_first_device.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE;
        break;
    case SEND_TO_SECOND_DEVICE:
        ZB_GET_OUT_BUF_DELAYED2(aps_unencrypted_tkey_tests_zc_unicast_send_buffer_aps_payload, zc_ctx.devices[1].nwk_addr);
        zc_ctx.button_second_device.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE;
        break;
    default:
        break;
    }
}

void aps_unencrypted_tkey_tests_zc_button_pressed(zb_uint8_t button_no)
{
    switch (button_no)
    {
    case APS_TKEY_SECURITY_BUTTON:
    {
        switch (zc_ctx.button_aps_unencrypted_tkey.button_state)
        {
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE:
            zc_ctx.button_aps_unencrypted_tkey.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED;
            zc_ctx.button_aps_unencrypted_tkey.timestamp = ZB_TIMER_GET();
            break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED:
            zc_ctx.button_aps_unencrypted_tkey.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED;
            ZB_SCHEDULE_APP_ALARM(aps_unencrypted_tkey_tests_zc_button_handler, button_no, APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_DEBOUNCE_PERIOD);
            break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED:
        default:
            break;
        }
        break;
    }
    case ZDO_LEAVE_BUTTON:
    {
        switch (zc_ctx.button_zdo_leave.button_state)
        {
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE:
            zc_ctx.button_zdo_leave.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED;
            zc_ctx.button_zdo_leave.timestamp = ZB_TIMER_GET();
            break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED:
            zc_ctx.button_zdo_leave.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED;
            ZB_SCHEDULE_APP_ALARM(aps_unencrypted_tkey_tests_zc_button_handler, button_no, APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_DEBOUNCE_PERIOD);
            break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED:
        default:
            break;
        }
        break;
    }
    case SEND_TO_FIRST_DEVICE:
    {
        switch (zc_ctx.button_first_device.button_state)
        {
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE:
            zc_ctx.button_first_device.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED;
            zc_ctx.button_first_device.timestamp = ZB_TIMER_GET();
            break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED:
            zc_ctx.button_first_device.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED;
            ZB_SCHEDULE_APP_ALARM(aps_unencrypted_tkey_tests_zc_button_handler, button_no, APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_DEBOUNCE_PERIOD);
            break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED:
        default:
            break;
        }
        break;
    }
    case SEND_TO_SECOND_DEVICE:
    {
        switch (zc_ctx.button_second_device.button_state)
        {
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE:
            zc_ctx.button_second_device.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED;
            zc_ctx.button_second_device.timestamp = ZB_TIMER_GET();
            break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED:
            zc_ctx.button_second_device.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED;
            ZB_SCHEDULE_APP_ALARM(aps_unencrypted_tkey_tests_zc_button_handler, button_no, APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_DEBOUNCE_PERIOD);
            break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED:
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

MAIN()
{
    ARGV_UNUSED;

    aps_unencrypted_tkey_zc_hal_init();
    init_ctx();

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_OFF();

    ZB_INIT("zdo_zc");

    zb_set_long_address(g_zc_addr);
    zb_set_network_coordinator_role(APS_UNENCRYPTED_TKEY_TESTS_CHANNEL_MASK);

    /* nRF52840: Erase NVRAM if BUTTON3 is pressed on start */
    zb_set_nvram_erase_at_start(aps_unencrypted_tkey_zc_hal_is_button_pressed());

    zb_aps_set_user_data_tx_cb(aps_unencrypted_tkey_tests_zc_send_buffer_aps_payload_cb);

#ifdef ZB_USE_NVRAM
    zb_nvram_register_app1_read_cb(nvram_read_app_data);
    zb_nvram_register_app1_write_cb(nvram_write_app_data, nvram_get_app_data_size);
#endif

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

static void device_annce_handler(zb_uint8_t buf_ref)
{
    zb_uint8_t i;

    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_signal_device_annce_params_t *dev_annce_params;

    zb_get_app_signal(buf_ref, &sg_p);
    dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

    TRACE_MSG(TRACE_APP1, "dev_annce short_addr 0x%x", (FMT__D, dev_annce_params->device_short_addr));

    for (i = 0; i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES; i++)
    {
        if (zc_ctx.devices[i].dev_type == SIMPLE_DEV_TYPE_UNUSED)
        {
            zc_ctx.devices[i].nwk_addr = dev_annce_params->device_short_addr;
            zb_address_ieee_by_short(zc_ctx.devices[i].nwk_addr, zc_ctx.devices[i].ieee_addr);
            zc_ctx.devices[i].dev_type = zb_is_transport_key_aps_encryption_enabled() ?
                                         (zb_uint8_t)SIMPLE_DEV_TYPE_APS_ENCRYPTED_TKEY :
                                         (zb_uint8_t)SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY;

            if (zc_ctx.devices[i].dev_type == (zb_uint8_t)SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY)
            {
                zb_uint8_t led_ind = aps_unencrypted_tkey_zc_device_joined_indication();
                zc_ctx.devices[i].led_num = led_ind;
            }
#ifdef ZB_USE_NVRAM
            /* If we fail, trace is given and assertion is triggered */
            (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
            break;
        }
    }
}


void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            zb_bdb_set_legacy_device_support(1);
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            ZB_GET_OUT_BUF_DELAYED(aps_unencrypted_tkey_tests_zc_unicast_send_buffer_aps_payload_delayed);
            break;
        case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
            ZB_SCHEDULE_APP_ALARM(device_annce_handler, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
            param = 0;
            break;
        case ZB_ZDO_SIGNAL_LEAVE_INDICATION:
        {
            zb_uint8_t i;
            zb_zdo_signal_leave_indication_params_t *leave_ind_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_indication_params_t);
            for (i = 0; i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES &&
                    (ZB_MEMCMP(&zc_ctx.devices[i].ieee_addr, &leave_ind_params->device_addr, sizeof(zb_ieee_addr_t)) != 0 ||
                     zc_ctx.devices[i].dev_type != SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY); i++);
            if (i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES)
            {
                zc_ctx.devices[i].dev_type = SIMPLE_DEV_TYPE_UNUSED;
                aps_unencrypted_tkey_zc_device_leaved_indication(zc_ctx.devices[i].led_num);
#ifdef ZB_USE_NVRAM
                /* If we fail, trace is given and assertion is triggered */
                (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
            }
        }
        break;
        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal 0x%hx", (FMT__H, sig));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }
}
