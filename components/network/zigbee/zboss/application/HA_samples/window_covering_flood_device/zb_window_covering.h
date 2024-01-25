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
/* PURPOSE: ZB WINDOW_COVERING device
*/

#ifndef ZB_WINDOW_COVERING_H
#define ZB_WINDOW_COVERING_H 1


/* ******************** ZED ******************** */
#define ZB_HA_DEVICE_VER_WINDOW_COVERING 0  /*!< Window covering device version */

/** @cond internals_doc */
#define ZB_HA_WINDOW_COVERING_IN_CLUSTER_NUM_ZED 6 /*!< Window covering IN (server) clusters number */
#define ZB_HA_WINDOW_COVERING_OUT_CLUSTER_NUM_ZED 0 /*!< Window covering OUT (client) clusters number */

/*!
  @brief Declare cluster list for window covering device
  @param cluster_list_name - cluster list variable name
  @param basic_attr_list - attribute list for Basic cluster
  @param identify_attr_list - attribute list for Identify cluster
  @param groups_attr_list - attribute list for Groups cluster
  @param scenes_attr_list - attribute list for Scenes cluster
  @param window_covering_attr_list - attribute list for Window covering cluster
 */
#define ZB_HA_DECLARE_WINDOW_COVERING_CLUSTER_LIST_ZED(                     \
  cluster_list_name,                                                    \
  basic_attr_list,                                                      \
  identify_attr_list,                                                   \
  groups_attr_list,                                                     \
  scenes_attr_list,                                                     \
  window_covering_attr_list,                                            \
  shade_config_attr_list)                                               \
  zb_zcl_cluster_desc_t cluster_list_name[] =                           \
  {                                                                     \
    ZB_ZCL_CLUSTER_DESC(                                                \
      ZB_ZCL_CLUSTER_ID_IDENTIFY,                                       \
      ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t),             \
      (identify_attr_list),                                             \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                       \
      ZB_ZCL_MANUF_CODE_INVALID                                         \
    ),                                                                  \
    ZB_ZCL_CLUSTER_DESC(                                                \
      ZB_ZCL_CLUSTER_ID_BASIC,                                          \
      ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),                \
      (basic_attr_list),                                                \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                       \
      ZB_ZCL_MANUF_CODE_INVALID                                         \
    ),                                                                  \
    ZB_ZCL_CLUSTER_DESC(                                                \
      ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,                                \
      ZB_ZCL_ARRAY_SIZE(window_covering_attr_list, zb_zcl_attr_t),      \
      (window_covering_attr_list),                                      \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                       \
      ZB_ZCL_MANUF_CODE_INVALID                                         \
    ),                                                                  \
    ZB_ZCL_CLUSTER_DESC(                                                \
      ZB_ZCL_CLUSTER_ID_SCENES,                                         \
      ZB_ZCL_ARRAY_SIZE(scenes_attr_list, zb_zcl_attr_t),               \
      (scenes_attr_list),                                               \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                       \
      ZB_ZCL_MANUF_CODE_INVALID                                         \
    ),                                                                  \
    ZB_ZCL_CLUSTER_DESC(                                                \
      ZB_ZCL_CLUSTER_ID_GROUPS,                                         \
      ZB_ZCL_ARRAY_SIZE(groups_attr_list, zb_zcl_attr_t),               \
      (groups_attr_list),                                               \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                       \
      ZB_ZCL_MANUF_CODE_INVALID                                         \
    ),                                                                  \
    ZB_ZCL_CLUSTER_DESC(                                                \
      ZB_ZCL_CLUSTER_ID_SHADE_CONFIG,                                   \
      ZB_ZCL_ARRAY_SIZE(shade_config_attr_list, zb_zcl_attr_t),  \
      (shade_config_attr_list),                                  \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                \
      ZB_ZCL_MANUF_CODE_INVALID                                  \
    ),                                                           \
  }


/*!
  @brief Declare simple descriptor for window covering device
  @param ep_name - endpoint variable name
  @param ep_id - endpoint ID
  @param in_clust_num - number of supported input clusters
  @param out_clust_num - number of supported output clusters
*/
#define ZB_ZCL_DECLARE_WINDOW_COVERING_SIMPLE_DESC_ZED(ep_name, ep_id, in_clust_num, out_clust_num) \
  ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num);                                          \
  ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name =                   \
  {                                                                                             \
    ep_id,                                                                                      \
    ZB_AF_HA_PROFILE_ID,                                                                        \
    ZB_HA_WINDOW_COVERING_DEVICE_ID,                                                            \
    ZB_HA_DEVICE_VER_WINDOW_COVERING,                                                           \
    0,                                                                                          \
    in_clust_num,                                                                               \
    out_clust_num,                                                                              \
    {                                                                                           \
      ZB_ZCL_CLUSTER_ID_IDENTIFY,                                                               \
      ZB_ZCL_CLUSTER_ID_BASIC,                                                                  \
      ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,                                                        \
      ZB_ZCL_CLUSTER_ID_SCENES,                                                                 \
      ZB_ZCL_CLUSTER_ID_GROUPS,                                                                 \
      ZB_ZCL_CLUSTER_ID_SHADE_CONFIG                                                            \
    }                                                                                           \
  }

/*!
  @brief Declare endpoint for window covering device
  @param ep_name - endpoint variable name
  @param ep_id - endpoint ID
  @param cluster_list - endpoint cluster list
 */
#define ZB_HA_DECLARE_WINDOW_COVERING_EP_ZED(ep_name, ep_id, cluster_list)            \
  ZB_ZCL_DECLARE_WINDOW_COVERING_SIMPLE_DESC(ep_name, ep_id,                      \
    ZB_HA_WINDOW_COVERING_IN_CLUSTER_NUM_ZED, ZB_HA_WINDOW_COVERING_OUT_CLUSTER_NUM_ZED); \
  ZBOSS_DEVICE_DECLARE_REPORTING_CTX(reporting_info## device_ctx_name,            \
                                     ZB_HA_WINDOW_COVERING_REPORT_ATTR_COUNT);    \
  ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID,                \
            0,                    \
            NULL,                                \
    ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list,         \
    (zb_af_simple_desc_1_1_t*)&simple_desc_##ep_name,                             \
    ZB_HA_WINDOW_COVERING_REPORT_ATTR_COUNT, reporting_info## device_ctx_name,    \
    0, NULL) /* No CVC ctx */


/*!
  @brief Declare application's device context for Window Covering device
  @param device_ctx - device context variable
  @param ep_name - endpoint variable name
*/
#define ZB_HA_DECLARE_WINDOW_COVERING_CTX_ZED(device_ctx, ep_name)  \
  ZBOSS_DECLARE_DEVICE_CTX_1_EP(device_ctx, ep_name)

typedef ZB_PACKED_PRE struct bulb_device_nvram_dataset_s
{
    zb_uint8_t onoff_state;
    zb_uint8_t current_level;
    zb_uint8_t reserved[2]; /* Alignment. Reserved for future use */
} ZB_PACKED_STRUCT
bulb_device_nvram_dataset_t;

#endif /* ZB_WINDOW_COVERING_H */

