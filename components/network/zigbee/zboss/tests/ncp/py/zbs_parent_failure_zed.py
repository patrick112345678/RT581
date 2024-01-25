#!/usr/bin/env python3.7

#/* ZBOSS Zigbee software protocol stack
# *
# * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
# * http://www.dsr-zboss.com
# * http://www.dsr-corporation.com
# * All rights reserved.
# *
# * This is unpublished proprietary source code of DSR Corporation
# * The copyright notice does not evidence any actual or intended
# * publication of such source code.
# *
# * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
# * Corporation
# *
# * Commercial Usage
# * Licensees holding valid DSR Commercial licenses may use
# * this file in accordance with the DSR Commercial License
# * Agreement provided with the Software or, alternatively, in accordance
# * with the terms contained in a written agreement between you and
# * DSR.
#
# PURPOSE: Host-side test implements ZED role, works out the functions of receiving
# NCP_HL_PARENT_LOST_IND indication
#
# It join to test network, then you need to shutdown the parent after appearing in the log
# "shutdown parent"

from host.ncp_hl import *
from host.base_test import BaseTest
from host.test_runner import TestRunner
from time import sleep

logger = logging.getLogger(__name__)
log_file_name = "zbs_parent_failure_zed.log"

#
# Test itself
#

CHANNEL_MASK = 0x80000  # channel 19

LONG_POLL_INTERVAL = 24 # 24 = 6seconds * 4 quarterseconds
FAST_POLL_INTERVAL = 4  # 4 = 1second * 4 quarterseconds
LONG_POLL_PKT_N    = 5  # number of packets for receiving in long polling to change state
FAST_POLL_PKT_N    = 5  # number of packets for receiving in fast polling to change state
STOP_POLL_TIME     = 10 # on that time all polling will be stopped


