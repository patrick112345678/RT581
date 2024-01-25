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
/* PURPOSE: ZLL Commissioning cluster: Identify process implementation
*/

#define ZB_TRACE_FILE_ID 2112
#include "zb_common.h"
#include "zb_zcl.h"
#include "zll/zb_zll_common.h"
#include "zll/zll_commissioning_internals.h"
#include "../nwk/nwk_internal.h"
#include "zb_bdb_internal.h"

#if defined ZB_ENABLE_ZLL

/* Enable ZB_ZLL_RESET_MODE to prepare reset-device:
   - Enable all channels in ZB_ZLL_PRIMARY_CHANNELS define
   - Use dimmable_light_tl/light_controller_zed device as application
 */
/* #define ZB_ZLL_RESET_MODE */

#if 0
static zb_uint16_t zll_get_new_addr_from_range();
#endif

/*
 * Cancel current transaction and clear address range, group id range,
 * finish scan request
 *
 * Used from start discovery when exsist current transaction
 */
void zll_cancel_transaction()
{
  TRACE_MSG(TRACE_ZLL1, "> zb_zll_cancel_transaction", (FMT__0));

  ZLL_TRAN_CTX().transaction_id = ZLL_NO_TRANSACTION_ID;

  ZLL_ACTION_SEQUENCE_ROLLBACK();
#ifndef ZB_BDB_TOUCHLINK
  ZB_SCHEDULE_ALARM_CANCEL(zll_intrp_transaction_guard, ZB_ALARM_ANY_PARAM);
  ZB_SCHEDULE_ALARM_CANCEL(zll_scan_req_guard, ZB_ALARM_ANY_PARAM);
#endif
  TRACE_MSG(TRACE_ZLL1, "< zb_zll_cancel_transaction", (FMT__0));
}

/*
 * Inform user about task result
 * @param buffer - buffer with zb_zll_transaction_task_status_t data or somebody else
 * @param _status - operation status
 *
 * Usualy invoke zb_zdo_startup_complete, but if commissioning invoke
 * zll_start_commissioning_process for goto next commissining step or
 * cancel by error.
 */
void zll_notify_task_result(zb_bufid_t  buffer, zb_uint8_t _status)
{
#ifndef ZB_BDB_TOUCHLINK
  zb_zll_transaction_task_status_t* report =
      ZB_BUF_GET_PARAM((buffer), zb_zll_transaction_task_status_t);
  TRACE_MSG(TRACE_ZLL1, "Task fun %p id = %hd status %hd", (FMT__P_H_H,
      ZLL_DEVICE_INFO().report_task_result, ZLL_TRAN_CTX().transaction_task, _status));
  /* Looks like a paranoid check,
  but it could be reset by user
  between in-transaction tasks.
  */
  if (ZLL_DEVICE_INFO().report_task_result != NULL)
  {
    ZB_BUF_REUSE(buffer);
    buffer->u.hdr.status = _status;
    report->status = (_status);
    report->task = ZLL_TRAN_CTX().transaction_task;
    ZLL_DEVICE_INFO().report_task_result(ZB_REF_FROM_BUF((buffer)));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "ERROR report task result callback is NULL", (FMT__0));
  }
#else
  ZVUNUSED(_status);
  TRACE_MSG(TRACE_ZLL1, "zll_notify_task_result status %hd", (FMT__H, _status));
  zb_buf_set_status(buffer, _status);
#ifdef BDB_OLD
  ZB_SCHEDULE_CALLBACK(bdb_commissioning_machine, buffer);
#else
  bdb_commissioning_signal(BDB_COMM_SIGNAL_TOUCHLINK_NOTIFY_TASK_RESULT, buffer);
#endif
#endif
}/* void zll_notify_task_result(buffer, _status) */



#ifdef ZB_ZLL_RESET_MODE
/**************** TEMP!!!  ZLLL **************/

