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
/* PURPOSE: Touchlink Non-color Scene controller device
*/

#define ZB_TRACE_FILE_ID 40192
#include "zboss_api.h"
#include "test_defs.h"
#include "zb_tl_non_color_scene_controller.h"

#ifdef ZB_USE_BUTTONS
#include "light_controller_hal.h"
#endif

/* Custom Touchlink master key. Should be set by calling zb_zdo_touchlink_set_master_key.
   If not set, the default Touchlink key will be used. */

/* zb_uint8_t master_key[ZB_CCM_KEY_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
*/

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile zed tests
#endif

/** Number of the endpoint device operates on. */
#define ENDPOINT  10
#define MOVE_TO_LEVEL_INTERVAL (15 * ZB_TIME_ONE_SECOND)
#define MOVE_TO_LEVEL_SHIFT_VALUE 200
#define MOVE_TO_LEVEL_TRANSITION_TIME 10

/** Start device parameters checker. */
void test_check_start_status(zb_uint8_t param);
void send_move_to_level_cmd(zb_uint8_t param);

zb_uint16_t light_control_get_nvram_data_size();
void light_control_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
zb_ret_t light_control_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);

/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
zb_uint16_t g_profile_id = ZB_AF_HA_PROFILE_ID;

/** security keys. */
zb_uint8_t g_key[16]               = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

#define LIGHT_CONTROL_BULBS_MAX_NUM 8

typedef ZB_PACKED_PRE struct light_bulb_info_s
{
    zb_ieee_addr_t ieee_addr;
    zb_uint16_t short_addr;
    zb_uint8_t endpoint;
    zb_uint8_t aligned;
} ZB_PACKED_STRUCT
light_bulb_info_t;

typedef ZB_PACKED_PRE struct light_control_device_nvram_dataset_s
{
    light_bulb_info_t bulbs[LIGHT_CONTROL_BULBS_MAX_NUM];
} ZB_PACKED_STRUCT
light_control_device_nvram_dataset_t;

typedef ZB_PACKED_PRE struct light_control_device_ctx_t
{
    light_control_device_nvram_dataset_t p; /* persistent */
    zb_ieee_addr_t pending_dev_addr;   /* addr of device which is pending for discovery */
    zb_uint8_t pending_dev_ep;   /* ep of device which is pending for discovery */
    zb_bool_t device_on_off;
    zb_int_t device_level;
} ZB_PACKED_STRUCT
light_control_device_ctx_t;

light_control_device_ctx_t g_device_ctx;


/********************* Declare device **************************/
ZB_TL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CLUSTER_LIST(non_color_scene_controller_clusters,
        ZB_ZCL_CLUSTER_MIXED_ROLE);

ZB_TL_DECLARE_NON_COLOR_SCENE_CONTROLLER_EP(non_color_scene_controller_ep, ENDPOINT,
        non_color_scene_controller_clusters);

ZB_TL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CTX(non_color_scene_controller_ctx,
        non_color_scene_controller_ep);

void light_control_bind_bulb_on_off(zb_uint8_t param, zb_uint8_t idx);
void light_control_bind_bulb_level_control(zb_uint8_t param);
void light_control_bind_bulb_completed(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

#ifdef ZB_USE_BUTTONS
    light_control_hal_init();
#endif

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_ON();

    ZB_INIT("light_controller_zed");

    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&non_color_scene_controller_ctx);

    zb_set_long_address(g_ed_addr);
    zb_set_network_ed_role(ZB_TRANSCEIVER_ALL_CHANNELS_MASK);
    zb_zdo_touchlink_set_nwk_channel(TEST_CHANNEL);
    zb_set_rx_on_when_idle(ZB_TRUE);
    /* 01/26/2018 EE CR:MINOR Add a comment about debug stuff */
    zb_secur_setup_nwk_key(g_key, 0);

#ifdef ZB_USE_BUTTONS
    /* Erase NVRAM if BUTTON1 is pressed on start */
    zb_set_nvram_erase_at_start(light_control_hal_is_button_pressed(BULB_BUTTON_2_IDX));
#else
    zb_set_nvram_erase_at_start(ZB_FALSE);
