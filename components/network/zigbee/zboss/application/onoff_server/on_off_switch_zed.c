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
/* PURPOSE: ZB Simple switch device
*/

#define ZB_TRACE_FILE_ID 40249
#include "zboss_api.h"
#include "zb_led_button.h"

/*#define TEST_USE_INSTALLCODE*/

#if ! defined ZB_ED_FUNC
#error define ZB_ED_ROLE to compile ze tests
#endif

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_ed_addr = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22}; /* IEEE address of the
                                                                              * device */
/* Used endpoint */
#define ZB_SWITCH_ENDPOINT          10

#ifdef TEST_USE_INSTALLCODE
char g_installcode[] = "966b9f3ef98ae605 9708";
#endif

zb_bool_t cmd_in_progress = ZB_FALSE;

/* Handler for specific ZCL commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/* Parse read attributes response */
void on_off_read_attr_resp_handler(zb_bufid_t cmd_buf);
void test_restart_join_nwk(zb_uint8_t param);

void send_toogle_req(zb_uint8_t param);
void button_press_handler(zb_uint8_t param);

/** [COMMON_DECLARATION] */
/**
 * Declaring attributes for each cluster
 */

/* Switch config cluster attributes */
zb_uint8_t attr_switch_type =
    ZB_ZCL_ON_OFF_SWITCH_CONFIGURATION_SWITCH_TYPE_TOGGLE;
zb_uint8_t attr_switch_actions =
    ZB_ZCL_ON_OFF_SWITCH_CONFIGURATION_SWITCH_ACTIONS_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_SWITCH_CONFIGURATION_ATTRIB_LIST(switch_cfg_attr_list,
        &attr_switch_type,
        &attr_switch_actions);

/* Basic cluster attributes */
zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes */
zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);


/* Declare cluster list for the device */
ZB_HA_DECLARE_ON_OFF_SWITCH_CLUSTER_LIST(on_off_switch_clusters,
        switch_cfg_attr_list,
        basic_attr_list,
        identify_attr_list);
/* Declare endpoint */
ZB_HA_DECLARE_ON_OFF_SWITCH_EP(on_off_switch_ep, ZB_SWITCH_ENDPOINT, on_off_switch_clusters);

/* Declare application's device context for single-endpoint device */
ZB_HA_DECLARE_ON_OFF_SWITCH_CTX(on_off_switch_ctx, on_off_switch_ep);
/** [COMMON_DECLARATION] */


/* [zboss_main_loop_iteration_declaring] */
/* Example of user-defined main loop.
 * If you don't need more complicated functionality inside your main loop, use zboss_main_loop() function. */
void my_main_loop()
{
    while (1)
    {
        /* ... User code ... */
        zboss_main_loop_iteration();
        /* ... User code ... */
    }
}
/* [zboss_main_loop_iteration_declaring] */

MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable */
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("on_off_switch_zed");

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_ed_addr);
#ifndef ZB_APP_ENABLE_SUBGHZ_MODE
    zb_set_network_ed_role(1l << 21);
#else
    {
        zb_channel_list_t channel_list;
        zb_channel_list_init(channel_list);
        zb_channel_page_list_set_mask(channel_list, (ZB_CHANNEL_LIST_PAGE28_IDX), (1l << 24));
        zb_set_network_ed_role_ext(channel_list);
    }
#endif
    zb_set_nvram_erase_at_start(ZB_FALSE);

    /* Set end-device configuration parameters */
    zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));
    //zb_set_rx_on_when_idle(ZB_FALSE);

    /** [REGISTER] */
    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&on_off_switch_ctx);
    /* Register cluster commands handler for a specific endpoint */
    ZB_AF_SET_ENDPOINT_HANDLER(ZB_SWITCH_ENDPOINT, zcl_specific_cluster_cmd_handler);
    /** [REGISTER] */

    /* [zboss_main_loop_iteration_usage] */
    /* Initiate the stack start without starting the commissioning */
    if (zboss_start_no_autostart() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        /* Call the application-specific main loop */
        my_main_loop();
    }
    /* [zboss_main_loop_iteration_usage] */

    /* Deinitialize trace */
    TRACE_DEINIT();

    MAIN_RETURN(0);
}