class Test(BaseTest):

    def __init__(self, channel_mask):
        super().__init__(channel_mask, ncp_hl_role_e.NCP_HL_ZED)
        self.update_response_switch({
            ncp_hl_call_code_e.NCP_HL_SET_RX_ON_WHEN_IDLE: self.set_rx_on_when_idle_rsp,
            ncp_hl_call_code_e.NCP_HL_GET_RX_ON_WHEN_IDLE: self.get_rx_on_when_idle_rsp,
            ncp_hl_call_code_e.NCP_HL_NWK_DISCOVERY: self.nwk_discovery_complete,
            ncp_hl_call_code_e.NCP_HL_GET_JOINED: self.get_joined_rsp,
            ncp_hl_call_code_e.NCP_HL_NWK_NLME_JOIN: self.nwk_join_complete_local,
            ncp_hl_call_code_e.NCP_HL_PIM_STOP_POLL: self.pim_stop_poll_rsp
        })

        self.required_packets = [[ncp_hl_call_code_e.NCP_HL_NCP_RESET_IND,              NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_NCP_RESET,                  NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_MODULE_VERSION,         NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_LOCAL_IEEE_ADDR,        NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SET_ZIGBEE_CHANNEL_MASK,    NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SET_ZIGBEE_ROLE,            NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SET_RX_ON_WHEN_IDLE,        NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_NWK_DISCOVERY,              NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_JOINED,                 NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_NWK_NLME_JOIN,              NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_PIM_STOP_POLL,              NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_APSDE_DATA_REQ,             NCP_HL_STATUS.ZB_APS_STATUS_NO_ACK],
                                 [ncp_hl_call_code_e.NCP_HL_APSDE_DATA_REQ,             NCP_HL_STATUS.ZB_APS_STATUS_NO_ACK],
                                 [ncp_hl_call_code_e.NCP_HL_PARENT_LOST_IND,            NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_JOINED,                 NCP_HL_STATUS.RET_OK],
                                 ]

        self.update_indication_switch({
            ncp_hl_call_code_e.NCP_HL_PARENT_LOST_IND: self.parent_lost_ind
        })

        self.send_pack = 0
        self.join_complete_cnt = 0
        self.set_base_test_log_file(log_file_name)
        self.set_ncp_host_hl_log_file(log_file_name)

    def begin(self):
        self.host.ncp_req_set_rx_on_when_idle(ncp_hl_on_off_e.OFF)

    def set_rx_on_when_idle_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.host.ncp_req_nwk_discovery(0, CHANNEL_MASK, 5)

    def get_rx_on_when_idle_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)

    def nwk_discovery_complete(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_nwk_discovery_rsp_hdr_t)
        dh = ncp_hl_nwk_discovery_rsp_hdr_t.from_buffer_copy(rsp.body)
        for i in range(0, dh.network_count):
            dsc = ncp_hl_nwk_discovery_dsc_t.from_buffer_copy(rsp.body, sizeof(dh) + i * sizeof(ncp_hl_nwk_discovery_dsc_t))
            self.rsp_log(rsp, ncp_hl_nwk_discovery_dsc_t)
        if dh.network_count > 0:
            # In our test we have only 1 network, so join to the first one
            dsc = ncp_hl_nwk_discovery_dsc_t.from_buffer_copy(rsp.body, sizeof(dh))
            self.extpanid = ncp_hl_ext_panid_t(dsc.extpanid.b8)
            self.mask = (1 << dsc.channel)
            # Join thru association to the pan and channel just found
            self.host.ncp_req_nwk_join_req(dsc.extpanid, 0, dsc.page, (1 << dsc.channel), 5,
                                            ncl_hl_mac_capability_e.NCP_HL_CAP_ALLOCATE_ADDRESS, 0)
            self.host.ncp_req_get_joined()
        else:
            self.host.ncp_req_nwk_discovery(0, CHANNEL_MASK, 5)

    def nwk_join_complete_local(self, rsp, rsp_len):
        self.rsp_log(rsp)
        if rsp.hdr.status_code == 0:
            self.rsp_log(rsp, ncp_hl_nwk_nlme_join_rsp_t)
            logger.info("SHUTDOWN THE PARENT DEVICE")
            self.join_complete_cnt = self.join_complete_cnt + 1
            sleep(5)
            self.host.ncp_req_pim_stop_poll()
        else:
            self.host.ncp_req_nwk_discovery(0, CHANNEL_MASK, 5)

    def pim_stop_poll_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.send_data()

    def parent_lost_ind(self, ind, ind_len):
        # here if parent is lost
        self.ind_log(ind)
        self.host.ncp_req_get_joined()

    def get_joined_rsp(self, rsp, rsp_len):
        if rsp.hdr.status_code == 0:
            jrsp = ncp_hl_get_joined_t.from_buffer_copy(rsp.body)
            logger.info("GET_JOINED rsp: joined %u", jrsp.joined)
        else:
            self.rsp_log(rsp)

    def on_data_conf(self, conf):
        if self.send_pack <= 4:
            self.send_data()
        self.send_pack += 1

    def send_data(self):
        params = ncp_hl_data_req_param_t()
        params.addr_mode = ncp_hl_addr_mode_e.NCP_HL_ADDR_MODE_16_ENDP_PRESENT
        params.dst_addr.short_addr = 0x0000
        params.dst_endpoint = 0
        params.profileid = 0x0104
        params.clusterid = 0x0003
        params.src_endpoint = 1
        params.tx_options = ncp_hl_tx_options_e.NCP_HL_TX_OPT_SECURITY_ENABLED | ncp_hl_tx_options_e.NCP_HL_TX_OPT_ACK_TX
        params.use_alias = 0
        params.alias_src_addr = 0
        params.alias_seq_num = 0
        params.radius = 30
        self.host.ncp_req_apsde_data_request(params, ncp_hl_apsde_data_t(1, 2, 3, 4, 5, 6, 7, 8, 9, 10), 10)

def main():
    logger = logging.getLogger(__name__)
    loggerFormat = '[%(levelname)8s : %(name)40s]\t%(asctime)s:\t%(message)s'
    loggerFormatter = logging.Formatter(loggerFormat)
    loggerLevel = logging.DEBUG
    logging.basicConfig(format=loggerFormat, level=loggerLevel)
    fh = logging.FileHandler(log_file_name)
    fh.setLevel(loggerLevel)
    fh.setFormatter(loggerFormatter)
    logger.addHandler(fh)

    try:
        test = Test(CHANNEL_MASK)
        logger.info("Running test")
        so_name = TestRunner().get_ll_so_name()
        test.init_host(so_name)
        test.run()
    except KeyboardInterrupt:
        return
    finally:
        logger.removeHandler(fh)


if __name__ == "__main__":
    main()