#endif

    /*Uncomment to set the custom master key*/
    /*zb_zdo_touchlink_set_master_key(master_key);*/

    zb_nvram_register_app1_read_cb(light_control_nvram_read_app_data);
    zb_nvram_register_app1_write_cb(light_control_nvram_write_app_data, light_control_get_nvram_data_size);

    ZB_BZERO(&g_device_ctx, sizeof(light_control_device_ctx_t));

    if (zboss_start_no_autostart() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR zboss_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

void send_on_off_cmd(zb_uint8_t param)
{
    zb_uint16_t addr = 0;

    if (!param)
    {
        /* Button is pressed, get buffer for outgoing command */
        zb_buf_get_out_delayed(send_on_off_cmd);
    }
    else
    {
        zb_bufid_t buf = param;
        if (g_device_ctx.device_on_off)
        {
            ZB_ZCL_ON_OFF_SEND_ON_REQ(
                buf,
                addr,
                ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
                0,
                ENDPOINT,
                g_profile_id,
                ZB_TRUE,
                NULL);
        }
        else
        {
            ZB_ZCL_ON_OFF_SEND_OFF_REQ(
                buf,
                addr,
                ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
                0,
                ENDPOINT,
                g_profile_id,
                ZB_TRUE,
                NULL);
        }

        g_device_ctx.device_on_off = (zb_bool_t)!g_device_ctx.device_on_off;
    }
}

void send_move_to_level_cmd(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ERROR, "send_move_to_level_cmd %i", (FMT__H, param));
    if (!param)
    {
        /* Button is pressed, get buffer for outgoing command */
        zb_buf_get_out_delayed(send_move_to_level_cmd);
    }
    else
    {
        zb_bufid_t buf = param;
        zb_uint16_t addr = 0;

        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_TO_LEVEL_REQ(buf,
                addr,
                ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
                0, ENDPOINT, g_profile_id,
                ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
                NULL,
                g_device_ctx.device_level,
                MOVE_TO_LEVEL_TRANSITION_TIME);
        g_device_ctx.device_level = ZB_ABS(g_device_ctx.device_level - MOVE_TO_LEVEL_SHIFT_VALUE); /* Shift value to 100 */

#ifndef ZB_USE_BUTTONS
        /* Do not have buttons in simulator - just start periodic cmd sending */
        ZB_SCHEDULE_APP_ALARM(send_move_to_level_cmd, 0, MOVE_TO_LEVEL_INTERVAL);
#endif
    }
}

#ifdef LIGHT_CONTROL_BDB_FINDING_BINDING
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_APP1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
    return ZB_TRUE;
}

void start_bdb_initiator(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_bdb_finding_binding_initiator(ENDPOINT, finding_binding_cb);
}
#endif

void find_light_bulb_cb(zb_uint8_t param);

void find_light_bulb(zb_uint8_t param, zb_uint16_t short_addr, zb_uint8_t ep)
{
    zb_bufid_t buf = param;
    /* zb_zdo_match_desc_param_t *req; */
    zb_zdo_simple_desc_req_t *req;

    TRACE_MSG(TRACE_ZCL1, ">> find_light_bulb: param %hd addr 0x%x ep %hd", (FMT__H_D_H, param, short_addr, ep));

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_simple_desc_req_t));
    ZB_BZERO(req, sizeof(zb_zdo_simple_desc_req_t));

    req->nwk_addr = short_addr;
    req->endpoint = ep;
    if (zb_zdo_simple_desc_req(param, find_light_bulb_cb) == ZB_ZDO_INVALID_TSN)
    {
        ZB_ASSERT(0);
    }

    ZB_IEEE_ADDR_ZERO(g_device_ctx.pending_dev_addr);
    g_device_ctx.pending_dev_ep = 0;

    TRACE_MSG(TRACE_ZCL1, "<< find_light_bulb", (FMT__0));
}

void find_light_bulb_cb(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(buf, zb_apsde_data_indication_t);
    zb_zdo_simple_desc_resp_t *resp = (zb_zdo_simple_desc_resp_t *)zb_buf_begin(buf);

    TRACE_MSG(TRACE_APP1, ">> find_light_bulb_cb param %hd", (FMT__H, param));

    if (resp->hdr.status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uint8_t i = 0;
        zb_uint8_t in_cluster_cnt = resp->simple_desc.app_input_cluster_count;
        zb_uint16_t *cluster_list = (zb_uint16_t *)resp->simple_desc.app_cluster_list;
        zb_int16_t checked = 0;
        zb_uint8_t on_off_server_found = 0;
        zb_uint8_t level_control_server_found = 0;

        /* check server clusters list */
        for (; (checked < in_cluster_cnt) &&
                !(on_off_server_found && level_control_server_found); checked++)
        {
            if (cluster_list[checked] == ZB_ZCL_CLUSTER_ID_ON_OFF)
            {
                on_off_server_found = 1;
            }
            else if (cluster_list[checked] == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL)
            {
                level_control_server_found = 1;
            }
        }

        if (on_off_server_found && level_control_server_found)
        {
            /* Skip busy slots */
            while (i < LIGHT_CONTROL_BULBS_MAX_NUM && g_device_ctx.p.bulbs[i].endpoint)
            {
                ++i;
            }

            /* Have a slot to store this bulb */
            if (i < LIGHT_CONTROL_BULBS_MAX_NUM)
            {
                g_device_ctx.p.bulbs[i].endpoint = resp->simple_desc.endpoint;
                g_device_ctx.p.bulbs[i].short_addr = ind->src_addr;
                zb_address_ieee_by_short(g_device_ctx.p.bulbs[i].short_addr, g_device_ctx.p.bulbs[i].ieee_addr);

                TRACE_MSG(TRACE_APP2, "find bulb addr %d ep %hd",
                          (FMT__D_H, g_device_ctx.p.bulbs[i].short_addr, g_device_ctx.p.bulbs[i].endpoint));

                /* Now may control this bulb independently of the others, create groups etc.
                   Let's do bind to this bulb to be able to control it with send_toggle_req(). */
                light_control_bind_bulb_on_off(param, i);
                param = 0;
            }
        }
    }

    if (param)
    {
        zb_buf_free(buf);
    }
    TRACE_MSG(TRACE_APP1, "<< find_light_bulb_cb param %hd", (FMT__H, param));
}

void light_control_bind_bulb_on_off(zb_uint8_t param, zb_uint8_t idx)
{
    zb_bufid_t buf = param;
    zb_apsme_binding_req_t *req;

    TRACE_MSG(TRACE_APP2, ">> light_control_bind_bulb_on_off param %hd", (FMT__H, param));

    /* Bind On/Off cluster */
    req = ZB_BUF_GET_PARAM(buf, zb_apsme_binding_req_t);

    ZB_IEEE_ADDR_COPY(req->src_addr, &g_ed_addr);
    ZB_IEEE_ADDR_COPY(req->dst_addr.addr_long, g_device_ctx.p.bulbs[idx].ieee_addr);

    req->src_endpoint = ENDPOINT;
    req->clusterid = ZB_ZCL_CLUSTER_ID_ON_OFF;
    req->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    req->dst_endpoint = g_device_ctx.p.bulbs[idx].endpoint;
    req->confirm_cb = light_control_bind_bulb_level_control;

    ZB_SCHEDULE_APP_CALLBACK(zb_apsme_bind_request, param);

    TRACE_MSG(TRACE_APP2, "<< light_control_bind_bulb_on_off", (FMT__0));
}

void light_control_bind_bulb_level_control(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_apsme_binding_req_t *req;

    TRACE_MSG(TRACE_APP2, ">> light_control_bind_bulb_level_control param %hd", (FMT__H, param));

    /* Bind Level Control cluster */
    req = ZB_BUF_GET_PARAM(buf, zb_apsme_binding_req_t);

    /* NOTE: zb_apsme_binding_req_t parameter is already filled here, so fill only changed fields */
    req->clusterid = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL;
    req->confirm_cb = light_control_bind_bulb_completed;

    ZB_SCHEDULE_APP_CALLBACK(zb_apsme_bind_request, param);

    TRACE_MSG(TRACE_APP2, "<< light_control_bind_bulb_level_control", (FMT__0));
}

void light_control_bind_bulb_completed(zb_uint8_t param)
{
    zb_bufid_t buf = param;

    TRACE_MSG(TRACE_APP2, ">> light_control_bind_bulb_completed param %hd", (FMT__H, param));

    zb_buf_free(buf);

    /* Store bulb data to the NVRAM */
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);

#ifndef ZB_USE_BUTTONS
    /* Restart periodic cmd sending */
    ZB_SCHEDULE_APP_ALARM_CANCEL(send_move_to_level_cmd, ZB_ALARM_ANY_PARAM);
    ZB_SCHEDULE_APP_ALARM(send_move_to_level_cmd, 0, MOVE_TO_LEVEL_INTERVAL);
