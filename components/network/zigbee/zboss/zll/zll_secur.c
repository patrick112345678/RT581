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
/* PURPOSE: Security support in ZLL
*/


#define ZB_TRACE_FILE_ID 2121
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"

#ifdef ZB_ENABLE_ZLL

/*! \addtogroup ZB_SECUR */
/*! @{ */

/* ZLL security, see ZLL specification 8.7.1 */

/* 04/03/2018 EE CR:MINOR It looks like that routine duplicates Distributed security.
   TODO: modify ZLL code to use Distributed security feature from r21 */
zb_uint8_t zll_calc_enc_dec_nwk_key(zb_uint8_t *key_encrypted,
                                    zb_uint8_t *key_decrypted,
                                    zb_uint16_t key_info,
                                    zb_uint32_t transaction_id,
                                    zb_uint32_t response_id,
                                    zb_bool_t is_out)

{
  zb_uint8_t key_idx = (zb_uint8_t)~0;
  zb_uint8_t *key = NULL;
  zb_uint8_t transport_key[ZB_CCM_KEY_SIZE];

  TRACE_MSG(TRACE_ZLL1, ">> zll_calc_enc_dec_nwk_key key_info %d transaction_id 0x%x response_id 0x%x is_out %d",
            (FMT__D_D_D_D, key_info, transaction_id, response_id, is_out));
  do
  {
    if (!key_encrypted)
    {
      TRACE_MSG(TRACE_ZLL1, "error: key_encrypted is null", (FMT__0));
      break;
    }
    if (!key_decrypted)
    {
      TRACE_MSG(TRACE_ZLL1, "error: key_decrypted is null", (FMT__0));
      break;
    }

    if (is_out)
    {
      /* here we process key bitmask from incoming scan response */
      zb_uint16_t common_bitmask = key_info & ZLL_DEVICE_INFO().key_info;
      if (!common_bitmask)
      {
        TRACE_MSG(TRACE_ZLL1, "error: in key bit mask is empty", (FMT__0));
        /* AT:
        See: 8.7.1 Transferring the network key during touchlink commissioning
        On receipt of each scan response inter-PAN command frame, the initiator shall compare the value in
        the received key bitmask field with its own stored key bitmask to find out if the two devices contain a
        common key. If no common key is found (i.e. the bitwise AND of the two is equal to zero), the
        initiator shall not select this target for further commissioning.
        */
        break;
      }
      if (common_bitmask & ZB_ZLL_CERTIFICATION_KEY)
      {
        key_idx = ZB_ZLL_CERTIFICATION_KEY_INDEX;
      }
      else if (common_bitmask & ZB_ZLL_MASTER_KEY)
      {
        key_idx = ZB_ZLL_MASTER_KEY_INDEX;
      }
      else
      {
        key_idx = ZB_ZLL_DEVELOPMENT_KEY_INDEX;
      }
    }
    else
    {
      key_idx = (zb_uint8_t)key_info;
    }

    if (key_idx == ZB_ZLL_CERTIFICATION_KEY_INDEX)
    {
      TRACE_MSG(TRACE_ZLL1, "use certification key " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(ZLL_DEVICE_INFO().certification_key)));
      key = ZLL_DEVICE_INFO().certification_key;
    }
    else if (key_idx == ZB_ZLL_MASTER_KEY_INDEX)
    {
      TRACE_MSG(TRACE_ZLL1, "use master key " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(ZLL_DEVICE_INFO().master_key)));
      key = ZLL_DEVICE_INFO().master_key;
    }
    else
    {
      TRACE_MSG(TRACE_ZLL1, "use development key", (FMT__0));
      key = NULL;               /* Not used */
    }

    /* calculate transport key */
    {
      zb_uint32_t expanded_input[(ZB_CCM_KEY_SIZE + 3)/4] = { 0 };

      if (key_idx == ZB_ZLL_DEVELOPMENT_KEY_INDEX)
      {
        /* TODO: check security with devKey */
        /* from specificaton:
        This algorithm encrypts the network key with AES in ECB mode
        in one single step where the AES key 16 is equal to:
        PhLi || TrID || CLSN || RsID
        */
        expanded_input[0] = 'P' << 24 | 'h' << 16 | 'L' << 8 | 'i';
        expanded_input[1] = transaction_id;
        expanded_input[2] = 'C' << 24 | 'L' << 16 | 'S' << 8 | 'N';
        expanded_input[3] = response_id;
        /*
        Note: The development key (key index 0) shall only be used during
        the development phase of Light 28 Link products.
        Commercial Light Link products shall not use nor indicate having
        support for the 29 development key.
        */
      }
      else
      {
        zb_uint32_t tmp0, tmp2;

        tmp0 = transaction_id;
        tmp2 = response_id;

#ifdef ZLL_TEST_WITH_EMBER
        ZB_HTOBE32(&expanded_input[0], &tmp0);
        ZB_HTOBE32(&expanded_input[2], &tmp2);
#else
        expanded_input[0] = tmp0;
        expanded_input[2] = tmp2;
#endif
        expanded_input[1] = expanded_input[0];
        expanded_input[3] = expanded_input[2];
      }

      TRACE_MSG(TRACE_ZLL1, "expanded info for tk " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128((zb_uint8_t*)expanded_input)));
      TRACE_MSG(TRACE_ZLL1, "ZLL security: build transport key", (FMT__0));
      zb_aes128(key, (zb_uint8_t*)expanded_input, transport_key);
    }

    if (is_out)
    {
      //encrypt outgoing nwk key
      TRACE_MSG(TRACE_ZLL1, "ZLL security: try to ecrypt nwk_key", (FMT__0));
      zb_aes128(transport_key, key_decrypted, key_encrypted);
    }
    else
    {
      zb_uint8_t i = 0;
      //decrypt incoming NWK key
      TRACE_MSG(TRACE_ZLL1, "ZLL security: try to decrypt nwk_key", (FMT__0));
      zb_aes128_dec(transport_key, key_encrypted, key_decrypted);

      i = (ZB_NIB().active_secur_material_i + ZB_NIB().active_key_seq_number) % ZB_SECUR_N_SECUR_MATERIAL;
      ZB_MEMCPY(ZB_NIB().secur_material_set[i].key, key_decrypted, ZB_CCM_KEY_SIZE);
      ZB_NIB().secur_material_set[i].key_seq_number = 0;
    }

    TRACE_MSG(TRACE_ZLL1, "transaction id 0x%x", (FMT__D, transaction_id));
    TRACE_MSG(TRACE_ZLL1, "response id 0x%x", (FMT__D, response_id));
    TRACE_MSG(TRACE_ZLL1, "transport key " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(transport_key)));
    TRACE_MSG(TRACE_ZLL1, "encrypted NWK key " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(key_encrypted)));
    TRACE_MSG(TRACE_ZLL1, "decrypted NWK key " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(key_decrypted)));
  } while (0);

  TRACE_MSG(TRACE_ZLL1, "<< zll_calc_enc_dec_nwk_key", (FMT__0));
  return key_idx;
}


#endif  /* ZB_ENABLE_ZLL */

/*! @} */


