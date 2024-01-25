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
/* PURPOSE: Simple GW application
*/

#define ZB_TRACE_FILE_ID 40016

#include "multiendpoint_zc.h"

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile
#endif

#ifdef ZB_ASSERT_SEND_NWK_REPORT
void assert_indication_cb(zb_uint16_t file_id, zb_int_t line_number);
#endif


/* -> Enable sending of:
        active_ep_req,
          for each active EP:
            -> simple_desc_req
            (...)
            for each cluster in EP:
              -> discover_attr_req
              (...)
   -> Store discovered info in g_device_ctx
*/
#define MUTI_EP_ZED_DISCOVERY

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_zc_addr = MULTI_EP_ZC_IEEE_ADDR;
zb_uint8_t g_active_cmds = 0;

/* Global device context */
multi_ep_zc_device_ctx_t g_device_ctx;

/* [zb_secur_setup_preconfigured_key_value] */
/* NWK key for the device */
static const zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
/* [zb_secur_setup_preconfigured_key_value] */

/*Declare simple_desc structure for EPs with 0 inputs and 3 output clusters (EP1 & EP2)*/
ZB_DECLARE_SIMPLE_DESC(0, 3);

/*Declare simple_desc structure for EPs with 0 inputs and 4 output clusters (EP3 & EP4)*/
ZB_DECLARE_SIMPLE_DESC(0, 4);

/* Declare EP1: Basic, On/Off, Level Control */
ZB_HA_DECLARE_MULTI_EP_ZC_CLUSTER_LIST_EP1(ep1_cluster_list);

ZB_HA_DECLARE_MULTI_EP_ZC_EP1(ep1, MULTI_EP_ZC_ENDPOINT_EP1, ep1_cluster_list);

/* Declare EP2: Basic, On/Off, Level Control */
ZB_HA_DECLARE_MULTI_EP_ZC_CLUSTER_LIST_EP2(ep2_cluster_list);

ZB_HA_DECLARE_MULTI_EP_ZC_EP2(ep2, MULTI_EP_ZC_ENDPOINT_EP2, ep2_cluster_list);

/* Declare EP3: Basic, Identify, Time */
ZB_HA_DECLARE_MULTI_EP_ZC_CLUSTER_LIST_EP3(ep3_cluster_list);

ZB_HA_DECLARE_MULTI_EP_ZC_EP3(ep3, MULTI_EP_ZC_ENDPOINT_EP3, ep3_cluster_list);

/* Declare EP4: Basic, Identify, Alarms */
ZB_HA_DECLARE_MULTI_EP_ZC_CLUSTER_LIST_EP4(ep4_cluster_list);

ZB_HA_DECLARE_MULTI_EP_ZC_EP4(ep4, MULTI_EP_ZC_ENDPOINT_EP4, ep4_cluster_list);

/* Declare EP5: Basic, On/Off, Level Control */
ZB_HA_DECLARE_MULTI_EP_ZC_CLUSTER_LIST_EP5(ep5_cluster_list);

ZB_HA_DECLARE_MULTI_EP_ZC_EP5(ep5, MULTI_EP_ZC_ENDPOINT_EP5, ep5_cluster_list);


/* Declare application's device context for single-endpoint device */
ZB_HA_DECLARE_MULTI_EP_ZC_CTX(multi_ep_zc_ctx, ep1, ep2, ep3, ep4, ep5);

void report_attribute_cb(zb_uint16_t addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                         zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value);

#ifdef MUTI_EP_ZED_DISCOVERY
static void attr_disc_cb(zb_uint8_t param);
#endif

void schedule_test_loop(zb_uint8_t dev_idx);

MAIN()
{
    ARGV_UNUSED;

    /* Global device context initialization */
    ZB_MEMSET(&g_device_ctx, 0, sizeof(g_device_ctx));

    /* Uncomment to change trace level and mask. */
    /* ZB_SET_TRACE_LEVEL(4); */
    /* ZB_SET_TRACE_MASK(0x0800); */

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable */
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("multiendpoint_zc");

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_zc_addr);
    zb_set_network_coordinator_role(MULTI_EP_ZC_CHANNEL_MASK);
    zb_set_nvram_erase_at_start(ZB_FALSE);
    zb_set_max_children(MULTI_EP_ZC_DEV_NUMBER);

    /* Optional step: Setup predefined nwk key - to easily decrypt ZB sniffer logs which does not
     * contain keys exchange. By default nwk key is randomly generated. */
    /* [zb_secur_setup_preconfigured_key] */
    zb_secur_setup_nwk_key((zb_uint8_t *) g_key_nwk, 0);
    /* [zb_secur_setup_preconfigured_key] */

    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&multi_ep_zc_ctx);

    /* Register cluster commands handler for EP1 and EP2 a specific endpoint */
    ZB_AF_SET_ENDPOINT_HANDLER(MULTI_EP_ZC_ENDPOINT_EP1, zcl_specific_cluster_cmd_handler);
    ZB_AF_SET_ENDPOINT_HANDLER(MULTI_EP_ZC_ENDPOINT_EP2, zcl_specific_cluster_cmd_handler);
    ZB_AF_SET_ENDPOINT_HANDLER(MULTI_EP_ZC_ENDPOINT_EP3, zcl_specific_cluster_cmd_handler);
    ZB_AF_SET_ENDPOINT_HANDLER(MULTI_EP_ZC_ENDPOINT_EP4, zcl_specific_cluster_cmd_handler);

    ZB_ZCL_SET_REPORT_ATTR_CB((zb_zcl_report_attr_cb_t)report_attribute_cb);


#ifdef ZB_ASSERT_SEND_NWK_REPORT
    zb_register_zboss_callback(ZB_ASSERT_INDICATION_CB, SET_ZBOSS_CB(assert_indication_cb));