#else
    light_control_led_on_off(BULB_LED_TOUCHLINK_IN_PROGRESS, 0);
    light_control_led_on_off(BULB_LED_NEW_DEV_JOINED, 0);
#endif

    TRACE_MSG(TRACE_APP2, "<< light_control_bind_bulb_completed", (FMT__0));
}

void bulb_nwk_addr_req_cb(zb_uint8_t param);

void bulb_nwk_addr_req(zb_uint8_t param, zb_ieee_addr_t ieee_addr)
{
    zb_bufid_t buf = param;
    zb_zdo_nwk_addr_req_param_t *req = zb_buf_alloc_tail(buf, sizeof(zb_zdo_nwk_addr_req_param_t));

    TRACE_MSG(TRACE_APP2, "bulb_nwk_addr_req: param %hd ieee "TRACE_FORMAT_64, (FMT__H_A, param, TRACE_ARG_64(ieee_addr)));
    req->dst_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
    ZB_IEEE_ADDR_COPY(req->ieee_addr, ieee_addr);
    req->start_index = 0;
    req->request_type = 0;
    zb_zdo_nwk_addr_req(buf, bulb_nwk_addr_req_cb);
}

void bulb_nwk_addr_req_cb(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_zdo_nwk_addr_resp_head_t *resp;
    zb_ieee_addr_t ieee_addr;
    zb_uint16_t nwk_addr;
    zb_address_ieee_ref_t addr_ref;

    TRACE_MSG(TRACE_APP2, ">> bulb_nwk_addr_req_cb param %hd", (FMT__H, param));

    resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(buf);
    TRACE_MSG(TRACE_APP2, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ZB_LETOH64(ieee_addr, resp->ieee_addr);
        ZB_LETOH16(&nwk_addr, &resp->nwk_addr);
        zb_address_update(ieee_addr, nwk_addr, ZB_TRUE, &addr_ref);

        /* The next step is to bind the Light control to the bulb */
        find_light_bulb(param, nwk_addr, g_device_ctx.pending_dev_ep);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_APP2, "<< bulb_ieee_addr_req_cb", (FMT__0));
}

void start_touchlink_commissioning(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, "Start Touchlink commissioning as initiator", (FMT__0));
    bdb_start_top_level_commissioning(ZB_BDB_TOUCHLINK_COMMISSIONING);
#ifdef ZB_USE_BUTTONS
    light_control_led_on_off(BULB_LED_TOUCHLINK_IN_PROGRESS, 1);
#endif
}

void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_SKIP_STARTUP: start join", (FMT__0));
        /* If factory new, start Touchlink commissioning, else start only steering (may additionaly
         * start Touchlink by some trigger). */
        if (zb_bdb_is_factory_new())
        {
            start_touchlink_commissioning(0);
        }
        else
        {
            bdb_start_top_level_commissioning(0);
        }
        break;
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device RESTARTED OK", (FMT__0));
#ifndef ZB_USE_BUTTONS
        /* Do not have buttons in simulator - just start periodic on/off sending */
        ZB_SCHEDULE_APP_ALARM_CANCEL(send_move_to_level_cmd, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_APP_ALARM(send_move_to_level_cmd, 0, MOVE_TO_LEVEL_INTERVAL);
#else
        light_control_led_on_off(BULB_LED_JOINED, 1);
#endif
        break;
    //! [signal_touchlink_nwk_started]
    case ZB_BDB_SIGNAL_TOUCHLINK_NWK_STARTED:
    {
        zb_bdb_signal_touchlink_nwk_started_params_t *sig_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_bdb_signal_touchlink_nwk_started_params_t);
#ifdef ZB_USE_BUTTONS
        light_control_led_on_off(BULB_LED_NEW_DEV_JOINED, 1);
#endif
        ZB_IEEE_ADDR_COPY(g_device_ctx.pending_dev_addr, sig_params->device_ieee_addr);
        g_device_ctx.pending_dev_ep = sig_params->endpoint;
        TRACE_MSG(TRACE_APP1, "ZB_BDB_SIGNAL_TOUCHLINK_NWK_STARTED: remember dev "TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(g_device_ctx.pending_dev_addr)));
        TRACE_MSG(TRACE_APP1, "profile 0x%x ep %hd", (FMT__D_H, sig_params->profile_id, sig_params->endpoint));
    }
    break;
    //! [signal_touchlink_nwk_started]
    //! [signal_touchlink_nwk_joined_router]
    case ZB_BDB_SIGNAL_TOUCHLINK_NWK_JOINED_ROUTER:
    {
        zb_bdb_signal_touchlink_nwk_joined_router_t *sig_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_bdb_signal_touchlink_nwk_joined_router_t);
