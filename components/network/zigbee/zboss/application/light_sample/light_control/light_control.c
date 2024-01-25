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
/* PURPOSE: Dimmer switch for HA profile
*/

#define ZB_TRACE_FILE_ID 40143

#include "light_control.h"
#ifdef ZB_USE_BUTTONS
#include "light_control_hal.h"
#endif

#define ZB_HA_DEFINE_DEVICE_DIMMER_SWITCH

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

zb_ieee_addr_t g_ed_addr = LIGHT_CONTROL_IEEE_ADDR; /* IEEE address of the device */
zb_uint16_t g_profile_id = ZB_AF_HA_PROFILE_ID;     /* Profile ID of the device */

/**
 * Declaration of Zigbee device data structures
 */
light_control_ctx_t g_device_ctx; /* Global device context */

/**
 * Declaring attributes for each cluster
 */

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Declare cluster list for a device */
ZB_HA_DECLARE_DIMMER_SWITCH_CLUSTER_LIST(dimmer_switch_clusters,
        basic_attr_list,
        identify_attr_list);

/* Declare endpoint */
ZB_HA_DECLARE_DIMMER_SWITCH_EP(dimmer_switch_ep,
                               LIGHT_CONTROL_ENDPOINT,
                               dimmer_switch_clusters);

/* Declare application's device context for single-endpoint device */
ZB_HA_DECLARE_DIMMER_SWITCH_CTX(dimmer_switch_ctx, dimmer_switch_ep);

MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable */
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("light_control");

#ifdef ZB_USE_BUTTONS
    light_control_hal_init();
#endif

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_ed_addr);
    zb_set_network_ed_role(LIGHT_CONTROL_CHANNEL_MASK);

#ifdef ZB_USE_BUTTONS
    /* Erase NVRAM if BUTTON2 is pressed on start */
    zb_set_nvram_erase_at_start(light_control_hal_is_button_pressed(LIGHT_CONTROL_BUTTON_2_IDX));
#else
    /*
    Do not erase NVRAM to save the network parameters after device reboot or power-off
    NOTE: If this option is set to ZB_FALSE then do full device erase for all network
    devices before running other samples.
    */
    zb_set_nvram_erase_at_start(ZB_FALSE);
#endif

    /* Set end device configuration */
    zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));
    zb_set_rx_on_when_idle(ZB_TRUE);

    /* Initialization of the global device context */
    light_control_app_ctx_init();

    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&dimmer_switch_ctx);
    /* Register cluster commands handler for a specific endpoint */
    ZB_AF_SET_ENDPOINT_HANDLER(LIGHT_CONTROL_ENDPOINT, zcl_specific_cluster_cmd_handler);

#ifdef ZB_USE_NVRAM
    /* Register application callback for reading application data from NVRAM */
    zb_nvram_register_app1_read_cb(light_control_nvram_read_app_data);
    /* Register application callback for writing application data to NVRAM */
    zb_nvram_register_app1_write_cb(light_control_nvram_write_app_data, light_control_get_nvram_data_size);
#endif

    /* Initiate the stack start with starting the commissioning */
    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "dev_start failed", (FMT__0));
    }
    else
    {
        /* Call the main loop */
        zboss_main_loop();
    }

    /* Deinitialize trace */
    TRACE_DEINIT();

    MAIN_RETURN(0);
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_bufid_t zcl_cmd_buf = param;
    zb_bool_t cmd_processed = ZB_FALSE;

    TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
    TRACE_MSG(TRACE_APP3, "payload size: %i", (FMT__D, zb_buf_len(zcl_cmd_buf)));
    TRACE_MSG(TRACE_APP1, "< zcl_specific_cluster_cmd_handler %hd", (FMT__H, cmd_processed));

    return cmd_processed;
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    /* Get application signal from the buffer */
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
#ifdef ZB_USE_BUTTONS
            light_control_hal_set_connect(ZB_TRUE);
#endif
#ifdef LIGHT_CONTROL_START_SEARCH_LIGHT
#ifndef ZB_USE_BUTTONS
            ZB_SCHEDULE_APP_ALARM_CANCEL(light_control_send_on_off_alarm_delayed, ZB_ALARM_ANY_PARAM);