#endif

    /* Initiate the stack start with starting the commissioning */
    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
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

#define ZC_EP1 MULTI_EP_ZC_ENDPOINT_EP1
#define ZC_EP2 MULTI_EP_ZC_ENDPOINT_EP2
#define ZC_EP3 MULTI_EP_ZC_ENDPOINT_EP3
#define ZC_EP4 MULTI_EP_ZC_ENDPOINT_EP4
#define ZC_EP5 MULTI_EP_ZC_ENDPOINT_EP5

#define ZED_EP1 21
#define ZED_EP2 22
#define ZED_EP3 23
#define ZED_EP4 24
#define ZED_EP5 25

void test_loop(zb_bufid_t param, zb_uint16_t dev_idx)
{
    static zb_uint8_t test_step = 0;
    zb_uint8_t attr_value = ZB_ZCL_ON_OFF_IS_ON;

    TRACE_MSG(TRACE_APP1, ">> test_loop test_step=%hd", (FMT__H, test_step));

    switch (test_step)
    {

    /* Write/Read Steps */

    case 0:
        /* Write Attribute ON_OFF of ZED EP1(read only expected) */
        TRACE_MSG(TRACE_APP1, "WRITE_ATTR: src_ep=%hd, dst_ep=%hd, cluster=ZB_ZCL_CLUSTER_ID_ON_OFF" \
                  ", attr=ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, value=%hd", (FMT__H_H_H, ZC_EP1, ZED_EP1, attr_value));

        TEST_WRITE_ATTR(param, dev_idx, ZC_EP1, ZED_EP1, ZB_ZCL_CLUSTER_ID_ON_OFF,
                        ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, ZB_ZCL_ATTR_TYPE_BOOL, (zb_uint8_t *)&attr_value);
        break;

    case 1:
        /* Read Attribute ON_OFF of ZED EP1 */
        TRACE_MSG(TRACE_APP1, "READ_ATTR: src_ep=%hd, dst_ep=%hd, cluster=ZB_ZCL_CLUSTER_ID_ON_OFF" \
                  ", attr=ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID", (FMT__H_H_H, ZC_EP1, ZED_EP1));
        TEST_READ_ATTR(param, dev_idx, ZC_EP1, ZED_EP1, ZB_ZCL_CLUSTER_ID_ON_OFF,
                       ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);
        break;

    case 2:
        /* Read Attribute CURRENT_LEVEL of ZED EP2 */
        TRACE_MSG(TRACE_APP1, "READ_ATTR: src_ep=%hd, dst_ep=%hd, cluster=ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL" \
                  ", attr=ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID", (FMT__H_H, ZC_EP2, ZED_EP2));
        TEST_READ_ATTR(param, dev_idx, ZC_EP2, ZED_EP2, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
                       ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case 3:
        /* Read Attribute Identify Time of ZED EP3 */
        TRACE_MSG(TRACE_APP1, "READ_ATTR: src_ep=%hd, dst_ep=%hd, cluster=ZB_ZCL_CLUSTER_ID_IDENTIFY" \
                  ", attr=ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID", (FMT__H_H, ZC_EP3, ZED_EP3));
        TEST_READ_ATTR(param, dev_idx, ZC_EP3, ZED_EP3, ZB_ZCL_CLUSTER_ID_IDENTIFY,
                       ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID);
        break;

    case 4:
        /* Read Attribute Power Source of ZED EP4 */
        TRACE_MSG(TRACE_APP1, "READ_ATTR: src_ep=%hd, dst_ep=%hd, cluster=ZB_ZCL_CLUSTER_ID_BASIC" \
                  ", attr=ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID", (FMT__H_H, ZC_EP4, ZED_EP4));
        TEST_READ_ATTR(param, dev_idx, ZC_EP4, ZED_EP4, ZB_ZCL_CLUSTER_ID_BASIC,
                       ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID);
        break;

    /* Binding steps */

    case 5:
        /* Bind Cluster ON_OFF: ZED_EP2 => ZC_EP2 */
        TRACE_MSG(TRACE_APP1, "CONFIGURE_BINDING: src_ep=%hd (ZED), dst_ep=%hd (ZC), cluster=ZB_ZCL_CLUSTER_ID_ON_OFF",
                  (FMT__H_H, ZED_EP2, ZC_EP2));
        TEST_CONFIGURE_BINDING(param, dev_idx, ZC_EP2, ZED_EP2, ZB_ZCL_CLUSTER_ID_ON_OFF);
        break;
    case 6:
        /* Bind Cluster ON_OFF: ZED_EP3 => ZC_EP3 */
        TRACE_MSG(TRACE_APP1, "CONFIGURE_BINDING: src_ep=%hd (ZED), dst_ep=%hd (ZC), cluster=ZB_ZCL_CLUSTER_ID_ON_OFF",
                  (FMT__H_H, ZED_EP3, ZC_EP3));
        TEST_CONFIGURE_BINDING(param, dev_idx, ZC_EP3, ZED_EP3, ZB_ZCL_CLUSTER_ID_ON_OFF);
        break;

    case 7:
        /* Bind Cluster ON_OFF: ZED_EP4 => ZC_EP4  (Cluster ON_OFF does not exist in EP4) */
        TRACE_MSG(TRACE_APP1, "CONFIGURE_BINDING: src_ep=%hd (ZED), dst_ep=%hd (ZC), cluster=ZB_ZCL_CLUSTER_ID_ON_OFF",
                  (FMT__H_H, ZED_EP4, ZC_EP4));
        TEST_CONFIGURE_BINDING(param, dev_idx, ZC_EP4, ZED_EP4, ZB_ZCL_CLUSTER_ID_ON_OFF);
        break;
    case 8:
        /* Bind Cluster ON_OFF: ZED_EP5 => ZC_EP5*/
        TRACE_MSG(TRACE_APP1, "CONFIGURE_BINDING: src_ep=%hd (ZED), dst_ep=%hd (ZC), cluster=ZB_ZCL_CLUSTER_ID_ON_OFF",
                  (FMT__H_H, ZED_EP5, ZC_EP5));
        TEST_CONFIGURE_BINDING(param, dev_idx, ZC_EP5, ZED_EP5, ZB_ZCL_CLUSTER_ID_ON_OFF);
        break;

    /* Configure Reporting steps*/

    case 9:
        /* Configure reporting ON_OFF: ZED_EP2 => ZC_EP2 */
        TRACE_MSG(TRACE_APP1, "CONFIGURE_REPORTING: src_ep=%hd (ZED), dst_ep=%hd (ZC), cluster=ZB_ZCL_CLUSTER_ID_ON_OFF",
                  (FMT__H_H, ZED_EP2, ZC_EP2));
        TEST_CONFIGURE_REPORTING_SRV(param, dev_idx, ZC_EP2, ZED_EP2, ZB_ZCL_CLUSTER_ID_ON_OFF,
                                     ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, ZB_ZCL_ATTR_TYPE_BOOL,
                                     1, 3, 0);
        break;
    case 10:
        /* Configure reporting ON_OFF: ZED_EP3 => ZC_EP3 */
        TRACE_MSG(TRACE_APP1, "CONFIGURE_REPORTING: src_ep=%hd (ZED), dst_ep=%hd (ZC), cluster=ZB_ZCL_CLUSTER_ID_ON_OFF",
                  (FMT__H_H, ZED_EP3, ZC_EP3));
        TEST_CONFIGURE_REPORTING_SRV(param, dev_idx, ZC_EP3, ZED_EP3, ZB_ZCL_CLUSTER_ID_ON_OFF,
                                     ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, ZB_ZCL_ATTR_TYPE_BOOL,
                                     1, 3, 0);
        break;

    case 11:
        /* Configure reporting ON_OFF: ZED_EP4 => ZC_EP1 (ZED_EP4 does not support ON_OFF, default response should return "Usuported Cluster") */
        TRACE_MSG(TRACE_APP1, "CONFIGURE_REPORTING: src_ep=%hd (ZED), dst_ep=%hd (ZC), cluster=ZB_ZCL_CLUSTER_ID_ON_OFF",
                  (FMT__H_H, ZED_EP4, ZC_EP1));
        TEST_CONFIGURE_REPORTING_SRV(param, dev_idx, ZED_EP4, ZC_EP4, ZB_ZCL_CLUSTER_ID_ON_OFF,
                                     ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, ZB_ZCL_ATTR_TYPE_BOOL,
                                     1, 3, 0);
        break;
    case 12:
        /* Configure reporting ON_OFF: ZED_EP5 => ZC_EP5 */
        TRACE_MSG(TRACE_APP1, "CONFIGURE_REPORTING: src_ep=%hd (ZED), dst_ep=%hd (ZC), cluster=ZB_ZCL_CLUSTER_ID_ON_OFF",
                  (FMT__H_H, ZED_EP5, ZC_EP5));
        TEST_CONFIGURE_REPORTING_SRV(param, dev_idx, ZC_EP5, ZED_EP5, ZB_ZCL_CLUSTER_ID_ON_OFF,
                                     ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, ZB_ZCL_ATTR_TYPE_BOOL,
                                     1, 3, 0);
        break;

    /* Command steps */

    case 13:
        /* Send On from ZC_EP1 to ZED_EP1 */
        TRACE_MSG(TRACE_APP1, "SEND_ON: src_ep=%hd (ZC), dst_ep=%hd (ZED)", (FMT__H_H, ZC_EP1, ZED_EP1));
        ZB_ZCL_ON_OFF_SEND_ON_REQ(param, g_device_ctx.devices[dev_idx].short_addr,
                                  ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZED_EP1, ZC_EP1, ZB_AF_HA_PROFILE_ID,
                                  ZB_FALSE, NULL);
        break;

    case 14:
        /* Send On from ZC_EP2 to ZED_EP2 */
        TRACE_MSG(TRACE_APP1, "SEND_ON: src_ep=%hd (ZC), dst_ep=%hd (ZED)", (FMT__H_H, ZC_EP2, ZED_EP2));
        ZB_ZCL_ON_OFF_SEND_ON_REQ(param, g_device_ctx.devices[dev_idx].short_addr,
                                  ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZED_EP2, ZC_EP2, ZB_AF_HA_PROFILE_ID,
                                  ZB_FALSE, NULL);
        break;

    case 15:
        /* Send OFF from ZC_EP3 to ZED_EP3 */
        TRACE_MSG(TRACE_APP1, "SEND_OFF: src_ep=%hd (ZC), dst_ep=%hd (ZED)", (FMT__H_H, ZC_EP3, ZED_EP3));
        ZB_ZCL_ON_OFF_SEND_OFF_REQ(param, g_device_ctx.devices[dev_idx].short_addr,
                                   ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZED_EP3, ZC_EP3, ZB_AF_HA_PROFILE_ID,
                                   ZB_FALSE, NULL);
        break;

    case 16:
        /* Send OFF from ZC_EP4 to ZED_EP4 (EP4 does not support ON_OFF cluster, default response must report "unsuported cluster") */
        TRACE_MSG(TRACE_APP1, "SEND_OFF: src_ep=%hd (ZC), dst_ep=%hd (ZED)", (FMT__H_H, ZC_EP4, ZED_EP4));
        ZB_ZCL_ON_OFF_SEND_OFF_REQ(param, g_device_ctx.devices[dev_idx].short_addr,
                                   ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZED_EP4, ZC_EP4, ZB_AF_HA_PROFILE_ID,
                                   ZB_FALSE, NULL);
        break;
    case 17:
        /* Send OFF to Broadcast EP from ZC_EP1 (Default response must be Success for all EPs that have ON_OFF on ZED, (all, except EP4))*/
        TRACE_MSG(TRACE_APP1, "SEND_OFF: src_ep=%hd (ZC), dst_ep=%hd (ZED)", (FMT__H_H, ZC_EP1,
                  ZB_APS_BROADCAST_ENDPOINT_NUMBER));
        ZB_ZCL_ON_OFF_SEND_OFF_REQ(param,  g_device_ctx.devices[dev_idx].short_addr,
                                   ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZB_APS_BROADCAST_ENDPOINT_NUMBER,
                                   ZC_EP1, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
        break;
    case 18:
        /* Send OFF to an EP that does not support ON_OFF cluster (since this is sent unicast, default response must report "Unuported Cluster")*/
        TRACE_MSG(TRACE_APP1, "SEND_OFF: src_ep=%hd (ZC), dst_ep=%hd (ZED)", (FMT__H_H, ZC_EP3,
                  ZED_EP4));
        ZB_ZCL_ON_OFF_SEND_OFF_REQ(param,  g_device_ctx.devices[dev_idx].short_addr,
                                   ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZED_EP4,
                                   ZC_EP3, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
        break;

    case 19:
        /* Send Move to Level from ZC_EP3 to broadcast endpoint */
        TRACE_MSG(TRACE_APP1, "SEND_MOVE_TO_LEVEL: src_ep=%hd (ZC), dst_ep=%hd (ZED)", (FMT__H_H, ZC_EP3, ZB_APS_BROADCAST_ENDPOINT_NUMBER));
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_TO_LEVEL_REQ(param, g_device_ctx.devices[dev_idx].short_addr,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZB_APS_BROADCAST_ENDPOINT_NUMBER, ZC_EP1, ZB_AF_HA_PROFILE_ID,
                ZB_FALSE, NULL, 0x10, 0x10);

        break;
    }

    test_step++;

    if (test_step < 20)
    {
        ZB_SCHEDULE_APP_ALARM(schedule_test_loop, dev_idx, 5 * ZB_TIME_ONE_SECOND);
    }

    TRACE_MSG(TRACE_APP1, "<< test_loop", (FMT__0));
}

void schedule_test_loop(zb_uint8_t dev_idx)
{
    zb_buf_get_out_delayed_ext(test_loop, dev_idx, 0);
}


void report_attribute_cb(zb_uint16_t addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                         zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value)
{
    ZVUNUSED(ep);
    ZVUNUSED(attr_type);
    ZVUNUSED(value);

    TRACE_MSG(TRACE_APP1, ">> report_attribute_cb", (FMT__0));
    TRACE_MSG(TRACE_APP1, ">> ATRR_REPORT: addr=%d, ep=%hd, cluster=0x%x, attr=%d, value=%d, attr_type=%x",
              (FMT__D_H_D_D_D_D, addr, ep, cluster_id, attr_id, *value, attr_type));

    TRACE_MSG(TRACE_APP1, "<< report_attribute_cb", (FMT__0));
}

#ifdef MUTI_EP_ZED_DISCOVERY

void disc_attr_resp_handler(zb_bufid_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_disc_attr_info_t *disc_attr_info;
    zb_uint8_t complete;
    multi_ep_zc_device_cluster_t *ep_cluster = NULL;
    multi_ep_zc_device_ep_t *ep;
    zb_uint8_t dev_idx = multi_ep_zc_get_dev_index_by_short_addr(ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr);
    zb_uint8_t ep_idx = multi_ep_zc_get_ep_idx_by_short_addr_and_ep_id(ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint);

    if (dev_idx != MULTI_EP_ZC_INVALID_DEV_INDEX && ep_idx != MULTI_EP_ZC_INVALID_EP_INDEX)
    {
        ep = &g_device_ctx.devices[dev_idx].endpoints[ep_idx];

        for (int i = 0; i < (ep->num_in_clusters + ep->num_out_clusters); i++)
        {
            if ( ep->ep_cluster[i].cluster_id == cmd_info->cluster_id)
            {
                ep_cluster = &ep->ep_cluster[i];
                break;
            }
        }

        if (ep_cluster)
        {
            TRACE_MSG(TRACE_APP1, ">> disc_attr_resp_handler", (FMT__0));

            ZB_ZCL_GENERAL_GET_COMPLETE_DISC_RES(param, complete);
            ZVUNUSED(complete);

            ZB_ZCL_GENERAL_GET_NEXT_DISC_ATTR_RES(param, disc_attr_info);
            while (disc_attr_info)
            {
                TRACE_MSG(TRACE_APP1, "Id: 0x%x - Data Type: 0x%x", (FMT__D_H, disc_attr_info->attr_id, disc_attr_info->data_type));

                if (ep_cluster->num_attrs < ZB_ARRAY_SIZE(ep_cluster->attr))
                {
                    ep_cluster->attr[ep_cluster->num_attrs].attr_id = disc_attr_info->attr_id;
                    ep_cluster->attr[ep_cluster->num_attrs].attr_type = disc_attr_info->data_type;
                    ep_cluster->num_attrs++;
                }
                ZB_ZCL_GENERAL_GET_NEXT_DISC_ATTR_RES(param, disc_attr_info);
            }

            ep_cluster->attrs_state = KNOWN_ATTRS;

            TRACE_MSG(TRACE_APP1, "<< disc_attr_resp_handler", (FMT__0));
        }
    }
}

static void send_attr_disc_req(zb_bufid_t param, zb_uint16_t dev_idx)
{
    zb_uint8_t ep_idx;
    zb_uint8_t ep_cluster_idx;
    zb_bool_t found_cluster_ep = 0;
    multi_ep_zc_device_params_t *dev = &g_device_ctx.devices[dev_idx];


    /* Cluster is not known, search in all EPs of the device for clusters without known attributes */
    for (int i = 0; i < dev->num_ep; i++ )
    {
        for ( int j = 0; j < (dev->endpoints[i].num_in_clusters + dev->endpoints[i].num_out_clusters); j++)
        {
            /* Break if a cluster with no know attributes is found */
            if (dev->endpoints[i].ep_cluster[j].attrs_state == NO_ATTRS_INFO)
            {
                ep_idx = i;
                ep_cluster_idx = j;
                found_cluster_ep = 1;
                break;
            }

        }
        if (found_cluster_ep)
        {
            break;
        }

    }

    if (found_cluster_ep)
    {
        TRACE_MSG(TRACE_APP1, ">> send_attr_disc_req ", (FMT__0));

        dev->endpoints[ep_idx].ep_cluster[ep_cluster_idx].attrs_state = REQUESTED_ATTRS_INFO;

        ZB_ZCL_GENERAL_DISC_READ_ATTR_REQ(param, ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                          0, /* start attribute id */
                                          0xff, /* maximum attribute id-s */
                                          dev->short_addr,
                                          ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                          dev->endpoints[ep_idx].ep_id,
                                          MULTI_EP_ZC_ENDPOINT_EP1, /*source EP*/
                                          dev->endpoints[ep_idx].profile_id,
                                          dev->endpoints[ep_idx].ep_cluster[ep_cluster_idx].cluster_id,
                                          attr_disc_cb);

        TRACE_MSG(TRACE_APP1, "<< send_attr_disc_req ", (FMT__0));
    }
}


void schedule_send_attr_disc_req(zb_uint8_t dev_idx)
{
    zb_buf_get_out_delayed_ext(send_attr_disc_req, dev_idx, 0);
}

static void attr_disc_cb(zb_uint8_t param)
{
    zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);
    zb_uint8_t dev_idx = MULTI_EP_ZC_INVALID_DEV_INDEX;

    TRACE_MSG(TRACE_APP2, ">> attr_disc_cb param %hd status %hd", (FMT__H_H, param, cmd_send_status->status));

    if (cmd_send_status->dst_addr.addr_type == ZB_ZCL_ADDR_TYPE_SHORT)
    {
        dev_idx = multi_ep_zc_get_dev_index_by_short_addr(cmd_send_status->dst_addr.u.short_addr);
    }

    if (dev_idx != MULTI_EP_ZC_INVALID_DEV_INDEX)
    {
        ZB_SCHEDULE_APP_ALARM(schedule_send_attr_disc_req, dev_idx, ZB_TIME_ONE_SECOND);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_APP2, "<< attr_disc_cb", (FMT__0));
}

static void simple_desc_callback(zb_bufid_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_simple_desc_resp_t *resp = (zb_zdo_simple_desc_resp_t *)(zdp_cmd);
    zb_uint_t i;

    zb_uint8_t dev_idx = multi_ep_zc_get_dev_index_by_short_addr(resp->hdr.nwk_addr);
    zb_uint8_t ep_idx = multi_ep_zc_get_ep_idx_by_short_addr_and_ep_id(resp->hdr.nwk_addr, resp->simple_desc.endpoint);

    TRACE_MSG(TRACE_APP1, ">> simple_desc_callback: status %hd, addr 0x%x",
              (FMT__H, resp->hdr.status, resp->hdr.nwk_addr));

    if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_APP1, "Error incorrect status", (FMT__0));
    }
    else
    {
        /* Check if the cluster array has space for the received cluster information */
        if ( ZB_ARRAY_SIZE(g_device_ctx.devices[dev_idx].endpoints[ep_idx].ep_cluster) <
                (resp->simple_desc.app_input_cluster_count + resp->simple_desc.app_output_cluster_count))
        {
            TRACE_MSG(TRACE_APP1, "No memory to store all Clusters of this EP", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "simple_desc_resp: ep %hd, app prof %d, dev id %d, dev ver %hd, input count 0x%hx, output count 0x%hx",
                      (FMT__H_D_D_H_H_H, resp->simple_desc.endpoint, resp->simple_desc.app_profile_id,
                       resp->simple_desc.app_device_id, resp->simple_desc.app_device_version,
                       resp->simple_desc.app_input_cluster_count, resp->simple_desc.app_output_cluster_count));

            TRACE_MSG(TRACE_APP1, "simple_desc_resp: clusters:", (FMT__0));

            /* Fill EP information relative to clusters*/
            g_device_ctx.devices[dev_idx].endpoints[ep_idx].clusters_state = KNOWN_CLUSTER;
            g_device_ctx.devices[dev_idx].endpoints[ep_idx].profile_id = resp->simple_desc.app_profile_id;
            g_device_ctx.devices[dev_idx].endpoints[ep_idx].num_in_clusters = resp->simple_desc.app_input_cluster_count;
            g_device_ctx.devices[dev_idx].endpoints[ep_idx].num_out_clusters = resp->simple_desc.app_output_cluster_count;

            /* Add the received clusters to cluster array*/
            for (i = 0; i < (resp->simple_desc.app_input_cluster_count + resp->simple_desc.app_output_cluster_count); i++)
            {
                g_device_ctx.devices[dev_idx].endpoints[ep_idx].ep_cluster[i].cluster_id = *(resp->simple_desc.app_cluster_list + i);
                TRACE_MSG(TRACE_APP1, " 0x%hx", (FMT__H, *(resp->simple_desc.app_cluster_list + i)));
            }

            /* For each cluster request info of their attributes */
            ZB_SCHEDULE_APP_ALARM(schedule_send_attr_disc_req, dev_idx, 2 * ZB_TIME_ONE_SECOND);
        }
    }

    TRACE_MSG(TRACE_APP1, "<< simple_desc_callback", (FMT__0));

    zb_buf_free(param);
}

