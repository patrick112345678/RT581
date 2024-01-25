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
/* PURPOSE: Generic FSM implementation.
*/

#define ZB_TRACE_FILE_ID 12201
#include "zboss_api_core.h"

#ifdef ZB_FSM_SUPPORT

#include "zb_fsm.h"

zb_ret_t zb_fsm_sys_init(zb_fsm_sys_t    *fsm_sys,
                         zb_fsm_desc_t   *desc,
                         zb_fsm_ev_hdr_t *evt_pool,
                         zb_fsm_ev_itr_t  ev_iterator)
{
  TRACE_MSG(TRACE_SPECIAL2, ">> zb_fsm_sys_init fsm_sys %p", (FMT__P, fsm_sys));

  ZB_ASSERT(fsm_sys != NULL);

  fsm_sys->fsms_desc = desc;
  fsm_sys->fsms_ev_pool = evt_pool;
  fsm_sys->fsms_ev_iterator = ev_iterator;

  TRACE_MSG(TRACE_SPECIAL2, "<< zb_fsm_sys_init", (FMT__0));
  return RET_OK;
}

zb_ret_t zb_fsm_init(zb_fsm_t           *fsm,
                     zb_fsm_sys_t       *fsm_sys,
                     zb_fsm_chain_1_t   *chain,
                     zb_fsm_finish_cb_t  finish_cb)
{
  TRACE_MSG(TRACE_SPECIAL1, ">> zb_fsm_init fsm %p fsm_sys %p", (FMT__P_P, fsm, fsm_sys));

  ZB_ASSERT(fsm != NULL);

  ZB_BZERO(fsm, sizeof(zb_fsm_t));

  fsm->fsm_sys = fsm_sys;
  fsm->fsm_state = ZB_FSM_STATE_NONE;
  fsm->fsm_run = ZB_FSM_WAIT;
  fsm->fsm_ev = NULL;
  fsm->fsm_desc = NULL;
  fsm->fsm_chain = chain;
  fsm->fsm_ret = RET_OK;
  fsm->fsm_finish_cb = finish_cb;
  zb_fsm_chain_clear(fsm);

  TRACE_MSG(TRACE_SPECIAL1, "<< zb_fsm_init", (FMT__0));

  return RET_OK;
}

zb_fsm_ev_hdr_t* zb_fsm_event_get(zb_fsm_t *fsm)
{
  zb_fsm_ev_hdr_t* evt = NULL;
  zb_fsm_ev_hdr_t* ev_pool;
  zb_fsm_ev_hdr_t* n_evt;

  TRACE_MSG(TRACE_SPECIAL3, ">> zb_fsm_event_get fsm %p", (FMT__P, fsm));

  ZB_ASSERT(fsm != NULL);
  ZB_ASSERT(fsm->fsm_sys != NULL);

  ev_pool = n_evt = fsm->fsm_sys->fsms_ev_pool;

  do
  {
    if (n_evt->fe_type == ZB_FSM_EV_NONE)
    {
      evt = n_evt;
      evt->fe_type = ZB_FSM_EV_UNDEF;
      break;
    }
    else
    {
      n_evt = fsm->fsm_sys->fsms_ev_iterator(ev_pool, n_evt);
    }
  }
  while (n_evt != NULL);

  ZB_ASSERT(evt != NULL);

  TRACE_MSG(TRACE_SPECIAL3, "<< zb_fsm_event_get event %p", (FMT__P, evt));

  return evt;
}

/**
 * Internal FSM function, should not be called by application directly.
 * Runs FSM main handler fsm_desc.fd_handler while FSM run state is ZB_FSM_RUN.
 *
 * Return codes:
 * RET_OK - Ok, event successfully handled
 * RET_EOF - Ok, event successfully handled and FSM processing is finished
 * RET_DOES_NOT_EXIST - Error, FSM descriptor is not set
 * RET_INVALID_PARAMETER
 */
static zb_ret_t run_fsm(zb_fsm_t *fsm)
{
  zb_ret_t ret = RET_INVALID_PARAMETER;

  TRACE_MSG(TRACE_SPECIAL3, ">> run_fsm fsm %p", (FMT__P, fsm));

  if (!fsm->fsm_desc)
  {
    TRACE_MSG(TRACE_SPECIAL1, "fsm_desc is NULL ", (FMT__0));
    /* release unhandled event */
    zb_fsm_event_put(fsm, fsm->fsm_ev);
    fsm->fsm_ev = NULL;
    ret = RET_NOT_FOUND;
  }
  else if (fsm->fsm_run == ZB_FSM_RUN)
  {
    ZB_ASSERT(fsm->fsm_desc->fd_hnd != NULL);

    do
    {
      TRACE_MSG(TRACE_SPECIAL3, "Handler iteration fsm_desc %hd state %hd",
                (FMT__H_H, ZB_FSM_DESC_GET_TYPE(fsm), fsm->fsm_state));

      /* [AV] fsm_desc can't be NULL if we're here: see line 132
       * [EN] It can. We set new fsm_desc at line 183 */
      if (fsm->fsm_desc)
      {
        ret = fsm->fsm_desc->fd_hnd(fsm, fsm->fsm_ev);
      }
      else
      {
        TRACE_MSG(TRACE_SPECIAL1, "fsm_desc is NULL ", (FMT__0));
        /* release unhandled event */
        zb_fsm_event_put(fsm, fsm->fsm_ev);
        fsm->fsm_ev = NULL;
        ret = RET_NOT_FOUND;
        break;
      }

      /* Event should be handled/released by fd_hnd(). Set parameter
       * to NULL in order to avoid handling the same evt in case
       * RET_AGAIN is returned */
      fsm->fsm_ev = NULL;

      if (fsm->fsm_state == ZB_FSM_STATE_FINAL && ret != RET_AGAIN)
      {
        zb_uint8_t fd_type;

        fd_type = zb_fsm_chain_next(fsm);
        if (fsm->fsm_ret != RET_OK || fd_type == ZB_FSM_DESC_DONE)
        {
          /*
           * Stop executing chain if some FSM descriptor in chain failed
           * or if that was the last one.
           */
          TRACE_MSG(TRACE_SPECIAL1, "fsm processing finished, stop ", (FMT__0));

          ret = RET_EOF;
          break;
        }
        else
        {
          /* Some valid descriptor - call handler again */
          ret = RET_AGAIN;
        }

        zb_fsm_desc_set(fsm, fd_type);
      }
    }
    while (ret == RET_AGAIN);

    fsm->fsm_run = ZB_FSM_WAIT;
  }

  if (ret == RET_EOF && fsm->fsm_finish_cb != NULL)
  {
    fsm->fsm_finish_cb(fsm);
  }
  TRACE_MSG(TRACE_SPECIAL3, "<< run_fsm ret %hd", (FMT__H, ret));

  return (ret == RET_BLOCKED ? RET_OK : ret);
}

void zb_fsm_failure(zb_fsm_t *fsm, zb_ret_t error_code, zb_uint16_t from_file,
    zb_uint16_t from_line)
{
  TRACE_MSG(TRACE_ERROR, ">> zb_fsm_failure ERROR: fsm %p error_code %hd from_file %d from_line %d",
      (FMT__P_H_D_D, fsm, error_code, from_file, from_line));
  ZB_ASSERT(error_code != RET_OK);
  fsm->fsm_ret = error_code;
  zb_fsm_state_set(fsm, ZB_FSM_STATE_FINAL);
  TRACE_MSG(TRACE_SPECIAL1, "<< zb_fsm_failure", (FMT__0));
}

zb_ret_t zb_fsm_event_post(zb_fsm_t *fsm, zb_fsm_ev_hdr_t *evt)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_SPECIAL3, ">> zb_fsm_post_event fsm %p evt %p", (FMT__P_P, fsm, evt));

  ZB_ASSERT(fsm != NULL);

  /* Set FSM running*/
  fsm->fsm_run = ZB_FSM_RUN;
  fsm->fsm_ev = evt;

  ret = run_fsm(fsm);

  TRACE_MSG(TRACE_SPECIAL3, "<< zb_fsm_post_event ret %hd", (FMT__H, ret));

  return ret;
}

zb_ret_t zb_fsm_event_put(zb_fsm_t *fsm, zb_fsm_ev_hdr_t *evt)
{
  TRACE_MSG(TRACE_SPECIAL3, ">> zb_fsm_event_put evt %p", (FMT__P, evt));

  ZB_ASSERT(fsm != NULL);

  fsm->fsm_ev = NULL;
  if (evt)
  {
    evt->fe_type = ZB_FSM_EV_NONE;
  }

  TRACE_MSG(TRACE_SPECIAL3, "<< zb_fsm_event_put", (FMT__0));

  return RET_OK;
}

static const zb_fsm_conf_t *fsm_get_state_conf(zb_fsm_t *fsm, zb_uint8_t state)
{
  const zb_fsm_conf_t *conf = NULL;

  ZB_ASSERT(fsm != NULL && fsm->fsm_desc != NULL);

  if (state != ZB_FSM_STATE_FINAL &&
      state < fsm->fsm_desc->fd_max_state)
  {
    conf = &fsm->fsm_desc->fd_conf[fsm->fsm_state];
  }

  TRACE_MSG(TRACE_SPECIAL3, "fsm_get_state_conf %p", (FMT__P, conf));
  return conf;
}