#endif
            /* Check the light device address */
            if (ZB_IS_64BIT_ADDR_ZERO(g_device_ctx.bulb_params.ieee_addr))
            {
                ZB_SCHEDULE_APP_ALARM(find_light_bulb, param, MATCH_DESC_REQ_START_DELAY);
                param = 0;
                ZB_SCHEDULE_APP_ALARM(find_light_bulb_alarm, 0, MATCH_DESC_REQ_TIMEOUT);
            }
            else
            {
                /* Normal operation */
#ifndef ZB_USE_BUTTONS
                ZB_SCHEDULE_APP_ALARM(light_control_send_on_off_alarm_delayed, 0, BULB_ON_OFF_TIMEOUT);
#endif
            }
#endif

            break;
        /* [signal_leave] */
        case ZB_ZDO_SIGNAL_LEAVE:
        {
            zb_zdo_signal_leave_params_t *leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_params_t);
            light_control_retry_join(leave_params->leave_type);
        }
        break;
        /* [signal_leave] */
        case ZB_COMMON_SIGNAL_CAN_SLEEP:
        {
            /* zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_can_sleep_params_t); */
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif
            break;
        }

        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        {
            TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));
            break;
        }

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
            /* ZB_SCHEDULE_APP_ALARM(light_control_leave_and_join, 0, ZB_TIME_ONE_SECOND); */
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
#ifdef ZB_USE_BUTTONS
        light_control_hal_set_connect(ZB_FALSE);
#endif

        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        ZB_SCHEDULE_APP_ALARM(light_control_leave_and_join, 0, ZB_TIME_ONE_SECOND);
    }

    /* Free the buffer if it is not used */
    if (param)
    {
        zb_buf_free(param);
    }
}

void light_control_app_ctx_init(void)
{
    TRACE_MSG(TRACE_APP1, ">> light_control_app_ctx_init", (FMT__0));

    ZB_MEMSET(&g_device_ctx, 0, sizeof(light_control_ctx_t));

    TRACE_MSG(TRACE_APP1, "<< light_control_app_ctx_init", (FMT__0));
}

/* [zdo_match_desc_req] */
void find_light_bulb(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_zdo_match_desc_param_t *req;

    TRACE_MSG(TRACE_APP1, ">> find_light_bulb %hd", (FMT__H, param));

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (1) * sizeof(zb_uint16_t));

    req->nwk_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    req->addr_of_interest = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    req->profile_id = g_profile_id;
    /* We are searching for On/Off of Level Control Server */
    req->num_in_clusters = 2;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_ON_OFF;
    req->cluster_list[1] = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL;

    zb_zdo_match_desc_req(param, find_light_bulb_cb);

    TRACE_MSG(TRACE_APP1, "<< find_light_bulb %hd", (FMT__H, param));
}

void find_light_bulb_cb(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zb_buf_begin(buf);
    zb_uint8_t *match_ep;

    TRACE_MSG(TRACE_APP1, ">> find_light_bulb_cb param %hd, resp match_len %hd", (FMT__H_H, param, resp->match_len));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len > 0)
    {
        TRACE_MSG(TRACE_APP2, "Server is found, continue normal work...", (FMT__0));

        /* Match EP list follows right after response header */
        match_ep = (zb_uint8_t *)(resp + 1);

        /* we are searching for exact cluster, so only 1 EP maybe found */
        g_device_ctx.bulb_params.endpoint = *match_ep;
        g_device_ctx.bulb_params.short_addr = resp->nwk_addr;

        TRACE_MSG(TRACE_APP2, "find bulb addr 0x%hx ep %hd",
                  (FMT__D_H, g_device_ctx.bulb_params.short_addr, g_device_ctx.bulb_params.endpoint));

        ZB_SCHEDULE_APP_ALARM_CANCEL(find_light_bulb_alarm, ZB_ALARM_ANY_PARAM);

        /* Next step is to resolve the IEEE address of the bulb */
        ZB_SCHEDULE_APP_CALLBACK(bulb_ieee_addr_req, param);
    }
    else
    {
        zb_buf_free(buf);
    }
}
/* [zdo_match_desc_req] */

void find_light_bulb_alarm(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> find_light_bulb_alarm param %hd", (FMT__H, param));

    if (param == 0)
    {
        zb_buf_get_out_delayed(find_light_bulb_alarm);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Bulb is NOT found, try again", (FMT__0));
        ZB_SCHEDULE_APP_ALARM(find_light_bulb, param, MATCH_DESC_REQ_START_DELAY);
    }
}

