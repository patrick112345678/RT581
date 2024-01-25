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
/* PURPOSE: APS frames security routines
*/

#define ZB_TRACE_FILE_ID 2463
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "../aps/aps_internal.h"
#include "zb_secur.h"
#include "zdo_diagnostics.h"
#include "zb_bdb_internal.h"

/*! \addtogroup ZB_SECUR */
/*! @{ */

/* let Zigbee TC key could be changed after pack in LIB */
__weak zb_uint8_t ZB_STANDARD_TC_KEY[] = {0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C, 0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39 };

#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
static zb_aps_device_key_pair_set_t * zb_secur_create_hashed_key(zb_ieee_addr_t src_addr);
#endif
static void zb_secur_delete_provisional_key(zb_ieee_addr_t src_addr);
static zb_ret_t zb_aps_unsecure_frame_bykey(zb_aps_device_key_pair_set_t *aps_key, zb_bufid_t buf, zb_ieee_addr_t src_addr);

static zb_uint32_t zb_increase_aps_outgoing_frame_counter(void)
{
  zb_uint32_t out_sec_counter;

  /* FIXME: HACK! Currently individual frame counters are not stored in NVRAM, use one global
   * counter instead - it is stored with lazy intervals
   * Note: it's incompatible with r23
   */
  out_sec_counter = ZB_AIB().outgoing_frame_counter++;
  TRACE_MSG(TRACE_SECUR3, "global outgoing_frame_counter %ld ++", (FMT__L, out_sec_counter));

#if defined ZB_STORE_COUNTERS && !defined ZB_LITE_NO_STORE_APS_COUNTERS
  if ((out_sec_counter % ZB_LAZY_COUNTER_INTERVAL == 0U)
      || out_sec_counter == 1U
      || ZB_U2B(ZB_NVRAM().refresh_flag))
  {
    /* Will store counter + interval */
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_IB_COUNTERS);
    ZB_NVRAM().refresh_flag = ZB_FALSE_U; /* In case it stored in nvram */
  }
#endif

  return out_sec_counter;
}

void zb_secur_aps_aux_hdr_fill(zb_uint8_t *p, zb_secur_key_id_t key_type, zb_bool_t ext_nonce)
{
  /* Casting from zb_uint8_t * to packet struct zb_aps_data_aux_nonce_frame_hdr_t */
  void *tmp = p;
  zb_aps_data_aux_nonce_frame_hdr_t *aux = (zb_aps_data_aux_nonce_frame_hdr_t *)tmp;

  /* See 4.4.1 Frame Security
   * 4.4.1.1 Security Processing of Outgoing Frames
   *
   * The extended nonce sub-field shall be set as follows: If the
   * ApsHeader indicates the frame type is an APS Command, then the
   * extended nonce sub-field shall be set to 1. Otherwise if the
   * TxOptions bit for include extended nonce is set (0x10) then the
   * extended nonce sub-field shall be set to 1. Otherwise it shall be
   * set to 0.
   */
  if (ext_nonce)
  {
    aux->secur_control = ZB_APS_SET_SECUR_CONTROL(5U, key_type, ZB_TRUE);
    ZB_IEEE_ADDR_COPY(aux->source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
  }
  else
  {
    aux->secur_control = ZB_APS_SET_SECUR_CONTROL(5U, key_type, ZB_FALSE);
  }
}


zb_ushort_t zb_aps_secur_aux_size(zb_uint8_t secur_control)
{
  /*
    4.5.1.1 Security Control Field
    Bit 5 is "Extended nonce"
   */
  zb_bool_t is_nonce = !ZB_BIT_IS_SET(secur_control, (1U << 5)) ? ZB_FALSE : ZB_TRUE;

  return (is_nonce ? sizeof(zb_aps_data_aux_nonce_frame_hdr_t) : sizeof(zb_aps_data_aux_frame_hdr_t));
}


void zb_aps_command_add_secur(zb_bufid_t buf, zb_uint8_t command_id, zb_secur_key_id_t secure_aps)
{
  zb_ushort_t hdr_size = sizeof(zb_aps_command_header_t);
  zb_aps_command_header_t *hdr;

  hdr_size += zb_aps_secur_aux_size(ZB_APS_SET_SECUR_CONTROL(5U, secure_aps, ZB_TRUE));

  TRACE_MSG(TRACE_SECUR3, ">>zb_aps_add_secur %hd hdr_size %d", (FMT__H_D, buf, hdr_size));

  hdr = zb_buf_alloc_left(buf, hdr_size);

  hdr->fc = 0;
  /* command id is just after aux hdr - at hdr end*/
  *((zb_uint8_t *)hdr + hdr_size - 1) = command_id;
  if (hdr_size != sizeof(zb_aps_command_header_t))
  {
    ZB_APS_FC_SET_SECURITY(hdr->fc, 1U);
    zb_buf_flags_or(buf, ZB_BUF_SECUR_APS_ENCR);
    zb_secur_aps_aux_hdr_fill(
      /* aux hdr is before command id */
      &hdr->aps_command_id,
      secure_aps,
      ZB_TRUE);
  }
  TRACE_MSG(TRACE_SECUR3, "<<zb_aps_add_secur", (FMT__0));
}


zb_ret_t zb_aps_secure_frame(zb_bufid_t src, zb_uint_t mac_hdr_size, zb_bufid_t dst, zb_bool_t is_tunnel)
{
  zb_uint8_t *payload;
  zb_secur_ccm_nonce_t nonce;
  zb_uint8_t *dp;
  zb_uint8_t key[ZB_CCM_KEY_SIZE];
  zb_ushort_t hdrs_size;
  zb_ret_t ret = RET_OK;
  zb_nwk_hdr_t *nwk_hdr;
  zb_uint8_t *aps_hdr;
  zb_aps_data_aux_nonce_frame_hdr_t *aux;
  zb_aps_device_key_pair_set_t *aps_key = NULL;
  zb_uint32_t out_sec_counter;

  TRACE_MSG(TRACE_SECUR3, ">>zb_aps_secure_frame mac_hdr_size %d src buf %hd dst buf %hd is_tunnel %d",
            (FMT__D_H_H_D, mac_hdr_size, src, dst, is_tunnel));

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,32} */
  nwk_hdr = (zb_nwk_hdr_t *)((zb_uint8_t*)zb_buf_begin(src) + mac_hdr_size);
  if (is_tunnel)
  {
    TRACE_MSG(TRACE_SECUR3, "Tunnel encryption", (FMT__0));
    aps_hdr = zb_buf_begin(src);
  }
  else
  {
    aps_hdr = (zb_uint8_t *)nwk_hdr + (ZB_NWK_HDR_SIZE(nwk_hdr));
  }

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,33} */
  aux = (zb_aps_data_aux_nonce_frame_hdr_t *)(aps_hdr + ZB_APS_HDR_SIZE(*aps_hdr));
  TRACE_MSG(TRACE_SECUR3, "Key type %hd", (FMT__H, ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control)));

  out_sec_counter = zb_increase_aps_outgoing_frame_counter();

  /* See: 4.4.1.1: If the outgoing frame counter has as its value the 4-octet representation of
   * the integer 2^32-1, or if the key cannot be obtained, security processing shall fail and no
   * further security processing shall be done on this frame.
   * But in case when we are using preconfigured APS-keys, we can't drop frame due to lack of
   * pair key
   */
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().secur_aps_counter_hack_cb)
  {
    ZB_CERT_HACKS().secur_aps_counter_hack_cb(&out_sec_counter);
  }
