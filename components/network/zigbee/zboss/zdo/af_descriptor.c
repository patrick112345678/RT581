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
/* PURPOSE: AF layer
*/

#define ZB_TRACE_FILE_ID 3216
#include "zb_common.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_nwk_mm.h"
#include "zdo_wwah_stubs.h"
/*! \addtogroup zb_af */
/*! @{ */

void zb_set_node_descriptor(zb_logical_type_t device_type, zb_bool_t power_src, zb_bool_t rx_on_when_idle, zb_bool_t alloc_addr)
{
    zb_uint16_t manufacturer_code = ZB_DEFAULT_MANUFACTURER_CODE;
    zb_mac_capability_info_t mac_cap = 0;
    zb_uint8_t freq_band;

    freq_band = zb_nwk_mm_get_freq_band();

    /* R21, 2.3.2.3.6  MAC Capability Flags Field */
    /* The alternate PAN coordinator sub-field is one bit in length and shall be set to 0 in R21 implementation */
    /* The device type sub-field is one bit in length and shall be set to 1 if this node is a full function device */
    /* (FFD). Otherwise, the device type sub-field shall be set to 0, indicating a reduced function device (RFD). */
    ZB_MAC_CAP_SET_DEVICE_TYPE(mac_cap, ZB_B2U(device_type == ZB_COORDINATOR || device_type == ZB_ROUTER));
    ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(mac_cap, ZB_B2U(rx_on_when_idle));

    ZB_MAC_CAP_SET_POWER_SOURCE(mac_cap, ZB_B2U(power_src));
    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(mac_cap, ZB_B2U(alloc_addr));

    /* gcc is unhappy with ifdefs inside macro call, so let's put ifdef there. */
#ifdef APS_FRAGMENTATION
#define MAX_FRAG_LEN ZB_ASDU_MAX_FRAG_LEN
#else
    /* 0 - is described in certification tests */
#define MAX_FRAG_LEN 0
#endif

    ZB_SET_NODE_DESCRIPTOR(
        device_type,
        freq_band,
        mac_cap,
        manufacturer_code,
        ZB_NSDU_MAX_LEN,
        MAX_FRAG_LEN,
        device_type == ZB_COORDINATOR ? ZB_PRIMARY_TRUST_CENTER | ZB_NETWORK_MANAGER : 0U,
        MAX_FRAG_LEN,
        0U);

#ifdef ZB_CERTIFICATION_HACKS
    /**
      Clear Stack Revision field for legacy device.
      Used in test tp_r21_bv-10 for simulating legacy ZC.
    */
    if (ZB_CERT_HACKS().report_legacy_stack_revision_in_node_descr)
    {
        //    ZB_ZDO_NODE_DESC()->server_mask &= (zb_uint16_t) 0x147f;
        /* bits 9-15 - stack revision. 0 for pre-r21 */
        ZB_ZDO_NODE_DESC()->server_mask &= (zb_uint16_t) 0x1ff;
    }
#endif /* ZB_CERTIFICATION_HACKS */

    TRACE_MSG(TRACE_ZDO3, "node desc, server_mask %x mac cap 0x%x",
              (FMT__D_H, ZB_ZDO_NODE_DESC()->server_mask, ZB_ZDO_NODE_DESC()->mac_capability_flags));

}

void zdo_set_node_descriptor_manufacturer_code(zb_uint16_t manuf_code)
{
    ZB_ZDO_NODE_DESC()->manufacturer_code = manuf_code;
}

#ifdef ZB_ROUTER
/*
  Set node descriptor for FFD
  param device_type - FFD device type ZB_COORDINATOR or ZB_ROUTER
*/
void zb_set_ffd_node_descriptor(zb_logical_type_t device_type)
{
    TRACE_MSG(TRACE_ZDO3, "zb_set_ffd_node_descriptor", (FMT__0));
    zb_set_node_descriptor(device_type, ZB_FALSE, ZB_TRUE, device_type != ZB_COORDINATOR);
}
#endif  /* ZB_ROUTER */