/* Send_simple_desc_req */
static void send_simple_desc_req(zb_bufid_t param, zb_uint16_t dev_idx)
{
    zb_zdo_simple_desc_req_t *req;

    if (!param)
    {
        zb_buf_get_out_delayed_ext(send_simple_desc_req, dev_idx, 0);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, ">> send_simple_desc_req", (FMT__0));

        req = zb_buf_initial_alloc(param, sizeof(zb_zdo_simple_desc_req_t));
        req->nwk_addr = g_device_ctx.devices[dev_idx].short_addr;

        for (int i = 0; i < g_device_ctx.devices[dev_idx].num_ep; i++)
        {
            if (g_device_ctx.devices[dev_idx].endpoints[i].clusters_state ==  NO_CLUSTER_INFO)
            {
                g_device_ctx.devices[dev_idx].endpoints[i].clusters_state = REQUESTED_CLUSTER_INFO;
                req->endpoint = g_device_ctx.devices[dev_idx].endpoints[i].ep_id;
                break;
            }
        }
        zb_zdo_simple_desc_req(param, simple_desc_callback);

        TRACE_MSG(TRACE_APP1, "<< send_simple_desc_req", (FMT__0));
    }
}

void schedule_send_simple_desc_req (zb_uint8_t dev_idx)
{
    zb_buf_get_out_delayed_ext(send_simple_desc_req, dev_idx, 0);
}