#endif

  ZB_HTOLE32((zb_uint8_t *)&aux->frame_counter, (zb_uint8_t *)&out_sec_counter);

  if (ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control) == ZB_SECUR_DATA_KEY)
  {
    aps_key = zb_secur_get_link_key_pair_set(nwk_hdr->dst_ieee_addr, ZB_TRUE);
    if (aps_key == NULL)
    {
      TRACE_MSG(TRACE_SECUR1, "key pair 'valid_only' not found", (FMT__0));
      aps_key = zb_secur_create_best_suitable_link_key_pair_set(nwk_hdr->dst_ieee_addr);
      if (aps_key != NULL)
      {
        TRACE_MSG(TRACE_SECUR1, "key found by zb_secur_create_best_suitable_link_key_pair_set()", (FMT__0));
      }
    }
    if (aps_key == NULL)
    {
      TRACE_MSG(TRACE_SECUR1, "Error adding key pair entry", (FMT__0));
      ret = RET_ERROR;
    }
    else
    {
      ZB_MEMCPY(key, aps_key->link_key, ZB_CCM_KEY_SIZE);
    }
  }
  else
  {
    zb_uint8_t n = 0;
    if (ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control) == ZB_SECUR_KEY_TRANSPORT_KEY)
    {
      zb_apsme_transport_key_req_t *req;

#ifdef ZB_CERTIFICATION_HACKS
      if (ZB_CERT_HACKS().use_transport_key_for_aps_frames)
      {
        aps_key = zb_secur_get_link_key_by_address(nwk_hdr->dst_ieee_addr, ZB_SECUR_PROVISIONAL_KEY);
        ZB_CERT_HACKS().use_transport_key_for_aps_frames = ZB_FALSE;
      }
      else
#endif
      {
        /* key-transport key is used to protect transported network keys */
        /* There can be only transport key aps command */
        req = ZB_BUF_GET_PARAM(src, zb_apsme_transport_key_req_t);
        /* Old logic got from pre-r21 code: get destination address from
         * apsme_transport_key. Seems it works. */
        TRACE_MSG(TRACE_SECUR3, "key-transport key dest from req " TRACE_FORMAT_64 " dest from nwk hdr "  TRACE_FORMAT_64,
                  (FMT__A_A, TRACE_ARG_64(req->dest_address.addr_long), TRACE_ARG_64(nwk_hdr->dst_ieee_addr)));

        aps_key = zb_secur_get_verified_or_provisional_link_key(req->dest_address.addr_long);

#ifdef ZB_DISTRIBUTED_SECURITY_ON
        if (aps_key == NULL)
        {
          if (IS_DISTRIBUTED_SECURITY())
          {
            /* maybe we already created keypair for distributed key */
            aps_key = zb_secur_get_link_key_pair_set((zb_uint8_t*)g_unknown_ieee_addr, ZB_FALSE);

            if (!aps_key)
            {
              /* if in distributed mode, use distributed global key */
              aps_key = zb_secur_update_key_pair(req->dest_address.addr_long, ZB_AIB().tc_standard_distributed_key,
                ZB_SECUR_GLOBAL_KEY, ZB_SECUR_PROVISIONAL_KEY, ZB_SECUR_KEY_SRC_UNKNOWN);
              TRACE_MSG(TRACE_SECUR1, "Created aps_key %p from default distributed security key", (FMT__P, aps_key));
            }
          }
        }
#endif  /* ZB_DISTRIBUTED_SECURITY_ON */
      }
      TRACE_MSG(TRACE_SECUR3, "aps_key %p", (FMT__P, aps_key));
    }
    else if (ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control) == ZB_SECUR_KEY_LOAD_KEY)
    {
      /* FIXME: eliminate copy-paste! */
      zb_apsme_transport_key_req_t *req = ZB_BUF_GET_PARAM(src, zb_apsme_transport_key_req_t);

      TRACE_MSG(TRACE_SECUR3, "key-key load dest from req " TRACE_FORMAT_64 " dest from nwk hdr "  TRACE_FORMAT_64,
                (FMT__A_A, TRACE_ARG_64(req->dest_address.addr_long), TRACE_ARG_64(nwk_hdr->dst_ieee_addr)));
      aps_key = zb_secur_get_link_key_pair_set(req->dest_address.addr_long, ZB_TRUE);
      if (aps_key == NULL)
      {
        aps_key = zb_secur_create_best_suitable_link_key_pair_set(req->dest_address.addr_long);
      }
      n = 2;
    }
