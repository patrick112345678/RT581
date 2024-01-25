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
/* PURPOSE: Custom cluster sample
*/

#define ZB_TRACE_FILE_ID 60778

#include "zboss_api.h"
#include "custom_cluster_zc.h"

#ifndef ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile sample!
#endif

/**
 * Global variables definitions
 */

zb_ieee_addr_t g_zc_addr = { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa }; /* IEEE address of
                                                                                * the device*/

zb_uint8_t dst_endpoint = 0xFF;
zb_uint16_t dst_short_addr = 0xFFFF;

/* Enumeration with test steps */
typedef enum test_steps_e
{
    TEST_STEP_SEND_CMD1,
    TEST_STEP_SEND_CMD2,
    TEST_STEP_SEND_CMD3,
    TEST_STEP_SEND_READ_ATTR_U8_ID,
    TEST_STEP_SEND_READ_ATTR_S16_ID,
    TEST_STEP_SEND_READ_ATTR_24BIT_ID,
    TEST_STEP_SEND_READ_ATTR_32BITMAP_ID,
    TEST_STEP_SEND_READ_ATTR_IEEE_ID,
    TEST_STEP_SEND_READ_ATTR_CHAR_STRING_ID,
    TEST_STEP_SEND_READ_ATTR_UTC_TIME_ID,
    TEST_STEP_SEND_READ_ATTR_BYTE_ARRAY_ID,
    TEST_STEP_SEND_READ_ATTR_BOOL_ID,
    TEST_STEP_SEND_READ_ATTR_128_BIT_KEY_ID,

    TEST_STEP_SEND_READ_ATTR_U8_ID_READ_ONLY,
    TEST_STEP_SEND_READ_ATTR_S16_ID_WRITE_ONLY,
    TEST_STEP_SEND_WRITE_TWO_ATTRS,
    TEST_STEP_SEND_READ_ATTR_U8_ID_READ_WRITE,
    TEST_STEP_SEND_READ_ATTR_S16_ID_READ_WRITE,

    TEST_STEP_SEND_WRITE_ATTR_U8_ID,
    TEST_STEP_SEND_WRITE_ATTR_S16_ID,
    TEST_STEP_SEND_WRITE_ATTR_24BIT_ID,
    TEST_STEP_SEND_WRITE_ATTR_32BITMAP_ID,
    TEST_STEP_SEND_WRITE_ATTR_IEEE_ID,
    TEST_STEP_SEND_WRITE_ATTR_CHAR_STRING_ID,
    TEST_STEP_SEND_WRITE_ATTR_UTC_TIME_ID,
    TEST_STEP_SEND_WRITE_ATTR_BYTE_ARRAY_ID,
    TEST_STEP_SEND_WRITE_ATTR_BOOL_ID,
    TEST_STEP_SEND_WRITE_ATTR_128_BIT_KEY_ID,
    TEST_STEP_FINISH
} test_steps_t;

#define NUMBER_TEST_STEPS 23

/* Current test step */
zb_uint8_t test_step = TEST_STEP_SEND_CMD1;
zb_bool_t test_is_started = ZB_FALSE;

/**
 * Declaring attributes for each cluster
 */

/* Basic cluster attributes */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
    basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Custom cluster attributes */
zb_uint8_t g_attr_u8 = ZB_ZCL_CUSTOM_CLUSTER_ATTR_U8_DEFAULT_VALUE;
zb_int16_t g_attr_s16 = ZB_ZCL_CUSTOM_CLUSTER_ATTR_S16_DEFAULT_VALUE;
zb_uint24_t g_attr_24bit = ZB_ZCL_CUSTOM_CLUSTER_ATTR_24BIT_DEFAULT_VALUE;
zb_uint32_t g_attr_32bitmap = ZB_ZCL_CUSTOM_CLUSTER_ATTR_32BITMAP_DEFAULT_VALUE;
zb_ieee_addr_t g_attr_ieee = ZB_ZCL_CUSTOM_CLUSTER_ATTR_IEEE_DEFAULT_VALUE;
zb_char_t g_attr_char_string[ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_MAX_SIZE] =
    ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_DEFAULT_VALUE;