/* [zb_bdb_finding_binding_initiator_cb_example] */
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
    return ZB_TRUE;
}
/* [zb_bdb_finding_binding_initiator_cb_example] */

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_bool_t unknown_cmd_received = ZB_TRUE;

    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
    TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, zb_buf_len(param)));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        if (cmd_info->cmd_id == ZB_ZCL_CMD_DEFAULT_RESP)
        {
            unknown_cmd_received = ZB_FALSE;

            cmd_in_progress = ZB_FALSE;

            zb_buf_free(param);
        }
    }

    TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
    return ! unknown_cmd_received;
}


void send_toggle_req(zb_uint8_t param)
{
    zb_uint16_t addr = 0;

    ZB_ASSERT(param);

    if (ZB_JOINED() /* && !cmd_in_progress */)
    {
        cmd_in_progress = ZB_TRUE;
        TRACE_MSG(TRACE_APS1, "send_toggle_req %hd - send toggle", (FMT__H, param));
        /* Destination address and endpoint are unknown; command will be sent via binding */
        ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(
            param,
            addr,
            ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
            0,
            ZB_SWITCH_ENDPOINT,
            ZB_AF_HA_PROFILE_ID,
            ZB_FALSE, NULL);
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "send_toggle_req %hd - not joined", (FMT__H, param));
        zb_buf_free(param);
    }
}

/* [system_server_discovery_cb] */
static void system_server_discovery_req_cb(zb_uint8_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_system_server_discovery_resp_t *resp = (zb_zdo_system_server_discovery_resp_t *)(zdp_cmd);

    if (resp->status == ZB_ZDP_STATUS_SUCCESS && resp->server_mask & ZB_NETWORK_MANAGER )
    {
        TRACE_MSG(TRACE_APS2, "system_server_discovery received, status: OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "ERROR receiving system_server_discovery status %x, mask %x",
                  (FMT__D_D, resp->status, resp->server_mask));
    }
    zb_buf_free(param);
}
/* [system_server_discovery_cb] */

/* [system_server_discovery_req] */
static void send_system_server_discovery_req(zb_bufid_t buf)
{
    if (!buf)
    {
        zb_buf_get_out_delayed(send_system_server_discovery_req);
    }
    else
    {
        zb_zdo_system_server_discovery_param_t *req_param;

        req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_system_server_discovery_param_t);
        req_param->server_mask = (ZB_NETWORK_MANAGER) | (ZB_STACK_SPEC_VERSION << 9);

        zb_zdo_system_server_discovery_req(buf, system_server_discovery_req_cb);
    }
}
/* [system_server_discovery_req] */

/* [active_ep_cb] */
static void active_ep_callback(zb_bufid_t buf)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
    zb_zdo_ep_resp_t *resp = (zb_zdo_ep_resp_t *)zdp_cmd;
    zb_uint8_t *ep_list = zdp_cmd + sizeof(zb_zdo_ep_resp_t);
    zb_uint8_t i;

    TRACE_MSG(TRACE_APS1, "active_ep_resp: status %hd, addr 0x%x",
              (FMT__H, resp->status, resp->nwk_addr));

    if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != 0x0)
    {
        TRACE_MSG(TRACE_APS1, "active_ep_resp: Error incorrect status/addr", (FMT__0));
    }

    TRACE_MSG(TRACE_APS1, "active_ep_resp: ep count %hd, ep numbers:", (FMT__H, resp->ep_count));
    for (i = 0; i < resp->ep_count; i++)
    {
        TRACE_MSG(TRACE_APS1, "active_ep_resp: ep %hd", (FMT__H_H, *(ep_list + i)));
    }

    ZB_SCHEDULE_APP_CALLBACK(send_system_server_discovery_req, buf);
}
/* [active_ep_cb] */

/* [send_active_ep_req] */
static void send_active_ep_req(zb_bufid_t buf)
{
    if (!buf)
    {
        zb_buf_get_out_delayed(send_active_ep_req);
    }
    else
    {
        zb_zdo_active_ep_req_t *req;
        req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_active_ep_req_t));
        req->nwk_addr = 0; /*send to coordinator*/
        zb_zdo_active_ep_req(buf, active_ep_callback);
    }
}
/* [send_active_ep_req] */