#ifdef ZB_CERTIFICATION_HACKS
    else if ( (ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control) == ZB_SECUR_NWK_KEY) &&
              ZB_CERT_HACKS().allow_nwk_encryption_for_aps_frames )
    {
      zb_uint8_t *nwk_key = secur_nwk_key_by_seq(ZB_NIB().active_key_seq_number);
      if (!nwk_key)
      {
        TRACE_MSG(TRACE_ERROR, "Can't get nwk key by seq# %hd", (FMT__H, ZB_NIB().active_key_seq_number));
        ret = RET_ERROR;
      }
      else
      {
        ZB_MEMCPY(key, nwk_key, ZB_CCM_KEY_SIZE);
      }
      /* nwk key used to protect aps frame - required by bdb tests: cs-nfs-tc-01 */
      TRACE_MSG(TRACE_SECUR3, "nwk key", (FMT__0));
    }
#endif
    else
    {
      TRACE_MSG(TRACE_SECUR1, "Wrong key type %hd - drop frame", (FMT__H, (zb_uint8_t)ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control)));
      ret = RET_ERROR;
    }
    if (aps_key == NULL
#ifdef ZB_CERTIFICATION_HACKS
        && !(ZB_CERT_HACKS().allow_nwk_encryption_for_aps_frames &&
             (ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control) == ZB_SECUR_NWK_KEY) )
#endif
       )
    {
      TRACE_MSG(TRACE_SECUR1, "Error adding key pair entry", (FMT__0));
      ret = RET_ERROR;
    }
    if (ret == RET_OK)
    {
#ifdef ZB_CERTIFICATION_HACKS
      if ( ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control) != ZB_SECUR_NWK_KEY )
#endif
      {
        /* hashed key pair key for transport key command */
        zb_cmm_key_hash(aps_key->link_key, n, key);
        TRACE_MSG(TRACE_SECUR3, "use key " TRACE_FORMAT_128 " hashed with %d " TRACE_FORMAT_128,
                  (FMT__B_H_B, TRACE_ARG_128(aps_key->link_key), n, TRACE_ARG_128(key)));
      }
#ifdef ZB_CERTIFICATION_HACKS
      else
      {
        /* do not hash nwk key */
        TRACE_MSG(TRACE_SECUR3, "use nwk key " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(key)));
      }
