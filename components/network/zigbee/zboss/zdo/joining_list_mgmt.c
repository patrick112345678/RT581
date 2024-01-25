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
/* PURPOSE: functions for handling joining list 
*/

#define ZB_TRACE_FILE_ID 64
#include "zb_common.h"

#if (defined ZB_JOINING_LIST_SUPPORT) && defined ZB_ROUTER_ROLE

static void zb_ieee_joining_list_announce_send_confirm(zb_uint8_t param);

zb_bool_t zb_ieee_joining_list_schedule(zb_callback_t func, zb_uint8_t param)
{
  zb_bool_t ret = ZB_TRUE;
  zb_jl_q_ent_t *entry;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_schedule %hd", (FMT__H, param));
  entry = ZB_RING_BUFFER_PUT_RESERVE(&ZDO_CTX().joining_list_ctx.operation_queue);

  if (entry == NULL)
  {
    ret = ZB_FALSE;
  }
  else
  {
    entry->func = func;
    entry->param = param;
    ZB_RING_BUFFER_FLUSH_PUT(&ZDO_CTX().joining_list_ctx.operation_queue);
  }

  TRACE_MSG(TRACE_ZDO3, "<< zb_ieee_joining_list_schedule ", (FMT__0));

  return ret;
}

/* Called when an operation is done */
void zb_ieee_joining_list_cb_completed(void)
{
  zb_jl_q_ent_t *entry;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_cb_completed", (FMT__0));
  
  ZB_ASSERT(!ZB_RING_BUFFER_IS_EMPTY(&ZDO_CTX().joining_list_ctx.operation_queue));
  ZB_RING_BUFFER_FLUSH_GET(&ZDO_CTX().joining_list_ctx.operation_queue);
  
  entry = ZB_RING_BUFFER_PEEK(&ZDO_CTX().joining_list_ctx.operation_queue);
  if (entry != NULL)
  {
    ZB_SCHEDULE_CALLBACK(entry->func, entry->param);
  }

  TRACE_MSG(TRACE_ZDO3, "<< zb_ieee_joining_list_cb_completed", (FMT__0));
}

/* Places current operation to the queue end */
void zb_ieee_joining_list_op_delay(void)
{
  zb_jl_q_ent_t entry;
  zb_bool_t run_now, ret;
  
  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_op_delay", (FMT__0));
  
  ZB_ASSERT(!ZB_RING_BUFFER_IS_EMPTY(&ZDO_CTX().joining_list_ctx.operation_queue));
  ZB_MEMCPY(&entry, ZB_RING_BUFFER_PEEK(&ZDO_CTX().joining_list_ctx.operation_queue), sizeof(entry));

  /* delete from queue and start next */
  zb_ieee_joining_list_cb_completed();

  /* if the queue is empty, then the element was the only one in the queue */
  run_now = (zb_bool_t)ZB_RING_BUFFER_IS_EMPTY(&ZDO_CTX().joining_list_ctx.operation_queue);

  /* schedule the element again */
  ret = zb_ieee_joining_list_schedule(entry.func, entry.param);
  if (ret && run_now)
  {
    ZB_SCHEDULE_ALARM(entry.func, entry.param, ZB_JOINING_LIST_DELAY_STEP);
  }

  TRACE_MSG(TRACE_ZDO3, "<< zb_ieee_joining_list_op_delay", (FMT__0));
}

/* 
 * Places a function into operation queue, starts it if nothing is running. 
 * Returns false if there is no space left.
 */
zb_bool_t zb_ieee_joining_list_put_cb(zb_callback_t func, zb_uint8_t param)
{
  zb_bool_t ret;
  zb_bool_t was_empty;
  
  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_put_cb %hd", (FMT__H, param));
  was_empty = (zb_bool_t)ZB_RING_BUFFER_IS_EMPTY(&ZDO_CTX().joining_list_ctx.operation_queue);

  ret = zb_ieee_joining_list_schedule(func, param);
  if (ret && was_empty)
  {
    ZB_SCHEDULE_CALLBACK(func, param);
  }

  TRACE_MSG(TRACE_ZDO3, "<< zb_ieee_joining_list_put_cb (ret %hd)", (FMT__H, ret));
  
  return ret;
}

