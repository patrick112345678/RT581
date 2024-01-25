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
/* PURPOSE: Mult Endpoint ZED sample (HA profile)
*/

#define ZB_TRACE_FILE_ID 40017

#include "multiendpoint_zed.h"


zb_ieee_addr_t g_zr_addr = MULTI_EP_ZED_IEEE_ADDRESS; /* IEEE address of the device */

/* Declare "zb_af_simple_desc_#num_in_#num_out_t" */
ZB_DECLARE_SIMPLE_DESC(3, 0);
ZB_DECLARE_SIMPLE_DESC(4, 0);

/* Declare EP1 (Server): Basic, On/Off, Level Control */

/* Basic cluster attributes */
zb_uint8_t attr_zcl_version_1  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t attr_power_source_1 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list_1,
                                 &attr_zcl_version_1,
                                 &attr_power_source_1);

/* On/Off cluster attributes */
zb_uint8_t on_off_1 = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list_1,
                                  &on_off_1);

/* Level Control cluster attributes */
zb_uint8_t current_level_1  = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t remaining_time_1 = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;
ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(level_control_attr_list_1,
        &current_level_1,
        &remaining_time_1);

/* Declare EP2 cluster list*/
MULTI_EP_ZED_DECLARE_CLUSTER_LIST_EP1(ep1_cluster_list,
                                      basic_attr_list_1,
                                      on_off_attr_list_1,
                                      level_control_attr_list_1);

MULTI_EP_ZED_DECLARE_EP1(ep1, MULTI_EP_ZED_ENDPOINT_EP1, ep1_cluster_list);

/* Declare EP2 (Server): Basic, On/Off, Level Control */

/* Basic cluster attributes */
zb_uint8_t attr_zcl_version_2  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t attr_power_source_2 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list_2,
                                 &attr_zcl_version_2,
                                 &attr_power_source_2);

/* On/Off cluster attributes */
zb_uint8_t on_off_2 = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list_2,
                                  &on_off_2);

/* Level Control cluster attributes */
zb_uint8_t current_level_2  = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t remaining_time_2 = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;
ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(level_control_attr_list_2,
        &current_level_2,
        &remaining_time_2);

/* Declare EP2 cluster list*/
MULTI_EP_ZED_DECLARE_CLUSTER_LIST_EP2(ep2_cluster_list,
                                      basic_attr_list_2,
                                      on_off_attr_list_2,
                                      level_control_attr_list_2);

MULTI_EP_ZED_DECLARE_EP2(ep2, MULTI_EP_ZED_ENDPOINT_EP2, ep2_cluster_list);


/* Declare EP3 : Basic, Identify, Time */

/* Basic cluster attributes */
zb_uint8_t attr_zcl_version_3  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t attr_power_source_3 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list_3,
                                 &attr_zcl_version_3,
                                 &attr_power_source_3);

zb_uint16_t identify_time_3 = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list_3,
                                    &identify_time_3);

zb_zcl_time_attrs_t time_3;
ZB_ZCL_DECLARE_TIME_ATTR_LIST(time_attr_list_3,
                              time_3);

/* On/Off cluster attributes */
zb_uint8_t on_off_3 = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list_3,
                                  &on_off_3);


MULTI_EP_ZED_DECLARE_CLUSTER_LIST_EP3(ep3_cluster_list,
                                      basic_attr_list_3,
                                      identify_attr_list_3,
                                      time_attr_list_3,
                                      on_off_attr_list_3);

MULTI_EP_ZED_DECLARE_EP3(ep3, MULTI_EP_ZED_ENDPOINT_EP3, ep3_cluster_list);

/* Declare EP4: Basic, Identify, Alarms */

/* Basic cluster attributes */
zb_uint8_t attr_zcl_version_4  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t attr_power_source_4 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list_4,
                                 &attr_zcl_version_4,
                                 &attr_power_source_4);

zb_uint16_t identify_time_4 = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list_4,
                                    &identify_time_4);

zb_uint16_t alarm_count_4 = 0;

ZB_ZCL_DECLARE_ALARMS_ATTRIB_LIST(alarms_attr_list_4,
                                  &alarm_count_4);


MULTI_EP_ZED_DECLARE_CLUSTER_LIST_EP4(ep4_cluster_list,
                                      basic_attr_list_4,
                                      identify_attr_list_4,
                                      alarms_attr_list_4);

MULTI_EP_ZED_DECLARE_EP4(ep4, MULTI_EP_ZED_ENDPOINT_EP4, ep4_cluster_list);


/* Declare EP5 (Server): Basic, On/Off, Level Control */

/* Basic cluster attributes */
zb_uint8_t attr_zcl_version_5  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t attr_power_source_5 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list_5,
                                 &attr_zcl_version_5,
                                 &attr_power_source_5);

/* On/Off cluster attributes */
zb_uint8_t on_off_5 = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list_5,
                                  &on_off_5);

/* Level Control cluster attributes */
zb_uint8_t current_level_5  = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t remaining_time_5 = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;
ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(level_control_attr_list_5,
        &current_level_5,
        &remaining_time_5);