typedef struct zb_zll_reset_factory_new_param_s
{
  zb_uchar_t device_index;
}
zb_zll_reset_factory_new_param_t;

zb_ret_t zb_zll_reset_to_fn(zb_uint8_t param);
#endif

/*******************************/

/*
 * Analog zb_zdo_startup_complete funcion.
 *
 * Used for presentation commissioning process as one command for
 * user app.
 */
void zll_start_commissioning_process(zb_uint8_t param)
{
  zb_uint16_t status = zb_buf_get_status(param);

  TRACE_MSG(TRACE_ZLL1,
      "> zll_start_commissioning_process %hd status %hd task id %hd", (FMT__H_H_H,
      param, status, ZLL_GET_TRANSACTION_TASK_ID()));

  // TODO Use commission process
  switch (ZLL_GET_TRANSACTION_TASK_ID())
  {
    case ZB_ZLL_DEVICE_DISCOVERY_TASK:
#ifdef ZB_ZLL_RESET_MODE
    {
      char my_addr[8];
      int ep_num = ZLL_TRAN_CTX().n_ep_infos;
/* 04/02/2018 EE CR:MAJOR What is that code for? You use non-ZBOSS
 * types, non-ZBOSS memcpy and do not use its result. Is it kind of
 * debug? If so, put it under appropriate ifdef. */
      memcpy(my_addr, &ZLL_TRAN_CTX().device_infos[0], 8);
      ep_num = ep_num;
      break;
    }
#else
      if (status == ZB_ZLL_TASK_STATUS_FINISHED)
      {
        zll_add_device_to_network(param);
      }
      else
      {
        ZLL_DEVICE_INFO().report_task_result = zb_zdo_startup_complete;
        zb_zdo_startup_complete(param);
      }
      break;
#endif
      /* TODO Add other cases. */

    default:
      {
        ZLL_DEVICE_INFO().report_task_result = zb_zdo_startup_complete;
        zb_zdo_startup_complete(param);
      }
      break;
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_start_commissioning_process", (FMT__0));
}

#ifndef ZB_BDB_TOUCHLINK
/* Start commissioning
 * 1 Test current variable: role, address assign capabilities,
 * group id & address ranges
 * 2 Change report_task_result finction for present Start commissioning as one
 * function
 * 3 Start discovery
 * */
zb_ret_t zb_zll_start_commissioning(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_ZLL1, ">> zb_zll_start_commissioning, param %hd", (FMT__H, param));

#ifndef ZB_ZLL_ENABLE_COMMISSIONING_CLIENT
  ret = RET_NOT_IMPLEMENTED;
#endif /* non ZB_ZLL_ENABLE_COMMISSIONING_CLIENT */

  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR commissioning client role not support", (FMT__0));
  }
  else if (!ZB_ZLL_IS_ADDR_ASSIGNMENT())
  {
    ret = RET_INVALID_STATE;
    TRACE_MSG(TRACE_ERROR, "ERROR current device has no address assign capabilities", (FMT__0));
  }
  else if ( ZB_ZLL_ADDR_RANGE_SIZE(ZLL_DEVICE_INFO().addr_range) == 0 )
  {
    ret = RET_INVALID_STATE;
    TRACE_MSG(TRACE_ERROR, "ERROR address range is empty", (FMT__0));
  }
  else if ( ZB_ZLL_GROUP_RANGE_SIZE(ZLL_DEVICE_INFO().group_id_range) == 0 )
  {
    ret = RET_INVALID_STATE;
    TRACE_MSG(TRACE_ERROR, "ERROR groups id range is empty", (FMT__0));
  }

  if (ret == RET_OK)
  {
    ZLL_SET_TRANSACTION_TASK_ID(ZB_ZLL_START_COMMISSIONING);

    ZLL_DEVICE_INFO().report_task_result = zll_start_commissioning_process;

    TRACE_MSG(TRACE_ZLL1, "start discovery: scheduling request", (FMT__0));

    /* TODO: extended_scan == ZB_FALSE. Why?*/
    ZB_ZLL_START_DEVICE_DISCOVERY(buf, ZB_TRUE, NULL, ret);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR,
          "ERROR Could not initiate device discovery: schedule status %hd",
          (FMT__H, ret));
    }
  }

  TRACE_MSG(TRACE_ZLL1, "<< zb_zll_start_commissioning ret %hd", (FMT__H, ret));
  return ret;
}
#endif