/*
 * Prepares a buffer with operation confirmation as payload.
 * Then schedules a user-defined callback. 
 */
static void zb_ieee_joining_list_fire_confirm_cb(zb_uint8_t param,
                                                 zb_callback_t callback,
                                                 zb_ieee_joining_list_result_status_t status)
{
  zb_ieee_joining_list_result_t *resp;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_fire_confirm_cb %hd (status %hd)", (FMT__H_H, param, status));
  
  resp = zb_buf_initial_alloc(param, sizeof(zb_ieee_joining_list_result_t));
  resp->status = status;

  if (callback != NULL)
  {
    ZB_SCHEDULE_CALLBACK(callback, param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO3, "<< zb_ieee_joining_list_fire_confirm_cb", (FMT__0));
}

static void zb_ieee_joining_list_fire_confirm_op(zb_uint8_t param,
                                                 zb_ieee_joining_list_result_status_t status)
{
  zb_ieee_joining_list_fire_confirm_cb(param, ZDO_CTX().joining_list_ctx.current_op_cb, status);
}

/*******************************************************************/

/* A generic mlme-set.confirm handler for all functions of the interface*/
static void zb_ieee_joining_list_mlme_set_handler(zb_uint8_t param)
{
  zb_mlme_set_confirm_t *set_confirm;

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_mlme_set_handler %hd", (FMT__H, param));
  
  ZB_ASSERT(zb_buf_len(param) >= sizeof(*set_confirm));  
  set_confirm = (zb_mlme_set_confirm_t *) zb_buf_begin(param);

  TRACE_MSG(TRACE_ZDO3, "status = %hd", (FMT__H, set_confirm->status));
  if (set_confirm->status == MAC_SUCCESS)
  {
    zb_ieee_joining_list_fire_confirm_op(param, ZB_IEEE_JOINING_LIST_RESULT_OK);
  }
  else
  {
    zb_ieee_joining_list_fire_confirm_op(param, ZB_IEEE_JOINING_LIST_RESULT_INTERNAL_ERROR);
  }

  zb_ieee_joining_list_cb_completed();
  
  TRACE_MSG(TRACE_ZDO2, "<< zb_ieee_joining_list_fire_confirm_cb", (FMT__0));
}

/*******************************************************************/

static void zb_ieee_joining_list_add_cb(zb_uint8_t param)
{
  void *ptr;
  zb_ieee_joining_list_add_params_t *add_request;
  zb_mlme_set_request_t *set_req;
  zb_mlme_set_ieee_joining_list_req_t *jl_set_req;
  zb_ieee_addr_t ieee_addr;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_add_cb %hd", (FMT__H, param));

  add_request = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_add_params_t);
  ZB_MEMCPY(&ieee_addr, &add_request->address, sizeof(ieee_addr));
  
  ZDO_CTX().joining_list_ctx.current_op_cb = add_request->callback;
  ZDO_CTX().joining_list_ctx.is_consistent = ZB_FALSE;

  set_req = zb_buf_initial_alloc(param, sizeof(*set_req) + sizeof(*jl_set_req));
  set_req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST;
  set_req->pib_index = 0;
  set_req->pib_length = (zb_uint8_t)sizeof(*jl_set_req);
  set_req->confirm_cb_u.cb = &zb_ieee_joining_list_mlme_set_handler;

  ptr = (void *)(set_req + 1);
  jl_set_req = (zb_mlme_set_ieee_joining_list_req_t *)(ptr);
  jl_set_req->op_type = (zb_uint8_t)ZB_MLME_SET_IEEE_JL_REQ_INSERT;
  ZB_MEMCPY(&jl_set_req->param.ieee_value, &ieee_addr, sizeof(ieee_addr));

  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
  zb_joining_list_reset_clear_timer();

  TRACE_MSG(TRACE_ZDO3, "<< zb_ieee_joining_list_add_cb", (FMT__0));
}