/* Declare EP5 cluster list*/
MULTI_EP_ZED_DECLARE_CLUSTER_LIST_EP5(ep5_cluster_list,
                                      basic_attr_list_5,
                                      on_off_attr_list_5,
                                      level_control_attr_list_5);

MULTI_EP_ZED_DECLARE_EP5(ep5, MULTI_EP_ZED_ENDPOINT_EP5, ep5_cluster_list);



MULTI_EP_ZED_DECLARE_CTX(multiendpoint_zed_device_zcl_ctx, ep1, ep2, ep3, ep4, ep5);


MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("multiendpoint_zed");

    zb_set_long_address(g_zr_addr);
    zb_set_network_ed_role(MULTI_EP_ZED_DEFAULT_APS_CHANNEL_MASK);
    zb_set_rx_on_when_idle(ZB_TRUE);

    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&multiendpoint_zed_device_zcl_ctx);

    /* Initiate the stack start with starting the commissioning */
    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR dev_start failed", (FMT__0));
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

/* Client EndPoints with on/off cluster of HA_multi_ep/multiendpoint_zc */
#define ZC_EP1  21
#define ZC_EP2  22
#define ZC_EP3  23
#define ZC_EP4  24

void configure_reporting_locally(zb_bufid_t param)
{
    zb_zcl_reporting_info_t rep_info;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, ">> configure_reporting_locally", (FMT__0));

    ZB_BZERO(&rep_info, sizeof(rep_info));

    /*  Report ZED_EP2 (according to binding to ZC_EP2)  */
    rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
    rep_info.ep = MULTI_EP_ZED_ENDPOINT_EP1;
    rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_ON_OFF;
    rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
    rep_info.attr_id = ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID;
    rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

    rep_info.u.send_info.def_min_interval = 1;
    rep_info.u.send_info.def_max_interval = 2;
    rep_info.u.send_info.delta.u32 = 0; /* not an analog data type*/

    zb_zcl_put_reporting_info(&rep_info, ZB_TRUE);

    TRACE_MSG(TRACE_APP1, "<< configure_reporting_locally", (FMT__0));

}


void toggle_on_off(zb_uint8_t on_off_state)
{
    TRACE_MSG(TRACE_APP1, ">> toggle_on_off on_off_state %hd ", (FMT__H, on_off_state));

    if (on_off_state == ZB_ZCL_ON_OFF_IS_ON)
    {
        on_off_state = ZB_ZCL_ON_OFF_IS_OFF;
    }
    else
    {
        on_off_state = ZB_ZCL_ON_OFF_IS_ON;
    }

    ZB_ZCL_SET_ATTRIBUTE(MULTI_EP_ZED_ENDPOINT_EP1, ZB_ZCL_CLUSTER_ID_ON_OFF,
                         ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                         &on_off_state, ZB_FALSE);


    ZB_SCHEDULE_APP_ALARM(toggle_on_off, on_off_state,  3 * ZB_TIME_ONE_SECOND);

    TRACE_MSG(TRACE_APP1, "<< toggle_on_off on_off_state %hd ", (FMT__H, on_off_state));
}

void configure_local_binding_completed(zb_bufid_t param)
{
    TRACE_MSG(TRACE_APP1, ">> configure_local_binding_completed", (FMT__0));

    zb_buf_free(param);

    ZB_SCHEDULE_APP_ALARM(configure_reporting_locally, 0, 1 * ZB_TIME_ONE_SECOND);
    ZB_SCHEDULE_APP_ALARM(toggle_on_off, ZB_ZCL_ON_OFF_IS_ON,  8 * ZB_TIME_ONE_SECOND);

    TRACE_MSG(TRACE_APP1, "<< configure_local_binding_completed", (FMT__0));
}

void configure_local_binding(zb_bufid_t param)
{
    zb_apsme_binding_req_t *req;

    /* Hardcoded address coordinator: HA_multi_ep/multiendpoint_zc */
    zb_ieee_addr_t coordinator_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

    if (!param)
    {
        zb_buf_get_out_delayed(configure_local_binding);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, ">> configure_local_binding", (FMT__0));

        req = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);

        zb_get_long_address(req->src_addr);

        ZB_IEEE_ADDR_COPY(req->dst_addr.addr_long, coordinator_addr);

        /* Bind On/Off cluster 1 */
        req = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);
        req->src_endpoint = MULTI_EP_ZED_ENDPOINT_EP1;
        req->clusterid = ZB_ZCL_CLUSTER_ID_ON_OFF;
        req->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        req->dst_endpoint = ZC_EP1;
        req->confirm_cb = configure_local_binding_completed;

        zb_apsme_bind_request(param);

        TRACE_MSG(TRACE_APP1, "<< configure_local_binding", (FMT__0));
    }
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
    /* Get application signal from the buffer */
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:

            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            if (ZB_JOINED())
            {
                ZB_SCHEDULE_APP_CALLBACK(configure_local_binding, param);
                param = 0;
            }
            break;

        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
            TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));

            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
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

    /* Free the buffer if it is not used */
    if (param)
    {
        zb_buf_free(param);
    }
}