#ifndef ZB_COORDINATOR_ONLY
/*
  Set node descriptor for end device
  param power_src - 1 if the current power source is mains power, 0 otherwise
  param rx_on_when_idle - receiver on when idle sub-field
  param alloc_addr - allocate address sub-field
*/
void zb_set_ed_node_descriptor(zb_bool_t power_src, zb_bool_t rx_on_when_idle, zb_bool_t alloc_addr)
{
    TRACE_MSG(TRACE_ZDO3, "zb_set_ed_node_descriptor", (FMT__0));
    zb_set_node_descriptor(ZB_END_DEVICE, power_src, rx_on_when_idle, alloc_addr);
}
#endif

/*
  Set node power descriptor
  param current_power_mode - current power mode
  param available_power_sources - available power sources
  param current_power_source - current power source
  param current_power_source_level - current power source level
*/
void zb_set_node_power_descriptor(zb_current_power_mode_t current_power_mode, zb_uint8_t available_power_sources,
                                  zb_uint8_t current_power_source, zb_power_source_level_t current_power_source_level)
{
    ZB_SET_POWER_DESC_CUR_POWER_MODE(ZB_ZDO_NODE_POWER_DESC(), current_power_mode);
    ZB_SET_POWER_DESC_AVAIL_POWER_SOURCES(ZB_ZDO_NODE_POWER_DESC(), available_power_sources);
    ZB_SET_POWER_DESC_CUR_POWER_SOURCE(ZB_ZDO_NODE_POWER_DESC(), current_power_source);
    ZB_SET_POWER_DESC_CUR_POWER_SOURCE_LEVEL(ZB_ZDO_NODE_POWER_DESC(), current_power_source_level);
    ZB_ZDO_SEND_POWER_DESCRIPTOR_CHANGE();
}

/*
  Set simple descriptor parameters
  param simple_desc - pointer to simple descriptor
  param endpoint - Endpoint
  param app_profile_id - Application profile identifier
  param app_device_id - Application device identifier
  param app_device_version - Application device version
  param app_input_cluster_count - Application input cluster count
  param app_output_cluster_count - Application output cluster count
*/
void zb_set_simple_descriptor(zb_af_simple_desc_1_1_t *simple_desc,
                              zb_uint8_t  endpoint,
                              zb_uint16_t app_profile_id,
                              zb_uint16_t app_device_id,
                              zb_bitfield_t app_device_version,
                              zb_uint8_t app_input_cluster_count,
                              zb_uint8_t app_output_cluster_count)
{
    simple_desc->endpoint = endpoint;
    simple_desc->app_profile_id = app_profile_id;
    simple_desc->app_device_id = app_device_id;
    simple_desc->app_device_version = app_device_version;
    simple_desc->app_input_cluster_count = app_input_cluster_count;
    simple_desc->app_output_cluster_count = app_output_cluster_count;
    simple_desc->reserved = 0;
}

/*
  Set input cluster item
  param simple_desc - pointer to simple descriptor
  param cluster_number - cluster item number
  param cluster_id - cluster id
*/
void zb_set_input_cluster_id(zb_af_simple_desc_1_1_t *simple_desc, zb_uint8_t cluster_number, zb_uint16_t cluster_id)
{
    TRACE_MSG(TRACE_ZDO2, "cluster_id 0x%hx, cluster_number %hu, input count %hd",
              (FMT__H_H_H, cluster_id, cluster_number, simple_desc->app_input_cluster_count));
    ZB_ASSERT(cluster_number < simple_desc->app_input_cluster_count);
    simple_desc->app_cluster_list[cluster_number] = cluster_id;
}

/*
  Set output cluster item
  param simple_desc - pointer to simple descriptor
  param cluster_number - cluster item number
  param cluster_id - cluster id
*/
void zb_set_output_cluster_id(zb_af_simple_desc_1_1_t *simple_desc, zb_uint8_t cluster_number, zb_uint16_t cluster_id)
{
    TRACE_MSG(TRACE_ZDO2, "cluster_id 0x%hx, cluster_number %hd, output count %hd",
              (FMT__H_H_H, cluster_id, cluster_number, simple_desc->app_output_cluster_count));
    ZB_ASSERT(cluster_number < simple_desc->app_output_cluster_count);
    simple_desc->app_cluster_list[simple_desc->app_input_cluster_count + cluster_number] = cluster_id;
}