void zb_ieee_joining_list_add(zb_uint8_t param)
{
  zb_ieee_joining_list_add_params_t *add_request;

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_add %hd", (FMT__H, param));
  add_request = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_add_params_t);

  if (!ZB_U2B(ZB_AIB().aps_designated_coordinator))
  {
    TRACE_MSG(TRACE_ERROR, "denied, for coordinators only", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         add_request->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_PERMISSION_DENIED);
  }
  else if (!zb_ieee_joining_list_put_cb(&zb_ieee_joining_list_add_cb, param))
  {
    /* make failure confirm */
    TRACE_MSG(TRACE_ERROR, "joining list operation queue is full", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         add_request->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_RESTART_LATER);
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  TRACE_MSG(TRACE_ZDO2, "<< zb_ieee_joining_list_add", (FMT__0));
}

/********************************************************/

static void zb_ieee_joining_list_delete_op(zb_uint8_t param)
{
  void *ptr;
  zb_ieee_joining_list_delete_params_t *del_request;
  zb_mlme_set_request_t *set_req;
  zb_mlme_set_ieee_joining_list_req_t *jl_set_req;
  zb_ieee_addr_t ieee_addr;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_delete_op %hd", (FMT__H, param));
  
  del_request = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_delete_params_t);
  ZB_MEMCPY(&ieee_addr, &del_request->address, sizeof(ieee_addr));

  ZDO_CTX().joining_list_ctx.current_op_cb = del_request->callback;
  ZDO_CTX().joining_list_ctx.is_consistent = ZB_FALSE;

  set_req = zb_buf_initial_alloc(param, sizeof(*set_req) + sizeof(*jl_set_req));
  set_req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST;
  set_req->pib_index = 0;
  set_req->pib_length = (zb_uint8_t)sizeof(*jl_set_req);
  set_req->confirm_cb_u.cb = &zb_ieee_joining_list_mlme_set_handler;

  ptr = (void *)(set_req + 1);
  jl_set_req = (zb_mlme_set_ieee_joining_list_req_t *)(ptr);
  jl_set_req->op_type = (zb_uint8_t)ZB_MLME_SET_IEEE_JL_REQ_ERASE;
  ZB_MEMCPY(&jl_set_req->param.ieee_value, &ieee_addr, sizeof(ieee_addr));

  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
  zb_joining_list_reset_clear_timer();

  TRACE_MSG(TRACE_ZDO3, "<< zb_ieee_joining_list_delete_op ", (FMT__0));
}

void zb_ieee_joining_list_delete(zb_uint8_t param)
{
  zb_ieee_joining_list_delete_params_t *del_request;

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_delete %hd", (FMT__H, param));
  del_request = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_delete_params_t);

  if (!ZB_U2B(ZB_AIB().aps_designated_coordinator))
  {
    TRACE_MSG(TRACE_ERROR, "denied, for coordinators only", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         del_request->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_PERMISSION_DENIED);
  }
  else if (!zb_ieee_joining_list_put_cb(&zb_ieee_joining_list_delete_op, param))
  {
    /* make failure confirm */
    TRACE_MSG(TRACE_ERROR, "joining list operation queue is full", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         del_request->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_RESTART_LATER);
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  TRACE_MSG(TRACE_ZDO2, "<< zb_ieee_joining_list_delete", (FMT__0));
}

/********************************************************/

