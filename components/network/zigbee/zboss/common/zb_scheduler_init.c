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
/* PURPOSE: Zigbee scheduler: init
*/

/*! \addtogroup ZB_BASE */
/*! @{ */

#define ZB_TRACE_FILE_ID 125
#include "zb_common.h"
#if (MODULE_ENABLE(SUPPORT_DEBUG_CONSOLE))
#include "shell.h"
#endif
void zb_sched_init() /* __reentrant for sdcc, to save DSEG space */
{
    zb_uint8_t i;

    ZB_POOLED_LIST8_INIT(ZG->sched.tm_freelist);
    ZB_POOLED_LIST8_INIT(ZG->sched.tm_queue);
    for (i = 0 ; i < ZB_SCHEDULER_Q_SIZE ; ++i)
    {
        ZB_POOLED_LIST8_INSERT_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_freelist, next, i);
    }
}

zb_bool_t zb_sheduler_is_stop()
{
#if (MODULE_ENABLE(SUPPORT_DEBUG_CONSOLE))
    sh_args_t sh_arg = {0};
    sh_arg.is_blocking = 0;
    shell_proc(&sh_arg);
#endif
    if (ZB_OSIF_IS_EXIT())
        /*cstat !MISRAC2012-Rule-2.1_b */
        /** @mdr{00011,0} */
    {
        return ZB_TRUE;
    }

    if (ZG->sched.stop)
    {
        return ZB_TRUE;
    }
    return ZB_FALSE;
}

void zb_sched_stop()
{
    TRACE_MSG(TRACE_ERROR, ">> zb_sched_stop", (FMT__0));
    ZG->sched.stop = ZB_TRUE;
}



/*! @} */