void zb_set_zdo_descriptor()
{
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,60} */
    zb_af_simple_desc_1_1_t *p_zdo_desc = (zb_af_simple_desc_1_1_t *)ZB_ZDO_SIMPLE_DESC();

    zb_set_simple_descriptor(p_zdo_desc,
                             0 /* endpoint */,                0 /* app_profile_id */,
                             0 /* app_device_id */,           0 /* app_device_version*/,
                             8 /* app_input_cluster_count */, 9 /* app_output_cluster_count */);
    ZB_ZDO_SIMPLE_DESC_NUMBER() = 0;

    {
        static const zb_uint16_t ZB_CODE s_iclids[] =
        {
            ZDO_NWK_ADDR_RESP_CLID,
            ZDO_IEEE_ADDR_RESP_CLID,
            ZDO_NODE_DESC_RESP_CLID,
            ZDO_POWER_DESC_RESP_CLID,
            ZDO_SIMPLE_DESC_RESP_CLID,
            ZDO_ACTIVE_EP_RESP_CLID,
            ZDO_MATCH_DESC_RESP_CLID,
            ZDO_PARENT_ANNCE_RESP_CLID
        };

        static const zb_uint8_t ZB_CODE s_oclids[] =
        {
            ZDO_NWK_ADDR_REQ_CLID,
            ZDO_IEEE_ADDR_REQ_CLID,
            ZDO_NODE_DESC_REQ_CLID,
            ZDO_POWER_DESC_REQ_CLID,
            ZDO_SIMPLE_DESC_REQ_CLID,
            ZDO_ACTIVE_EP_REQ_CLID,
            ZDO_MATCH_DESC_REQ_CLID,
            ZDO_DEVICE_ANNCE_CLID,
            ZDO_PARENT_ANNCE_CLID,
        };

        zb_uint8_t i;

        TRACE_MSG(TRACE_ZDO3, "> zb_set_zdo_descriptor", (FMT__0));
        for (i = 0 ; i < sizeof(s_iclids) / sizeof(s_iclids[0]) ; ++i)
        {
            zb_set_input_cluster_id(p_zdo_desc, i, s_iclids[i]);
        }
        for (i = 0 ; i < sizeof(s_oclids) / sizeof(s_oclids[0]) ; ++i)
        {
            zb_set_output_cluster_id(p_zdo_desc, i, s_oclids[i]);
        }
        TRACE_MSG(TRACE_ZDO3, "< zb_set_zdo_descriptor", (FMT__0));
    }
}

#if defined ZB_ROUTER_ROLE
void zb_set_default_ffd_descriptor_values(zb_logical_type_t device_type)
{
    TRACE_MSG(TRACE_ZDO3, "> zb_set_default_ffd_descriptor_values", (FMT__0));
    zb_set_node_descriptor(device_type,
                           /* Suppose FFD is AC-powered */
                           ZB_TRUE,
                           /* FFD is rx-on-when-idle */
                           ZB_TRUE, device_type != ZB_COORDINATOR);

#ifdef ZB_SET_DEFAULT_POWER_DESCRIPTOR
    zb_set_node_power_descriptor(ZB_POWER_MODE_SYNC_ON_WHEN_IDLE,
                                 ZB_POWER_SRC_CONSTANT | ZB_POWER_SRC_RECHARGEABLE_BATTERY | ZB_POWER_SRC_DISPOSABLE_BATTERY,
                                 ZB_POWER_SRC_CONSTANT | ZB_POWER_SRC_RECHARGEABLE_BATTERY, ZB_POWER_LEVEL_100);
#endif /* ZB_SET_DEFAULT_POWER_DESCRIPTOR */

    TRACE_MSG(TRACE_ZDO3, "< zb_set_default_ffd_descriptor_values", (FMT__0));
}
#endif