/* active_ep_req callback */
void active_ep_callback(zb_bufid_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_ep_resp_t *resp = (zb_zdo_ep_resp_t *)zdp_cmd;
    zb_uint8_t *ep_list = zdp_cmd + sizeof(zb_zdo_ep_resp_t);

    zb_uint8_t dev_idx = multi_ep_zc_get_dev_index_by_short_addr(resp->nwk_addr);

    TRACE_MSG(TRACE_APS1, "active_ep_resp: status %hd, addr 0x%x",
              (FMT__H, resp->status, resp->nwk_addr));

    if (dev_idx != MULTI_EP_ZC_INVALID_DEV_INDEX)
    {
        if (resp->status != ZB_ZDP_STATUS_SUCCESS)
        {
            TRACE_MSG(TRACE_APS1, "active_ep_resp: Error incorrect status", (FMT__0));
        }
        else
        {
            /*Ensure the endpoints array has space for the Eps received */
            if (ZB_ARRAY_SIZE(g_device_ctx.devices[dev_idx].endpoints) < resp->ep_count)
            {
                TRACE_MSG(TRACE_APS1, "No memory to store all EndPoints", (FMT__0));
            }
            else
            {
                g_device_ctx.devices[dev_idx].num_ep = resp->ep_count;
                TRACE_MSG(TRACE_APS1, "active_ep_resp: ep count %hd, ep numbers:", (FMT__H, resp->ep_count));
                /* Add all received EPs to the EP array */
                for (int i = 0; i < resp->ep_count; i++)
                {
                    g_device_ctx.devices[dev_idx].endpoints[i].ep_id = *(ep_list + i);
                    TRACE_MSG(TRACE_APS1, "active_ep_resp: ep %hd", (FMT__H_H, *(ep_list + i)));
                    /*Send a simple_dec_req per active EP received*/
                    ZB_SCHEDULE_APP_ALARM(schedule_send_simple_desc_req, dev_idx, 2 * ZB_TIME_ONE_SECOND);

                }
            }
        }
    }
    else
    {
        /* Received active_ep_rsp from unknown device */
    }

    zb_buf_free(param);

}