static void zb_ieee_joining_list_clear_op(zb_uint8_t param)
{
  void *ptr;
  zb_ieee_joining_list_clear_params_t *clr_request;
  zb_mlme_set_request_t *set_req;
  zb_mlme_set_ieee_joining_list_req_t *jl_set_req;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_clear_op %hd", (FMT__H, param));

  clr_request = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_clear_params_t);
  ZDO_CTX().joining_list_ctx.current_op_cb = clr_request->callback;
  ZDO_CTX().joining_list_ctx.is_consistent = ZB_FALSE;

  set_req = zb_buf_initial_alloc(param, sizeof(*set_req) + sizeof(*jl_set_req));
  set_req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST;
  set_req->pib_index = 0;
  set_req->pib_length = (zb_uint8_t)sizeof(*jl_set_req);
  set_req->confirm_cb_u.cb = &zb_ieee_joining_list_mlme_set_handler;

  ptr = (void *)(set_req + 1);
  jl_set_req = (zb_mlme_set_ieee_joining_list_req_t *)(ptr);
  jl_set_req->op_type = (zb_uint8_t)ZB_MLME_SET_IEEE_JL_REQ_CLEAR;
  jl_set_req->param.clear_params.joining_policy = (zb_uint8_t)clr_request->new_joining_policy;

  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
  zb_joining_list_reset_clear_timer();
  
  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_clear_op", (FMT__0));
}


void zb_ieee_joining_list_clear(zb_uint8_t param)
{
  zb_ieee_joining_list_clear_params_t *clr_request;

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_clear %hd", (FMT__H, param));
  clr_request = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_clear_params_t);

  if (!ZB_U2B(ZB_AIB().aps_designated_coordinator))
  {
    TRACE_MSG(TRACE_ERROR, "denied, for coordinators only", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         clr_request->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_PERMISSION_DENIED);
  }
  else if (!zb_ieee_joining_list_put_cb(&zb_ieee_joining_list_clear_op, param))
  {
    /* make failure confirm */
    TRACE_MSG(TRACE_ERROR, "joining list operation queue is full", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         clr_request->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_RESTART_LATER);
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_clear", (FMT__0));
}

/********************************************************/

static void zb_ieee_joining_list_set_policy_op(zb_uint8_t param)
{
  zb_ieee_joining_list_set_policy_t *request;
  zb_uint8_t new_policy;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_set_policy_op %hd", (FMT__H, param));

  
  request = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_set_policy_t);
  new_policy = (zb_uint8_t)request->new_joining_policy;
  ZDO_CTX().joining_list_ctx.current_op_cb = request->callback;
  ZDO_CTX().joining_list_ctx.is_consistent = ZB_FALSE;

  zb_nwk_pib_set(param,
                 ZB_PIB_ATTRIBUTE_JOINING_POLICY,
                 &new_policy,
                 sizeof(new_policy),
                 &zb_ieee_joining_list_mlme_set_handler
    );
  
  zb_joining_list_reset_clear_timer();

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_set_policy_op", (FMT__0));
}

void zb_ieee_joining_list_set_policy(zb_uint8_t param)
{
  zb_ieee_joining_list_set_policy_t *request;
    
  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_set_policy %hd", (FMT__H, param));
  request = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_set_policy_t);
  
  if (!ZB_U2B(ZB_AIB().aps_designated_coordinator))
  {
    TRACE_MSG(TRACE_ERROR, "denied, for coordinators only", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         request->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_PERMISSION_DENIED);
  }
  else if (!zb_ieee_joining_list_put_cb(&zb_ieee_joining_list_set_policy_op, param))
  {
    /* make failure confirm */
    TRACE_MSG(TRACE_ERROR, "joining list operation queue is full", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         request->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_INSUFFICIENT_SPACE);
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_set_policy", (FMT__0));
}

/*******************************************************/