zb_time_t g_attr_utc_time = ZB_ZCL_CUSTOM_CLUSTER_ATTR_UTC_TIME_DEFAULT_VALUE;
zb_uint8_t g_attr_byte_array[ZB_ZCL_CUSTOM_CLUSTER_ATTR_BYTE_ARRAY_MAX_SIZE] =
    ZB_ZCL_CUSTOM_CLUSTER_ATTR_BYTE_ARRAY_DEFAULT_VALUE;
zb_bool_t g_attr_bool = ZB_ZCL_CUSTOM_CLUSTER_ATTR_BOOL_DEFAULT_VALUE;
zb_uint8_t g_attr_128_bit_key[ZB_CCM_KEY_SIZE] = ZB_ZCL_CUSTOM_CLUSTER_ATTR_128_BIT_KEY_DEFAULT_VALUE;

ZB_ZCL_DECLARE_CUSTOM_ATTR_CLUSTER_ATTRIB_LIST(custom_attr_list,
        &g_attr_u8,
        &g_attr_s16,
        &g_attr_24bit,
        &g_attr_32bitmap,
        g_attr_ieee,
        g_attr_char_string,
        &g_attr_utc_time,
        g_attr_byte_array,
        &g_attr_bool,
        g_attr_128_bit_key);

ZB_DECLARE_CUSTOM_CLUSTER_LIST(custom_clusters,
                               basic_attr_list,
                               identify_attr_list,
                               custom_attr_list);

/* Declare endpoint */
ZB_DECLARE_CUSTOM_EP(custom_ep, ENDPOINT_CLIENT, custom_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_CUSTOM_CTX(custom_ctx, custom_ep);

/**
 * Declaring of application functions
 */

/* Handler for specific ZCL commands */
static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

static void start_fb_initiator(zb_uint8_t param);
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t long_addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);

/* Function for executing current test step */
static void custom_cluster_do_test_step(zb_uint8_t param);
/* Callback which is called after each next test step, change test_step value */
static void custom_cluster_do_test_step_cb(zb_uint8_t param);

/* Handler for specific Custom cluster commands, called in zcl_specific_cluster_cmd_handler */
static zb_zcl_status_t zb_zcl_custom_cluster_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info);

/* Handler for CMD1 of the Custom cluster */
static zb_zcl_status_t zb_zcl_custom_cluster_cmd1_resp_handler(zb_bufid_t buf);
/* Handler for CMD2 of the Custom cluster */
static zb_zcl_status_t zb_zcl_custom_cluster_cmd2_resp_handler(zb_bufid_t buf);

static void send_read_attr(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID)
{
    zb_uint8_t *cmd_ptr;
    ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));
    ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ((buffer), cmd_ptr, dst_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                      dst_endpoint, ENDPOINT_CLIENT, ZB_AF_HA_PROFILE_ID, (clusterID), custom_cluster_do_test_step_cb);
}

static void send_write_attr(
    zb_bufid_t buffer,
    zb_uint16_t clusterID,
    zb_uint16_t attributeID,
    zb_uint8_t attrType,
    zb_uint8_t *attrVal)
{
    zb_uint8_t *cmd_ptr;
    ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
    ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(cmd_ptr, (attributeID), (attrType), (attrVal));
    ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ((buffer), cmd_ptr, dst_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                       dst_endpoint, ENDPOINT_CLIENT, ZB_AF_HA_PROFILE_ID, (clusterID), custom_cluster_do_test_step_cb);
}

MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRAF_DUMP_ON();

    /* Trace configure */
    ZB_SET_TRACE_LEVEL(4);
    ZB_SET_TRACE_MASK(0x0800);

    /* Global ZBOSS initialization */
    ZB_INIT("custom_cluster_zc");

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_zc_addr);
    zb_set_network_coordinator_role(ZB_CUSTOM_CHANNEL_MASK);
    zb_set_max_children(1);

    /* Disable NVRAM erasing after start */
    zb_set_nvram_erase_at_start(ZB_FALSE);

    /* Set PAN ID */
    zb_set_pan_id(0x1aaa);

    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&custom_ctx);

    /* Register cluster commands handler for a specific endpoint */
    /* callback will be call BEFORE stack handle */
    ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT_CLIENT, zcl_specific_cluster_cmd_handler);

    /* Initiate the stack start with starting the commissioning */
    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
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