void bulb_ieee_addr_req(zb_uint8_t param)
{
    zb_bufid_t  buf = param;
    zb_zdo_ieee_addr_req_param_t *req_param;

    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

    req_param->nwk_addr = g_device_ctx.bulb_params.short_addr;
    req_param->dst_addr = req_param->nwk_addr;
    req_param->start_index = 0;
    req_param->request_type = 0;
    zb_zdo_ieee_addr_req(buf, bulb_ieee_addr_req_cb);
}

void bulb_ieee_addr_req_cb(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_zdo_nwk_addr_resp_head_t *resp;
    zb_ieee_addr_t ieee_addr;

    TRACE_MSG(TRACE_APP2, ">> bulb_ieee_addr_req_cb param %hd", (FMT__H, param));

    resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(buf);
    TRACE_MSG(TRACE_APP2, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ZB_LETOH64(ieee_addr, resp->ieee_addr);

        ZB_MEMCPY(g_device_ctx.bulb_params.ieee_addr, ieee_addr, sizeof(zb_ieee_addr_t));

        /* The next step is to bind the Light control to the bulb */
        ZB_SCHEDULE_APP_CALLBACK(light_control_bind_bulb, param);
    }
    else
    {
        light_control_leave_and_join(param);
    }

    TRACE_MSG(TRACE_APP2, "<< bulb_ieee_addr_req_cb", (FMT__0));
}

static void bulb_binding_completed(zb_uint8_t param)
{
    zb_ret_t status = zb_buf_get_status(param);
    /* 07/29/2020 EE CR:MINOR check status and at least print it */
    /* DD: Done. I am not sure we can do anything meaningful if an error occurs, so - just trace it. */
    zb_buf_free(param);

    if (status != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "binding error, buf: %d, status 0x%x", (FMT__D_D, param, status));
    }
    else
    {
#ifdef ZB_USE_NVRAM
        /* Save all application data to the NVRAM */
        /* If we fail, trace is given and assertion is triggered */
        (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif

#ifndef ZB_USE_BUTTONS
        ZB_SCHEDULE_APP_ALARM(light_control_send_on_off_alarm_delayed, 0, BULB_ON_OFF_TIMEOUT);
#endif
    }
}

static void bind_level_control_cluster(zb_uint8_t param)
{
    zb_apsme_binding_req_t *req;

    /* Bind Level Control cluster */
    req = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);

    ZB_IEEE_ADDR_COPY(req->src_addr, &g_ed_addr);
    ZB_IEEE_ADDR_COPY(req->dst_addr.addr_long, g_device_ctx.bulb_params.ieee_addr);

    req->src_endpoint = LIGHT_CONTROL_ENDPOINT;
    req->clusterid = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL;
    req->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    req->dst_endpoint = g_device_ctx.bulb_params.endpoint;
    req->confirm_cb = bulb_binding_completed;

    ZB_SCHEDULE_APP_CALLBACK(zb_apsme_bind_request, param);
}


static void bind_on_off_cluster(zb_uint8_t param)
{
    zb_apsme_binding_req_t *req;

    /* Bind On/Off cluster */
    req = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);

    ZB_IEEE_ADDR_COPY(req->src_addr, &g_ed_addr);
    ZB_IEEE_ADDR_COPY(req->dst_addr.addr_long, g_device_ctx.bulb_params.ieee_addr);

    req->src_endpoint = LIGHT_CONTROL_ENDPOINT;
    req->clusterid = ZB_ZCL_CLUSTER_ID_ON_OFF;
    req->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    req->dst_endpoint = g_device_ctx.bulb_params.endpoint;
    req->confirm_cb = bind_level_control_cluster;

    ZB_SCHEDULE_APP_CALLBACK(zb_apsme_bind_request, param);
}

/* [apsme_bind_req] */
void light_control_bind_bulb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP2, ">> light_control_bind_bulb param %hd", (FMT__H, param));

    bind_on_off_cluster(param);

    TRACE_MSG(TRACE_APP2, "<< light_control_bind_bulb", (FMT__0));
}
/* [apsme_bind_req] */

/* Perform local operation - leave network */
void light_control_leave_nwk(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ERROR, ">> light_control_leave_nwk param %hd", (FMT__H, param));

    /* We are going to leave */
    if (!param)
    {
        zb_buf_get_out_delayed(light_control_leave_nwk);
    }
    else
    {
        zb_bufid_t buf = param;
        zb_zdo_mgmt_leave_param_t *req_param;

        req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);
        ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

        /* Set dst_addr == local address for local leave */
        req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
        zdo_mgmt_leave_req(param, NULL);
    }

    TRACE_MSG(TRACE_ERROR, "<< light_control_leave_nwk", (FMT__0));
}