#ifdef ZB_USE_BUTTONS
        light_control_led_on_off(BULB_LED_NEW_DEV_JOINED, 1);
#endif
        ZB_IEEE_ADDR_COPY(g_device_ctx.pending_dev_addr, sig_params->device_ieee_addr);
        g_device_ctx.pending_dev_ep = sig_params->endpoint;
        TRACE_MSG(TRACE_APP1, "ZB_BDB_SIGNAL_TOUCHLINK_NWK_JOINED_ROUTER: remember dev "TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(g_device_ctx.pending_dev_addr)));
        TRACE_MSG(TRACE_APP1, "profile 0x%x ep %hd", (FMT__D_H, sig_params->profile_id, sig_params->endpoint));
    }
    break;
    //! [signal_touchlink_nwk_joined_router]
    case ZB_BDB_SIGNAL_TOUCHLINK:
        TRACE_MSG(TRACE_APP1, "Touchlink commissioning as initiator done ok", (FMT__0));
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        /* ZB_SCHEDULE_APP_ALARM(start_bdb_initiator, 0, ZB_TIME_ONE_SECOND * 10); */
        if (ZB_GET_APP_SIGNAL_STATUS(param) == ZB_BDB_STATUS_SUCCESS)
        {
#ifdef ZB_USE_BUTTONS
            light_control_led_on_off(BULB_LED_JOINED, 1);
#endif
            if (!ZB_IEEE_ADDR_IS_ZERO(g_device_ctx.pending_dev_addr))
            {
                zb_uint16_t short_addr = zb_address_short_by_ieee(g_device_ctx.pending_dev_addr);

                if (short_addr == ZB_UNKNOWN_SHORT_ADDR)
                {
                    bulb_nwk_addr_req(param, g_device_ctx.pending_dev_addr);
                }
                else
                {
                    find_light_bulb(param, short_addr, g_device_ctx.pending_dev_ep);
                }
                param = 0;
            }
#ifndef ZB_USE_BUTTONS
            else
            {
                /* Restart periodic on/off sending */
                ZB_SCHEDULE_APP_ALARM_CANCEL(send_move_to_level_cmd, ZB_ALARM_ANY_PARAM);
                ZB_SCHEDULE_APP_ALARM(send_move_to_level_cmd, 0, MOVE_TO_LEVEL_INTERVAL);
            }
#endif
        }
        else
        {
            /* Repeat touchlink until any bulb will be found */
            start_touchlink_commissioning(0);
        }
        break;
#ifdef LIGHT_CONTROL_BDB_FINDING_BINDING
    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APP1, "f&b complete", (FMT__0));
#ifndef ZB_USE_BUTTONS
        /* Do not have buttons in simulator - just start periodic on/off sending */
        ZB_SCHEDULE_APP_ALARM_CANCEL(send_move_to_level_cmd, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_APP_ALARM(send_move_to_level_cmd, 0, MOVE_TO_LEVEL_INTERVAL);
#endif
        break;
#endif
    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal %hd status %hd", (FMT__H_H, sig, ZB_GET_APP_SIGNAL_STATUS(param)));
        break;
    }

    if (param)
    {
        zb_buf_free(param);
    }
}

zb_uint16_t light_control_get_nvram_data_size()
{
    TRACE_MSG(TRACE_APP1, "light_control_get_nvram_data_size, ret %hd", (FMT__H, sizeof(light_control_device_nvram_dataset_t)));
    return sizeof(light_control_device_nvram_dataset_t);
}

void light_control_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    zb_ret_t ret;
    TRACE_MSG(TRACE_APP1, ">> light_control_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(light_control_device_nvram_dataset_t));

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_read_data(page, pos, (zb_uint8_t *)&g_device_ctx.p, sizeof(light_control_device_nvram_dataset_t));

    TRACE_MSG(TRACE_APP1, "<< light_control_nvram_read_app_data ret %d", (FMT__D, ret));
}

zb_ret_t light_control_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> light_control_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&g_device_ctx.p, sizeof(light_control_device_nvram_dataset_t));

    TRACE_MSG(TRACE_APP1, "<< light_control_nvram_write_app_data, ret %d", (FMT__D, ret));

    return ret;
}
