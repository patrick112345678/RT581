/**
 * Copyright (c) 2020 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/* PURPOSE:
*/

#define ZB_TRACE_FILE_ID 15944

#include "ll_dev_tests.h"

typedef struct stats_obj_s
{
  zb_uint32_t n_packs_loss;
  zb_uint32_t n_packs_corrupted;
  zb_uint32_t n_packs;
}
stats_obj_t;

stats_obj_t stats;

static zb_uint8_t pack_num = 0;

/*
 * packet format: [0]-num, [1]-len, [2..len]-data  
*/
void stats_add(zb_uint8_t *buf, zb_ushort_t len)
{
  static zb_uint8_t first_run = 1;
  zb_uint32_t i = 0;
  zb_uint8_t expected_data = 0;
  zb_uint8_t expected_len = 0;

  if (first_run)
  {
    stats.n_packs_loss = 0;
    stats.n_packs_corrupted = 0;
    stats.n_packs = 0;
    first_run = 0;

    /* already loss some packets, before first packet was received */
    if (buf[0])
    {
      stats.n_packs_loss = buf[0];
    }
  }
  else
  {
    if ((pack_num + 1) != buf[0])
    {
      if (buf[0] > (pack_num + 1))
      {
        stats.n_packs_loss += (buf[0] - (pack_num + 1));
      }
      else
      {
        stats.n_packs_loss += (buf[0] + 256 - (pack_num + 1));
      }
    }
  }

  pack_num = buf[0];

#ifdef TEST_STATS_CORRUPTED
  expected_len = buf[1];
  if (len == expected_len)
  {
    if (pack_num & 0x01)
    {
      expected_data = TEST_STATS_ODD_BYTE;
    }
    else
    {
      expected_data = TEST_STATS_EVEN_BYTE;
    }

    for(i = 2; i < len; i++)
    {
      if (buf[i] != expected_data)
      {
        stats.n_packs_corrupted++;
        break;
      }
    }
  }
  else
  {
    stats.n_packs_corrupted++;
  }
#endif

  stats.n_packs++;
}

void stats_print(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, "STATS: n_packs = %d, n_packs_corrupted = %d",
            (FMT__D_D, stats.n_packs, stats.n_packs_corrupted));

  ZB_SCHEDULE_ALARM(stats_print, 0,
                        ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_STATS_PERIOD_MS));
}