/*
 * Select index of known ZLL devices by maximum RSSI and FN/NFN factor
 * @param route_only - select from ZLL Router only
 *
 *  Function prefer
 *  1 FN > NFN
 *  2 max RSSI
 * */
zb_int8_t zll_find_device_info_by_max_rssi(zb_bool_t route_only)
{
  zb_int8_t index = -1;
  zb_uint8_t rssi=0;
  zb_uint8_t fn=0;
  zb_uint8_t dev_fn;
  zb_uint8_t dev_rssi;
  zb_uindex_t i;
  TRACE_MSG(TRACE_ZLL1,
      ">> zll_find_device_info_by_max_rssi, cnt %hd bool %hd",
      (FMT__H_H, ZLL_TRAN_CTX().n_device_infos, route_only));

  for ( i = 0; i< ZLL_TRAN_CTX().n_device_infos; ++i)
  {
    TRACE_MSG(TRACE_ZLL1, "i %hd rssi %hd zll %hd", (FMT__H_H_H, i,
        ZLL_TRAN_CTX().device_infos[i].rssi_correction,
        ZLL_TRAN_CTX().device_infos[i].zll_info));

    if (!route_only ||
        ZB_ZLL_ZB_INFO_GET_DEVICE_TYPE( ZLL_TRAN_CTX().device_infos[i].zb_info) ==
            ZB_ZLL_ZB_INFO_ROUTER_DEVICE_TYPE)
    {
      dev_fn = ZLL_TRAN_CTX().device_infos[i].zll_info & ZB_ZLL_INFO_FACTORY_NEW;
      dev_rssi = ZLL_TRAN_CTX().device_infos[i].rssi_correction;
      if ( dev_fn > fn
           || (dev_fn == fn && dev_rssi > rssi)
           || index < 0
          )
      {
        index = i;
        rssi = ZLL_TRAN_CTX().device_infos[i].rssi_correction;
        fn = ZLL_TRAN_CTX().device_infos[i].zll_info & ZB_ZLL_INFO_FACTORY_NEW;
      }
    }
  }
  TRACE_MSG(TRACE_ZLL1, "<< zll_find_device_info_by_max_rssi %hd", (FMT__H, index));
  return index;
}


zb_uint8_t zll_find_device_ep_by_short_addr(zb_uint16_t addr)
{
  zb_int_t i, j;
  zb_uint8_t ep = 0xff;

  for (i = 0 ; i < ZLL_TRAN_CTX().n_device_infos ; ++i)
  {
    if (ZLL_TRAN_CTX().device_infos[i].nwk_addr == addr)
    {
      for (j = 0 ; j < ZLL_TRAN_CTX().n_ep_infos; ++j)
      {
        if (ZLL_TRAN_CTX().ep_infos[j].dev_idx == i)
        {
          ep = ZLL_TRAN_CTX().ep_infos[j].ep_id;
          break;
        }
      }
      break;
    }
  }
  return ep;
}