/* [simple_desc_cb] */
static void simple_desc_callback(zb_bufid_t buf)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
    zb_zdo_simple_desc_resp_t *resp = (zb_zdo_simple_desc_resp_t *)(zdp_cmd);
    zb_uint_t i;

    TRACE_MSG(TRACE_APS1, "simple_desc_resp: status %hd, addr 0x%x",
              (FMT__H, resp->hdr.status, resp->hdr.nwk_addr));
    if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0)
    {
        TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    }

    TRACE_MSG(TRACE_APS1, "simple_desc_resp: ep %hd, app prof %d, dev id %d, dev ver %hd, input count 0x%hx, output count 0x%hx",
              (FMT__H_D_D_H_H_H, resp->simple_desc.endpoint, resp->simple_desc.app_profile_id,
               resp->simple_desc.app_device_id, resp->simple_desc.app_device_version,
               resp->simple_desc.app_input_cluster_count, resp->simple_desc.app_output_cluster_count));

    TRACE_MSG(TRACE_APS1, "simple_desc_resp: clusters:", (FMT__0));
    for (i = 0; i < resp->simple_desc.app_input_cluster_count + resp->simple_desc.app_output_cluster_count; i++)
    {
        TRACE_MSG(TRACE_APS1, " 0x%hx", (FMT__H, *(resp->simple_desc.app_cluster_list + i)));
    }
    /* [simple_desc_cb] */

    ZB_SCHEDULE_APP_CALLBACK(send_active_ep_req, buf);
}

/* [send_simple_desc_req] */
static void send_simple_desc_req(zb_bufid_t buf)
{
    if (!buf)
    {
        zb_buf_get_out_delayed(send_simple_desc_req);
    }
    else
    {
        zb_zdo_simple_desc_req_t *req;
        req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_simple_desc_req_t));
        req->nwk_addr = 0; /* send to coordinator */
        req->endpoint = 5;
        zb_zdo_simple_desc_req(buf, simple_desc_callback);
    }
}
/* [send_simple_desc_req] */

/* [power_desc_cb] */
static void power_desc_callback(zb_bufid_t buf)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
    zb_zdo_power_desc_resp_t *resp = (zb_zdo_power_desc_resp_t *)(zdp_cmd);

    TRACE_MSG(TRACE_APS1, "power_desc_resp: status %hd, addr 0x%x",
              (FMT__H, resp->hdr.status, resp->hdr.nwk_addr));
    if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0)
    {
        TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    }

    TRACE_MSG(TRACE_APS1, "power mode %hd, avail power src %hd, cur power src %hd, cur power level %hd",
              (FMT__H_H_H_H, (resp->power_desc.power_desc_flags & 0x000F),
               ((resp->power_desc.power_desc_flags & 0x00F0) >> 4),
               ((resp->power_desc.power_desc_flags & 0x0F00) >> 8),
               ((resp->power_desc.power_desc_flags & 0xF000) >> 12)));
    /* [power_desc_cb] */
    ZB_SCHEDULE_APP_CALLBACK(send_simple_desc_req, buf);
}

/* [send_power_desc_req] */
static void send_power_desc_req(zb_bufid_t buf)
{
    if (!buf)
    {
        zb_buf_get_out_delayed(send_power_desc_req);
    }
    else
    {
        zb_zdo_power_desc_req_t *req;
        req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_power_desc_req_t));
        req->nwk_addr = 0; /* send to coordinator*/
        zb_zdo_power_desc_req(buf, power_desc_callback);
    }
}
/* [send_power_desc_req] */

/* [node_req_cb] */
static void node_desc_callback(zb_bufid_t buf)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
    zb_zdo_node_desc_resp_t *resp = (zb_zdo_node_desc_resp_t *)(zdp_cmd);

    TRACE_MSG(TRACE_APS1, "node_desc_response: status %hd, addr 0x%x",
              (FMT__H_D, resp->hdr.status, resp->hdr.nwk_addr));

    if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0)
    {
        TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    }

    TRACE_MSG(TRACE_APS1, "node_desc_response: logic type %hd, aps flag %hd, frequency %hd",
              (FMT__H_H_H, (resp->node_desc.node_desc_flags & 0x7),
               ((resp->node_desc.node_desc_flags & 0x700) >> 8),
               ((resp->node_desc.node_desc_flags & 0xF800) >> 11)));

    TRACE_MSG(TRACE_APS1, "node_desc_response: mac cap 0x%hx, manufact code %d, max buf %hd, max transfer %hd",
              (FMT__H_H_D_H, resp->node_desc.mac_capability_flags, resp->node_desc.manufacturer_code,
               resp->node_desc.max_buf_size, resp->node_desc.max_incoming_transfer_size));

    /* [node_req_cb] */
    ZB_SCHEDULE_APP_CALLBACK(send_power_desc_req, buf);

}


