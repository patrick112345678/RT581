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
/* PURPOSE:
*/

#define ZB_TRACE_FILE_ID 41678

#include "spi_test.h"

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

        for (i = 2; i < len; i++)
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
    TRACE_MSG(TRACE_APP1, "STATS: n_packs = %d, n_packs_corrupted = %d",
              (FMT__D_D, stats.n_packs, stats.n_packs_corrupted));

    ZB_SCHEDULE_ALARM(stats_print, 0,
                      ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_STATS_PERIOD_MS));
}
