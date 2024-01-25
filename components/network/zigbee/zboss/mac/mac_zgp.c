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
/* PURPOSE: MAC. GreenPower-related stuff
*/

#define ZB_TRACE_FILE_ID 289
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

/*! \addtogroup ZB_MAC */
/*! @{ */


#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_mac_globals.h"

#ifdef ZB_ENABLE_ZGP_DIRECT



void zb_mac_send_zgpd_frame(zb_uint8_t param)
{
    zb_mcps_data_req_params_t *data_req_params = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);

    TRACE_MSG(TRACE_MAC3, "zb_mac_send_zgpd_frame frame %hd, channel %hd, tx_time %ld",
              (FMT__H_H_L, param, MAC_PIB().phy_current_channel, data_req_params->src_addr.tx_at));

    ZB_ASSERT(MAC_CTX().flags.tx_ok_to_send);
    MAC_CTX().flags.tx_q_busy = ZB_TRUE;
    MAC_CTX().flags.tx_radio_busy = ZB_TRUE;
    ZB_MAC_CLEAR_ACK_NEEDED();
    ZB_TRANS_SEND_FRAME(data_req_params->mhr_len, param, ZB_MAC_TX_WAIT_ZGP);

    TRACE_MSG(TRACE_MAC3, "<< zb_mac_send_zgpd_frame", (FMT__0));
}

#endif  /* ZB_ENABLE_ZGP_DIRECT */

/*! @} */

#endif /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */
