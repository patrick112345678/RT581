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
/* PURPOSE: ZGP TX queue implementation
*/

#define ZB_TRACE_FILE_ID 2108

#include "zb_common.h"

#ifdef ZB_ENABLE_ZGP_TX_QUEUE
#include "zboss_api_zgp.h"
#include "zgp/zgp_internal.h"

#ifdef ZB_ZGP_IMMED_TX
#define SET_START_END_POS(start_pos, end_pos, search_mode)      \
do                                                              \
{                                                               \
  start_pos = 0;                                                \
  end_pos   = ZB_ZGP_TX_PACKET_INFO_COUNT;                      \
                                                                \
  switch (search_mode)                                          \
  {                                                             \
    case ZB_ZGP_TX_PACKET_INFO_ALL_PACKETS:                     \
      break;                                                    \
    case ZB_ZGP_TX_PACKET_INFO_PENDING_PACKETS:                 \
      end_pos = ZB_ZGP_TX_QUEUE_SIZE;                           \
      break;                                                    \
    case ZB_ZGP_TX_PACKET_INFO_IMMED_PACKETS:                   \
      start_pos = ZB_ZGP_TX_QUEUE_SIZE;                         \
      break;                                                    \
    default:                                                    \
      ZB_ASSERT(0);                                             \
      break;                                                    \
  }                                                             \
}                                                               \
while (ZB_FALSE)
#else
#define SET_START_END_POS(start_pos, end_pos, search_mode)      \
ZVUNUSED(search_mode);                                          \
do                                                              \
{                                                               \
  start_pos = 0;                                                \
  end_pos   =  ZB_ZGP_TX_PACKET_INFO_COUNT;                     \
}                                                               \
while (ZB_FALSE)
#endif /* ZB_ZGP_IMMED_TX */