/* Send active_ep_req */
void send_active_ep_req(zb_bufid_t param, zb_uint16_t dev_idx)
{
    if (!param)
    {
        zb_buf_get_out_delayed_ext(send_active_ep_req, dev_idx, 0);
    }
    else
    {
        zb_zdo_active_ep_req_t *req;
        req = zb_buf_initial_alloc(param, sizeof(zb_zdo_active_ep_req_t));

        req->nwk_addr = g_device_ctx.devices[dev_idx].short_addr;
        zb_zdo_active_ep_req(param, active_ep_callback);
    }
}

void schedule_send_active_ep_req (zb_uint8_t dev_idx)
{
    zb_buf_get_out_delayed_ext(send_active_ep_req, dev_idx, 0);
}
#endif /* MUTI_EP_ZED_DISCOVERY */

/* Callback which will be called when associated a device. */
zb_uint16_t multi_ep_zc_associate_cb(zb_uint16_t short_addr)
{
    zb_uint8_t dev_idx = multi_ep_zc_get_dev_index_by_short_addr(short_addr);

    TRACE_MSG(TRACE_APP1, ">> multi_ep_zc_associate_cb short_addr 0x%x", (FMT__D, short_addr));

    if (dev_idx != MULTI_EP_ZC_INVALID_DEV_INDEX)
    {
        TRACE_MSG(TRACE_APP1, "It is known device, but it attempts to associate, strange...", (FMT__0));
        ZB_SCHEDULE_APP_ALARM(multi_ep_zc_remove_and_rejoin_device_delayed, dev_idx, 5 * ZB_TIME_ONE_SECOND);
    }
    else
    {
        /* Ok, device is unknown, add to dev list. */
        dev_idx = multi_ep_zc_get_dev_index_by_state(NO_DEVICE);
        if (dev_idx != MULTI_EP_ZC_INVALID_DEV_INDEX)
        {
            TRACE_MSG(TRACE_APP1, "Somebody associated! Start searching onoff device.", (FMT__0));

            g_device_ctx.devices[dev_idx].short_addr = short_addr;
            g_device_ctx.devices[dev_idx].dev_state = IEEE_ADDR_DISCOVERY;

            ZB_SCHEDULE_APP_CALLBACK2(device_ieee_addr_req, 0, dev_idx);

#ifdef MUTI_EP_ZED_DISCOVERY
            ZB_SCHEDULE_APP_ALARM(schedule_send_active_ep_req, dev_idx, 1 * ZB_TIME_ONE_SECOND);
#endif
        }
    }

    TRACE_MSG(TRACE_APP1, "<< multi_ep_zc_associate_cb", (FMT__0));
    return 0;
}

