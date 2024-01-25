#!/usr/bin/env python3.7

#/* ZBOSS Zigbee software protocol stack
# *
# * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
# * www.dsr-zboss.com
# * www.dsr-corporation.com
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
# PURPOSE: Host-side test implements APS bind/unbind requests in ZR role.
#

#1. ZR starts sending packets by using addr_mode=NCP_HL_ADDR_MODE_BIND_TBL_ID to a not existent bind ID and received RET_NOT_FOUND
#2. Then ZR creates a binding and specifies the TEST_BIND_ID to use for that binding
#3. ZR sends packets by using addr_mode=NCP_HL_ADDR_MODE_BIND_TBL_ID" to a , now, existent bind ID and it is successfull
#4. ZR unbinds binding create on 2. and data_req with addr_mode=NCP_HL_ADDR_MODE_BIND_TBL_ID returns RET_NOT_FOUND again


from host.ncp_hl import *
from host.base_test import BaseTest
from host.test_runner import TestRunner

logger = logging.getLogger(__name__)
log_file_name = "zbs_data_req_by_bind_tbl_id_zed.log"

#
# Test itself
#

TEST_BIND_ID = 10

CHANNEL_MASK = 0x80000  # channel 19


class Test(BaseTest):

    def __init__(self, channel_mask):
        super().__init__(channel_mask, ncp_hl_role_e.NCP_HL_ZED)
        self.update_response_switch({
            ncp_hl_call_code_e.NCP_HL_SET_RX_ON_WHEN_IDLE: self.set_rx_on_when_idle_rsp,
            ncp_hl_call_code_e.NCP_HL_APSME_BIND: self.apsme_bind_rsp,
            ncp_hl_call_code_e.NCP_HL_APSME_RM_BIND_ENTRY_BY_ID: self.apsme_rm_bind_id_rsp,
            ncp_hl_call_code_e.NCP_HL_AF_SET_SIMPLE_DESC: self.af_simple_desc_set,
        })

        self.update_indication_switch({
        })

        self.required_packets = [[ncp_hl_call_code_e.NCP_HL_NCP_RESET_IND,              NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_NCP_RESET,                  NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_MODULE_VERSION,         NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_LOCAL_IEEE_ADDR,        NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SET_ZIGBEE_CHANNEL_MASK,    NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_ZIGBEE_CHANNEL_MASK,    NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_ZIGBEE_CHANNEL,         NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_PAN_ID,                 NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SET_ZIGBEE_ROLE,            NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_ZIGBEE_ROLE,            NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SET_RX_ON_WHEN_IDLE,        NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_AF_SET_SIMPLE_DESC,         NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_AF_SET_SIMPLE_DESC,         NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_NWK_DISCOVERY,              NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_NWK_NLME_JOIN,              NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_APSDE_DATA_REQ,             NCP_HL_STATUS.RET_NOT_FOUND],
                                 [ncp_hl_call_code_e.NCP_HL_APSME_BIND,                 NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_APSDE_DATA_IND,             NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_APSDE_DATA_REQ,             NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_APSME_RM_BIND_ENTRY_BY_ID,  NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_APSDE_DATA_REQ,             NCP_HL_STATUS.RET_NOT_FOUND],
                                 ]

        self.set_base_test_log_file(log_file_name)
        self.set_ncp_host_hl_log_file(log_file_name)

    def begin(self):
        self.host.ncp_req_set_rx_on_when_idle(ncp_hl_on_off_e.OFF)

    def set_rx_on_when_idle_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.set_simple_desc_ep(1)
        self.set_ep = 1

    def af_simple_desc_set(self, rsp, rsp_len):
        self.rsp_log(rsp)
        if self.set_ep == 1:
            self.set_simple_desc_ep(2)
            self.set_ep = 2
        elif self.set_ep == 2:
            self.set_ep = 3
            self.host.ncp_req_nwk_discovery(0, self.channel_mask, 5)

    def on_nwk_join_complete(self, body):
        self.short_addr = ncp_hl_nwk_addr_t(body.short_addr.u16)
        self.send_data()

    def on_data_ind(self, dataind):
        if self.set_ep == 4:
            self.set_ep = 5
            self.host.ncp_apsme_rm_bind_entry_by_id(TEST_BIND_ID)

    def on_data_conf(self, conf):
        if conf.addr_mode != ncp_hl_addr_mode_e.NCP_HL_ADDR_MODE_BIND_TBL_ID:
            logger.error("addr_mode mismatch: addr_mode in apsde_data_req response is different than the one used in apsde_data_req")
        if self.set_ep == 3:
            self.set_ep = 4
            self.host.ncp_req_apsme_bind_request(ncp_hl_ieee_addr_t(self.ieee_addr), 0x01, 0x0003,
                                                 ncp_hl_addr_mode_e.NCP_HL_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT,
                                                 self.short_addr, 0x02, TEST_BIND_ID)

    def apsme_bind_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        body = ncp_hl_bind_rsp_t.from_buffer_copy(rsp.body, 0)
        logger.info("New binding id: {}".format(body.id))
        self.send_data()

    def apsme_rm_bind_id_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.send_data()

    def send_data(self):
        params = ncp_hl_data_req_param_t()
        params.addr_mode = ncp_hl_addr_mode_e.NCP_HL_ADDR_MODE_BIND_TBL_ID
        params.profileid = 0x0104
        params.clusterid = 0x0003
        params.src_endpoint = 1
        params.dst_endpoint = TEST_BIND_ID
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
