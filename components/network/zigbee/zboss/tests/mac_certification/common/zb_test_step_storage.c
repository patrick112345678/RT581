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

#define ZB_TRACE_FILE_ID 40372
#include "zb_common.h"
#include "zb_test_step.h"

static zb_test_step_info_t gs_steps[ZB_TEST_STEP_MAX_COUNT];
static zb_uint16_t gs_steps_count = 0;

void test_step_register(zb_callback_t cb, zb_uint8_t param, zb_uint32_t step_time)
{
    ZB_ASSERT(gs_steps_count < ZB_TEST_STEP_MAX_COUNT);

    gs_steps[gs_steps_count].callback = cb;
    gs_steps[gs_steps_count].param = param;
    gs_steps[gs_steps_count].step_time = step_time;

    gs_steps_count++;
}

zb_uint16_t test_step_count()
{
    return gs_steps_count;
}

zb_test_step_info_t *test_step_get_info(zb_uint16_t idx)
{
    ZB_ASSERT(idx < gs_steps_count);
    return &gs_steps[idx];
}