#endif
    }
  }
  if (ret == RET_OK)
  {
    payload = (zb_uint8_t *)aux + zb_aps_secur_aux_size(aux->secur_control);

    /* fill nonce - see 4.5.2.2 */

    /* frame counter in aux is in the right endian already */
    nonce.frame_counter = aux->frame_counter;
    nonce.secur_control = aux->secur_control;
    ZB_IEEE_ADDR_COPY(nonce.source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());

    hdrs_size = (zb_ushort_t)(payload - (zb_uint8_t *)zb_buf_begin(src));

    TRACE_MSG(TRACE_SECUR3, "secure aps_hdr %p size %hd payload %p size %hd", (FMT__P_H_P_H,
                                                                            (zb_uint8_t *)aps_hdr, (payload - aps_hdr),
                                                                            (zb_uint8_t *)payload, (zb_buf_len(src) - hdrs_size)));

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().break_aps_key)
    {
      TRACE_MSG(TRACE_ERROR, "break_aps_key", (FMT__0));
      /* the key[] is local array, can distort it in place */
      key[1] ^= 0xF0;
      /* switch off the action */
      ZB_CERT_HACKS().break_aps_key = ZB_FALSE;
    }
#endif /* ZB_CERTIFICATION_HACKS */

    DUMP_TRAF("before: a", (zb_uint8_t *)aps_hdr, (payload - aps_hdr), 0);
    DUMP_TRAF("before: m", (zb_uint8_t *)payload, (zb_buf_len(src) - hdrs_size), 0);
    DUMP_TRAF("key", (zb_uint8_t *)key, 16, 0);
    DUMP_TRAF("nonce", (zb_uint8_t *)&nonce, 13, 0);

    /* Secure  */
    (void)zb_ccm_encrypt_n_auth(key,
                                (zb_uint8_t *)&nonce,
                                (zb_uint8_t *)aps_hdr,
                                (zb_ushort_t)(payload - aps_hdr),
                                (zb_uint8_t *)payload,
                                (zb_buf_len(src) - hdrs_size),
                                dst);
    DUMP_TRAF("after: a", zb_buf_begin(dst), zb_buf_len(dst), 0);

    dp = zb_buf_alloc_left(dst, (zb_uint32_t)(aps_hdr - (zb_uint8_t *)zb_buf_begin(src)));
    /* copy headers */
    ZB_MEMCPY(dp, zb_buf_begin(src), (zb_uint32_t)(aps_hdr - (zb_uint8_t *)zb_buf_begin(src)));
    /* clear security level - see 4.4.1.1/11 */
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,34} */
    aux = (zb_aps_data_aux_nonce_frame_hdr_t *)((zb_uint8_t*)zb_buf_begin(dst) + ((zb_uint8_t*)aux - (zb_uint8_t*)zb_buf_begin(src)));
    ZB_SECUR_SET_ZEROED_LEVEL(aux->secur_control);

    /* hold the flags ZB_BUF_HAS_APS_PAYLOAD and ZB_BUF_HAS_APS_USER_PAYLOAD further */
    ZB_BUF_COPY_FLAG_APS_PAYLOAD(dst, src);

    TRACE_MSG(TRACE_SECUR3, "secured aps frm %p[%hd] -> %p hdrs_size %hd frame_counter %lx",
              (FMT__P_H_P_H_L,
               src, zb_buf_len(src), dst, hdrs_size, aux->frame_counter));
    TRACE_MSG(TRACE_SECUR3, "<<zb_aps_secure_frame %hd", (FMT__H, ret));
  }
  return ret;
}