static void zb_ieee_joining_list_announce_get_confirm(zb_uint8_t param)
{
  zb_mlme_get_confirm_t *cfm;
  zb_mlme_get_ieee_joining_list_res_t *res_params;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_announce_get_confirm %hd", (FMT__H, param));
  
  ZB_ASSERT(zb_buf_len(param) >= sizeof(*cfm));
  
  cfm = (zb_mlme_get_confirm_t *)zb_buf_begin(param);

  TRACE_MSG(TRACE_ZDO2, "mlme-get response status is %hd", (FMT__H, cfm->status));
  
  if (cfm->status != MAC_SUCCESS)
  {
    zb_ieee_joining_list_fire_confirm_op(param,ZB_IEEE_JOINING_LIST_RESULT_INTERNAL_ERROR);
  }
  else
  {
    void *ptr;
    ZB_ASSERT(zb_buf_len(param) >= (sizeof(*cfm) + sizeof(*res_params)));  
    ptr = (void *)(cfm + 1);
    res_params = (zb_mlme_get_ieee_joining_list_res_t *)(ptr);
    ZDO_CTX().joining_list_ctx.current_list_size = res_params->total_length;
    ZDO_CTX().joining_list_ctx.dst_addr = 0xFFFF;

    /* now send response */
    ZB_SCHEDULE_CALLBACK(zdo_nwk_joining_list_resp_send, param);
  }

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_announce_get_confirm", (FMT__0));
}

static void zb_ieee_joining_list_announce_query_next(zb_uint8_t param)
{
  zb_mlme_get_request_t *joining_list_req;
  zb_mlme_get_ieee_joining_list_req_t *joining_list_req_params;
  void *ptr;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_announce_query_next %hd", (FMT__H, param));

  joining_list_req = zb_buf_initial_alloc(param, sizeof(*joining_list_req) + sizeof(*joining_list_req_params));

  ptr = (void *)(joining_list_req + 1);
  joining_list_req_params = (zb_mlme_get_ieee_joining_list_req_t *)(ptr);

  joining_list_req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST;
  joining_list_req->pib_index = 0;
  joining_list_req->confirm_cb_u.cb = &zb_ieee_joining_list_announce_get_confirm;

  /* put request params */
  joining_list_req_params->start_index = ZDO_CTX().joining_list_ctx.next_start_index;
  joining_list_req_params->count = ZB_JOINING_LIST_RESP_ITEMS_LIMIT;

  ZDO_CTX().joining_list_ctx.dst_addr = 0xFFFF;
  ZDO_CTX().joining_list_ctx.broadcast_confirm_cb = &zb_ieee_joining_list_announce_send_confirm;
  ZDO_CTX().joining_list_ctx.next_start_index += joining_list_req_params->count;

  ZB_SCHEDULE_CALLBACK(zb_mlme_get_request, param);

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_announce_query_next", (FMT__0));
}

/* called after a broadcast message is sent */
static void zb_ieee_joining_list_announce_send_confirm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_announce_send_confirm %hd", (FMT__H, param));

  if (zb_buf_get_status(param) == RET_OK)
  {
    if (ZDO_CTX().joining_list_ctx.next_start_index < ZDO_CTX().joining_list_ctx.current_list_size)
    {
      ZB_SCHEDULE_CALLBACK(zb_ieee_joining_list_announce_query_next, param);
    }
    else
    {
      zb_ieee_joining_list_fire_confirm_op(param, ZB_IEEE_JOINING_LIST_RESULT_OK);
      zb_ieee_joining_list_cb_completed();
    }
  }
  else
  {
    zb_ieee_joining_list_fire_confirm_op(param, ZB_IEEE_JOINING_LIST_RESULT_BAD_RESPONSE);
    zb_ieee_joining_list_cb_completed();
  }

  TRACE_MSG(TRACE_ZDO3, "<< zb_ieee_joining_list_announce_send_confirm", (FMT__0));
}