/* Callback which will be called on incoming Device Announce packet. */
void multi_ep_zc_dev_annce_cb(zb_uint16_t short_addr)
{
    zb_uint8_t idx = multi_ep_zc_get_dev_index_by_short_addr(short_addr);
    if (idx != MULTI_EP_ZC_INVALID_DEV_INDEX)
    {
        if (g_device_ctx.devices[idx].dev_state == COMPLETED ||
                g_device_ctx.devices[idx].dev_state == COMPLETED_NO_TOGGLE)
        {
            /* If it is known device - restart communications with it. */
            TRACE_MSG(TRACE_APP1, "Restart communication with device 0x%x (idx %hd)", (FMT__D_H, short_addr, idx));
        }
    }
    else
    {
        /* Associated a new device */
        multi_ep_zc_associate_cb(short_addr);
    }
}

/* Callback which will be called on incoming nwk Leave packet. */
void multi_ep_zc_leave_indication(zb_ieee_addr_t dev_addr)
{
    zb_uint8_t dev_idx;

    TRACE_MSG(TRACE_APP1, "> multi_ep_zc_leave_indication device_addr ieee" TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(dev_addr)));

    dev_idx = multi_ep_zc_get_dev_index_by_ieee_addr(dev_addr);

    if (dev_idx != MULTI_EP_ZC_INVALID_DEV_INDEX)
    {
        /* It is the device which we controlled before. Remove it from the dev list. */
        ZB_SCHEDULE_APP_ALARM_CANCEL(multi_ep_zc_remove_device_delayed, dev_idx);
        g_device_ctx.devices[dev_idx].dev_state = NO_DEVICE;

    }

    TRACE_MSG(TRACE_APP1, "< multi_ep_zc_leave_indication", (FMT__0));
}