zb_ret_t zb_aps_unsecure_frame(zb_bufid_t buf, zb_uint16_t *keypair_i_p, zb_secur_key_id_t *key_id_p, zb_bool_t *is_verified_tclk)
{
  zb_ret_t ret = RET_OK;
  zb_aps_device_key_pair_set_t *aps_key;
  zb_aps_device_key_pair_set_t *provisional_aps_key = NULL;
#ifdef ZB_DISTRIBUTED_SECURITY_ON
  zb_aps_device_key_pair_set_t *distributed_aps_key = NULL;
#endif
  zb_ieee_addr_t src_addr;
  zb_bool_t key_created = ZB_FALSE;
#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
  zb_bool_t have_verified_key = ZB_FALSE;
#endif /* TC_SWAPOUT && !ZB_COORDINATOR_ONLY */

  TRACE_MSG(TRACE_SECUR1, ">>zb_aps_unsecure_frame buf %hd", (FMT__H, buf));
  *is_verified_tclk = ZB_FALSE;
  {
    zb_uint8_t *aps_hdr;
    zb_aps_data_aux_nonce_frame_hdr_t *aux;
    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(buf, zb_apsde_data_indication_t);

    /* We can have 1 or 2 keys per address (old verified + provisional or
     * unverified. Not sure which one peer can use, so try both.  */

    aps_hdr = zb_buf_begin(buf);
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,35} */
    aux = (zb_aps_data_aux_nonce_frame_hdr_t *)(aps_hdr + ZB_APS_HDR_SIZE(*aps_hdr));
    ZB_SECUR_SET_SECURITY_LEVEL(aux->secur_control, 5U);

    /* Get key type used to protect frame to pass up */
    *key_id_p = (zb_secur_key_id_t)ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control);

    /* Get source address either from aux hdr or by short source address */
    TRACE_MSG(TRACE_SECUR3, "security control field 0x%x", (FMT__H, aux->secur_control));
    if (ZB_SECUR_GET_SECURITY_NONCE(aux->secur_control) != 0U)
    {
      ZB_IEEE_ADDR_COPY(src_addr, aux->source_address);
      TRACE_MSG(TRACE_SECUR3, "using ieee from aux hdr: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(src_addr)));
    }
    else
    {
      if (zb_address_ieee_by_short(ind->src_addr, src_addr) != RET_OK)
      {
        TRACE_MSG(TRACE_SECUR3, "can't get addr by short %d: can't decrypt!", (FMT__D, ind->src_addr));
        ret = RET_NOT_FOUND;
      }
      else
      {
        TRACE_MSG(TRACE_SECUR3, "get ieee by ref: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(src_addr)));
      }
    }
  }

  if (ret == RET_OK)
  {
    ret = RET_UNAUTHORIZED;
    provisional_aps_key = zb_secur_get_link_key_by_address(src_addr, ZB_SECUR_PROVISIONAL_KEY);
    if (provisional_aps_key != NULL)
    {
      TRACE_MSG(TRACE_SECUR3, "Try unsecure by existing provisional_aps_key %p", (FMT__P, provisional_aps_key));
      ret = zb_aps_unsecure_frame_bykey(provisional_aps_key, buf, src_addr);
    }
  }
  if (ret == RET_UNAUTHORIZED)
  {
    aps_key = zb_secur_get_link_key_by_address(src_addr, ZB_SECUR_VERIFIED_KEY);
    if (aps_key != NULL)
    {
#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
      have_verified_key = ZB_TRUE;
#endif /* TC_SWAPOUT && !ZB_COORDINATOR_ONLY */
      TRACE_MSG(TRACE_SECUR3, "Try unsecure by verified aps_key %p", (FMT__P, aps_key));
      ret = zb_aps_unsecure_frame_bykey(aps_key, buf, src_addr);
      *is_verified_tclk = (zb_bool_t)
        (
        ret == RET_OK
#if defined ZB_DISTRIBUTED_SECURITY_ON
        && !IS_DISTRIBUTED_SECURITY()
#endif
        && ((ZB_ZDO_NODE_DESC()->server_mask & ZB_PRIMARY_TRUST_CENTER) != 0U
            || ZB_IEEE_ADDR_CMP(src_addr, ZB_AIB().trust_center_address))
        );
    }
  }
  if (ret == RET_UNAUTHORIZED)
  {
    aps_key = zb_secur_get_link_key_by_address(src_addr, ZB_SECUR_UNVERIFIED_KEY);
    if (aps_key != NULL)
    {
      TRACE_MSG(TRACE_SECUR3, "Try unsecure by unverified aps_key %p", (FMT__P, aps_key));
      ret = zb_aps_unsecure_frame_bykey(aps_key, buf, src_addr);
    }
  }
#ifdef ZB_DISTRIBUTED_SECURITY_ON
  if (ret == RET_UNAUTHORIZED)
  {
    /* Try -1 as source address: this is Desiributed global */
    aps_key = distributed_aps_key = zb_secur_get_link_key_by_address((zb_uint8_t*)g_unknown_ieee_addr, ZB_SECUR_PROVISIONAL_KEY);
    if (distributed_aps_key)
    {
      TRACE_MSG(TRACE_SECUR3, "Try unsecure by Distributed default aps_key %p", (FMT__P, aps_key));
      ret = zb_aps_unsecure_frame_bykey(distributed_aps_key, buf, src_addr);
    }
  }
#endif
  if (ret == RET_UNAUTHORIZED)
  {
    zb_ret_t ret_addr;
    zb_address_ieee_ref_t ref;

    /* In case when we joins to network via router we don't know TC long address and
       can not create link key due address does not stored into addr_map.
     */
    ret_addr = zb_address_by_ieee(src_addr, ZB_TRUE, ZB_FALSE, &ref);
    ZB_ASSERT(ret_addr == RET_OK);

#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
    if (!have_verified_key)
    {
      /* In case of TC swap use TCLK to the old TC */
      aps_key = zb_secur_get_link_key_by_address(ZB_AIB().trust_center_address, ZB_SECUR_VERIFIED_KEY);

      if (aps_key != NULL)
      {
        have_verified_key = ZB_TRUE;
      }
    }

    if (have_verified_key
        && !ZB_IS_TC()
        && *key_id_p == ZB_SECUR_KEY_TRANSPORT_KEY)
    {
      /* Keep Verified key. Drop existing Provisional key, if any. Create new
       * Provosional key as a hash of Verified key. */
      aps_key = zb_secur_create_hashed_key(src_addr);
      /* Already checked that we have verified key */
      ZB_ASSERT(aps_key != NULL);
      ret = zb_aps_unsecure_frame_bykey(aps_key, buf, src_addr);
      if (ret == RET_UNAUTHORIZED
          || ret == RET_ERROR)
      {
        TRACE_MSG(TRACE_SECUR1, "Can't unsecure by hashed key", (FMT__0));
        /* Delete hashed key if can't decrypt. Will try IC or zigbee09 below. */
        zb_secur_delete_provisional_key(src_addr);
      }
      else
      {
        TRACE_MSG(TRACE_SECUR1, "Unsecured by hashed TCLK - TC swapout detected!", (FMT__0));
        ZB_TCPOL().tc_swapped = ZB_TRUE;
        if (ret == RET_OUT_OF_RANGE)
        {
          /* ignore counters error in case of TC swap */
          zb_uint8_t idx = ZB_AIB().aps_device_key_pair_storage.cached_i;
          ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].incoming_frame_counter = 0;
          ret = RET_OK;
          TRACE_MSG(TRACE_SECUR1, "Ignire counters error after TC swapout", (FMT__0));
        }
        /* Indicate TC swapout to the app later, when successfully processed NWK key transport
           in zdo_commissioning_authenticated() */
      }
    }
#endif
  }

  if (ret == RET_UNAUTHORIZED
      && provisional_aps_key == NULL)
  {
    /* Create new provisional keypair set using IC or zigbee09. */
    aps_key = zb_secur_create_best_suitable_link_key_pair_set(src_addr);
    if (aps_key == NULL)
    {
      TRACE_MSG(TRACE_SECUR1, "Can't create key pair set - drop packet", (FMT__0));
      ret = RET_ERROR;
    }
    else
    {
      TRACE_MSG(TRACE_SECUR3, "Created new provisional aps_key %p; try unsecure", (FMT__P, aps_key));
      key_created = ZB_TRUE;
      ret = zb_aps_unsecure_frame_bykey(aps_key, buf, src_addr);
    }

    /* Do not delete created key if not unsecured - will reuse keypair below.  */
  }