static void zb_ieee_joining_list_announce_op(zb_uint8_t param)
{
  zb_ieee_joining_list_announce_t *ann_req;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_announce_op %hd", (FMT__H, param));

  ann_req = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_announce_t);
  
  if (!ZDO_CTX().joining_list_ctx.is_consistent)
  {
    ZDO_CTX().joining_list_ctx.update_id++;
    ZDO_CTX().joining_list_ctx.is_consistent = ZB_TRUE;
  }

  if (ann_req->silent)
  {
    zb_ieee_joining_list_fire_confirm_op(param, ZB_IEEE_JOINING_LIST_RESULT_OK);
    zb_ieee_joining_list_cb_completed();
  }
  else
  {
    ZDO_CTX().joining_list_ctx.current_op_cb = ann_req->callback;
    ZDO_CTX().joining_list_ctx.next_start_index = 0;
    ZB_SCHEDULE_CALLBACK(zb_ieee_joining_list_announce_query_next, param);
  }
  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_announce_op", (FMT__0));
}

void zb_ieee_joining_list_announce(zb_uint8_t param)
{
  zb_ieee_joining_list_announce_t *ann_req;

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_announce %hd", (FMT__H, param));
  ann_req = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_announce_t);

  if (!ZB_U2B(ZB_AIB().aps_designated_coordinator))
  {
    TRACE_MSG(TRACE_ERROR, "denied, for coordinators only", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         ann_req->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_PERMISSION_DENIED);
  }
  else if (!zb_ieee_joining_list_put_cb(&zb_ieee_joining_list_announce_op, param))
  {
    /* make failure confirm */
    TRACE_MSG(TRACE_ERROR, "joining list operation queue is full", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         ann_req->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_RESTART_LATER);
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_announce", (FMT__0));
}

/**************************************************************/

static void zb_ieee_joining_list_request_resp_handler(zb_uint8_t param)
{
  zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t *resp;
  zb_zdo_mgmt_nwk_ieee_joining_list_param_t *req_param;
  zb_bool_t done = ZB_FALSE;

  TRACE_MSG(TRACE_ZDO1, ">> zb_ieee_joining_list_request_resp_handler %hd", (FMT__H, param));
  resp = (zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t *)zb_buf_begin(param);

  if (resp->status != ZB_ZDP_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ZDO1, "status is not success (%hd)", (FMT__H, resp->status));
    zb_ieee_joining_list_fire_confirm_op(param, ZB_IEEE_JOINING_LIST_RESULT_BAD_RESPONSE);
    zb_ieee_joining_list_cb_completed();    
    done = ZB_TRUE;
  }

  if (!done
      && resp->start_index > 0U
      && ZDO_CTX().joining_list_ctx.prev_update_id != resp->ieee_joining_list_update_id)
  {
    /* update id has changed since the previous query */
    /* request must be started again a bit later */
    TRACE_MSG(TRACE_ZDO1, "update id changed while querying", (FMT__0));
    zb_ieee_joining_list_fire_confirm_op(param, ZB_IEEE_JOINING_LIST_RESULT_RESTART_LATER);
    zb_ieee_joining_list_cb_completed();    
    done = ZB_TRUE;
  }

  if (!done)
  {
    ZDO_CTX().joining_list_ctx.prev_update_id = resp->ieee_joining_list_update_id;

    if (resp->start_index + resp->ieee_joining_count < resp->ieee_joining_list_total)
    {
      zb_nwk_unlock_in(param);

      req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_ieee_joining_list_param_t);
      req_param->start_index = resp->start_index + resp->ieee_joining_count;
      req_param->dst_addr = 0; /* Q: take as param? */
      (void)zb_zdo_mgmt_nwk_ieee_joining_list_req(param, &zb_ieee_joining_list_request_resp_handler);
    }
    else
    {
      TRACE_MSG(TRACE_ZDO1, "done", (FMT__0));

      zb_ieee_joining_list_fire_confirm_op(param, ZB_IEEE_JOINING_LIST_RESULT_OK);
      zb_ieee_joining_list_cb_completed();

      zb_joining_list_reset_clear_timer();
      
      ZDO_CTX().joining_list_ctx.is_consistent = ZB_TRUE;
    }
  }
  
  TRACE_MSG(TRACE_ZDO1, "<< zb_ieee_joining_list_request_resp_handler", (FMT__0));
}

