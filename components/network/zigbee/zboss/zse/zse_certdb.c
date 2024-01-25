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
/*  PURPOSE: ECC Certificates DataBase low level routines
*/

#define ZB_TRACE_FILE_ID 3521
#include "zb_common.h"

#ifdef ZB_ENABLE_SE_MIN_CONFIG

#if defined ZB_SE_ENABLE_KEC_CLUSTER

#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_se.h"
#include "zb_zdo_globals.h"

void zse_certdb_init(void)
{
  TRACE_MSG(TRACE_ZSE1, "zse_certdb_init", (FMT__0));
  /* Suppose memory is already zeroed. */
/*
#if defined ZB_COORDINATOR_ROLE
  ZSE_CTXC().certdb.base.tbl_size = ZB_ZSE_CERTDB_TBL_SIZE;
  ZSE_CTXC().certdb.base.entry_size = sizeof(zse_certdb_tbl_ent_t);
#ifdef ZB_USE_NVRAM
  ZSE_CTXC().certdb.base.nvram_dataset = ZB_NVRAM_DATASET_SE_CERTDB;
#endif
  ZSE_CTXC().certdb.base.cached_i = (-1);
#endif
*/
}


static zb_ret_t zse_certdb_check_crc(zb_uint8_t *ic)
{
  zb_uint16_t calculated_crc = ~zb_crc16(ic, 0xffff, ZB_CCM_KEY_SIZE);
  zb_uint16_t ic_crc;

  ZB_LETOH16(&ic_crc, ic + ZB_CCM_KEY_SIZE);
  if (calculated_crc != ic_crc)
  {
    return RET_ERROR;
  }
  return RET_OK;
}


/*
zb_uint8_t* zse_certdb_get_from_client_storage()
{
  TRACE_MSG(TRACE_SECUR3, "zse_certdb_get_from_client_storage: ret installcode %p", (FMT__P, ZB_AIB().installcode));
  return ZB_AIB().installcode;
}
*/


zb_ret_t zse_certdb_set(zb_uint8_t *ic)
{
  if (zse_certdb_check_crc(ic) == RET_OK)
  {
    ZB_MEMCPY(ZB_AIB().installcode, ic, ZB_CCM_KEY_SIZE+ZB_CCM_KEY_CRC_SIZE);
    TRACE_MSG(TRACE_SECUR1, "zse_certdb_set: setup my installcode " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(ic)));
    return RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "zse_certdb_set: bad crc", (FMT__0));
    return RET_CONVERSION_ERROR;
  }
}


#ifdef ZB_PRODUCTION_CONFIG