#ifdef ZB_DISTRIBUTED_SECURITY_ON
  if (ret == RET_UNAUTHORIZED
      && !distributed_aps_key)
  {
    zb_address_ieee_ref_t ref;

    /* There is no TC in distributed network - store unknown address. */
    zb_address_by_ieee((zb_uint8_t*)g_unknown_ieee_addr, ZB_TRUE, ZB_FALSE, &ref);
    /* Create new provisional keypair set */
    aps_key = zb_secur_create_best_suitable_link_key_pair_set((zb_uint8_t*)g_unknown_ieee_addr);
    if (!aps_key)
    {
      TRACE_MSG(TRACE_SECUR1, "Can't create key pair set - drop packet", (FMT__0));
      ret = RET_ERROR;
    }
    else
    {
      TRACE_MSG(TRACE_SECUR3, "Created new distributed provisional aps_key %p; try unsecure", (FMT__P, aps_key));
      ret = zb_aps_unsecure_frame_bykey(aps_key, buf, src_addr);

      if (ret == RET_UNAUTHORIZED)
      {
        zb_secur_delete_link_keys_by_long_addr((zb_uint8_t*)g_unknown_ieee_addr);
      }
    }
  }
#endif
/*cstat -MISRAC2012-Rule-2.1_b -MISRAC2012-Rule-14.3_b */
  if (ret == RET_UNAUTHORIZED
      && !ZB_JOIN_USES_INSTALL_CODE_KEY(ZB_TRUE)
    )
  {
    /* Last chance to unsecure - try TC standard key */
    /*cstat +MISRAC2012-Rule-2.1_b +MISRAC2012-Rule-14.3_b*/
    /** @mdr{00014,0} */
    aps_key = zb_secur_update_key_pair(src_addr, ZB_AIB().tc_standard_key, ZB_SECUR_GLOBAL_KEY, ZB_SECUR_PROVISIONAL_KEY, ZB_SECUR_KEY_SRC_UNKNOWN);

    if (aps_key == NULL)
    {
      TRACE_MSG(TRACE_SECUR1, "Can't create key pair set - drop packet", (FMT__0));
      ret = RET_ERROR;
    }
    else
    {
      TRACE_MSG(TRACE_SECUR3, "Created new TC standard provisional aps_key %p; try unsecure", (FMT__P, aps_key));
      ret = zb_aps_unsecure_frame_bykey(aps_key, buf, src_addr);
    }
  }

  /* If key was created/updated and frame is not unsecured, delete this key.
     WARNING: We may update (in zb_secur_create_best_suitable_link_key_pair_set() or on the last
     try) and then delete provisional key which was created before but decrypt with it was
     failed. Looks like it is ok... Is it possible to have multiple provisional keys?
  */
  if (key_created && ret == RET_UNAUTHORIZED)
  {
    zb_secur_delete_provisional_key(src_addr);
  }

  if (ret == RET_OK)
  {
    *keypair_i_p = ZB_AIB().aps_device_key_pair_storage.cached_i;
    TRACE_MSG(TRACE_SECUR1, "aps unsecured ok by keypair_i %d", (FMT__D, *keypair_i_p));
  }
  else if (ret == RET_OUT_OF_RANGE)
  {
    /* RET_OUT_OF_RANGE means frame_counter checks fail.
       Do not increment counter ZDO_DIAGNOSTICS_APS_DECRYPT_FAILURES_ID */
    ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APSFC_FAILURE_ID);
  }
  else
  {
    ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_DECRYPT_FAILURES_ID);
  }

  return ret;
}


