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
/* PURPOSE: WWAH Parent classification routines
*/

#define ZB_TRACE_FILE_ID 12086

#include "zb_common.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zcl/zb_zcl_wwah.h"
#include "zdo_wwah_parent_classification.h"
#include "zdo_common.h"

#ifdef ZB_PARENT_CLASSIFICATION

void zdo_wwah_parent_classification_set(zb_bool_t enable)
{
  HUBS_CTX().parent_classification_enabled = !!enable;
}

static zb_bool_t zdo_wwah_parent_classification_is_enabled(void)
{
  return HUBS_CTX().parent_classification_enabled;
}

zb_zcl_wwah_parent_priority_t zdo_wwah_get_parent_priority_by_classification_mask(zb_uint8_t mask)
{
  zb_zcl_wwah_parent_priority_t priority;

  TRACE_MSG(
    TRACE_ZDO2,
    ">>zdo_wwah_get_parent_priority_by_classification_mask, mask 0x%hx",
    (FMT__H, mask));

  switch (mask & ZB_ZCL_WWAH_TC_CONNECTIVITY_AND_LONG_UPTIME_MASK)
  {
    case ZB_ZCL_WWAH_NO_TC_CONNECTIVITY_AND_SHORT_UPTIME_MASK:
      priority = ZB_ZCL_WWAH_PARENT_PRIORITY_VERY_LOW;
      break;

    case ZB_ZCL_WWAH_NO_TC_CONNECTIVITY_AND_LONG_UPTIME_MASK:
      priority = ZB_ZCL_WWAH_PARENT_PRIORITY_LOW;
      break;

    case ZB_ZCL_WWAH_TC_CONNECTIVITY_AND_SHORT_UPTIME_MASK:
      priority = ZB_ZCL_WWAH_PARENT_PRIORITY_HIGH;
      break;

    case ZB_ZCL_WWAH_TC_CONNECTIVITY_AND_LONG_UPTIME_MASK:
      priority = ZB_ZCL_WWAH_PARENT_PRIORITY_VERY_HIGH;
      break;

    default:
      priority = ZB_ZCL_WWAH_PARENT_PRIORITY_INVALID;
      break;
  }

  TRACE_MSG(
    TRACE_ZDO2,
    "<<zdo_wwah_get_parent_priority_by_classification_mask, priority %d",
    (FMT__D, priority));

  return priority;
}


zb_bool_t zdo_wwah_compare_neighbors(
  zb_ext_neighbor_tbl_ent_t *nbte_first, zb_ext_neighbor_tbl_ent_t *nbte_second)
{
  zb_bool_t res;

  TRACE_MSG(TRACE_ZDO1, ">> zdo_wwah_compare_neighbors", (FMT__0));

  ZB_ASSERT(nbte_first);
  ZB_ASSERT(nbte_second);

  TRACE_MSG(TRACE_ZDO1, "first_rssi %hd, second_rssi %hd", (FMT__H_H, nbte_first->rssi, nbte_second->rssi));

  if (zdo_wwah_parent_classification_is_enabled())
  {
    zb_uint8_t first_rssi_group = ZDO_WWAH_GET_LINK_QUALITY_GROUP_BY_RSSI(nbte_first->rssi);
    zb_uint8_t second_rssi_group = ZDO_WWAH_GET_LINK_QUALITY_GROUP_BY_RSSI(nbte_second->rssi);
    zb_zcl_wwah_parent_priority_t first_parent_priority =
      zdo_wwah_get_parent_priority_by_classification_mask(nbte_first->u.ext.classification_mask);
    zb_zcl_wwah_parent_priority_t second_parent_priority =
      zdo_wwah_get_parent_priority_by_classification_mask(nbte_second->u.ext.classification_mask);

    TRACE_MSG(TRACE_ZDO1, "first_parent_priority %hd, second_parent_priority %hd",
              (FMT__H_H, first_parent_priority, second_parent_priority));

    if (first_rssi_group == second_rssi_group)
    {
      TRACE_MSG(TRACE_ZDO1, "RSSI groups are the same, choose by Parent Priority", (FMT__0));
      if (first_parent_priority == second_parent_priority)
      {
        TRACE_MSG(TRACE_ZDO1, "Parents Priorities are the same, choose by RSSI", (FMT__0));
        res = (zb_bool_t)(nbte_first->rssi > nbte_second->rssi);
      }
      else
      {
        TRACE_MSG(TRACE_ZDO1, "Parents Priorities are different, choose the best", (FMT__0));
        res = (zb_bool_t)(first_parent_priority > second_parent_priority);
      }
    }
    else
    {
      TRACE_MSG(TRACE_ZDO1, "RSSI groups are different, choose parent from the Good group", (FMT__0));
      res = (zb_bool_t)(first_rssi_group > second_rssi_group);
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "WWAH Parent Classification is disabled! Use only LQI to find the best", (FMT__0));
    res = (zb_bool_t)(nbte_first->lqi > nbte_second->lqi);
  }

  TRACE_MSG(TRACE_ZDO1, "fisrt is better - %hd", (FMT__H, res));
  TRACE_MSG(TRACE_ZDO1, "<< zdo_wwah_compare_neighbors", (FMT__0));

  return res;
}

#endif /* ZB_PARENT_CLASSIFICATION */