static void zb_ieee_joining_list_request_op(zb_uint8_t param)
{
  zb_ieee_joining_list_request_t *in_req;
  zb_zdo_mgmt_nwk_ieee_joining_list_param_t *req_param;

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_request_op %hd", (FMT__H, param));

  in_req = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_request_t);
  ZDO_CTX().joining_list_ctx.current_op_cb = in_req->callback;
  
  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_ieee_joining_list_param_t);

  req_param->start_index = 0;
  req_param->dst_addr = 0; /* Q: take as param? */
  (void)zb_zdo_mgmt_nwk_ieee_joining_list_req(param, zb_ieee_joining_list_request_resp_handler); 

  TRACE_MSG(TRACE_ZDO3, ">> zb_ieee_joining_list_request_op", (FMT__0));
}

void zb_ieee_joining_list_request(zb_uint8_t param)
{
  zb_ieee_joining_list_request_t *in_req;

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_request %hd", (FMT__H, param));
  in_req = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_request_t);

  if (ZB_U2B(ZB_AIB().aps_designated_coordinator))
  {
    TRACE_MSG(TRACE_ERROR, "denied, for routers only", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         in_req->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_PERMISSION_DENIED);
  }
  else if (!zb_ieee_joining_list_put_cb(&zb_ieee_joining_list_request_op, param))
  {
    /* make failure confirm */
    TRACE_MSG(TRACE_ERROR, "joining list operation queue is full", (FMT__0));
    zb_ieee_joining_list_fire_confirm_cb(param,
                                         in_req->callback,
                                         ZB_IEEE_JOINING_LIST_RESULT_RESTART_LATER);
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  TRACE_MSG(TRACE_ZDO2, ">> zb_ieee_joining_list_request", (FMT__0));
}

/***************************************/
/* 
 * Functions below are responsible for clearing IEEE joining list after mibIeeeExpiryInterval 
 * For details see D.9.1.2 of r22 spec
 */

static void zb_joining_list_do_clear_cont(zb_uint8_t param)
{
  zb_ieee_joining_list_announce_t *params;

  params = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_announce_t);
  params->callback = NULL;
  params->silent = ZB_TRUE;
    
  zb_ieee_joining_list_announce(param);
}

static void zb_joining_list_do_clear(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO3, ">> zb_joining_list_do_clear %hd", (FMT__H, param));

  if (param != 0U)
  {
    zb_ieee_joining_list_clear_params_t *params;

    params = ZB_BUF_GET_PARAM(param, zb_ieee_joining_list_clear_params_t);
    params->callback = zb_joining_list_do_clear_cont;
    params->new_joining_policy = ZB_MAC_JOINING_POLICY_NO_JOIN;
    
    zb_ieee_joining_list_clear(param);
  }
  else
  {
    (void)zb_buf_get_out_delayed(zb_joining_list_do_clear);
  }

  TRACE_MSG(TRACE_ZDO3, "<< zb_joining_list_do_clear", (FMT__0));
}

void zb_joining_list_reset_clear_timer(void)
{
  TRACE_MSG(TRACE_ZDO3, ">> zb_joining_list_reset_clear_timer", (FMT__0));

  ZB_SCHEDULE_ALARM_CANCEL(zb_joining_list_do_clear, 0);
  ZB_SCHEDULE_ALARM(zb_joining_list_do_clear,
                     0,
                     ZB_TIME_ONE_SECOND * 60U * ZDO_CTX().joining_list_ctx.list_expiry_interval);

  TRACE_MSG(TRACE_ZDO3, "<< zb_joining_list_reset_clear_timer", (FMT__0));
}

#endif /* (defined ZB_JOINING_LIST_SUPPORT) && defined ZB_ROUTER_ROLE */