#ifndef ZB_COORDINATOR_ONLY
void zb_set_default_ed_descriptor_values()
{
    TRACE_MSG(TRACE_ZDO3, "> zb_set_default_ed_descriptor_values", (FMT__0));
    zb_set_node_descriptor(ZB_END_DEVICE,
                           ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()),
                           ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()),
                           ZB_TRUE);

#ifdef ZB_SET_DEFAULT_POWER_DESCRIPTOR
    zb_set_node_power_descriptor(ZB_POWER_MODE_SYNC_ON_WHEN_IDLE, ZB_POWER_SRC_CONSTANT,
                                 ZB_POWER_SRC_CONSTANT, ZB_POWER_LEVEL_100);
#endif /* ZB_SET_DEFAULT_POWER_DESCRIPTOR */

    TRACE_MSG(TRACE_ZDO3, "< zb_set_default_ed_descriptor_values", (FMT__0));
}
#endif

/*! @brief Adds simple descriptor.
    @param simple_desc - pointer to simple descriptor to add
    @return RET_OK, RET_OVERFLOW
 */
zb_ret_t zb_add_simple_descriptor(zb_af_simple_desc_1_1_t *simple_desc)
{
    zb_ret_t ret = RET_OK;

    if (ZB_ZDO_SIMPLE_DESC_NUMBER() < ZB_MAX_EP_NUMBER)
    {
        ZB_ZDO_SIMPLE_DESC_LIST()[ZB_ZDO_SIMPLE_DESC_NUMBER()++] = simple_desc;

#if TRACE_ENABLED(TRACE_ZDO3)
        {
            zb_uint_t i;
            TRACE_MSG(TRACE_ZDO3, "ZB_ZDO_SIMPLE_DESC_NUMBER %d", (FMT__D, ZB_ZDO_SIMPLE_DESC_NUMBER()));
            for (i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
            {
                zb_af_simple_desc_1_1_t *dsc;
                dsc = ZB_ZDO_SIMPLE_DESC_LIST()[i];
                TRACE_MSG(TRACE_ZDO3, "dsc %p ep %hd prof_id 0x%x dev_id 0x%x dev_ver 0x%hx in_clu_c %hd out_clu_c %hd",
                          (FMT__P_H_D_D_D_H_H, dsc, dsc->endpoint, dsc->app_profile_id, dsc->app_device_id, dsc->app_device_version, dsc->app_input_cluster_count, dsc->app_output_cluster_count));
            }
        }
#endif /* TRACE_ENABLED(TRACE_ZDO3) */
    }
    else
    {
        printf("123");
        TRACE_MSG(TRACE_ERROR, "Error, simple_desc_number overflows %i", (FMT__D, (int)ZB_ZDO_SIMPLE_DESC_NUMBER()));
        ret = RET_OVERFLOW;
    }
    return ret;
}

#if defined ZB_ENABLE_ZCL
/*! @brief Register device context.
    @param device_ctx - pointer to device context to register
 */
void zb_af_register_device_ctx(zb_af_device_ctx_t *device_ctx)
{
    zb_zcl_register_device_ctx(device_ctx);
    {
        zb_ret_t ret;
        zb_uindex_t i;
        zb_uint8_t ep_count = ZCL_CTX().device_ctx->ep_count;

        TRACE_MSG(TRACE_INFO1, "ep_count %hd", (FMT__H, ZCL_CTX().device_ctx->ep_count));
        for (i = 0; i < ep_count; i++)
        {
            ret = zb_add_simple_descriptor(ZCL_CTX().device_ctx->ep_desc_list[i]->simple_desc);
            ZB_ASSERT(ret == RET_OK);
            zb_zcl_init_endpoint(ZCL_CTX().device_ctx->ep_desc_list[i]);
        }
    }
    ZB_ASSERT(zb_zcl_check_cluster_list());
}
#endif /* ZB_ENABLE_ZCL */

/*! @} */