static zse_cert_nvram_t * zse_certdb_get_from_prodconf(zb_uint8_t suite_no, zb_uint8_t *issuer, zse_cert_nvram_t * buf)
{
  zb_ret_t ret;
  zb_production_config_hdr_t prod_cfg_hdr;
  zb_uint8_t options = 0;

  TRACE_MSG(TRACE_ZSE1, ">> zse_certdb_get_from_prodconf", (FMT__0));
  if (issuer != NULL)
  {
    TRACE_MSG(TRACE_ZSE1, "for issuer " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(issuer)));
  }

  ret = zb_production_config_check(&prod_cfg_hdr);

  /* Check that version of production configuration is compatible with us
   *
   * For the future:
   * if production config is extended or modified, consider handling legacy versions
   */
  if (ret == RET_OK)
  {
    if (prod_cfg_hdr.version != ZB_PRODUCTION_CONFIG_CURRENT_VERSION)
    {
      TRACE_MSG(TRACE_ERROR, "production config version mismatch expected %d actual %d",
                (FMT__D_D, ZB_PRODUCTION_CONFIG_CURRENT_VERSION, prod_cfg_hdr.version));
      ret = RET_ERROR;
    }
  }

  if (ret == RET_OK)
  {
    ret = zb_osif_prod_cfg_read(
      &options, (zb_uint16_t)sizeof(zb_uint8_t),
      (zb_uint16_t)ZB_OFFSETOF(zb_production_config_ver_2_t,
      options));
  }

  /* Can apply the configuration data now */
  if (ret == RET_OK)
  {
    zb_uint8_t i;
    if ((options & ZB_PROD_CFG_OPTIONS_CERT_PRESENT_MASK) != 0U)
    {
      zb_uint16_t c_offset = (zb_uint16_t)sizeof(zb_production_config_ver_2_t);
      zb_uint16_t certmask = 0;
      //zb_uint8_t buf[ZB_KEC_MAX_PUBLIC_KEY_SIZE + ZB_KEC_MAX_PRIVATE_KEY_SIZE + ZB_KEC_MAX_CERT_SIZE];
      const zb_uint8_t c_certsize[2]={21+22+48, 36+37+74};
      const zb_uint8_t c_issueroffset[2]={30, 11};
      const zb_uint8_t c_certoffset[2]={22, 37};
      const zb_uint8_t c_prvoffset[2]={22+48, 37+74};
      TRACE_MSG(TRACE_COMMON3, "Certificates present", (FMT__0));
      /*cstat !MISRAC2012-Rule-14.3_b */
      /** @mdr{00009,10} */
      if (zb_osif_prod_cfg_read((zb_uint8_t *)&certmask, 2U, c_offset) != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "cannot load certificates mask", (FMT__0));
        ret = RET_ERROR;
        ZVUNUSED(ret);
      }
      c_offset+=2U;
      certmask &= ZB_KEC_SUPPORTED_CRYPTO_ATTR;
      TRACE_MSG(TRACE_COMMON3, "Certificates mask:0x%04X", (FMT__D, certmask));
      for(i = 0; i < 16U; i++)
      {
        zb_uint16_t imask = (zb_uint16_t)1U << i;

        if(ZB_BIT_IS_SET(imask, certmask))
        {
          TRACE_MSG(TRACE_COMMON3, "Cert for cs%d present at offset: %d, size = %d", (FMT__D_D_D, i+1, c_offset, c_certsize[i]));
          if( i==suite_no
              /* Strict condition for prevention array borders violation */
              &&  ZB_KEC_VALID_SUITE_NUMBER(i))
          {
            zb_bool_t ok = ZB_TRUE;
            if(issuer != NULL)
            {
              zb_uint8_t t_issuer[8];
              (void)zb_osif_prod_cfg_read((zb_uint8_t *)t_issuer, 8, c_offset+c_certoffset[i]+c_issueroffset[i]);
              TRACE_MSG(TRACE_SECUR1, "zse_certdb_get_from_prodconf: got issuer " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(t_issuer)));
              if (ZB_MEMCMP(t_issuer, issuer, 8) != 0)
              {
                ok = ZB_FALSE;
              }
            }
            if(ok)
            {
              ZB_MEMSET(buf, 0, sizeof(zse_cert_nvram_t));
              buf->suite_no = i;
              (void)zb_osif_prod_cfg_read((zb_uint8_t *)buf->ca.u8, ZB_QE_SIZE[i], c_offset);
              (void)zb_osif_prod_cfg_read((zb_uint8_t *)buf->pr.u8, ZB_PR_SIZE[i], c_offset+c_prvoffset[i]);
              (void)zb_osif_prod_cfg_read((zb_uint8_t *)buf->cert.u8,ZB_CERT_SIZE[i], c_offset+c_certoffset[i]);
              DUMP_TRAF("cert_all:", (zb_uint8_t *)buf, sizeof(zse_cert_nvram_t),0);
              return buf;
            }
          }
          /* Strict condition for prevention array borders violation */
          if (i < ZB_ARRAY_SIZE(c_certsize))
          {
            c_offset+=c_certsize[i];
          }
        }
      }
    }
  }
  TRACE_MSG(TRACE_ERROR, "<< no certificates found in prod config", (FMT__0));
  return NULL;
}
#endif //production_config
/**
   Read installcodes dataset and create installcodes hash data structure

*/
#ifdef ZB_USE_NVRAM

