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
/* PURPOSE: It was zdo_neighbor_utility.c file.
Actually there is nothing ZDO specific, so move it to nwk.
*/


#define ZB_TRACE_FILE_ID 2098
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_hash.h"
#include "zb_nwk.h"
#include "zb_aps.h"

/*! \addtogroup ZB_NWK */
/*! @{ */

zb_uint8_t zb_nwk_get_nbr_rel_by_short(zb_uint16_t addr)
{
  zb_uint8_t ret = ZB_NWK_NEIGHBOR_ERROR_VALUE;
  zb_ret_t status;
  zb_neighbor_tbl_ent_t *nbt;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_get_nbr_rel_by_short addr 0x%x", (FMT__D, addr));

  status = zb_nwk_neighbor_get_by_short(addr, &nbt);
  if (status == RET_OK)
  {
    ret = nbt->relationship;
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_get_nbr_rel_by_short ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint8_t zb_nwk_get_nbr_rel_by_ieee(const zb_ieee_addr_t addr)
{
  zb_uint8_t ret = ZB_NWK_NEIGHBOR_ERROR_VALUE;
  zb_ret_t status;
  zb_neighbor_tbl_ent_t *nbt;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_get_nbr_rel_by_ieee", (FMT__0));

  status = zb_nwk_neighbor_get_by_ieee(addr, &nbt);
  if (status == RET_OK)
  {
    ret = nbt->relationship;
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_get_nbr_rel_by_ieee ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint8_t zb_nwk_get_nbr_dvc_type_by_short(zb_uint16_t addr)
{
  zb_uint8_t ret = ZB_NWK_NEIGHBOR_ERROR_VALUE;
  zb_ret_t status;
  zb_neighbor_tbl_ent_t *nbt;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_get_nbr_dvc_type_by_short addr 0x%x", (FMT__D, addr));

  status = zb_nwk_neighbor_get_by_short(addr, &nbt);
  if (status == RET_OK)
  {
    ret = nbt->device_type;
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_get_nbr_dvc_type_by_short ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint8_t zb_nwk_get_nbr_dvc_type_by_ieee(const zb_ieee_addr_t addr)
{
  zb_uint8_t ret = ZB_NWK_NEIGHBOR_ERROR_VALUE;
  zb_ret_t status;
  zb_neighbor_tbl_ent_t *nbt;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_get_nbr_dvc_type_by_ieee", (FMT__0));

  status = zb_nwk_neighbor_get_by_ieee(addr, &nbt);
  if (status == RET_OK)
  {
    ret = nbt->device_type;
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_get_nbr_dvc_type_by_ieee ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint8_t zb_nwk_get_nbr_rx_on_idle_by_short(zb_uint16_t addr)
{
  zb_uint8_t ret = ZB_NWK_NEIGHBOR_ERROR_VALUE;
  zb_ret_t status;
  zb_neighbor_tbl_ent_t *nbt;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_get_neighbor_rx_on_when_idle_by_short addr 0x%x", (FMT__D, addr));

  status = zb_nwk_neighbor_get_by_short(addr, &nbt);
  if (status == RET_OK)
  {
    ret = nbt->rx_on_when_idle;
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_get_neighbor_rx_on_when_idle_by_short ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint8_t zb_nwk_get_nbr_rx_on_idle_short_or_false(zb_uint16_t addr)
{
   zb_uint8_t ret = zb_nwk_get_nbr_rx_on_idle_by_short(addr);
  /*
    If rx_on_when_idle wasn't successfully retrieved,
    lets assume that req is sent to sleepy dev
  */
   if (ret == ZB_NWK_NEIGHBOR_ERROR_VALUE)
   {
     ret = ZB_FALSE;
   }
   return ret;
}

zb_uint8_t zb_nwk_get_nbr_rx_on_idle_by_ieee(zb_ieee_addr_t addr)
{
  zb_uint8_t ret = ZB_NWK_NEIGHBOR_ERROR_VALUE;
  zb_ret_t status;
  zb_neighbor_tbl_ent_t *nbt;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_get_neighbor_rx_on_when_idle_by_ieee", (FMT__0));

  status = zb_nwk_neighbor_get_by_ieee(addr, &nbt);
  if (status == RET_OK)
  {
    ret = nbt->rx_on_when_idle;
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_get_neighbor_rx_on_when_idle_by_ieee ret %hd", (FMT__H, ret));

  return ret;
}


zb_ret_t zb_nwk_get_neighbor_element(zb_uint16_t addr, zb_bool_t create_if_absent, zb_nwk_neighbor_element_t *update)
{
  zb_ret_t status;
  zb_neighbor_tbl_ent_t *nbt = NULL;
  zb_address_ieee_ref_t addr_ref;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_get_neighbor_element", (FMT__0));
  status = zb_address_by_short(addr, ZB_FALSE, ZB_FALSE, &addr_ref);

  if( status == RET_OK)
  {
    status = zb_nwk_neighbor_get(addr_ref, create_if_absent, &nbt);
  }

  if( status == RET_OK)
  {
    update->relationship = nbt->relationship;
    update->device_type = nbt->device_type;
    update->rx_on_when_idle = (zb_bool_t)nbt->rx_on_when_idle;
    update->depth = nbt->depth;
    update->permit_joining = nbt->permit_joining;
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_get_neighbor_element ret %hd", (FMT__H, status));

  return status;
}

zb_ret_t zb_nwk_set_neighbor_element(zb_uint16_t addr, zb_nwk_neighbor_element_t *update)
{
  zb_ret_t status;
  zb_neighbor_tbl_ent_t *nbt = NULL;
  zb_address_ieee_ref_t addr_ref;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_set_neighbor_element", (FMT__0));
  status = zb_address_by_short(addr, ZB_FALSE, ZB_FALSE, &addr_ref);

  if( status == RET_OK)
  {
    status = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbt);
  }

  if( status == RET_OK &&
      /* Store only if data is different */
      (nbt->relationship != update->relationship ||
       nbt->device_type != update->device_type ||
       ZB_U2B(nbt->rx_on_when_idle) != update->rx_on_when_idle ||
       nbt->depth != update->depth ||
       nbt->permit_joining != update->permit_joining)
    )
  {
    nbt->relationship = update->relationship;
    nbt->device_type = update->device_type;
    nbt->rx_on_when_idle = ZB_B2U(update->rx_on_when_idle);
    nbt->depth = update->depth;
    nbt->permit_joining = update->permit_joining;

    /* FIXME: Do we really want to dump all addr+nbt on EVERY device annce? This
     * data does not seem so critical. */
#ifdef ZB_USE_NVRAM
    zb_nvram_store_addr_n_nbt();
#endif
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_set_neighbor_element ret %hd", (FMT__H, status));

  return status;
}

zb_ret_t zb_nwk_delete_neighbor_by_short(zb_uint16_t addr)
{
  zb_ret_t ret;
  zb_neighbor_tbl_ent_t *nbt;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_delete_neighbor_by_short addr 0x%x", (FMT__D, addr));

  ret = zb_nwk_neighbor_get_by_short(addr, &nbt);
  if(ret==RET_OK)
  {
    ret = zb_nwk_neighbor_delete(nbt->u.base.addr_ref);
#ifdef ZB_USE_NVRAM
    zb_nvram_store_addr_n_nbt();
#endif
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_delete_neighbor_by_short ret %hd", (FMT__H, ret));

  return ret;
}

zb_ret_t zb_nwk_delete_neighbor_by_ieee(zb_ieee_addr_t addr)
{
  zb_ret_t ret;
  zb_neighbor_tbl_ent_t *nbt;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_delete_neighbor_by_ieee", (FMT__0));

  ret = zb_nwk_neighbor_get_by_ieee(addr, &nbt);
  if(ret==RET_OK)
  {
    ret = zb_nwk_neighbor_delete(nbt->u.base.addr_ref);
#ifdef ZB_ROUTER_ROLE
    if (nbt->relationship == ZB_NWK_RELATIONSHIP_CHILD)
    {
      if (nbt->device_type == ZB_NWK_DEVICE_TYPE_ROUTER)
      {
        ZB_NIB().router_child_num--;
      }
      else
      {
        ZB_NIB().ed_child_num--;
      }

      TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
      ret = zb_buf_get_out_delayed(zb_nwk_update_beacon_payload);
      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
      }
    }
#endif
#ifdef ZB_USE_NVRAM
    /* [DT]: Call to zb_nvram_store_addr_n_nbt() removed, since 
       nvm store is called from zb_nwk_neighbor_delete(), and this call looks excessive */
#endif
  }
  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_delete_neighbor_by_ieee ret %hd", (FMT__H, ret));

  return ret;
}

/*! @} */