zb_int_t zll_get_ep_info(zb_int_t i, zb_uint16_t *addr, zb_uint8_t *ep)
{
  zb_int_t ret = -1;

  if (i < ZLL_TRAN_CTX().n_ep_infos)
  {
    *ep = ZLL_TRAN_CTX().ep_infos[i].ep_id;
    *addr = ZLL_TRAN_CTX().device_infos[ZLL_TRAN_CTX().ep_infos[i].dev_idx].nwk_addr;

    // HACK
    // Texas Inst device return nwk addr == 0xFFFF
    // rerplace nwk addr from cmd info
    if ( *addr == ZB_NWK_BROADCAST_ALL_DEVICES)
    {
     *addr = zb_address_short_by_ieee(ZLL_TRAN_CTX().device_infos[ZLL_TRAN_CTX().ep_infos[i].dev_idx].device_addr);
    }

    ret = 0;
  }
  return ret;
}


static zb_bool_t zll_calc_and_set_encr_nwk_key(zb_zll_ext_device_info_t *dev_info_)
{
  zb_bool_t ret = ZB_FALSE;
  if (ZB_NIB_SECURITY_LEVEL() && dev_info_)
  {
    /* build encrypted network key and save value into device info ctx */
    zb_uint8_t key_idx = zll_calc_enc_dec_nwk_key(ZLL_DEVICE_INFO().encr_nwk_key,
                                                  secur_nwk_key_by_seq(ZB_NIB().active_key_seq_number),
                                                  dev_info_->key_info,
                                                  ZLL_TRAN_CTX().transaction_id,
                                                  dev_info_->resp_id,
                                                  ZB_TRUE);
    if (key_idx != (zb_uint8_t)~0)
    {
      ZLL_DEVICE_INFO().key_index = key_idx;
      TRACE_MSG(TRACE_ZLL3, "check key bitmask from scanResponse and generating encrypted NWK key is OK", (FMT__0));
    }
    else
    {
      ZLL_DEVICE_INFO().key_index = 0;
      TRACE_MSG(TRACE_ZLL3, "check key bitmask from scanResponse and generating encrypted NWK key is FAILED", (FMT__0));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZLL3, "Warning security is Off", (FMT__0));
  }

  /*
  AT: This function can return False value only in case when key_bitmask values don't match.
  By specification we should ignore incoming scanResponses which contain for example 0
  key bitmask (see function zll_handle_scan_res), but id disabled by default
  */
  return ret;
}

/** @internal @brief Processes confirmation on unicast nwkUpdareRequest frame.
  * @param param reference to the packet containing confirmation.
  */
static void zll_nwk_update_req_sent(zb_uint8_t param)
{
  zb_bufid_t  buffer = param;
  /* AT: see comments about task_id below, comments for planning_task variable */
  zb_uint8_t task_id = (ZB_ZLL_IS_FACTORY_NEW())?(ZB_ZLL_TRANSACTION_NWK_START_TASK)
                                                :(ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK);

  TRACE_MSG(TRACE_ZLL1, "> zll_nwk_update_req_sent param %hd", (FMT__H, param));

  if (zb_buf_get_status(param) == ZB_APS_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ZLL3, "nwk update request by unicast Ok", (FMT__0));
    /* AT: TODO: Fix if needed. What to do? Need clarification.
    Currently i repeat touchlink procedure automatically
    but in test TP-PRE-TC-06 second touchlink is initiated by the user
    */
    ZLL_SET_TRANSACTION_TASK_ID(task_id);
    zll_notify_task_result(buffer, ZB_ZLL_TASK_STATUS_NETWORK_UPDATED);
  }
  else
  {
    /* notify about error */
    TRACE_MSG(TRACE_ERROR, "ERROR could not send nwkUpdateRequest, failing transaction", (FMT__0));
    /* AT: We must send notification about error with correct task_id */
    ZLL_SET_TRANSACTION_TASK_ID(task_id);
    zll_notify_task_result(buffer, ZB_ZLL_TASK_STATUS_FAILED);
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_nwk_update_req_sent", (FMT__0));
}