void zb_zcl_custom_attr_init_client(void)
{
    TRACE_MSG(TRACE_APP1, ">> zb_zcl_custom_attr_init_client", (FMT__0));

    /* Implement initialization of a custom cluster client here */

    TRACE_MSG(TRACE_APP1, "<< zb_zcl_custom_attr_init_client", (FMT__0));
}


static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_uint8_t lqi = ZB_MAC_LQI_UNDEFINED;
    zb_int8_t rssi = ZB_MAC_RSSI_UNDEFINED;
    zb_bool_t handle_status = ZB_FALSE;
    zb_zcl_status_t resp_status = ZB_ZCL_STATUS_ABORT;
    zb_zcl_parsed_hdr_t cmd_info;

    ZB_ZCL_COPY_PARSED_HEADER(buf, &cmd_info);

    TRACE_MSG(TRACE_APP1, ">> zcl_specific_cluster_cmd_handler, param %d", (FMT__D, param));
    TRACE_MSG(TRACE_APP1, "dir %d, profile_id 0x%x, cluster_id 0x%x",
              (FMT__D_D_D, cmd_info.cmd_direction, cmd_info.profile_id, cmd_info.cluster_id));

    zb_zdo_get_diag_data(
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr,
        &lqi, &rssi);
    TRACE_MSG(TRACE_APP1, "lqi %hd rssi %d", (FMT__H_H, lqi, rssi));

    if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
    {
        TRACE_MSG(TRACE_ERROR, "Unsupported \"from client\" command direction", (FMT__0));
    }
    else
    {
        switch (cmd_info.profile_id)
        {
        case ZB_AF_HA_PROFILE_ID:
            switch (cmd_info.cluster_id)
            {
            case ZB_ZCL_CLUSTER_ID_CUSTOM:
                resp_status = zb_zcl_custom_cluster_handler(buf, &cmd_info);
                break;

            /* Implement the processing of other clusters here */

            default:
                break;
            }
            break;

        /* Implement the processing of other profiles here */

        default:
            break;
        }

        TRACE_MSG(TRACE_APP1, "resp_status %d", (FMT__D, resp_status));

        if (resp_status == ZB_ZCL_STATUS_ABORT)
        {
        }
        else if (resp_status != ZB_ZCL_STATUS_SUCCESS
                 || (resp_status == ZB_ZCL_STATUS_SUCCESS
                     && !(cmd_info.disable_default_response)))
        {
            TRACE_MSG(TRACE_APP1, "send default response from app", (FMT__0));
            ZB_ZCL_SEND_DEFAULT_RESP(
                buf,
                ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint,
                ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint,
                cmd_info.profile_id,
                ZB_ZCL_CLUSTER_ID_CUSTOM,
                cmd_info.seq_number,
                cmd_info.cmd_id,
                resp_status);
        }
    }

    if (ZB_ZCL_STATUS_SUCCESS == resp_status)
    {
        handle_status = ZB_TRUE;
        zb_buf_free(buf);
    }

    TRACE_MSG(TRACE_APP1, "handle_status %d", (FMT__D, handle_status));
    TRACE_MSG(TRACE_APP1, "<< zcl_specific_cluster_cmd_handler", (FMT__0));

    return handle_status;
}


static zb_zcl_status_t zb_zcl_custom_cluster_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_status_t resp_status;

    switch (cmd_info->cmd_id)
    {
    case ZB_ZCL_CUSTOM_CLUSTER_CMD1_RESP_ID:
        TRACE_MSG(TRACE_APP1, "Custom cluster Response on Command 1 received!", (FMT__0));
        resp_status = zb_zcl_custom_cluster_cmd1_resp_handler(buf);
        break;

    case ZB_ZCL_CUSTOM_CLUSTER_CMD2_RESP_ID:
        TRACE_MSG(TRACE_APP1, "Custom cluster Response on Command 2 received!", (FMT__0));
        resp_status = zb_zcl_custom_cluster_cmd2_resp_handler(buf);
        break;

    case ZB_ZCL_CMD_DEFAULT_RESP:
        TRACE_MSG(TRACE_ERROR, "Default response received!", (FMT__0));
        resp_status = ZB_ZCL_STATUS_SUCCESS;
        break;

    default:
        TRACE_MSG(TRACE_ERROR, "Unknown command received!", (FMT__0));
        resp_status = ZB_ZCL_STATUS_ABORT;
        break;
    }

    TRACE_MSG(TRACE_APP1, "resp_stauts %d", (FMT__D, resp_status));
    return resp_status;
}