zb_uint8_t zb_zgp_tx_packet_info_q_grab_free_ent_pos(zb_zgp_tx_packet_info_q_t* tx_p_i_q,
                                                     zb_uint8_t                 search_mode)
{
  zb_uint8_t ret = 0xFF;
  zb_uindex_t i;
  zb_uint8_t start_pos;
  zb_uint8_t end_pos;

  SET_START_END_POS(start_pos, end_pos, search_mode);

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_tx_q_grab_free_ent tx_p_i_q %p", (FMT__P, tx_p_i_q));

  for (i = start_pos; i < end_pos; i++)
  {
    if (!ZB_CHECK_BIT_IN_BIT_VECTOR(tx_p_i_q->used_mask, i))
    {
      ret = i;
      ZB_SET_BIT_IN_BIT_VECTOR(tx_p_i_q->used_mask, i);
      break;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_tx_q_grab_free_ent ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint8_t zb_zgp_tx_packet_info_q_find_pos(zb_zgp_tx_packet_info_q_t *tx_p_i_q,
                                            zb_zgpd_id_t              *id,
                                            zb_uint8_t                 search_mode)
{
  zb_uint8_t ret = 0XFF;
  zb_uindex_t i;
  zb_uint8_t start_pos;
  zb_uint8_t end_pos;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_tx_packet_info_q_find_pos tx_p_i_q %p id %p",
      (FMT__P_P, tx_p_i_q, id));

  ZB_DUMP_ZGPD_ID(*id);

  SET_START_END_POS(start_pos, end_pos, search_mode);

  for (i = start_pos; i < end_pos; i++)
  {
    if (ZB_CHECK_BIT_IN_BIT_VECTOR(tx_p_i_q->used_mask, i))
    {
      if (ZB_ZGPD_IDS_ARE_EQUAL(&tx_p_i_q->queue[i].zgpd_id, id))
      {
        ret = i;
        break;
      }
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_tx_packet_info_q_find_pos ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint8_t zb_zgp_tx_q_find_ent_pos_for_cfm(zb_zgp_tx_packet_info_q_t *tx_p_i_q,
                                            zb_uint8_t                 buf_ref)
{
  zb_uint8_t ret = 0xFF;
  zb_uindex_t i;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_tx_q_find_ent_for_cfm tx_p_i_q %p buf_ref %hd",
      (FMT__P_H, tx_p_i_q, buf_ref));

  for (i = 0; i < ZB_ZGP_TX_PACKET_INFO_COUNT; i++)
  {
    if (ZB_CHECK_BIT_IN_BIT_VECTOR(tx_p_i_q->used_mask, i) &&
        (tx_p_i_q->queue[i].buf_ref == buf_ref))
    {
      ret = i;
      break;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_tx_q_find_ent_for_cfm ret %p", (FMT__P, ret));

  return ret;
}

zb_uint8_t zb_zgp_tx_q_find_ent_pos_for_send(zb_zgp_tx_packet_info_q_t *tx_p_i_q,
                                             zb_zgp_tx_q_t             *tx_q,
                                             zb_zgpd_id_t              *id)
{
  zb_uint8_t ret = 0xFF;
  zb_uindex_t i;

  TRACE_MSG(TRACE_ZGP3, ">> zb_zgp_tx_q_find_ent_for_send tx_p_i_q %p tx_q %p id %p",
            (FMT__P_P_P, tx_p_i_q, tx_q, id));

  ZB_DUMP_ZGPD_ID(*id);

  for (i = 0; i < ZB_ZGP_TX_QUEUE_SIZE; i++)
  {
    if (ZB_CHECK_BIT_IN_BIT_VECTOR(tx_p_i_q->used_mask, i))
    {
      TRACE_MSG(TRACE_ZGP3, "i %d frame_type %d",
                (FMT__D_D, i, ZB_GP_DATA_REQ_FRAME_TYPE(tx_q->queue[i].tx_options)));
      if ((!tx_q->queue[i].is_expired && !tx_q->queue[i].sent)
          && (ZB_ZGPD_IDS_ARE_EQUAL(&tx_p_i_q->queue[i].zgpd_id, id) ||
              (ZB_GP_DATA_REQ_FRAME_TYPE(tx_q->queue[i].tx_options) == ZGP_FRAME_TYPE_MAINTENANCE) ||
              (tx_p_i_q->queue[i].zgpd_id.addr.src_id == ZB_ZGP_SRC_ID_ALL)))
      {
        ret = i;
        break;
      }
    }
  }

  TRACE_MSG(TRACE_ZGP3, "<< zb_zgp_tx_q_find_ent_for_send ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint8_t zb_zgp_tx_q_find_expired_ent_pos(zb_zgp_tx_packet_info_q_t *tx_p_i_q,
                                            zb_zgp_tx_q_t             *tx_q)
{
  zb_uint8_t ret = 0xFF;
  zb_uindex_t i;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_tx_q_find_expired_ent tx_p_i_q %p tx_q %p",
      (FMT__P, tx_p_i_q, tx_q));

  for (i = 0; i < ZB_ZGP_TX_QUEUE_SIZE; i++)
  {
    if (ZB_CHECK_BIT_IN_BIT_VECTOR(tx_p_i_q->used_mask, i))
    {
      if (tx_q->queue[i].is_expired)
      {
        ret = i;
        break;
      }
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_tx_q_find_expired_ent ret %hd", (FMT__H, i));

  return ret;
}

void zb_zgp_tx_packet_info_q_delete_ent(zb_zgp_tx_packet_info_q_t *tx_p_i_q, zb_uint8_t pos)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_tx_q_delete_ent tx_p_i_q %p pos %hd", (FMT__P_H, tx_p_i_q, pos));

  ZB_CLR_BIT_IN_BIT_VECTOR(tx_p_i_q->used_mask, pos);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_tx_q_delete_ent", (FMT__0));
}

zb_bool_t zb_has_zgp_tx_packet_info_q_capacity_to_store(zb_zgp_tx_packet_info_q_t *tx_p_i_q,
                                                        zb_uint8_t                 search_mode)
{
  zb_uint8_t start_pos;
  zb_uint8_t end_pos;
  zb_uindex_t i;
  zb_bool_t res;

  res = ZB_FALSE;

  SET_START_END_POS(start_pos, end_pos, search_mode);

  for (i = start_pos; i < end_pos; i++)
  {
    if (!ZB_CHECK_BIT_IN_BIT_VECTOR(tx_p_i_q->used_mask, i))
    {
      res = ZB_TRUE;
      break;
    }
  }

  return res;
}

#endif  /* ZB_ENABLE_ZGP_TX_QUEUE */