/* Select target device and send start newtwork command */
void zll_add_device_to_network(zb_uint8_t param)
{
  zb_ret_t ret = RET_ERROR;
  zb_zll_ext_device_info_t *device_info;
  zb_int8_t index;
  zb_bool_t search_routers_only;
  /* AT: this var is necessary in case when we get error below and don't call START or JOIN nwk functions */
  zb_uint8_t planning_task;

  TRACE_MSG(TRACE_ZLL1, ">> zll_add_device_to_network, param %hd", (FMT__H, param));

  if (ZB_ZLL_IS_FACTORY_NEW())
  {
    TRACE_MSG(TRACE_ZLL2, "FN device, search only router", (FMT__0));
    search_routers_only = ZB_TRUE;
    planning_task = ZB_ZLL_TRANSACTION_NWK_START_TASK;
  }
  else
  {
    TRACE_MSG(TRACE_ZLL2, "NFN device, call leave nwk", (FMT__0));
    search_routers_only = ZB_FALSE;
    /* AT: also it can be join_ed task, but on current step i don't know what it will be router or ed target */
    planning_task = ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK;
  }

  do
  {
    /* try to find appropriate device */
    index = zll_find_device_info_by_max_rssi(search_routers_only);
    if (index < 0)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR unable to find appropriate device", (FMT__0));
      break;
    }

    /* get device information from transaction ctx */
    device_info = &ZLL_TRAN_CTX().device_infos[index];

    /* at first we should check channel index,
    compare our phy channel and channel field from scan response from
    target device, and toggle our radio on appropriate channel
    */
    if (ZB_PIBCACHE_CURRENT_CHANNEL() != device_info->channel_number)
    {
      if ((zb_aib_channel_page_list_get_2_4GHz_mask()/* MMDEVSTUBS */ & (((zb_uint32_t)1) << device_info->channel_number)))
      {
        TRACE_MSG(TRACE_ERROR, "###current channel %hd, but need to use %hd", (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), device_info->channel_number));
        ZB_PIBCACHE_CURRENT_CHANNEL() = device_info->channel_number;
        zb_nwk_pib_set(param,
                       ZB_PHY_PIB_CURRENT_CHANNEL,
                       &ZB_PIBCACHE_CURRENT_CHANNEL(),
                       1,
                       zll_add_device_to_network); /* call this function again after channel setup */
        ret = RET_OK;
      }
      break;
    }

    /* Here we must check network update id value (from scan response) */
    /* See ZLL specification 8.6:
    If a touch-link initiator wants to bring a router back into the network
    (i.e. if the value of the nwkUpdateId indicated in the scan response command
    frame is older than the value of the nwkUpdateId attribute of the scan
    initiator), it shall send a unicast inter-PAN network update request
    command frame.

    See sub-clause 8.4.6.1 from ZLL specification:
    If an initiator finds a device during device discovery that is part of
    the same network as the initiator but that reports a network update
    identifier in its scan response inter-PAN command frame that is lower than
    that of the initiator, it may generate and transmit a network update
    request inter-PAN command frame (see 7.1.2.2.8) to the target
    using the unicast data service.

    Also you can see certification test TP-PRE-TC-06 (frequency agility)
    */
    if (!ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID())
        && ZB_EXTPANID_CMP(ZB_NIB_EXT_PAN_ID(), device_info->ext_pan_id))
    {
      if (  device_info->nwk_update_id < ZB_NIB_UPDATE_ID()  )
      {
      TRACE_MSG(TRACE_ZLL3, "nwk_update_id is old, sending NETWORK_UPDATE_REQ", (FMT__0));
      /*  It works via long addr (cmd_info.addr_data.intrp_data.src_addr), im not sure why
      (probably because of _request->dst_addr_mode = ZB_INTRP_ADDR_IEEE in ZB_ZLL_SEND_PACKET).
      So set correct nwk_addr is not necessary (for packet delivery) here.
      */
      ZB_ZLL_COMMISSIONING_SEND_NETWORK_UPDATE_REQ_WITH_CHANNEL(
          param,                                  /* memory buffer */
          device_info->nwk_addr,                /* intrp src short addr (0xFFFF in case of FND) */
          device_info->device_addr,             /* intrp src ieee addr */
          ZLL_DEVICE_INFO().freqagility_channel,/* channel value */
          zll_nwk_update_req_sent,              /* cb function */
          ret);                                 /* ret code of ZB_SCHEDULE_CALLBACK when we schedule zb_intrp_data_request */
      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "ERROR could not schedule network update request for sending.", (FMT__0));
        break;
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "intr nwk update request is sheduled for sending", (FMT__0));
        /* touchlink procedure will be continued from nwkUpdateReq confirm, see above */
        break;
      }
    }
      else if (  device_info->nwk_update_id == ZB_NIB_UPDATE_ID()  )
      {
        TRACE_MSG(TRACE_ERROR, "###device is in our network and has actual update_id, do nothing", (FMT__0));
        break;
      }
    }

    TRACE_MSG(TRACE_ERROR, "###start or join nwk on channel %hd", (FMT__H, ZB_PIBCACHE_CURRENT_CHANNEL()));

    /* here we prepare encrypted nwk key */
    zll_calc_and_set_encr_nwk_key(device_info);

    if (search_routers_only) /* it means that we are FN */
    {
#ifdef ZB_ZLL_RESET_MODE
      {
        /* TEMP!!!! send reset command */
        zb_zll_reset_factory_new_param_t *reset_fn_param =
          ZB_BUF_GET_PARAM(param, zb_zll_reset_factory_new_param_t);
        reset_fn_param->device_index = 0;
        zb_zll_reset_to_fn(param);
        return;
      }
#else
      /* macro set params and call zb_zll_start_new_network function */
      /* 01/26/2018 EE CR:MINOR Copy-paste of channel choice. maybe, crate a function? */
      ZB_ZLL_START_NEW_NETWORK(
        param,
                               device_info->pan_id,
                               device_info->ext_pan_id,
        (ZLL_DEVICE_INFO().nwk_channel) ? ZLL_DEVICE_INFO().nwk_channel : device_info->channel_number,
                               device_info->device_addr,
                               ret);
#endif
    }
    else
    {
      if (ZB_ZLL_ZB_INFO_GET_DEVICE_TYPE(device_info->zb_info) == ZB_ZLL_ZB_INFO_ROUTER_DEVICE_TYPE)
      {
        ZB_ZLL_JOIN_ROUTER(param, index, NULL, ret);
      }
      else
      {
        ZB_ZLL_JOIN_ED(param, index, NULL, ret);
      }
    }

  } while (0);

  if (ret != RET_OK)
  {
    /* here notification will be send about
    fail to start joining/start nwk or scheduling sending nwkUpdateReq
    it will be send via ZLL_DEVICE_INFO().report_task_result cb
    */
    ZLL_SET_TRANSACTION_TASK_ID(planning_task);
    zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_add_device_to_network", (FMT__0));
}