/* Callback which will be called on incoming ZCL packet. */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_bufid_t zcl_cmd_buf = param;
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_uint8_t cmd_processed = 0;
    zb_uint16_t dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
    zb_uint8_t dst_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;

    TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler", (FMT__0));

    TRACE_MSG(TRACE_APP1, "payload size: %i", (FMT__D, zb_buf_len(zcl_cmd_buf)));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        switch ( cmd_info->cmd_id)
        {
        case ZB_ZCL_CMD_DISC_ATTRIB_RESP:
#ifdef MULTI_EP_ZED_DISCOVERY
            disc_attr_resp_handler(zcl_cmd_buf, cmd_info);
#endif
            break;

        case ZB_ZCL_CMD_REPORT_ATTRIB:
            TRACE_MSG(TRACE_APP1, "Got reporting from cluster 0x%04x, dst_addr=%hd, dst_ep=%hd",
                      (FMT__D_H_H, cmd_info->cluster_id, dst_addr, dst_ep));
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
            break;
        }

    }

    TRACE_MSG(TRACE_APP1, "< zcl_specific_cluster_cmd_handler", (FMT__0));
    return cmd_processed;
}

void device_ieee_addr_req(zb_uint8_t param, zb_uint16_t dev_idx)
{
    zb_bufid_t  buf = param;
    zb_zdo_ieee_addr_req_param_t *req_param;

    if (!param)
    {
        zb_buf_get_out_delayed_ext(device_ieee_addr_req, dev_idx, 0);
    }
    else
    {
        if (dev_idx != MULTI_EP_ZC_INVALID_DEV_INDEX)
        {
            req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

            req_param->nwk_addr = g_device_ctx.devices[dev_idx].short_addr;
            req_param->dst_addr = req_param->nwk_addr;
            req_param->start_index = 0;
            req_param->request_type = 0;
            zb_zdo_ieee_addr_req(buf, device_ieee_addr_req_cb);
        }
        else
        {
            TRACE_MSG(TRACE_APP2, "No devices in discovery state were found!", (FMT__0));
            zb_buf_free(buf);
        }
    }
}

void device_ieee_addr_req_cb(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_zdo_nwk_addr_resp_head_t *resp;
    zb_ieee_addr_t ieee_addr;
    zb_uint16_t nwk_addr;
    zb_uint8_t dev_idx;

    TRACE_MSG(TRACE_APP2, ">> device_ieee_addr_req_cb param %hd", (FMT__H, param));

    resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(buf);
    TRACE_MSG(TRACE_APP2, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ZB_LETOH64(ieee_addr, resp->ieee_addr);
        ZB_LETOH16(&nwk_addr, &resp->nwk_addr);

        dev_idx = multi_ep_zc_get_dev_index_by_short_addr(nwk_addr);

        if (dev_idx != MULTI_EP_ZC_INVALID_DEV_INDEX)
        {
            ZB_MEMCPY(g_device_ctx.devices[dev_idx].ieee_addr, ieee_addr, sizeof(zb_ieee_addr_t));
            g_device_ctx.devices[dev_idx].dev_state = COMPLETED;

            ZB_SCHEDULE_APP_ALARM(schedule_test_loop, dev_idx, 1 * ZB_TIME_ONE_SECOND);

        }
        else
        {
            TRACE_MSG(TRACE_APP2, "This resp is not for our device", (FMT__0));
        }
    }

    if (param)
    {
        zb_buf_free(buf);
    }

    TRACE_MSG(TRACE_APP2, "<< device_ieee_addr_req_cb", (FMT__0));
}