/**
   Read certificate from the storage.

   Use hash to find installcode position in the nvram. Actual installcode data
   is in nvram only.
 */
zse_cert_nvram_t * zse_certdb_get_from_tc_storage(zb_uint8_t suite_no, zb_uint8_t *issuer, zse_cert_nvram_t * buf)
{
  zb_uint_t n = 0;
  zb_uint32_t idx;
  zb_bool_t found_cert_of_selected_suite = ZB_FALSE;
#ifdef ZB_PRODUCTION_CONFIG
    zse_cert_nvram_t *rv;
#endif
    TRACE_MSG( TRACE_ZSE1, "zse_certdb_get_from_tc_storage: suite_no %hd", (FMT__H, suite_no));

#ifdef ZB_PRODUCTION_CONFIG
#ifdef ZB_CERTIFICATION_HACKS
  if (ZG->cert_hacks.certificate_priority_from_certdb != 0)
  {
    rv = NULL;
  }
  else
#endif /* ZB_CERTIFICATION_HACKS */
  {
    rv = zse_certdb_get_from_prodconf(suite_no, issuer, buf);
  }
  if ( rv!=NULL )
  {
    return rv;
  }
  TRACE_MSG( TRACE_ZSE1, "zse_certdb_get_from_tc_storage: not found in production block", (FMT__0));
#endif /* ZB_PRODUCTION_CONFIG */

  if(issuer != NULL)
  {
    idx = zb_64bit_hash(issuer) % ZB_ARRAY_SIZE(ZSE_CTXC().certdb);
    TRACE_MSG(TRACE_ERROR, "zse_certdb_get_from_tc_storage: searching for suite no %d and issuer " TRACE_FORMAT_64, (FMT__D_A, suite_no, TRACE_ARG_64(issuer)));
  }
  else
  {
    idx = 0;
    TRACE_MSG(TRACE_ERROR, "zse_certdb_get_from_tc_storage: searching for suite no %d and ANY issuer", (FMT__D, suite_no));
  }

  buf->suite_no = 0;

  while (n < ZB_ARRAY_SIZE(ZSE_CTXC().certdb))
  {
    TRACE_MSG(TRACE_SECUR3, "ZSE_CTXC().certdb[%d].nvram_offset = %d", (FMT__D_D, idx, ZSE_CTXC().certdb[idx].nvram_offset));
    if (ZSE_CTXC().certdb[idx].nvram_offset != 0U)
    {
      zse_cert_nvram_t rec;
      zb_ret_t ret = zb_osif_nvram_read(ZSE_CTXC().certdb[idx].nvram_page, ZSE_CTXC().certdb[idx].nvram_offset, (zb_uint8_t*)&rec, (zb_uint16_t)sizeof(rec));
      /*cstat !MISRAC2012-Rule-14.3_a */
      /** @mdr{00007,17} */
      if (ret == RET_OK)
      {
        //zb_uint8_t * t_issuer = zb_kec_get_issuer(rec.suite_no, (zb_kec_icu_t *)&rec.cert);
        //TRACE_MSG(TRACE_SECUR3, "rec.suite_no[%d] =%d= suite_no[%d]", (FMT__D_D_D, rec.suite_no, rec.suite_no==suite_no, suite_no));
        //TRACE_MSG(TRACE_SECUR1, "issuer_db[" TRACE_FORMAT_64 "] =%d= issuer given = [" TRACE_FORMAT_64 "] %p",
        //(FMT__A_D_A_P, TRACE_ARG_64(zb_kec_get_issuer(rec.suite_no, &rec.cert)),
        //ZB_MEMCMP(zb_kec_get_issuer(rec.suite_no, &rec.cert), issuer, 8),TRACE_ARG_64(issuer), NULL));
        zb_uint8_t *is = zb_kec_get_issuer(rec.suite_no, &rec.cert);
        if (rec.suite_no == suite_no)
        {
          found_cert_of_selected_suite = ZB_TRUE;
          if ((issuer == NULL) || ((issuer!=NULL) && (0 == ZB_MEMCMP(is, issuer, 8))))
          {
            ZB_MEMCPY(buf, &rec, sizeof(rec));
            TRACE_MSG(TRACE_SECUR1, "zse_certdb_get_from_tc_storage: got certdb by hash idx %d for issuer " TRACE_FORMAT_64, (FMT__D_A, idx, TRACE_ARG_64(is)));
            return buf;
          }
        }
      }
    }
    idx = (idx + 1U) % ZB_ARRAY_SIZE(ZSE_CTXC().certdb);
    n++;
  }

#if defined(ZB_CERTIFICATION_HACKS) && defined(ZB_PRODUCTION_CONFIG)
  if (ZG->cert_hacks.certificate_priority_from_certdb != 0)
  {
    rv = zse_certdb_get_from_prodconf(suite_no, issuer, buf);
    if ( rv!=NULL )
    {
      TRACE_MSG( TRACE_ZSE1, "zse_certdb_get_from_tc_storage: found in production block", (FMT__0));
      return rv;
    }
  }
#endif /* ZB_CERTIFICATION_HACKS && ZB_PRODUCTION_CONFIG */

  TRACE_MSG(TRACE_SECUR1, "zse_certdb_get_from_tc_storage: no certdb for CryptoSuite%hd", (FMT__H, suite_no+1));
  if(found_cert_of_selected_suite==ZB_TRUE)
  {
    buf->suite_no = 0xff;
  }
  return NULL;
}


