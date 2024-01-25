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
/*     PURPOSE: Reserved flash area routines
*/

#define ZB_TRACE_FILE_ID 14002
#include "zb_config.h"
#include "zb_common.h"
#include "zb_ncp_internal.h"
#include "ncp_hl_proto.h"
#include "zb_ncp.h"
#include "zboss_api.h"

/*
 * Reserved flash space format:
 *
 *    Octets         4        varied        varied        varied
 *  Field Name  Signature  Parameter #1  Parameter #2  Parameter #3
 *
 *  Parameter format:
 *    Octets         1     1      1-4     2
 *                Length   ID    Value	 CRC
 *
 *  Length - size of ID + Value + CRC (4-7 octets)
 *  CRC - crc16 - dataset of Length, ID, Value
 */


#ifdef ZB_HAVE_CALIBRATION

#define ZBS_NCP_RESERV_SIGNATURE       0x5AA555AAU
#define ZBS_NCP_RESERV_MAX_VALUE_SIZE  4U              /* Maximum size of value in bytes */
#define ZBS_NCP_RESERV_CRC_SIZE        2U              /* Currently in use CRC16 */
#define ZBS_NCP_RESERV_MAX_RECORD_SIZE (ZBS_NCP_RESERV_CRC_SIZE + ZBS_NCP_RESERV_MAX_VALUE_SIZE + 1U) /* CRC16 + value max size + id (1 byte) */
#define ZBS_NCP_RESERV_MIN_RECORD_SIZE (ZBS_NCP_RESERV_CRC_SIZE + 2U) /* CRC16 + id (1 byte) + value (minimum 1 byte) */
#define ZBS_NCP_RESERV_END             ZBS_NCP_PROD_CFG_BASE

typedef ZB_PACKED_PRE struct ncp_reserv_hdr_s
{
  zb_uint8_t field_length;
  zb_uint8_t field_id;
}
ZB_PACKED_STRUCT ncp_reserv_hdr_t;

typedef ZB_PACKED_PRE struct ncp_reserv_rec_s
{
  ncp_reserv_hdr_t hdr;
  zb_uint8_t data[ZBS_NCP_RESERV_MAX_VALUE_SIZE + ZBS_NCP_RESERV_CRC_SIZE];
}
ZB_PACKED_STRUCT ncp_reserv_rec_t;

static zb_ret_t ncp_res_check_signature(void)
{
  zb_ret_t ret;
  zb_uint32_t signature;

  ret = zb_osif_nvram_read_memory(ZBS_NCP_RESERV_BASE, sizeof(signature), (zb_uint8_t *)&signature);
  ZB_ASSERT(ret == RET_OK);

  ZB_LETOH32_ONPLACE(&signature);
  if (signature != ZBS_NCP_RESERV_SIGNATURE)
  {
    /* Signature is invalid, probably the reserved flash space is not initialized */
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_res_check_signature signature invalid 0x%x", (FMT__D, signature));
    ret = RET_UNINITIALIZED;
  }

  return ret;
}

static zb_ret_t ncp_res_read_data(zb_uint32_t address, ncp_reserv_rec_t *rec)
{
  zb_ret_t ret;
  zb_uint16_t crc_calc;
  zb_uint16_t crc_read;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_res_read_data offset %u size %hu", (FMT__D_H, address - ZBS_NCP_RESERV_BASE, rec->hdr.field_length - 1U));
  /* Read the data and CRC. ID is included in length and already readed */
  ret = zb_osif_nvram_read_memory(address, (zb_uint32_t)rec->hdr.field_length - 1U, rec->data);
  ZB_ASSERT(ret == RET_OK);
  ZB_LETOH16(&crc_read, &rec->data[rec->hdr.field_length - ZBS_NCP_RESERV_CRC_SIZE - 1U]);
  crc_calc = ~zb_crc16(&rec->hdr.field_length, 0x0000U, (zb_uint32_t)rec->hdr.field_length - ZBS_NCP_RESERV_CRC_SIZE + 1U);
  if (crc_read != crc_calc)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_res_read_data crc error crc_read 0x%x crc_calc 0x%x", (FMT__H_H, crc_read, crc_calc));
    ret = RET_ERROR;
  }

  return ret;
}

static zb_uint32_t ncp_res_decode_value(ncp_reserv_rec_t *rec)
{
  zb_uint32_t ret;
  zb_uint16_t val16;
  zb_uint8_t value_size = rec->hdr.field_length - ZBS_NCP_RESERV_CRC_SIZE - 1U;

  switch (value_size)
  {
    case 1U:
      ret = rec->data[0];
      break;

    case 2U:
      ZB_LETOH16(&val16, rec->data);
      ret = val16;
      break;

    case 4U:
      ZB_LETOH32(&ret, rec->data);
      break;

    default:
      ZB_ASSERT(ZB_FALSE);
      ret = 0xFFU;
      break;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_res_decode_value value_size %hu ret %u", (FMT__H_L, value_size, ret));

  return ret;
}

zb_ret_t ncp_res_flash_read_by_id(zb_uint8_t id, zb_uint32_t *out_value)
{
  zb_ret_t ret;
  zb_bool_t id_found = ZB_FALSE;
  zb_uint32_t address;
  ncp_reserv_rec_t record;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_res_flash_read_by_id id %hu", (FMT__H, id));

  ret = ncp_res_check_signature();

  if (ret == RET_OK)
  {
    address = ZBS_NCP_RESERV_BASE + sizeof(zb_uint32_t); /* sizeof reserved flash signature */
    do
    {
      ZB_BZERO(&record.hdr, sizeof(record.hdr));
      ret = zb_osif_nvram_read_memory(address, sizeof(record.hdr), (zb_uint8_t *)&record.hdr);
      ZB_ASSERT(ret == RET_OK);

      TRACE_MSG(TRACE_TRANSPORT3, "read hdr from offset %u length %hu id %hu",
                (FMT__D_H_H, address - ZBS_NCP_RESERV_BASE, record.hdr.field_length, record.hdr.field_id));

      if (record.hdr.field_length < ZBS_NCP_RESERV_MIN_RECORD_SIZE ||
          record.hdr.field_length > ZBS_NCP_RESERV_MAX_RECORD_SIZE)
      {
        ret = RET_NOT_FOUND;
      }

      if (ret == RET_OK)
      {
        if (record.hdr.field_id == id)
        {
          id_found = ZB_TRUE;
          address += sizeof(record.hdr);
          ret = ncp_res_read_data(address, &record);
          *out_value = ncp_res_decode_value(&record);
        }
        else
        {
          address += (zb_uint32_t)record.hdr.field_length + 1U; /* + sizeof(length) */
        }
      }

      if (address >= (ZBS_NCP_RESERV_END - ZBS_NCP_RESERV_MIN_RECORD_SIZE))
      {
        ret = RET_NOT_FOUND;
      }
    } while (!id_found && ret == RET_OK);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_res_flash_read_by_id out_value %u (0x%x) ret %d", (FMT__D_D_D, *out_value, *out_value, ret));

  return ret;
}

#endif /* ZB_HAVE_CALIBRATION */