void light_control_retry_join(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ERROR, "light_control_retry_join %hd", (FMT__H, param));
    if (param == ZB_NWK_LEAVE_TYPE_RESET)
    {
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
    }
}


void light_control_leave_and_join(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ERROR, ">> light_control_leave_and_join param %hd", (FMT__H, param));
    if (ZB_JOINED())
    {
        light_control_leave_nwk(param);
    }
    else
    {
        light_control_retry_join(ZB_NWK_LEAVE_TYPE_RESET);
        if (param)
        {
            zb_buf_free(param);
        }
    }
    TRACE_MSG(TRACE_ERROR, "<< light_control_leave_and_join", (FMT__0));
}

#ifndef ZB_USE_BUTTONS
void light_control_send_on_off(zb_uint8_t param, zb_uint16_t on_off);

void light_control_send_on_off_alarm(zb_uint8_t param)
{
    light_control_send_on_off(param, g_device_ctx.bulb_on_off_state);
    g_device_ctx.bulb_on_off_state = !g_device_ctx.bulb_on_off_state;
    ZB_SCHEDULE_APP_ALARM(light_control_send_on_off_alarm_delayed, 0, BULB_ON_OFF_TIMEOUT);
}

void light_control_send_on_off_alarm_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);

    zb_buf_get_out_delayed(light_control_send_on_off_alarm);
}

void light_control_send_on_off_cb(zb_uint8_t param)
{
    zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);
    TRACE_MSG(TRACE_APP2, ">> light_control_send_on_off_cb param %hd status %hd", (FMT__H_H, param, cmd_send_status->status));

    if (cmd_send_status->status != RET_OK)
    {
        ++g_device_ctx.bulb_on_off_failure_cnt;
        /* Stop on too many cmd failures. */
        if (g_device_ctx.bulb_on_off_failure_cnt == BULB_ON_OFF_FAILURE_CNT)
        {
            ZB_SCHEDULE_APP_ALARM_CANCEL(light_control_send_on_off_alarm_delayed, ZB_ALARM_ANY_PARAM);
        }
    }
    else
    {
        /* If cmd is ok, reset failure counter. */
        g_device_ctx.bulb_on_off_failure_cnt = 0;
    }
    zb_buf_free(param);
    TRACE_MSG(TRACE_APP2, "<< light_control_send_on_off_cb", (FMT__0));
}
#endif

void light_control_send_on_off(zb_uint8_t param, zb_uint16_t on_off)
{
    zb_bufid_t buf = param;
    zb_uint8_t cmd_id = (on_off) ? (ZB_ZCL_CMD_ON_OFF_ON_ID) : (ZB_ZCL_CMD_ON_OFF_OFF_ID);
    zb_uint16_t addr = 0;

    /* Dst addr and endpoint are unknown; command will be sent via binding */
    ZB_ZCL_ON_OFF_SEND_REQ(
        buf,
        addr,
        ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
        0,
        LIGHT_CONTROL_ENDPOINT,
        g_profile_id,
        ZB_FALSE,
        cmd_id,
#ifndef ZB_USE_BUTTONS
        /* If we do not have buttons, control failure cmd number. */
        light_control_send_on_off_cb
#else
        NULL
#endif
    );
}

void light_bulb_send_on_off_cmd(zb_bool_t on_off)
{
    zb_buf_get_out_delayed_ext(light_control_send_on_off, on_off, 0);
}

void light_control_send_step(zb_uint8_t param, zb_uint16_t dir)
{
    zb_bufid_t buf = param;
    zb_uint8_t step_dir = (dir) ? (ZB_ZCL_LEVEL_CONTROL_STEP_MODE_UP) :
                          (ZB_ZCL_LEVEL_CONTROL_STEP_MODE_DOWN);
    zb_uint16_t addr = 0;

    /* Dst addr and endpoint are unknown; command will be sent via binding */
    ZB_ZCL_LEVEL_CONTROL_SEND_STEP_REQ(
        buf,
        addr,
        ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
        0,
        LIGHT_CONTROL_ENDPOINT,
        g_profile_id,
        ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
        NULL, step_dir, LIGHT_CONTROL_DIMM_STEP,
        LIGHT_CONTROL_DIMM_TRANSACTION_TIME);
}