static zb_zcl_status_t zb_zcl_custom_cluster_cmd1_resp_handler(zb_bufid_t buf)
{
    zb_zcl_status_t resp_status = ZB_ZCL_STATUS_SUCCESS;
    zb_zcl_parse_status_t parse_status;
    zb_zcl_custom_cluster_cmd1_resp_t cmd1_resp;

    ZB_ZCL_CUSTOM_CLUSTER_GET_CMD1_RESP(buf, cmd1_resp, parse_status);

    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        /* Implement the processing of the command response here */
    }
    else
    {
        resp_status = ZB_ZCL_STATUS_MALFORMED_CMD;
    }

    return resp_status;
}


static zb_zcl_status_t zb_zcl_custom_cluster_cmd2_resp_handler(zb_bufid_t buf)
{
    zb_zcl_status_t resp_status = ZB_ZCL_STATUS_SUCCESS;
    zb_zcl_parse_status_t parse_status;
    zb_zcl_custom_cluster_cmd2_resp_t cmd2_resp;

    ZB_ZCL_CUSTOM_CLUSTER_GET_CMD2_RESP(buf, cmd2_resp, parse_status);

    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        /* Implement the processing of the command response here */
    }
    else
    {
        resp_status = ZB_ZCL_STATUS_MALFORMED_CMD;
    }

    return resp_status;
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
            ZB_SCHEDULE_APP_ALARM(start_fb_initiator, 0, 3 * ZB_TIME_ONE_SECOND);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APP1, "Finding&binding done", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal %hd", (FMT__H, sig));
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR,
                  "Device started FAILED status %d",
                  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    /* Free the buffer if it is not used */
    if (param)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}


static void start_fb_initiator(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_bdb_finding_binding_initiator(ENDPOINT_CLIENT, finding_binding_cb);
}


static zb_bool_t finding_binding_cb(
    zb_int16_t status,
    zb_ieee_addr_t long_addr,
    zb_uint8_t ep,
    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_APP1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(long_addr), ep, cluster));

    if (ZB_BDB_COMM_BIND_SUCCESS == status
            && ZB_ZCL_CLUSTER_ID_CUSTOM == cluster)
    {
        test_is_started = ZB_TRUE;
        dst_endpoint = ep;
        dst_short_addr = zb_address_short_by_ieee(long_addr);

        ZB_SCHEDULE_APP_CALLBACK(custom_cluster_do_test_step, test_step);
    }

    return ZB_TRUE;
}


