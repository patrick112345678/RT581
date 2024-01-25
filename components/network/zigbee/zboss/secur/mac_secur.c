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
/* PURPOSE: MAC security routines
*/

#define ZB_TRACE_FILE_ID 2464
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_secur.h"

#ifdef ZB_MAC_SECURITY

/*! \addtogroup ZB_SECUR */
/*! @{ */

/**
   Secure MAC frame. Frame already have aux header filled

   @param src - source (unencrypted) frame
   @param mac_hdr_size_wo_aux size of mac hdr, without aux security header
   @param dst - destination (encrypted) frame
 */
zb_ret_t zb_mac_secure_frame(zb_bufid_t src, zb_uint_t mac_hdr_size, zb_bufid_t dst)
{
  zb_ret_t ret = RET_OK;
  zb_secur_ccm_nonce_t nonce;
  zb_uint8_t *aux;

  ZB_CHK_ARR(ZB_BUF_BEGIN(src), zb_buf_len(src));

  aux = ZB_BUF_BEGIN(src) + mac_hdr_size;
  mac_hdr_size += MAC_SECUR_LEV5_KEYID1_AUX_HDR_SIZE;

  /* nonce - see 7.6.3.2 CCM* Nonce. */
  ZB_IEEE_ADDR_COPY(nonce.source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
  /* frame counter */
  ZB_MEMCPY(&nonce.frame_counter, aux+1, 4);
  nonce.secur_control = ZB_MAC_SECURITY_LEVEL;

  /* Secure. zb_ccm_encrypt_n_auth() allocs space for a and m, but not mhr.  */
  ret = zb_ccm_encrypt_n_auth(MAC_PIB().mac_key,
                              (zb_uint8_t *)(&nonce),
                              ZB_BUF_BEGIN(src), mac_hdr_size, /* a */
                              ZB_BUF_BEGIN(src) + mac_hdr_size, zb_buf_len(src) - mac_hdr_size, /* m */
                              dst);

  /* hold the flags ZB_BUF_HAS_APS_PAYLOAD and ZB_BUF_HAS_APS_USER_PAYLOAD further */
  ZB_BUF_COPY_FLAG_APS_PAYLOAD(dst, src);

  TRACE_MSG(TRACE_SECUR2, "ret zb_mac_secure_frame %hd", (FMT__D, ret));
  return ret;
}


zb_ret_t zb_mac_unsecure_frame(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;
  zb_ushort_t i;
  zb_secur_ccm_nonce_t nonce;
  zb_ushort_t mhr_len;
  zb_ushort_t is_long_addr;

  {
    zb_mac_mhr_t mhr;

    mhr_len = zb_parse_mhr(&mhr, param);
    is_long_addr = (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control) == ZB_ADDR_64BIT_DEV);

    /* fill nonce, check frame counter */

    /* to fill nonce, needs long address. If device or its address absent in
     * macDeviceTable, can't decrypt. */

    /* find dev in device table: need long address and packets counter. */
    for (i = 0 ; i < MAC_PIB().mac_device_table_entries ; ++i)
    {
      TRACE_MSG(TRACE_SECUR2, "i %hd is_long %hd panid 0x%x mhr_panid 0x%x",
                (FMT__H_H_D_D, i, is_long_addr, MAC_PIB().mac_device_table[i].pan_id, mhr.src_pan_id));

      if (MAC_PIB().mac_device_table[i].pan_id == mhr.src_pan_id)
      {
        if (is_long_addr)
        {
          if (ZB_IEEE_ADDR_CMP(MAC_PIB().mac_device_table[i].long_address, mhr.src_addr.addr_long))
          {
            break;
          }
        }
        else
        {
          TRACE_MSG(TRACE_SECUR2, "i %hd addr 0x%x mhr_addr 0x%x",
                    (FMT__H_D_D, i, MAC_PIB().mac_device_table[i].short_address, mhr.src_addr.addr_short));
          if (MAC_PIB().mac_device_table[i].short_address == mhr.src_addr.addr_short)
          {
            break;
          }
        }
      }
    }
    if (i == MAC_PIB().mac_device_table_entries)
    {
      /* no such device - can't decrypt */
      TRACE_MSG(TRACE_SECUR1, "device not found - MAC unsecure failed", (FMT__0));
      ret = RET_ERROR;
    }
    else if (MAC_PIB().mac_device_table[i].frame_counter > mhr.frame_counter
             || MAC_PIB().mac_device_table[i].frame_counter == (zb_uint32_t)~0)
    {
      ret = RET_ERROR;
      TRACE_MSG(TRACE_SECUR1, "frm cnt %ld->%ld shift back - MAC unsecure failed",
                (FMT__L_L, MAC_PIB().mac_device_table[i].frame_counter > mhr.frame_counter));
    }

    if (ret == RET_OK)
    {
      /* nonce - see 7.6.3.2 CCM* Nonce. */
      ZB_IEEE_ADDR_COPY(nonce.source_address, MAC_PIB().mac_device_table[i].long_address);
      ZB_HTOLE32(&nonce.frame_counter, &mhr.frame_counter);
      nonce.secur_control = ZB_MAC_SECURITY_LEVEL;
    }
  }

  if (ret == RET_OK)
  {
    zb_uint8_t save_tail[ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME];
    zb_uint8_t *p;
    ZB_MEMCPY(save_tail,
              zb_buf_begin(param) + zb_buf_len(buf) - ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME,
              ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME);
    ZB_BUF_CUT_RIGHT(buf, ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME);

    ret = zb_ccm_decrypt_n_auth_stdsecur(MAC_PIB().mac_key,
                                         (zb_uint8_t *)&nonce,
                                         buf, mhr_len,
                                         zb_buf_len(buf) - mhr_len);
    TRACE_MSG(TRACE_SECUR3, "MAC packet unsecure ret %d", (FMT__D, ret));
    ZB_BUF_ALLOC_RIGHT(buf, ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME, p);
    ZB_MEMCPY(p, save_tail, ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME);
    /* Note: FCS became invalid, but we never use it. */
  }
  return ret;
}

#endif  /* ZB_MAC_SECURITY */

/*! @} */