/* [send_node_desc_req] */
static void send_node_desc_req(zb_bufid_t buf)
{
    zb_zdo_node_desc_req_t *req;

    TRACE_MSG(TRACE_APS1, "send_node_desc_req", (FMT__0));

    if (!buf)
    {
        zb_buf_get_out_delayed(send_node_desc_req);
    }
    else
    {
        req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_node_desc_req_t));
        req->nwk_addr = 0; //send to coordinator
        zb_zdo_node_desc_req(buf, node_desc_callback);
    }
}
/* [send_node_desc_req] */

void button_press_handler(zb_uint8_t param)
{
    if (!param)
    {
        /* Button is pressed, gets buffer for outgoing command */
        zb_buf_get_out_delayed(button_press_handler);
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "button_press_handler %hd", (FMT__H, param));
        send_toggle_req(param);
#ifndef ZB_USE_BUTTONS
        /* Do not have buttons in simulator - just start periodic on/off sending */
        ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 7 * ZB_TIME_ONE_SECOND);
#endif
    }
}

void test_leave_nwk(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ERROR, ">> test_leave_nwk param %hd", (FMT__H, param));

    /* We are going to leave */
    if (!param)
    {
        zb_buf_get_out_delayed(test_leave_nwk);
    }
    else
    {
        zb_zdo_mgmt_leave_param_t *req_param;

        req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
        ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

        /* Set dst_addr == local address for local leave */
        req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
        zdo_mgmt_leave_req(param, NULL);
    }

    TRACE_MSG(TRACE_ERROR, "<< test_leave_nwk", (FMT__0));
}

void test_restart_join_nwk(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ERROR, "test_restart_join_nwk %hd", (FMT__H, param));
    if (param == ZB_NWK_LEAVE_TYPE_RESET)
    {
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
    }
}

void test_leave_and_join(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ERROR, ">> test_leave_and_join param %hd", (FMT__H, param));
    if (ZB_JOINED())
    {
        test_leave_nwk(param);
    }
    else
    {
        test_restart_join_nwk(ZB_NWK_LEAVE_TYPE_RESET);
        if (param)
        {
            zb_buf_free(param);
        }
    }
    TRACE_MSG(TRACE_ERROR, "<< test_leave_and_join", (FMT__0));
}

void permit_joining_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ERROR, "permit joining done", (FMT__0));
    zb_buf_free(param);
}

void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);
    zb_bufid_t buf;
    zb_bufid_t req_buf = 0;
    zb_zdo_mgmt_permit_joining_req_param_t *req_param;

#ifdef ZB_USE_BUTTONS
    /* Now register handlers for buttons */
    zb_int32_t i;
    for (i = 0; i < ZB_N_BUTTONS; ++i)
    {
        zb_button_register_handler(i, 0, button_press_handler);
    }
#endif

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
#ifndef ZB_MACSPLIT_HOST
            TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_SKIP_STARTUP: boot, not started yet", (FMT__0));
#ifdef TEST_USE_INSTALLCODE
            zb_secur_ic_str_set(g_installcode);
#endif
            zboss_start_continue();
#endif /* ZB_MACSPLIT_HOST */
            break;

#ifdef ZB_MACSPLIT_HOST
        case ZB_MACSPLIT_DEVICE_BOOT:
            TRACE_MSG(TRACE_APP1, "ZB_MACSPLIT_DEVICE_BOOT: boot, not started yet", (FMT__0));
#ifdef TEST_USE_INSTALLCODE
            zb_secur_ic_str_set(g_installcode);
#endif
            zboss_start_continue();
            break;
#endif /* ZB_MACSPLIT_HOST */
        /* [signal_first] */
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APP1, "FIRST_START: start steering", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
#ifdef TEST_APS_FRAGMENTATION
            ZB_SCHEDULE_APP_ALARM(send_frag_data, 0, ZB_TIME_ONE_SECOND / 2);