zb_uint8_t multi_ep_zc_get_dev_index_by_state(zb_uint8_t dev_state)
{
    zb_uint8_t i;
    for (i = 0; i < ZB_ARRAY_SIZE(g_device_ctx.devices); ++i)
    {
        if (g_device_ctx.devices[i].dev_state == dev_state)
        {
            return i;
        }
    }

    return MULTI_EP_ZC_INVALID_DEV_INDEX;
}

zb_uint8_t multi_ep_zc_get_dev_index_by_short_addr(zb_uint16_t short_addr)
{
    zb_uint8_t i;
    for (i = 0; i < ZB_ARRAY_SIZE(g_device_ctx.devices); ++i)
    {
        if (g_device_ctx.devices[i].short_addr == short_addr)
        {
            return i;
        }
    }

    return MULTI_EP_ZC_INVALID_DEV_INDEX;
}

zb_uint8_t multi_ep_zc_get_ep_idx_by_short_addr_and_ep_id(zb_uint16_t short_addr, zb_uint8_t ep_id)
{
    zb_uint8_t i;
    zb_uint8_t dev_idx = multi_ep_zc_get_dev_index_by_short_addr(short_addr);

    for ( i = 0; i < g_device_ctx.devices[dev_idx].num_ep; i++)
    {
        if ( g_device_ctx.devices[dev_idx].endpoints[i].ep_id == ep_id)
        {
            return i;
        }
    }

    return MULTI_EP_ZC_INVALID_EP_INDEX;
}

zb_uint8_t multi_ep_zc_get_dev_index_by_ieee_addr(zb_ieee_addr_t ieee_addr)
{
    zb_uint8_t i;
    for (i = 0; i < ZB_ARRAY_SIZE(g_device_ctx.devices); ++i)
    {
        if (ZB_IEEE_ADDR_CMP(g_device_ctx.devices[i].ieee_addr, ieee_addr))
        {
            return i;
        }
    }

    return MULTI_EP_ZC_INVALID_DEV_INDEX;
}

/* [address_by_short] */
void multi_ep_zc_send_leave_req(zb_uint8_t param, zb_uint16_t short_addr, zb_bool_t rejoin_flag)
{
    zb_bufid_t buf = param;
    zb_zdo_mgmt_leave_param_t *req_param;
    zb_address_ieee_ref_t addr_ref;

    if (zb_address_by_short(short_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
        req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);
        ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

        req_param->dst_addr = short_addr;
        req_param->rejoin = (rejoin_flag ? 1 : 0);
        zdo_mgmt_leave_req(param, NULL);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "tried to remove 0x%xd, but device is already left", (FMT__D, short_addr));
        zb_buf_free(buf);
    }
}
/* [address_by_short] */

void multi_ep_zc_leave_device(zb_uint8_t param, zb_uint16_t short_addr)
{
    TRACE_MSG(TRACE_APP1, ">> multi_ep_zc_leave_device param %hd short_addr %d", (FMT__H_D, param, short_addr));

    if (!param)
    {
        zb_buf_get_out_delayed_ext(multi_ep_zc_leave_device, short_addr, 0);
    }
    else
    {
        multi_ep_zc_send_leave_req(param, short_addr, ZB_FALSE);
    }

    TRACE_MSG(TRACE_APP1, "<< multi_ep_zc_leave_device", (FMT__0));
}

void multi_ep_zc_leave_and_rejoin_device(zb_uint8_t param, zb_uint16_t short_addr)
{
    TRACE_MSG(TRACE_APP1, ">> multi_ep_zc_leave_and_rejoin_device param %hd short_addr %d", (FMT__H_D, param, short_addr));

    if (!param)
    {
        zb_buf_get_out_delayed_ext(multi_ep_zc_leave_and_rejoin_device, short_addr, 0);
    }
    else
    {
        multi_ep_zc_send_leave_req(param, short_addr, ZB_TRUE);
    }

    TRACE_MSG(TRACE_APP1, "<< multi_ep_zc_leave_and_rejoin_device", (FMT__0));
}

void multi_ep_zc_remove_device(zb_uint8_t idx)
{
    ZB_BZERO(&g_device_ctx.devices[idx], sizeof(multi_ep_zc_device_params_t));
}

void multi_ep_zc_remove_and_rejoin_device_delayed(zb_uint8_t idx)
{
    TRACE_MSG(TRACE_APP1, "multi_ep_zc_remove_and_rejoin_device_delayed: short_addr 0x%x", (FMT__D, g_device_ctx.devices[idx].short_addr));

    zb_buf_get_out_delayed_ext(multi_ep_zc_leave_and_rejoin_device, g_device_ctx.devices[idx].short_addr, 0);
    multi_ep_zc_remove_device(idx);
}

void multi_ep_zc_remove_device_delayed(zb_uint8_t idx)
{
    TRACE_MSG(TRACE_APP1, "multi_ep_zc_remove_device_delayed: short_addr 0x%x", (FMT__D, g_device_ctx.devices[idx].short_addr));

    zb_buf_get_out_delayed_ext(multi_ep_zc_leave_device, g_device_ctx.devices[idx].short_addr, 0);
    multi_ep_zc_remove_device(idx);
}

/* Callback which will be called on startup procedure complete (successfull or not). */
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
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APP1, "Successfull steering", (FMT__0));
            break;

        case ZB_ZDO_SIGNAL_LEAVE_INDICATION:
        {
            zb_zdo_signal_leave_indication_params_t *leave_ind_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_indication_params_t);
            if (!leave_ind_params->rejoin)
            {
                multi_ep_zc_leave_indication(leave_ind_params->device_addr);
            }
        }
        break;

        case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        {
            zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
            multi_ep_zc_dev_annce_cb(dev_annce_params->device_short_addr);
        }
        break;

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
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
        zb_buf_free(param);
    }
}