#ifdef ZB_ZLL_ADDR_ASSIGN_CAPABLE

/* Return new address, genrated using ZLL or classic ZB algorithm */
zb_uint16_t zll_get_new_addr()
{
  zb_uint16_t result_addr = ZB_ZLL_ADDR_ERROR;

  TRACE_MSG(TRACE_ZLL1, "> zll_get_new_addr", (FMT__0));

  /* 04/03/2018 EE CR:MINOR Is that commented call a debug result, or
   * we really must always use only stochastic addresses assignment?
   * If so, complete cleanup by removing dead branches here. */
  /* result_addr = zll_get_new_addr_from_range(); */
  if (result_addr == ZB_ZLL_ADDR_ERROR)
  {
    result_addr = zb_nwk_get_stoch_addr();
  }
  else if (result_addr == ZB_ZLL_NO_MORE_ADDR)
  {
    result_addr = ZB_ZLL_ADDR_ERROR;
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_get_new_addr result_addr %x", (FMT__D, result_addr));
  return result_addr;
}/* zb_uint16_t zll_get_new_addr() */


/* Return next address using ZLL address assignment logic */
#if 0                           /* call is commented above */
static zb_uint16_t zll_get_new_addr_from_range()
{
  zb_uint16_t addr = ZB_ZLL_ADDR_ERROR;

  TRACE_MSG(TRACE_ZLL3, "zll_get_new_addr_from_range: addr begin %d, addr end %d",
            (FMT__D_D, ZLL_DEVICE_INFO().addr_range.addr_begin,
                ZLL_DEVICE_INFO().addr_range.addr_end));
  if (ZLL_DEVICE_INFO().addr_range.addr_begin)
  {
    if (ZLL_DEVICE_INFO().addr_range.addr_begin < ZLL_DEVICE_INFO().addr_range.addr_end)
    {
      addr = ZLL_DEVICE_INFO().addr_range.addr_begin++;
    }
    else
    {
      addr = ZB_ZLL_NO_MORE_ADDR;
    }
  }
  TRACE_MSG(TRACE_ZLL3, "zll_get_new_addr_from_range, ret %d", (FMT__D, addr));

  return addr;
}
#endif

/* Alloc to target group id range from own group id range
 * @param range - pointer to target group id range
 * @param range_len - len allocate range
 * @return - RET_ERROR if own range is too small, else RET_OK
 * */
zb_ret_t zll_get_group_id_range(zb_zll_group_id_range_t* range, zb_uint16_t range_len)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_ZLL1, "> zll_get_group_id_range range %p range_len %hd",
            (FMT__P_H, range, range_len));

  range->group_id_begin = range->group_id_end = 0;
  TRACE_MSG(TRACE_ZLL3, "range begin %d, range end %d",
            (FMT__D_D, ZLL_DEVICE_INFO().group_id_range.group_id_begin,
             ZLL_DEVICE_INFO().group_id_range.group_id_end));

  if (ZB_ZLL_GROUP_RANGE_SIZE(ZLL_DEVICE_INFO().group_id_range) < (ZB_ZLL_MIN_GROUP_ID_RANGE + range_len))
  {
    ret = RET_ERROR;
  }
  else
  {
    range->group_id_begin = ZLL_DEVICE_INFO().group_id_range.group_id_end - range_len;
    range->group_id_end = ZLL_DEVICE_INFO().group_id_range.group_id_end;
    ZLL_DEVICE_INFO().group_id_range.group_id_end -= range_len;
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_get_group_id_range begin %d end %d ret %hd",
            (FMT__D_D_H, range->group_id_begin, range->group_id_end, ret));
  return ret;
}