#endif

            buf = zb_buf_get_out();
            if (!buf)
            {
                TRACE_MSG(TRACE_APP1, "no buffer available", (FMT__0));
                break;
            }

            /** [zdo_mgmt_permit_joining_req] */
            /* Example: send permit join request to close network */
            req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_permit_joining_req_param_t);

            req_param->dest_addr = 0xfffc;
            req_param->permit_duration = 0;
            req_param->tc_significance = 1;

            zb_zdo_mgmt_permit_joining_req(buf, permit_joining_cb);
            /** [zdo_mgmt_permit_joining_req] */

            break;
        /* [signal_first] */
        /* [signal_reboot] */
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device RESTARTED OK", (FMT__0));
#ifndef ZB_USE_BUTTONS
            /* Do not have buttons in simulator - just start periodic on/off sending */
            cmd_in_progress = ZB_FALSE;
            ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
            ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 7 * ZB_TIME_ONE_SECOND);
#endif
            break;
        /* [signal_reboot] */
        /* [zb_bdb_finding_binding_initiator] */
        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            TRACE_MSG(TRACE_APP1, "Successfull steering, start f&b initiator", (FMT__0));
            zb_bdb_finding_binding_initiator(ZB_SWITCH_ENDPOINT, finding_binding_cb);
            break;
        /* [zb_bdb_finding_binding_initiator] */

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        {
            TRACE_MSG(TRACE_APP1, "Finding&binding done", (FMT__0));
            ZB_SCHEDULE_APP_CALLBACK(send_node_desc_req, req_buf); /* starts sendindg a sequence of
                * descriptors to the coordinator,
                * just for an example of descriptors
                * requests usage */
#ifndef ZB_USE_BUTTONS
            /* Do not have buttons in simulator - just start periodic on/off sending */
            cmd_in_progress = ZB_FALSE;
            ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
            ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 7 * ZB_TIME_ONE_SECOND);
#endif
        }
        break;

        case ZB_ZDO_SIGNAL_LEAVE:
        {
            zb_zdo_signal_leave_params_t *leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_params_t);
            test_restart_join_nwk(leave_params->leave_type);
        }
        break;
        /* [signal_can_sleep] */
        case ZB_COMMON_SIGNAL_CAN_SLEEP:
        {
            /* zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_can_sleep_params_t); */
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif
            break;
        }
        /* [signal_can_sleep] */
        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        {
            TRACE_MSG(TRACE_APP1, "Production config is ready", (FMT__0));
            break;
        }

        case ZB_BDB_SIGNAL_TC_REJOIN_DONE:
            TRACE_MSG(TRACE_APP1, "TC rejoin is completed successfully", (FMT__0));
            break; /* ZB_BDB_SIGNAL_TC_REJOIN_DONE */

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd, do nothing", (FMT__H, sig));
        }
    }
    else
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APP1, "Device can not find any network on start, so try to perform network steering", (FMT__0));
            ZB_SCHEDULE_APP_ALARM(test_leave_and_join, 0, ZB_TIME_ONE_SECOND);
            break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device can not find any network on restart", (FMT__0));

            if (zb_bdb_is_factory_new())
            {
                /* Device tried to perform TC rejoin after reboot and lost its authentication flag.
                 * Do nothing here and wait for ZB_BDB_SIGNAL_TC_REJOIN_DONE to handle TC rejoin error */
                TRACE_MSG(TRACE_APP1, "Device lost authentication flag", (FMT__0));
            }
            else
            {
                /* Device tried to perform secure rejoin, but didn't found any networks or can't decrypt Rejoin Response
                 * (it is possible when Trust Center changes network key when ZED is powered off) */
                TRACE_MSG(TRACE_APP1, "Device is still authenticated, try to perform TC rejoin", (FMT__0));
                ZB_SCHEDULE_APP_ALARM(zb_bdb_initiate_tc_rejoin, 0, ZB_TIME_ONE_SECOND);
            }
            break; /* ZB_BDB_SIGNAL_DEVICE_REBOOT */

        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
            TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
            break; /* ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY */

        /* [signal_bdb_tc_rejoin_done] */
        case ZB_BDB_SIGNAL_TC_REJOIN_DONE:
            TRACE_MSG(TRACE_APP1, "TC rejoin failed, so try it again with interval", (FMT__0));

            ZB_SCHEDULE_APP_ALARM(zb_bdb_initiate_tc_rejoin, 0, 3 * ZB_TIME_ONE_SECOND);
            break; /* ZB_BDB_SIGNAL_TC_REJOIN_DONE */
        /* [signal_bdb_tc_rejoin_done] */

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd with error status, do nothing", (FMT__H, sig));
            break;
        }
    }

    if (param)
    {
        zb_buf_free(param);
    }
}
