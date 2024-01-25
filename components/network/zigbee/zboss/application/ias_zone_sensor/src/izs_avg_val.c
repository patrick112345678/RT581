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
/* PURPOSE: General IAS zone device average value calculation routines
*/

#define ZB_TRACE_FILE_ID 63293
#include "izs_avg_val.h"

void izs_avg_val_calc_avg(izs_avg_data_t *avg_data)
{
  zb_uint32_t sum = 0;
  zb_uint8_t i;
  zb_uint8_t item_number;

  item_number = (avg_data->count < avg_data->data_size) ?
    avg_data->count : avg_data->data_size;

  for (i = 0; i < item_number; i++)
  {
    sum += avg_data->data_val[i];
  }
  avg_data->avg_val = (zb_uint16_t)(sum / item_number);
  avg_data->avg_valid = ZB_TRUE;
}


void izs_avg_val_put_value(izs_avg_data_t *avg_data, zb_int16_t value)
{
  avg_data->data_val[avg_data->index] = value;
  avg_data->index++;
  avg_data->index %= avg_data->data_size;

  avg_data->count++;
  if (avg_data->count > avg_data->data_size)
  {
    avg_data->count = avg_data->data_size;
  }

  izs_avg_val_calc_avg(avg_data);
}


void izs_avg_val_calc_signed_avg(izs_avg_data_t *avg_data)
{
  zb_int32_t sum = 0;
  zb_uint8_t i;
  zb_uint8_t item_number;

  item_number = (avg_data->count < avg_data->data_size) ?
    avg_data->count : avg_data->data_size;

  for (i = 0; i < item_number; i++)
  {
    sum += (zb_int32_t)((zb_int16_t)avg_data->data_val[i]);
  }
  avg_data->avg_val = (zb_int16_t)(sum / (zb_int32_t)item_number);
  avg_data->avg_valid = ZB_TRUE;
}


void izs_avg_val_put_signed_value(izs_avg_data_t *avg_data, zb_int16_t value)
{
  avg_data->data_val[avg_data->index] = value;
  avg_data->index++;
  avg_data->index %= avg_data->data_size;

  avg_data->count++;
  if (avg_data->count > avg_data->data_size)
  {
    avg_data->count = avg_data->data_size;
  }

  izs_avg_val_calc_signed_avg(avg_data);
}