static void zb_secur_delete_provisional_key(zb_ieee_addr_t src_addr)
{
  zb_uint16_t keypair_i = zb_aps_keypair_get_index_by_addr(src_addr, ZB_SECUR_PROVISIONAL_KEY);
  if (keypair_i != (zb_uint16_t)-1)
  {
    TRACE_MSG(TRACE_SECUR2, "Delete provisioning key idx %d for addr " TRACE_FORMAT_64,
              (FMT__D_A, keypair_i, TRACE_ARG_64(src_addr)));
    zb_secur_delete_link_key_by_idx(keypair_i);
  }
}


#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
static zb_aps_device_key_pair_set_t * zb_secur_create_hashed_key(zb_ieee_addr_t src_addr)
{
  zb_aps_device_key_pair_set_t *verified_key = zb_secur_get_link_key_by_address(src_addr, ZB_SECUR_VERIFIED_KEY);
  zb_aps_device_key_pair_set_t *hashed_key = NULL;

  if (verified_key == NULL)
  {
    verified_key = zb_secur_get_link_key_by_address(ZB_AIB().trust_center_address, ZB_SECUR_VERIFIED_KEY);
  }

  if (verified_key != NULL)
  {
    zb_uint8_t key[ZB_CCM_KEY_SIZE];

    (void)zb_sec_b6_hash(verified_key->link_key, ZB_CCM_KEY_SIZE, key);
    /* Just in case we create a provisional key 1 step before when trying to decode. */
    zb_secur_delete_provisional_key(src_addr);
    /* IN case of TC swapout create new provisional key as a hash of previous verified TCLK */
    hashed_key = zb_secur_update_key_pair(src_addr, key, ZB_SECUR_UNIQUE_KEY, ZB_SECUR_PROVISIONAL_KEY, ZB_SECUR_KEY_SRC_UNKNOWN);
  }

  return hashed_key;
}
#endif  /* TC_SWAPOUT */