static zb_ret_t zse_check_cert(zb_uint8_t suite_no, zb_kec_icu_t *cert)
{
  if( suite_no == zb_kec_get_suite_num(KEC_CS2) )
  {
/*The receiving device shall also check the Type, Curve and Hash fields of
such a certificate.*/
    if ((cert->cs2.type != 0x00U)||(cert->cs2.curve != 0x0DU)||(cert->cs2.hash != 0x08U))
    {
      TRACE_MSG( TRACE_ZSE1, "zse_check_cert:kec: type fields failed for CS2: type(00?%02X), curve(0D?%02X), hash(08?%02X)", (FMT__D_D_D, cert->cs2.type, cert->cs2.curve, cert->cs2.hash));
      return RET_ERROR;
    }
  }
  return RET_OK;
}

/*cstat !MISRAC2012-Rule-2.2_b */
/* Cstat wrongly assumes in this line that cert_add.issuer, cert_add.subject and cert_add.suite_no are not used */
zb_ret_t zb_se_load_ecc_cert(  zb_uint16_t suite,
                               zb_uint8_t *ca_public_key,
                               zb_uint8_t *certificate,
                               zb_uint8_t *private_key  )
//zb_ret_t zse_cert_add(zb_ieee_addr_t address, zb_uint8_t ic_type, zb_uint8_t *ic)
{
   //return RET_ERROR;
  zb_uint8_t suite_no = zb_kec_get_suite_num(suite);
  /* Here and throughout. The fields of ZB_PACKED_STRUCT struct are two packed structs and zb_uint8_t array, so
   * this struct doesn't need any strict alignment and we can just cast zb_uint8_t *certificate to zb_kec_icu_t *
   * via pointer to void */
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,88} */
  if (zse_check_cert(suite_no, (zb_kec_icu_t *)certificate) == RET_OK)
  {
    zse_cert_nvram_t buf;
    zse_cert_nvram_t *existing_cert;
    zse_cert_add_t cert_add;

    /* Here and throughout. The fields of ZB_PACKED_STRUCT struct are two packed structs and zb_uint8_t array, so
     * this struct doesn't need any strict alignment and we can just cast zb_uint8_t *certificate to zb_kec_icu_t *
     * via pointer to void */
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,89} */
    existing_cert = zse_certdb_get_from_tc_storage(suite_no, zb_kec_get_issuer(suite_no, (zb_kec_icu_t *)certificate), &buf);
    if (/* Strict condition for prevention array borders violation */
        ZB_KEC_VALID_SUITE_NUMBER(suite_no) &&
        (NULL == existing_cert || (existing_cert->suite_no!=suite_no)
        || 0 != ZB_MEMCMP(&existing_cert->cert, certificate, ZB_CERT_SIZE[suite_no])
        || 0 != ZB_MEMCMP(&existing_cert->ca, ca_public_key, ZB_QE_SIZE[suite_no])
        || 0 != ZB_MEMCMP(&existing_cert->pr, private_key, ZB_PR_SIZE[suite_no]))
      )
    {
      ZB_MEMSET(&cert_add, 0, sizeof(zse_cert_add_t));
      /*cstat -MISRAC2012-Rule-2.2_b */
      /* Cstat wrongly assumes in this line that cert_add.suite_no is not used */
      cert_add.full_cert.suite_no = suite_no;
      ZB_MEMCPY(&cert_add.full_cert.cert, certificate, ZB_CERT_SIZE[suite_no]);
      ZB_MEMCPY(&cert_add.full_cert.ca, ca_public_key, ZB_QE_SIZE[suite_no]);
      ZB_MEMCPY(&cert_add.full_cert.pr, private_key, ZB_PR_SIZE[suite_no]);
      /* Cstat wrongly assumes in this line that cert_add.do_update is not used */
      cert_add.do_update = ZB_B2U(existing_cert != NULL);

      if (ZB_U2B(cert_add.do_update))
      {
        /* Cstat wrongly assumes in this line that cert_add.suite_no is not used */
        cert_add.suite_no = suite_no;
        /*cstat !MISRAC2012-Rule-11.3 */
        /** @mdr{00002,90} */
        /* Cstat wrongly assumes in this line that cert_add.issuer is not used */
        cert_add.issuer = zb_kec_get_issuer(suite_no, (zb_kec_icu_t *)certificate);
        /*cstat !MISRAC2012-Rule-11.3 */
        /** @mdr{00002,91} */
        /* Cstat wrongly assumes in this line that cert_add.issuer is not used */
        cert_add.subject = zb_kec_get_subject(suite_no, (zb_kec_icu_t *)certificate);
        /*cstat +MISRAC2012-Rule-2.2_b */
      }
#ifdef ZB_CERTIFICATION_HACKS
      if (ZG->cert_hacks.certificate_priority_from_certdb != 0)
      {
        cert_add.do_update = 0;
      }
#endif
      ZSE_CTXC().cert_to_add = &cert_add;
      TRACE_MSG(TRACE_SECUR1, "zse_load_ecc_cert: adding (do_update %hd) sute %hd for issuer " TRACE_FORMAT_64,
                (FMT__H_H_A, cert_add.do_update, suite_no, TRACE_ARG_64(zb_kec_get_issuer(suite_no, (zb_kec_icu_t *)certificate))));
      /* Note: NVRAM write is synchronous, so it is safe to use ptr to the local variable. */
      /* If we fail, trace is given and assertion is triggered */
      (void)zb_nvram_write_dataset(ZB_NVRAM_DATASET_SE_CERTDB);
      ZSE_CTXC().cert_to_add = NULL;
    }
    return RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "zb_secur_cert_add: invalid cert for issuer " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(zb_kec_get_issuer(suite_no, (zb_kec_icu_t *)certificate))));
    return RET_CONVERSION_ERROR;
  }
}