void light_bulb_send_step_cmd(zb_bool_t dir)
{
    zb_buf_get_out_delayed_ext(light_control_send_step, dir, 0);
}

#ifdef ZB_USE_NVRAM
zb_uint16_t light_control_get_nvram_data_size(void)
{
    TRACE_MSG(TRACE_APP1, "light_control_get_nvram_data_size, ret %hd", (FMT__H, sizeof(light_control_device_nvram_dataset_t)));
    return sizeof(light_control_device_nvram_dataset_t);
}

void light_control_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    light_control_device_nvram_dataset_t ds;
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> light_control_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(ds));

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_read_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    if (ret == RET_OK)
    {
        ZB_MEMCPY(g_device_ctx.bulb_params.ieee_addr, ds.bulb_ieee_addr, sizeof(zb_ieee_addr_t));
        g_device_ctx.bulb_params.short_addr = ds.bulb_short_addr;
        g_device_ctx.bulb_params.endpoint = ds.bulb_endpoint;
    }

    TRACE_MSG(TRACE_APP1, "<< light_control_nvram_read_app_data ret %d", (FMT__D, ret));
}

zb_ret_t light_control_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;
    light_control_device_nvram_dataset_t ds;

    TRACE_MSG(TRACE_APP1, ">> light_control_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

    ZB_MEMCPY(ds.bulb_ieee_addr, g_device_ctx.bulb_params.ieee_addr, sizeof(zb_ieee_addr_t));
    ds.bulb_short_addr = g_device_ctx.bulb_params.short_addr;
    ds.bulb_endpoint = g_device_ctx.bulb_params.endpoint;

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    TRACE_MSG(TRACE_APP1, "<< light_control_nvram_write_app_data, ret %d", (FMT__D, ret));

    return ret;
}

#endif  /* ZB_USE_NVRAM */

#ifdef ZB_USE_BUTTONS
void light_control_button_handler(zb_uint8_t button_no)
{
    zb_time_t current_time;
    zb_bool_t short_expired;

    current_time = ZB_TIMER_GET();

    short_expired = (ZB_TIME_SUBTRACT(current_time, g_device_ctx.button.timestamp) > LIGHT_CONTROL_BUTTON_TRESHOLD) ?
                    (ZB_TRUE) : (ZB_FALSE);

    if (light_control_hal_is_button_pressed(button_no))
    {
        if (short_expired)
        {
            /* The button is still pressed - dimm the light */
            light_bulb_send_step_cmd((button_no == LIGHT_CONTROL_BUTTON_ON) ? (ZB_TRUE) : (ZB_FALSE));
            ZB_SCHEDULE_APP_ALARM(light_control_button_handler, button_no, LIGHT_CONTROL_BUTTON_LONG_POLL_TMO);
        }
        else
        {
            /* Try another one iteration */
            ZB_SCHEDULE_APP_ALARM(light_control_button_handler, button_no, LIGHT_CONTROL_BUTTON_SHORT_POLL_TMO);
        }
    }
    else
    {
        if (short_expired)
        {
            /* The last step command to be sent */
            light_bulb_send_step_cmd((button_no == LIGHT_CONTROL_BUTTON_ON) ? (ZB_TRUE) : (ZB_FALSE));
        }
        else
        {
            light_bulb_send_on_off_cmd((button_no == LIGHT_CONTROL_BUTTON_ON) ? (ZB_TRUE) : (ZB_FALSE));
        }

        /* ... and exit this logic */
        g_device_ctx.button.button_state = LIGHT_CONTROL_BUTTON_STATE_IDLE;
    }
}

void light_control_button_pressed(zb_uint8_t button_no)
{
    switch (g_device_ctx.button.button_state)
    {
    case LIGHT_CONTROL_BUTTON_STATE_IDLE:
        g_device_ctx.button.button_state = LIGHT_CONTROL_BUTTON_STATE_PRESSED;
        g_device_ctx.button.timestamp = ZB_TIMER_GET();
        break;
    case LIGHT_CONTROL_BUTTON_STATE_PRESSED:
        g_device_ctx.button.button_state = LIGHT_CONTROL_BUTTON_STATE_UNPRESSED;
        ZB_SCHEDULE_APP_ALARM(light_control_button_handler, button_no, LIGHT_CONTROL_BUTTON_SHORT_POLL_TMO);
        break;
    case LIGHT_CONTROL_BUTTON_STATE_UNPRESSED:
    default:
        break;
    }
}
#endif