static zb_ret_t zb_aps_unsecure_frame_bykey(zb_aps_device_key_pair_set_t *aps_key, zb_bufid_t buf, zb_ieee_addr_t src_addr)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t key[ZB_CCM_KEY_SIZE];
  zb_ushort_t a_size;
  zb_uint8_t *aps_hdr;
  zb_aps_data_aux_nonce_frame_hdr_t *aux;
	#if 0
  zb_uint8_t standard_key[ZB_CCM_KEY_SIZE] = ZB_STANDARD_TC_KEY;
	#else
  zb_uint8_t standard_key[ZB_CCM_KEY_SIZE];
  ZB_MEMCPY(&(standard_key), ZB_STANDARD_TC_KEY, ZB_CCM_KEY_SIZE);	
	#endif
	
  TRACE_MSG(TRACE_SECUR2, ">>zb_aps_unsecure_frame_bykey aps_key %p buf %hu", (FMT__P_H, aps_key, buf));

  aps_hdr = zb_buf_begin(buf);

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,36} */
  aux = (zb_aps_data_aux_nonce_frame_hdr_t *)(aps_hdr + ZB_APS_HDR_SIZE(*aps_hdr));
  a_size = ((zb_uint8_t *)aux - aps_hdr) + zb_aps_secur_aux_size(aux->secur_control);

  TRACE_MSG(TRACE_SECUR1, "aux->secur_control 0x%x a_size %hd", (FMT__H_H, aux->secur_control, a_size));
  if (ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control) == ZB_SECUR_DATA_KEY)
  {
    ZB_MEMCPY(key, aps_key->link_key, ZB_CCM_KEY_SIZE);
    TRACE_MSG(TRACE_SECUR3, "data key " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(key)));
  }
  else
  {
    zb_uint8_t n = 0;
    if (ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control) == ZB_SECUR_KEY_TRANSPORT_KEY)
    {
      TRACE_MSG(TRACE_SECUR3, "key-transport key", (FMT__0));

      if (!zb_aib_tcpol_get_allow_unsecure_tc_rejoins())
      {
          if (COMM_SELECTOR().is_in_tc_rejoin != NULL)
          {
              if ((zb_bool_t)COMM_SELECTOR().is_in_tc_rejoin()
                  && (ZB_MEMCMP(standard_key, aps_key->link_key, ZB_CCM_KEY_SIZE) == 0))
              {
                  TRACE_MSG(TRACE_SECUR1, "Don't use ZB_TC_STANDARD_KEY for decrypting in case of TC rejoin",
                            (FMT__0));
                  ret = RET_UNAUTHORIZED;
              }
          }
      }
    }
    else if (ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control) == ZB_SECUR_KEY_LOAD_KEY)
    {
      TRACE_MSG(TRACE_SECUR3, "key-load key", (FMT__0));
      n = 2;
    }
    else
    {
      TRACE_MSG(TRACE_SECUR1, "Wrong key type %hd - drop frame", (FMT__H, (zb_uint8_t)ZB_SECUR_AUX_HDR_GET_KEY_TYPE(aux->secur_control)));
      ret = RET_ERROR;
    }
    if (ret == RET_OK)
    {
      /* hashed key pair key for transport key command */
      zb_cmm_key_hash(aps_key->link_key, n, key);
      TRACE_MSG(TRACE_SECUR3, "use key attr %hd " TRACE_FORMAT_128 " hashed with %d " TRACE_FORMAT_128, (FMT__H_B_H_B, aps_key->key_attributes, TRACE_ARG_128(aps_key->link_key), n, TRACE_ARG_128(key)));
    }
  }

  if (ret == RET_OK)
  {
    /* decrypt */
    zb_secur_ccm_nonce_t nonce = {0};
    ZB_IEEE_ADDR_COPY(&nonce.source_address, src_addr);
    nonce.frame_counter = aux->frame_counter;
    nonce.secur_control = aux->secur_control;

    TRACE_MSG(TRACE_SECUR3, "a_size %hd, unsecure frm %p[%hd] c_len %hd",
              (FMT__H_P_H_H, a_size, buf, zb_buf_len(buf),
               (zb_uint8_t)(((zb_uint8_t*)zb_buf_begin(buf) + zb_buf_len(buf)) - (aps_hdr + a_size))));

    DUMP_TRAF("Array aps decrypt before", (zb_uint8_t *)zb_buf_begin(buf), zb_buf_len(buf), 0);
    DUMP_TRAF("key", (zb_uint8_t *)key, 16, 0);
    DUMP_TRAF("nonce", (zb_uint8_t *)&nonce, 13, 0);
    ret = zb_ccm_decrypt_n_auth_stdsecur(key,
                                         (zb_uint8_t *)&nonce,
                                         buf, a_size,
                                         ((zb_uint8_t*)zb_buf_begin(buf) + zb_buf_len(buf)) - (aps_hdr + a_size));
    DUMP_TRAF("Array aps decrypt after", (zb_uint8_t *)zb_buf_begin(buf), zb_buf_len(buf), 0);

    if (ret == RET_OK)
    {
      TRACE_MSG(TRACE_SECUR3, "unsecured frm %p[%hd] ok", (FMT__P_H, buf, zb_buf_len(buf)));
      /* Set APS encryption for upper layers */
      zb_buf_flags_or(buf, ZB_BUF_SECUR_APS_ENCR);
    }
    else
    {
      TRACE_MSG(TRACE_SECUR3, "unsecure failed aps_key %p", (FMT__P, aps_key));
      ret = RET_UNAUTHORIZED;
    }
  }

  /* Check APS frame counter. Note: we may try wrong key, so counter is also
   * wrong. This is not a problem - let's try another key. */
#ifndef ZB_NO_CHECK_INCOMING_SECURE_APS_FRAME_COUNTERS
  if (ret == RET_OK)
  {
    zb_uint32_t frame_counter;
    ZB_LETOH32((zb_uint8_t *)&frame_counter, (zb_uint8_t *)&aux->frame_counter);
    if (frame_counter == (zb_uint32_t)~0U - 1U)
    {
      TRACE_MSG(TRACE_SECUR1, "Drop frame with counter %ld", (FMT__L, frame_counter));
      ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APSFC_FAILURE_ID);
      ret = RET_ERROR;
    }
    /* compare counters */
    else
    {
      zb_uint8_t idx = ZB_AIB().aps_device_key_pair_storage.cached_i;
      zb_uint32_t in_frame_counter = ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].incoming_frame_counter;

      if (frame_counter < in_frame_counter)
      {
        TRACE_MSG(TRACE_SECUR3,
                  "aps_key %p check aps sec frame counters failed(pair counter): in %hd wait >= %hd ---> drop packet",
                  (FMT__P_H_H,
                   aps_key,
                   frame_counter,
                   in_frame_counter));
        ret = RET_OUT_OF_RANGE;
        /* If the frame counter error results in a decrypt frame error,
           then two diag counters will increase simultaneously, this one
           and ZDO_DIAGNOSTICS_APS_DECRYPT_FAILURES_ID.
           Therefore, ret == RET_OUT_OF_RANGE is checked further.
        ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APSFC_FAILURE_ID);
        */
      }
      else
      {
        ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].incoming_frame_counter = frame_counter;
        TRACE_MSG(TRACE_SECUR3, "aps_key %p assign aps sec frame counter %u",
                  (FMT__P_D, aps_key, frame_counter));
      }
    }
  }
#endif  /* ZB_NO_CHECK_INCOMING_SECURE_APS_FRAME_COUNTERS */

  TRACE_MSG(TRACE_SECUR1, "<<zb_aps_unsecure_frame_bykey %d", (FMT__D, ret));
  return ret;
}


/*! @} */