/* Erase certificate from NVRAM and remove from database.
   Firstly try to find cert by issuer hash, then fill it with 0xFF. */
zb_ret_t zb_se_erase_ecc_cert(zb_uint8_t suite_no,
                              zb_uint8_t *issuer,
                              zb_uint8_t *subject)
{
  zse_cert_add_t cert_add;
  zb_ret_t ret;
  zb_uint_t idx;
  zb_uint_t n = 0;
  zse_cert_nvram_t rec;
  zb_bool_t cert_found = ZB_FALSE;

  idx = zb_64bit_hash(issuer) % ZB_ARRAY_SIZE(ZSE_CTXC().certdb);
  while (n < ZB_ARRAY_SIZE(ZSE_CTXC().certdb))
  {
    TRACE_MSG(TRACE_SECUR3, "ZSE_CTXC().certdb[%d].nvram_offset = %d",
              (FMT__D_D, idx, ZSE_CTXC().certdb[idx].nvram_offset));
    if (ZSE_CTXC().certdb[idx].nvram_offset != 0U)
    {
      ret = zb_osif_nvram_read(ZSE_CTXC().certdb[idx].nvram_page,
                               ZSE_CTXC().certdb[idx].nvram_offset, (zb_uint8_t *)&rec,
                               (zb_uint16_t)sizeof(rec));
      /*cstat !MISRAC2012-Rule-14.3_a */
      /** @mdr{00007,16} */
      if (ret == RET_OK)
      {
        if ((rec.suite_no == suite_no)
            /*cstat -MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be a false
             * positive. There are no side effect to 'ZB_MEMCMP()'. This violation seems to be caused
             * by the fact that 'ZB_MEMCMP()' is an external macro, which cannot be analyzed by
             * C-STAT. */
            && ZB_MEMCMP(zb_kec_get_issuer(rec.suite_no, &rec.cert), issuer, 8) == 0
            && ZB_MEMCMP(zb_kec_get_subject(rec.suite_no, &rec.cert), subject, 8) == 0
            /*cstat +MISRAC2012-Rule-13.5 */
        )
        {
          cert_found = ZB_TRUE;
          break;
        }
      }
    }
    idx = (idx + 1UL) % ZB_ARRAY_SIZE(ZSE_CTXC().certdb);
    n++;
  }

  if (cert_found)
  {
    ZB_MEMSET(&cert_add, 0, sizeof(zse_cert_add_t));
    /*cstat -MISRAC2012-Rule-2.2_b */
    /* Cstat wrongly assumes that cert_add.do_update, cert_add.suite_no, cert_add.issuer,
     * cert_add.subject and cert_add.full_cert are not used */
    cert_add.do_update = ZB_TRUE;
    /* save certificate identifiers here, since the update means erasing of certificate,
      so full_cert will be filled with 0xFF and identifiers cannot be extracted further in that case */
    cert_add.suite_no = suite_no;
    cert_add.issuer = issuer;
    cert_add.subject = subject;
    ZB_MEMSET(&cert_add.full_cert, 0xFF, sizeof(cert_add.full_cert));
    /*cstat +MISRAC2012-Rule-2.2_b */
    ZSE_CTXC().cert_to_add = &cert_add;

    TRACE_MSG(TRACE_SECUR1, "zb_se_erase_ecc_cert: erasing (do_update %hd) suite %hd for issuer " TRACE_FORMAT_64,
              (FMT__H_H_A, cert_add.do_update, suite_no, TRACE_ARG_64(issuer)));
    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_dataset(ZB_NVRAM_DATASET_SE_CERTDB);
    ZSE_CTXC().cert_to_add = NULL;

    /* remove reference to certificate from db*/
    ZSE_CTXC().certdb[idx].nvram_offset = 0;
    ZSE_CTXC().certdb[idx].nvram_page = 0;
  }
  else
  {
    ret = RET_NOT_FOUND;
  }

  return ret;
}

#endif /* ZB_USE_NVRAM */
/*
static zb_uint8_t zse_certdb_entry_count(zse_cert_storage_t *tbl)
{
  zb_uint8_t n = 0;
  zb_uint8_t i;
  for (i = 0 ; i < tbl->tbl_size ; ++i)
  {
    n += (tbl->array[i].nvram_offset != 0);
  }
  return n;
}*/


#endif  /* ZB_SE_ENABLE_KEC_CLUSTER */

#endif  /* ZB_ENABLE_SE_MIN_CONFIG */