/* Alloc to target address range from own address range
 * @param range - pointer to target address range
 * @return - RET_ERROR if own range is too small, else RET_OK
 * */
zb_ret_t zll_get_addr_range(zb_zll_addr_range_t* range)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t range_size;
  zb_uint16_t assign_range = 0;

  TRACE_MSG(TRACE_ZLL1, "> zll_get_addr_range range %p ",
            (FMT__P, range));

  TRACE_MSG(TRACE_ZLL3, "range begin %d, range end %d",
            (FMT__D_D, ZLL_DEVICE_INFO().addr_range.addr_begin,
                ZLL_DEVICE_INFO().addr_range.addr_end));

  range_size = ZB_ZLL_ADDR_RANGE_SIZE(ZLL_DEVICE_INFO().addr_range);
  TRACE_MSG(TRACE_ZLL3, "range %d", (FMT__D, range_size));

  if (range_size < ZB_ZLL_MIN_ADDR_RANGE * 2)
  {
    TRACE_MSG(TRACE_ZLL1, "Error, too small addr range", (FMT__0));
    ret = RET_ERROR;
  }
  else if (range_size < (ZB_ZLL_MIN_ADDR_RANGE + ZB_ZLL_DEFAULT_ADDR_RANGE))
  {
    assign_range = ZB_ZLL_MIN_ADDR_RANGE;
  }
  else
  {
    assign_range = ZB_ZLL_DEFAULT_ADDR_RANGE;
  }

  TRACE_MSG(TRACE_ZLL3, "assign range %d", (FMT__D, assign_range));
  if (assign_range)
  {
    range->addr_begin = ZLL_DEVICE_INFO().addr_range.addr_end - assign_range;
    range->addr_end = ZLL_DEVICE_INFO().addr_range.addr_end;
    ZLL_DEVICE_INFO().addr_range.addr_end -= assign_range;
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_get_addr_range begin %d end %d ret %hd",
            (FMT__D_D_H, range->addr_begin, range->addr_end, ret));
  return ret;
}