void zb_fsm_state_set(zb_fsm_t *fsm, zb_uint8_t new_state)
{
  const zb_fsm_conf_t *conf;

  TRACE_MSG(TRACE_SPECIAL3, ">> zb_fsm_state_set fsm %p fsm_desc %hd state %hd",
            (FMT__P_H_H, fsm, ZB_FSM_DESC_GET_TYPE(fsm), fsm->fsm_state));

  ZB_ASSERT(fsm != NULL);

  conf = fsm_get_state_conf(fsm, fsm->fsm_state);

  /* Check transtion is allowed */
  if ( conf && (conf->fc_allowed & ZB_BITS1(new_state)) )
  {
    fsm->fsm_state = new_state;
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Forbidden state transition %p %hd %hd",
              (FMT__P_H_H, fsm, fsm->fsm_state, new_state));
    ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_SPECIAL3, "<< zb_fsm_state_set", (FMT__0));
}

zb_ret_t zb_fsm_desc_set(zb_fsm_t *fsm, zb_uint8_t desc_type)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_SPECIAL1, ">> zb_fsm_desc_set fsm %p current_desc %hd new_desc %hd",
            (FMT__P_H_H, fsm, ZB_FSM_DESC_GET_TYPE(fsm), desc_type));
  if (desc_type != ZB_FSM_DESC_NULL && desc_type != ZB_FSM_DESC_DONE)
  {
    fsm->fsm_desc = &fsm->fsm_sys->fsms_desc[desc_type];
  }
  else
  {
    fsm->fsm_desc = NULL;
  }
  fsm->fsm_state = ZB_FSM_STATE_APP_INITIAL;

  TRACE_MSG(TRACE_SPECIAL1, "<< zb_fsm_desc_set", (FMT__0));

  return ret;
}

static void fsm_chain_clear_int(zb_fsm_chain_1_t *chain)
{
  ZB_ASSERT(chain);

  /* Must use -1 instead of ZB_FSM_DESC_DONE due to bug in some compilers */
  ZB_MEMSET(chain->fsc_chain, -1, chain->fsc_chain_size * sizeof(zb_uint8_t));
  chain->fsc_read_idx = 0;
}

void zb_fsm_chain_init(zb_fsm_chain_1_t *chain, zb_uint8_t chain_size)
{
  TRACE_MSG(TRACE_SPECIAL1, ">> zb_fsm_chain_init chain %p chain_size %hd",
            (FMT__P_H, chain, chain_size));

  ZB_ASSERT(chain);
  chain->fsc_chain_size = chain_size;
  fsm_chain_clear_int(chain);
  TRACE_MSG(TRACE_SPECIAL1, "<< zb_fsm_chain_init", (FMT__0));
}

zb_ret_t zb_fsm_chain_add(zb_fsm_t *fsm, zb_fsm_desc_type_t desc_type)
{
  zb_uindex_t i;
  zb_ret_t   ret = RET_TABLE_FULL;

  TRACE_MSG(TRACE_SPECIAL1, ">> zb_fsm_chain_add fsm %p desc_type %hd",
            (FMT__P_H, fsm, desc_type));

  ZB_ASSERT(fsm);

  if (fsm->fsm_chain)
  {
    for (i = 0; i < fsm->fsm_chain->fsc_chain_size; i++)
    {
      if (fsm->fsm_chain->fsc_chain[i] == ZB_FSM_DESC_DONE)
      {
        fsm->fsm_chain->fsc_chain[i] = desc_type;
        ret = RET_OK;
        break;
      }
    }
  }

  TRACE_MSG(TRACE_SPECIAL1, "<< zb_fsm_chain_add ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint8_t zb_fsm_chain_next(zb_fsm_t *fsm)
{
  zb_uint8_t read_idx;
  zb_uint8_t desc_type = ZB_FSM_DESC_DONE;

  TRACE_MSG(TRACE_SPECIAL1, ">> zb_fsm_chain_next fsm %p", (FMT__P, fsm));

  ZB_ASSERT(fsm);

  if (fsm->fsm_chain)
  {
    read_idx = fsm->fsm_chain->fsc_read_idx;
    TRACE_MSG(TRACE_SPECIAL2, "read_idx %hd", (FMT__H, read_idx));

    if (read_idx < fsm->fsm_chain->fsc_chain_size &&
        fsm->fsm_chain->fsc_chain[read_idx] != ZB_FSM_DESC_DONE)
    {
      desc_type = fsm->fsm_chain->fsc_chain[read_idx];
      fsm->fsm_chain->fsc_read_idx++;
    }
  }

  TRACE_MSG(TRACE_SPECIAL1, "<< zb_fsm_chain_next desc %hd", (FMT__H, desc_type));

  return desc_type;
}

zb_ret_t zb_fsm_chain_clear(zb_fsm_t *fsm)
{
  TRACE_MSG(TRACE_SPECIAL1, ">> zb_fsm_chain_clear fsm %p", (FMT__P, fsm));

  if (fsm->fsm_chain)
  {
    fsm_chain_clear_int(fsm->fsm_chain);
  }

  TRACE_MSG(TRACE_SPECIAL1, "<< zb_fsm_chain_clear", (FMT__0));
  return RET_OK;
}

#endif /* ZB_FSM_SUPPORT */