void custom_cluster_do_test_step(zb_uint8_t param)
{
    zb_bufid_t buf;
    TRACE_MSG(TRACE_APP1, ">> custom_cluster_do_test_step", (FMT__0));

    if (dst_endpoint == 0xFF
            || dst_short_addr == 0xFFFF)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR: invalid params to send command!", (FMT__0));
        return;
    }

    buf = zb_buf_get_out();
    if (!buf)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR: could not get out buf!", (FMT__0));
        return;
    }

    TRACE_MSG(TRACE_APP1, "buf ref %d", (FMT__D, param));

    switch (param)
    {
    /**
     * Custom Cluster Commands sending
     */
    case TEST_STEP_SEND_CMD1:
        ZB_ZCL_CUSTOM_CLUSTER_SEND_CMD1_REQ(
            buf,
            dst_short_addr,
            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            dst_endpoint,
            ENDPOINT_CLIENT,
            DISABLE_DEFAULT_RESPONSE_FLAG,
            custom_cluster_do_test_step_cb,
            ZB_ZCL_CUSTOM_CLUSTER_CMD1_MODE1,
            0xaa);

        TRACE_MSG(TRACE_APP1, "Send ", (FMT__0));
        break;

    case TEST_STEP_SEND_CMD2:
        ZB_ZCL_CUSTOM_CLUSTER_SEND_CMD2_REQ(
            buf,
            dst_short_addr,
            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            dst_endpoint,
            ENDPOINT_CLIENT,
            DISABLE_DEFAULT_RESPONSE_FLAG,
            custom_cluster_do_test_step_cb,
            ZB_ZCL_CUSTOM_CLUSTER_CMD2_PARAM1,
            0xaaaa);

        TRACE_MSG(TRACE_APP1, "Send ", (FMT__0));
        break;

    case TEST_STEP_SEND_CMD3:
    {
        zb_char_t zcl_str[ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_MAX_SIZE] = { 0xaa };

        zcl_str[0] = ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_MAX_SIZE - 1;

        ZB_ZCL_CUSTOM_CLUSTER_SEND_CMD3_REQ(
            buf,
            dst_short_addr,
            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            dst_endpoint,
            ENDPOINT_CLIENT,
            DISABLE_DEFAULT_RESPONSE_FLAG,
            custom_cluster_do_test_step_cb,
            zcl_str);

        TRACE_MSG(TRACE_APP1, "Send ", (FMT__0));
    }
    break;

    /**
     * 'Read Attribute' commands sending
     */
    case TEST_STEP_SEND_READ_ATTR_U8_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_U8_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_U8", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_S16_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_S16_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_S16", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_24BIT_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_24BIT_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_24BIT", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_32BITMAP_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_32BITMAP_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_32BITMAP", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_IEEE_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_IEEE_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_IEEE", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_CHAR_STRING_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_CHAR_STRING", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_UTC_TIME_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_UTC_TIME_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_UTC_TIME", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_BYTE_ARRAY_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_OCTET_STRING_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_BYTE_ARRAY", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_BOOL_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_BOOL_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_BOOL", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_128_BIT_KEY_ID:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_128_BIT_KEY_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_128_BIT_KEY", (FMT__0));
        break;

    /**
     * Sequence of steps to check WriteAttrUndivided command
     */
    case TEST_STEP_SEND_READ_ATTR_U8_ID_READ_ONLY:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_U8_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_U8", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_S16_ID_WRITE_ONLY:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_S16_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_S16", (FMT__0));
        break;

    case TEST_STEP_SEND_WRITE_TWO_ATTRS:
    {
        zb_uint8_t value_1 = 0x01;
        zb_int16_t value_2 = -1234;
        zb_uint8_t *cmd_ptr;

        ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ_UNDIV(
            buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
            cmd_ptr, ZB_ZCL_CUSTOM_CLUSTER_ATTR_U8_ID, ZB_ZCL_ATTR_TYPE_U8, &value_1);
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
            cmd_ptr, ZB_ZCL_CUSTOM_CLUSTER_ATTR_S16_ID, ZB_ZCL_ATTR_TYPE_S16, (zb_uint8_t *)&value_2);
        ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ(
            buf, cmd_ptr, dst_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, dst_endpoint, ENDPOINT_CLIENT,
            ZB_AF_HA_PROFILE_ID, ZB_ZCL_CLUSTER_ID_CUSTOM, custom_cluster_do_test_step_cb);
    }
    break;

    case TEST_STEP_SEND_READ_ATTR_U8_ID_READ_WRITE:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_U8_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_U8", (FMT__0));
        break;

    case TEST_STEP_SEND_READ_ATTR_S16_ID_READ_WRITE:
        send_read_attr(buf, ZB_ZCL_CLUSTER_ID_CUSTOM, ZB_ZCL_CUSTOM_CLUSTER_ATTR_S16_ID);
        TRACE_MSG(TRACE_APP1, "Read ATTR_S16", (FMT__0));
        break;

    /**
     * 'Write Attribute' commands sending
     */
    case TEST_STEP_SEND_WRITE_ATTR_U8_ID:
    {
        zb_uint8_t value = 0x20;
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_U8_ID,
            ZB_ZCL_ATTR_TYPE_U8,
            &value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_U8", (FMT__0));
    }
    break;

    case TEST_STEP_SEND_WRITE_ATTR_S16_ID:
    {
        zb_int16_t value = -9876;
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_S16_ID,
            ZB_ZCL_ATTR_TYPE_S16,
            (zb_uint8_t *)&value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_S16", (FMT__0));
    }
    break;

    case TEST_STEP_SEND_WRITE_ATTR_24BIT_ID:
    {
        zb_uint24_t value;
        ZB_ASSIGN_UINT24(value, 0x96, 0xC30F);
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_24BIT_ID,
            ZB_ZCL_ATTR_TYPE_24BIT,
            (zb_uint8_t *)&value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_24BIT", (FMT__0));
    }
    break;

    case TEST_STEP_SEND_WRITE_ATTR_32BITMAP_ID:
    {
        zb_uint32_t value = 0xABCD1234;
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_32BITMAP_ID,
            ZB_ZCL_ATTR_TYPE_32BITMAP,
            (zb_uint8_t *)&value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_32BITMAP", (FMT__0));
    }
    break;

    case TEST_STEP_SEND_WRITE_ATTR_IEEE_ID:
    {
        zb_ieee_addr_t value = { 0x01, 0x02, 0x03, 0x04, 0x40, 0x30, 0x20, 0x10 };
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_IEEE_ID,
            ZB_ZCL_ATTR_TYPE_IEEE_ADDR,
            (zb_uint8_t *)&value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_IEEE", (FMT__0));
    }
    break;

    case TEST_STEP_SEND_WRITE_ATTR_CHAR_STRING_ID:
    {
        zb_char_t value[15];
        zb_char_t str[14] = "Hellow, World!";
        ZB_ZCL_SET_STRING_VAL(value, str, 14);
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_ID,
            ZB_ZCL_ATTR_TYPE_CHAR_STRING,
            (zb_uint8_t *)value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_CHAR_STRING", (FMT__0));
    }
    break;

    case TEST_STEP_SEND_WRITE_ATTR_UTC_TIME_ID:
    {
        zb_time_t value = 896140801;
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_UTC_TIME_ID,
            ZB_ZCL_ATTR_TYPE_UTC_TIME,
            (zb_uint8_t *)&value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_UTC_TIME", (FMT__0));
    }
    break;

    case TEST_STEP_SEND_WRITE_ATTR_BYTE_ARRAY_ID:
    {
        zb_uint8_t value[16];
        ZB_MEMSET(value, 0xff, 16);
        /* ZB_ZCL_ARRAY_SET_SIZE(value, 14); */
        value[0] = 15;
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_OCTET_STRING_ID,
            ZB_ZCL_ATTR_TYPE_OCTET_STRING,
            value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_BYTE_ARRAY", (FMT__0));
    }
    break;

    case TEST_STEP_SEND_WRITE_ATTR_BOOL_ID:
    {
        zb_bool_t value = ZB_TRUE;
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_BOOL_ID,
            ZB_ZCL_ATTR_TYPE_BOOL,
            (zb_uint8_t *)&value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_BOOL", (FMT__0));
    }
    break;

    case TEST_STEP_SEND_WRITE_ATTR_128_BIT_KEY_ID:
    {
        zb_uint8_t value[ZB_CCM_KEY_SIZE] =
        {
            0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        send_write_attr(
            buf,
            ZB_ZCL_CLUSTER_ID_CUSTOM,
            ZB_ZCL_CUSTOM_CLUSTER_ATTR_128_BIT_KEY_ID,
            ZB_ZCL_ATTR_TYPE_128_BIT_KEY,
            value);
        TRACE_MSG(TRACE_APP1, "Write ATTR_128_BIT_KEY", (FMT__0));
    }
    break;

    default:
        TRACE_MSG(TRACE_APP1, "Unknown command type!", (FMT__0));
        break;
    }

    TRACE_MSG(TRACE_APP1, "<< custom_cluster_do_test_step", (FMT__0));
}


static void custom_cluster_do_test_step_cb(zb_uint8_t param)
{
    zb_zcl_command_send_status_t *cmd_send_status =
        ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);

    TRACE_MSG(
        TRACE_APP1,
        "custom_cluster_do_test_step_cb, param %hd, status %hd",
        (FMT__H_H, param, cmd_send_status->status));

    zb_buf_free(param);

    ++test_step;

    ZB_SCHEDULE_APP_ALARM(
        custom_cluster_do_test_step,
        test_step,
        DELAY_OF_SEND_CMD);
}
