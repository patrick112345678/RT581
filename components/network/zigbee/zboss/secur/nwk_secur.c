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
/* PURPOSE: NWK security routines
*/

#define ZB_TRACE_FILE_ID 2465
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_secur.h"

/*! \addtogroup ZB_SECUR */
/*! @{ */

static void zb_secur_nwk_status(zb_uint8_t param, zb_uint16_t addr_short, zb_uint8_t status);


zb_ret_t zb_nwk_secure_frame(zb_bufid_t src, zb_uint_t mac_hdr_size, zb_bufid_t dst)
{
    zb_ret_t ret;
    zb_secur_ccm_nonce_t nonce = {0};

    /* Casting from zb_uint8_t * to packet struct zb_nwk_hdr_t */
    void *tmp = (zb_uint8_t *)zb_buf_begin(src) + mac_hdr_size;
    zb_nwk_hdr_t *nwhdr = (zb_nwk_hdr_t *)tmp;
    zb_ushort_t hdr_size = ZB_NWK_HDR_SIZE(nwhdr);
    zb_nwk_aux_frame_hdr_t *aux;

    ZB_CHK_ARR(ZB_BUF_BEGIN(src), zb_buf_len(src));

    /* Casting from packet struct zb_nwk_hdr_t * to packet struct zb_nwk_aux_frame_hdr_t */
    tmp = (zb_uint8_t *)nwhdr + hdr_size - sizeof(*aux);
    aux = (zb_nwk_aux_frame_hdr_t *)tmp;


    TRACE_MSG(TRACE_SECUR3, "zb_nwk_secure_frame src %hd mac_hdr_size %hd dst %hd hdr_size %d aux ctrl 0x%hx frcnt %ld keyseq %hd *nwhdr 0x%hx",
              (FMT__H_H_H_D_H_L_H_H, src, mac_hdr_size, dst, hdr_size, aux->secur_control, aux->frame_counter, aux->key_seq_number, *(zb_uint8_t *)nwhdr));

    /* fill aux header - see 4.5.1 */
    aux->secur_control = ZB_NWK_STD_SECUR_CONTROL;
#ifdef ZB_ROUTER_ROLE
    if (ZB_BIT_IS_SET(zb_buf_flags_get(src), ZB_BUF_USE_SAME_KEY)
            && aux->key_seq_number != ZB_NIB().active_key_seq_number)
    {
        TRACE_MSG(TRACE_SECUR3, "use same key number (%hd) and counter %hd", (FMT__H_H, aux->key_seq_number, ZB_NIB().prev_outgoing_frame_counter));
        ZB_HTOLE32((zb_uint8_t *)&aux->frame_counter, (zb_uint8_t *)&ZB_NIB().prev_outgoing_frame_counter);
        ZB_NIB().prev_outgoing_frame_counter++;
    }
    else
#endif
    {
        ZB_HTOLE32((zb_uint8_t *)&aux->frame_counter, (zb_uint8_t *)&ZB_NIB().outgoing_frame_counter);
        ZB_NIB().outgoing_frame_counter++;
#ifdef ZB_CERTIFICATION_HACKS
        if (ZB_CERT_HACKS().break_nwk_fcf_counter)
        {
            TRACE_MSG(TRACE_ERROR, "break_nwk_fcf_counter", (FMT__0));

            ZB_BZERO(&aux->frame_counter, sizeof(aux->frame_counter));
            ZB_CERT_HACKS().break_nwk_fcf_counter = ZB_FALSE;
        }
#endif /* ZB_CERTIFICATION_HACKS */
#ifdef ZB_STORE_COUNTERS
        if ((ZB_NIB().outgoing_frame_counter % ZB_LAZY_COUNTER_INTERVAL == 0U)
                || ZB_NIB().outgoing_frame_counter == 1U
                || ZB_U2B(ZB_NVRAM().refresh_flag))
        {
            /* Will store counter + interval */
            /* If we fail, trace is given and assertion is triggered */
            (void)zb_nvram_write_dataset(ZB_IB_COUNTERS);
            ZB_NVRAM().refresh_flag = 0; /* In case it stored in nvram */
        }
#endif
        aux->key_seq_number = ZB_NIB().active_key_seq_number;
        TRACE_MSG(TRACE_SECUR3, "use active key number - %hd", (FMT__H, aux->key_seq_number));
    }
    ZB_IEEE_ADDR_COPY(aux->source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());

    /* fill nonce - see 4.5.2.2 */
    ZB_IEEE_ADDR_COPY(nonce.source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
#ifdef ZB_CERTIFICATION_HACKS
    {
        /* find leave pkt using test_address */
        if (ZB_CERT_HACKS().nwk_leave_from_unknown_addr == ZB_TRUE &&
                ZB_IEEE_ADDR_CMP(nwhdr->src_ieee_addr, ZB_CERT_HACKS().nwk_leave_from_unknown_ieee_addr))
        {
            ZB_IEEE_ADDR_COPY(nonce.source_address, nwhdr->src_ieee_addr);
            ZB_IEEE_ADDR_COPY(aux->source_address, nwhdr->src_ieee_addr);
        }
    }
#endif
    nonce.frame_counter = aux->frame_counter;
    nonce.secur_control = aux->secur_control;
    TRACE_MSG(TRACE_SECUR2, "secure nwk frm %p[%hd - %hd] hdr_size %hd fcnt %lx",
              (FMT__P_H_H_H_L, src, zb_buf_len(src), mac_hdr_size, hdr_size, aux->frame_counter));

    {
        zb_uint8_t *key = secur_nwk_key_by_seq(aux->key_seq_number);
        if (key == NULL)
        {
            TRACE_MSG(TRACE_ERROR, "Can't get key by seq# %hd", (FMT__H, aux->key_seq_number));
            ret = RET_ERROR;
        }
        else
        {
#ifdef ZB_CERTIFICATION_HACKS
            zb_uint8_t distorted_key[ZB_CCM_KEY_SIZE];
            if (ZB_CERT_HACKS().break_nwk_key)
            {
                TRACE_MSG(TRACE_ERROR, "break_nwk_key", (FMT__0));
                ZB_MEMCPY(distorted_key, key, sizeof(distorted_key));
                /* distort the key */
                distorted_key[1] ^= 0xF0;
                key = distorted_key;
                /* switch off the action */
                ZB_CERT_HACKS().break_nwk_key = ZB_FALSE;
            }
#endif /* ZB_CERTIFICATION_HACKS */
            /* Secure. zb_ccm_encrypt_n_auth() allocs space for a and m, but not mhr.  */
            ret = zb_ccm_encrypt_n_auth(key,
                                        (zb_uint8_t *)&nonce,
                                        (zb_uint8_t *)nwhdr, hdr_size,
                                        (zb_uint8_t *)nwhdr + hdr_size, zb_buf_len(src) - hdr_size - mac_hdr_size,
                                        dst);
        }
    }
    if (ret == RET_OK)
    {
        {
            zb_uint8_t *dp;
            dp = zb_buf_alloc_left(dst, mac_hdr_size);
            ZB_MEMCPY(dp, zb_buf_begin(src), mac_hdr_size);
        }

        /* clear security level - see 4.3.1.1/8 */
        /* Casting from packet struct zb_uint8_t * to packet struct zb_nwk_aux_frame_hdr_t */
        tmp = (zb_uint8_t *)zb_buf_begin(dst) + ((zb_uint8_t *)aux - (zb_uint8_t *)zb_buf_begin(src));
        aux = (zb_nwk_aux_frame_hdr_t *)tmp;
        ZB_SECUR_SET_ZEROED_LEVEL(aux->secur_control);

        /* hold the flags ZB_BUF_HAS_APS_PAYLOAD and ZB_BUF_HAS_APS_USER_PAYLOAD further */
        ZB_BUF_COPY_FLAG_APS_PAYLOAD(dst, src);

        TRACE_MSG(TRACE_SECUR2, "secured frm %p[%hd] %d fcnt %ld", (FMT__P_H_D_L,
                  dst, zb_buf_len(dst), ret, ZB_NIB().outgoing_frame_counter));

#ifdef ZB_COORDINATOR_ROLE
        /* If counter is near limit, switch key */
        if (ZB_IS_TC() && ZB_NIB().outgoing_frame_counter == ZB_SECUR_NWK_COUNTER_LIMIT)
        {
            TRACE_MSG(TRACE_SECUR3, "time to switch nwk key", (FMT__0));
            zb_buf_get_out_delayed(zb_secur_switch_nwk_key_br);
        }
#endif
    }
    return ret;
}


zb_ret_t zb_nwk_unsecure_frame(zb_uint8_t param)
{
    zb_neighbor_tbl_ent_t *nbe = NULL;
    zb_uint8_t *key = NULL;
    zb_ret_t ret;
    zb_ushort_t hdr_size;
    zb_nwk_aux_frame_hdr_t *aux;
    zb_uint16_t mac_addr;
    zb_uint8_t     key_seq_number = 0;
    zb_uint32_t frame_counter = 0;
    zb_bool_t inc_decrypt_fail_counter = ZB_TRUE;

    TRACE_MSG(TRACE_SECUR1, "> zb_nwk_unsecure_frame param %hd", (FMT__H, param));

    {
        zb_nwk_hdr_t *nwk_hdr = zb_buf_begin(param);

        hdr_size = ZB_NWK_HDR_SIZE(nwk_hdr);
        /*cstat !MISRAC2012-Rule-11.3 */
        /** @mdr{00002,58} */
        /* [tag_mdr_00002_58] */
        aux = (zb_nwk_aux_frame_hdr_t *)((zb_uint8_t *)nwk_hdr + hdr_size - sizeof(zb_nwk_aux_frame_hdr_t));
        /* [tag_mdr_00002_58] */

        TRACE_MSG(TRACE_SECUR3, "hdr_size %d aux ctrl 0x%hx frcnt %ld keyseq %hd *nwhdr 0x%hx",
                  (FMT__D_H_L_H_H, hdr_size, aux->secur_control, aux->frame_counter, aux->key_seq_number, *(zb_uint8_t *)nwk_hdr));

        /*
          Can't use src_addr from the nwk header: frame can be not from its originator (and
          re-encrypted). Must use address got from MHR.
        */
        {
            zb_apsde_data_ind_params_t *mac_addrs = ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t);
            mac_addr = mac_addrs->mac_src_addr;
            /* Get neighbor table entry.
               It is possible to have no dev in the neighbor if this is first packet from it.
               Firstly we try to find the neighbor by IEEE address (it will help in case of
               address conflicts), then by short address.
            */
            ret = zb_nwk_neighbor_get_by_ieee(aux->source_address, &nbe);
            if (ret == RET_NOT_FOUND)
            {
                ret = zb_nwk_neighbor_get_by_short(mac_addr, &nbe);
            }

            if (ret == RET_NOT_FOUND)
            {
                TRACE_MSG(TRACE_NWK1, "warning: device 0x%x is not a neighbor",
                          (FMT__D, mac_addr));
                ret = RET_OK;
            }
        }
    }
    if (ret == RET_OK)
    {
        key = secur_nwk_key_by_seq(aux->key_seq_number);

        /* If this is child, set its state to 'not authenticated' */
        if (nbe != NULL && nbe->relationship == ZB_NWK_RELATIONSHIP_CHILD)
        {
            nbe->relationship = ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD;
        }

        key_seq_number = aux->key_seq_number;
        ZB_LETOH32((zb_uint8_t *)&frame_counter, (zb_uint8_t *)&aux->frame_counter);

        if (key == NULL)
        {
            /* set 'frame security failed' */
            ret = RET_ERROR;
            TRACE_MSG(TRACE_SECUR3, "no key by seq %hd", (FMT__H, key_seq_number));
            zb_secur_nwk_status(param, mac_addr, ZB_NWK_COMMAND_STATUS_BAD_KEY_SEQUENCE_NUMBER);
        }
        else if (nbe != NULL)
        {
            /* Check FrameCount. If neighbor switched it's network key - skip this check.
              incoming_frame_counter will be updated in case of successful decrypting. */
            if ((nbe->u.base.incoming_frame_counter > frame_counter && nbe->u.base.key_seq_number == key_seq_number)
                    || (nbe->u.base.incoming_frame_counter == (zb_uint32_t)~0U))
            {
                ret = RET_ERROR;
                TRACE_MSG(TRACE_SECUR1, "frm cnt %ld->%ld shift back - indicate status", (FMT__L_L,
                          nbe->u.base.incoming_frame_counter, frame_counter));
                zb_secur_nwk_status(param, mac_addr, ZB_NWK_COMMAND_STATUS_BAD_FRAME_COUNTER);
                /* The "Bad frame counter" status will cause the ZDO_DIAGNOSTICS_NWKFC_FAILURE_ID
                 * counter in zb_nlme_status_indication_process() to increase later.
                 * Therefore, prevent the increase of the ZDO_DIAGNOSTICS_NWK_DECRYPT_FAILURES_ID
                 * counter further in this function.
                 */
                inc_decrypt_fail_counter = ZB_FALSE;
            }
        }
        else
        {
            /* MISRA rule 15.7 requires empty 'else' branch. */
        }
    }
    else
    {
        TRACE_MSG(TRACE_SECUR1, "can't get neighbor - indicate nwk status", (FMT__0));
        zb_secur_nwk_status(param, mac_addr, ZB_NWK_COMMAND_STATUS_FRAME_SECURITY_FAILED);
    }

    if (ret == RET_OK)
    {
        zb_ushort_t payload_size = (zb_buf_len(param) > hdr_size ? zb_buf_len(param) - hdr_size : 0U);
        /* Check that payload len >= ZB_CCM_M or source ieee_addr is not == zero */
        if (ret == RET_OK
                && (payload_size < ZB_CCM_M
                    || !ZB_IEEE_ADDR_IS_VALID(aux->source_address)))
        {
            ret = RET_ERROR;
            TRACE_MSG(TRACE_SECUR3, "too short nsdu: %hd hdr %hd - indicate status", (FMT__H_H, zb_buf_len(param), hdr_size));
        }

        if (ret == RET_OK)
        {
            /* decrypt */
            zb_secur_ccm_nonce_t nonce = {0};

            aux->secur_control = ZB_NWK_STD_SECUR_CONTROL;
            ZB_IEEE_ADDR_COPY(nonce.source_address, aux->source_address);
            nonce.frame_counter = aux->frame_counter;
            nonce.secur_control = aux->secur_control;

            ZG->nwk.handle.unsecure_frame_attempt = 0;
            do
            {
#ifdef DEBUG
                DUMP_TRAF("key", key, 16, 0);
#endif
                /* Decrypt can move data, so aux pointer become invalud. Remember key here. */
                ret = zb_ccm_decrypt_n_auth_stdsecur(key,
                                                     (zb_uint8_t *)&nonce,
                                                     param, hdr_size,
                                                     payload_size);

                if (ret == RET_OK)
                {
                    /* Set NWK encryption for upper layers */
                    nwk_mark_nwk_encr(param);
                    ZG->nwk.handle.unsecure_frame_attempt = 0;

                    TRACE_MSG(TRACE_SECUR3, "unsecured frm %hd [%hd] ok", (FMT__H_H, param, zb_buf_len(param)));
                }
                else
                {
                    /* On some platfoms we have unsecure problems when radio pass up
                       frames with incorrect packet length (i.e. greater than it really has).
                       Try to use workaround: cut 1 byte at end of buffer and unsecure frame again.
                       Note: this is very rare platform bug.
                    */
                    TRACE_MSG(TRACE_SECUR1, "unsecure failed %hd, retry - attempt %hd",
                              (FMT__H_H, ret, ZG->nwk.handle.unsecure_frame_attempt));
                    ++(ZG->nwk.handle.unsecure_frame_attempt);
                    zb_buf_cut_right(param, 1);
                }
            }
#ifdef ZB_NWK_SECUR_UNSECURE_FRAME_ATTEMPTS
            while (ret != RET_OK &&
                    (ZG->nwk.handle.unsecure_frame_attempt < ZB_NWK_SECUR_UNSECURE_FRAME_ATTEMPTS));
#else
            while (0);
#endif
        }

        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_SECUR1, "unsecure failed %d - indicate nwk status", (FMT__D, ret));
            zb_secur_nwk_status(param, mac_addr, ZB_NWK_COMMAND_STATUS_FRAME_SECURITY_FAILED);
        }
    }

    /* TODO: move that logic into nwk_add_into_addr_and_nbt() */
    /* If frame is NWK secured, we have long/short pair of the last hop - add it to the NBT and
     * update relevant fields */
    if (ret == RET_OK)
    {
        /* Reassign nwk_hdr and aux: buffer contents can be moved */
        zb_address_ieee_ref_t addr_ref;
        zb_nwk_hdr_t *nwk_hdr = zb_buf_begin(param);
        /*cstat !MISRAC2012-Rule-11.3 */
        /** @mdr{00002,59} */
        /* [tag_mdr_00002_59] */
        aux = (zb_nwk_aux_frame_hdr_t *)((zb_uint8_t *)nwk_hdr + hdr_size - sizeof(zb_nwk_aux_frame_hdr_t));
        /* [tag_mdr_00002_59] */
        /* If we are connecting IEEE addr from Security Header to mac src address without any
           conditions, there is no chance to detect an Address Conflict in case when there is no
           IEEE address in the NWK header. E.g. in the 10.12 test case some TH send a spoofed
           DeviceAnnounce packet using only short source address, and we fail to detect address conflict
           because when the packet is processed at ZDO level we have already modified the addrmap.

           The solution below is probably not the best, but at least it does not break anything (which
           is totally undesirable because of the currently (03/25/19) ongoing r22 cert procedure). */

        /* TODO: consider alternative solutions */
        if (zb_address_by_short(mac_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_NOT_FOUND)
        {
            /* update only when there is no such record yet */
            ret = zb_address_update(aux->source_address, mac_addr, ZB_FALSE, &addr_ref);
        }

        if (ret == RET_OK
                /*cstat !MISRAC2012-Rule-13.5 */
                /* After some investigation, the following violations of Rule 13.5 and 13.6 seem to be false
                 * positives. There are no side effect to 'ZB_PIBCACHE_RX_ON_WHEN_IDLE()'. This violation
                 * seems to be caused by the fact that 'ZB_PIBCACHE_RX_ON_WHEN_IDLE()' is an external macro,
                 * which cannot be analyzed by C-STAT. */
                && ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
                && nbe == NULL)
        {
            ret = zb_nwk_neighbor_get(addr_ref, ZB_TRUE, &nbe);
        }

        if (ret != RET_OK)
        {
            /* Don't drop a packet if we can't update address or add to NBT */
            TRACE_MSG(TRACE_SECUR3, "Can't update Address map or NBT entries of the last hop: ret %hd", (FMT__H, ret));
            ret = RET_OK;
        }
    }
    if (ret == RET_OK && nbe != NULL)
    {
        /* if this is child and packet decrypted ok, set its state to 'authenticated' */
        if (nbe->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
        {
            nbe->relationship = ZB_NWK_RELATIONSHIP_CHILD;
        }

        nbe->u.base.incoming_frame_counter = frame_counter;
        if (nbe->u.base.key_seq_number != key_seq_number)
        {
            nbe->u.base.key_seq_number = key_seq_number;
            TRACE_MSG(TRACE_SECUR3, "peer switched key to seq %hd", (FMT__H, nbe->u.base.key_seq_number));
#ifdef ZB_USE_NVRAM
            zb_nvram_store_addr_n_nbt();
#endif
        }
    }

    if (ret == RET_OK
            /* exclude key switch back. More precise checks done in secur_nwk_key_switch. */
            && ((key_seq_number - ZB_NIB().active_key_seq_number) != 0U)
            && ((zb_uint8_t)(key_seq_number - ZB_NIB().active_key_seq_number) < (zb_uint8_t)127)
            /* Do not switch implicitly if we are TC. Possible scenario is: send
             * switch_key using unicasts. We do not want tobe switched to the next key
             * by receiving APS ACK from some device: we must send switch using old
             * key */
            && !ZB_IS_TC())
    {
        /*
          Implicit key switch: according to 4.3.1.2 Security Processing of Incoming
          Frames

          "7 If the sequence number of the received frame belongs to a newer entry in the
          nwkSecurityMaterialSet, set the nwkActiveKeySeqNumber to the received
          sequence number."
         */
        TRACE_MSG(TRACE_SECUR1, "implicit key switch to %hd", (FMT__H, key_seq_number));
        secur_nwk_key_switch(key_seq_number);
    }

    if (ret != RET_OK && inc_decrypt_fail_counter)
    {
        ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NWK_DECRYPT_FAILURES_ID);
    }

    TRACE_MSG(TRACE_SECUR1, "< zb_nwk_unsecure_frame ret %hd", (FMT__H, ret));

    return ret;
}


static void zb_secur_nwk_status(zb_uint8_t param, zb_uint16_t addr_short, zb_nwk_command_status_t status)
{
    /* if (ZG->nwk.handle.unsecure_frame_attempt >= ZB_NWK_SECUR_UNSECURE_FRAME_ATTEMPTS) */
    {
        zb_nlme_status_indication_t *cmd = ZB_BUF_GET_PARAM(param, zb_nlme_status_indication_t);
        cmd->status = status;
        cmd->network_addr = addr_short;
        ZB_SCHEDULE_CALLBACK(zb_nlme_status_indication, param);
    }
}
#ifdef ZB_LOW_SECURITY_MODE
void zb_disable_nwk_security(void)
{
    ZB_SET_NIB_SECURITY_LEVEL(0);
}

void zb_enable_nwk_security(void)
{
    ZB_SET_NIB_SECURITY_LEVEL(5);
}
#endif /*ZB_LOW_SECURITY_MODE*/
/*! @} */