#endif /* ZB_ZLL_ADDR_ASSIGN_CAPABLE */

void zll_save_nwk_prefs(zb_ext_pan_id_t ext_pan_id, zb_uint16_t pan_id,
                             zb_uint16_t short_addr, zb_uint8_t channel)
{
  zb_ext_pan_id_t restore_ext_pan_id;
  zb_uint16_t restore_pan_id;
  zb_uint16_t restore_short_addr;
  zb_uint8_t restore_channel;

  /* Backup(save) current channel, panid, short address, extended panid, what else?
   */
  ZB_MEMCPY(restore_ext_pan_id, ZB_NIB_EXT_PAN_ID(), sizeof(zb_ext_pan_id_t));
  restore_channel = ZB_PIBCACHE_CURRENT_CHANNEL();
  restore_pan_id = ZB_PIBCACHE_PAN_ID();
  restore_short_addr = ZB_PIBCACHE_NETWORK_ADDRESS();

  /* Save new values into NVRAM */
  ZB_MEMCPY(ZB_NIB_EXT_PAN_ID(), ext_pan_id, sizeof(zb_ext_pan_id_t));
  ZB_PIBCACHE_CURRENT_CHANNEL() = channel;
  ZB_PIBCACHE_PAN_ID() = pan_id;
  ZB_PIBCACHE_NETWORK_ADDRESS() = short_addr;

  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);

  /* Restore backuped values */
  ZB_MEMCPY(ZB_NIB_EXT_PAN_ID(), restore_ext_pan_id, sizeof(zb_ext_pan_id_t));
  ZB_PIBCACHE_CURRENT_CHANNEL() = restore_channel;
  ZB_PIBCACHE_PAN_ID() = restore_pan_id;
  ZB_PIBCACHE_NETWORK_ADDRESS() = restore_short_addr;

}

zb_uint8_t zb_zll_get_info_current_value(void)
{
#if defined ZB_ED_ROLE
  return (ZB_ZLL_ZB_INFO_ED_DEVICE_TYPE |
          (ZB_PIBCACHE_RX_ON_WHEN_IDLE() ? ZB_ZLL_ZB_INFO_RX_ON_WHEN_IDLE_MASK : 0));
#else /* defined ZB_ED_ROLE */
  return ((ZB_AIB().aps_designated_coordinator ? ZB_ZLL_ZB_INFO_COORD_DEVICE_TYPE : ZB_ZLL_ZB_INFO_ROUTER_DEVICE_TYPE) |
          (ZB_PIBCACHE_RX_ON_WHEN_IDLE() ? ZB_ZLL_ZB_INFO_RX_ON_WHEN_IDLE_MASK : 0));
#endif /* defined ZB_ED_ROLE */
}

zb_uint8_t* zb_zll_get_nib_ext_pan_id(void)
{
  return ZB_NIB_EXT_PAN_ID();
}

zb_uint8_t zb_zll_get_nib_update_id(void)
{
  return ZB_NIB_UPDATE_ID();
}

#endif /* defined ZB_ENABLE_ZLL */
