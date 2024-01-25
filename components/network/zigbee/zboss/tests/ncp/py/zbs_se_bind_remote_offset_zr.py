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
# PURPOSE: Host-side test implements ZR part of Key Establishment.
#
# SE join and key establishment - ZR role. Keys & IC for IHD in samples/se
# As a ZC can use standalone SE test esi_device from samples/se/energy_service_interface
# As ZED for partner lk establishment use standalone SE test samples/se/metering/metering_device
# That test requires 3 certificates while SE spec provides just 2, so generate your own certificates.
#

#
# Note: that test tests both Partner LK establishment from our side (NCP_HL_SECUR_START_PARTNER_LK req and resp)
# and Partner LK establishment from remote side (we got NCP_HL_SECUR_PARTNER_LK_FINISHED_IND indication in such case).
# Second Partner LK establishment from the standalon test is not a mistake - this is the intended behavior!
#


from host.ncp_hl import *
from host.base_test import BaseTest
from host.test_runner import TestRunner
from time import sleep
import os

logger = logging.getLogger(__name__)

#log_file_name = "zbs_se_bind_remote_offset.log"
log_file_name = os.path.basename(__file__).split(".")[0] + ".log"

#
# Test itself
#

CHANNEL_MASK = 0x80000  # channel 19

# CS1 - 1, CS2 - 2.
#
# Note about testing using esi_device ZC: be sure channel and CS is
# same at both tests. To change CS for esi_device update
# se/common/se_common.h and recompile.
# Note that installcodes are different for different CS, too, so with wrong CS we can't even authenticate.

# Set remote bind offset in local NCP ZR
# Test partner LK establishment with samples/se/metering/metering_device.
# Test that remote bind triggered by Partner LK establisment is bigger than the remote binding offset set

# Keys from SE specification. Can use it for testing freely.
# Got from samples/se/common/se_cert_spec.h
#
# ZC is ESI from se_cert_spec.h.
# ZR is IHD from se_cert_spec.h.
#

# explicit id for local binding and id to use remove by id
TEST_BIND_ID = 75


class Test(BaseTest):

    def __init__(self, channel_mask):
        super().__init__(channel_mask, ncp_hl_role_e.NCP_HL_ZR)
        self.update_response_switch({
            ncp_hl_call_code_e.NCP_HL_GET_MODULE_VERSION: self.get_module_version_rsp_se,
            ncp_hl_call_code_e.NCP_HL_NWK_NLME_JOIN: self.nwk_join_complete,
            ncp_hl_call_code_e.NCP_HL_SECUR_START_KE: self.secur_sbke_rsp,
            ncp_hl_call_code_e.NCP_HL_ZDO_NODE_DESC_REQ: self.zdo_node_desc_rsp,
            ncp_hl_call_code_e.NCP_HL_AF_SET_SIMPLE_DESC: self.af_simple_desc_set,
            ncp_hl_call_code_e.NCP_HL_SECUR_START_PARTNER_LK: self.partner_lk_rsp,
            ncp_hl_call_code_e.NCP_HL_SET_REMOTE_BIND_OFFSET: self.set_remote_bind_offset_rsp,
            ncp_hl_call_code_e.NCP_HL_GET_REMOTE_BIND_OFFSET: self.get_remote_bind_offset_rsp,
            ncp_hl_call_code_e.NCP_HL_APSME_BIND: self.apsme_bind_rsp,
            ncp_hl_call_code_e.NCP_HL_APSME_RM_BIND_ENTRY_BY_ID: self.apsme_rm_bind_entry_by_id_rsp
        })

        self.update_indication_switch({
            ncp_hl_call_code_e.NCP_HL_ZDO_DEV_ANNCE_IND: self.dev_annce_ind,
            ncp_hl_call_code_e.NCP_HL_SECUR_PARTNER_LK_FINISHED_IND: self.partner_lk_ind,
            ncp_hl_call_code_e.NCP_HL_APSME_REMOTE_BIND_IND: self.remote_bind_ind
        })

        self.required_packets = [[ncp_hl_call_code_e.NCP_HL_NCP_RESET_IND,                  NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_NCP_RESET,                      NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_MODULE_VERSION,             NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_LOCAL_IEEE_ADDR,            NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SET_ZIGBEE_CHANNEL_MASK,        NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_ZIGBEE_CHANNEL_MASK,        NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_ZIGBEE_CHANNEL,             NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_PAN_ID,                     NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SET_ZIGBEE_ROLE,                NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_ZIGBEE_ROLE,                NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_PIM_START_POLL,                 NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_APSME_BIND,                     NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_APSME_RM_BIND_ENTRY_BY_ID,      NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SET_REMOTE_BIND_OFFSET,         NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_GET_REMOTE_BIND_OFFSET,         NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_AF_SET_SIMPLE_DESC,             NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_AF_SET_SIMPLE_DESC,             NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_NWK_DISCOVERY,                  NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_NWK_NLME_JOIN,                  NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SECUR_START_KE,                 NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_ZDO_DEV_ANNCE_IND,              NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SECUR_START_PARTNER_LK,         NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_APSME_REMOTE_BIND_IND,          NCP_HL_STATUS.RET_OK],
                                 [ncp_hl_call_code_e.NCP_HL_SECUR_PARTNER_LK_FINISHED_IND,  NCP_HL_STATUS.RET_OK]
                                 ]

        self.use_cs = 1
        self.key_idx = 0
        self.remote_bind_offset = 75
        self.test_ieee_addr = bytes([0x00, 0x00, 0xef, 0xcd, 0xab, 0x50, 0x50, 0x50])


        self.set_base_test_log_file(log_file_name)
        self.set_ncp_host_hl_log_file(log_file_name)

    def get_module_version_rsp_se(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_module_ver_t)
        # Use local ieee which hard-coded in the certificate.
        # Certificates from CE spec uses different local addresses.
        if self.use_cs == 1:
            self.zc_ieee = esi_dev_addr_cs1
            self.zr_ieee = ihd_dev_addr_cs1
        else:
            self.zc_ieee = esi_dev_addr_cs2
            self.zr_ieee = ihd_dev_addr_cs2
        self.host.ncp_req_get_local_ieee_addr(0)

    def begin(self):
        self.host.ncp_req_apsme_bind_request(ncp_hl_ieee_addr_t(self.ieee_addr), 0x01, 0x0003,
                                                 ncp_hl_addr_mode_e.NCP_HL_ADDR_MODE_64_ENDP_PRESENT,
                                                 ncp_hl_ieee_addr_t(self.test_ieee_addr), 0xff, TEST_BIND_ID)

    def apsme_bind_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        body = ncp_hl_bind_rsp_t.from_buffer_copy(rsp.body, 0)
        logger.info("New binding id:{}".format(body.id))

        if body.id != TEST_BIND_ID:
            logger.error("Bind id requested does not match the one attributed")

        self.host.ncp_apsme_rm_bind_entry_by_id(TEST_BIND_ID)

    def apsme_rm_bind_entry_by_id_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        if (rsp.hdr.status_code != 0):
            logger.error("rm_bind_entry_by_id rsp status != 0")

        self.host.ncp_apsme_set_remote_bind_offset(self.remote_bind_offset)

    def set_remote_bind_offset_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.host.ncp_apsme_get_remote_bind_offset()

    def get_remote_bind_offset_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        offset = ncp_hl_uint8_t.from_buffer_copy(rsp.body, 0).uint8
        logger.debug("remote offset = {}".format(offset))
        if offset != self.remote_bind_offset:
            logger.error("remote offset set and get do not match")
        self.set_simple_desc_ep1_se()
        self.set_ep = 1

    def nwk_join_complete(self, rsp, rsp_len):
        self.rsp_log(rsp)
        body = ncp_hl_nwk_nlme_join_rsp_t.from_buffer_copy(rsp.body)
        if rsp.hdr.status_code == 0:
            logger.debug("joined ok short_addr {:#x} extpanid {} page {} channel {} enh_beacon {} mac_if {}".format(
                body.short_addr, body.extpanid,
                body.page, body.channel, body.enh_beacon, body.mac_iface_idx))
            self.host.ncp_req_secur_start_ke(self.use_cs, 0x0000)

    def secur_sbke_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)

        # resp to partner link key establishment initiated by us

    def partner_lk_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        if status_id(rsp.hdr.status_category, rsp.hdr.status_code) == NCP_HL_STATUS.RET_TIMEOUT:
            # timeout, make request again
            self.host.ncp_req_establish_partner_lk(self.partner_addr)

        # indication of partner link key establishment initiated by other device

    def partner_lk_ind(self, ind, ind_len):
        peer_addr = ncp_hl_8b_t.from_buffer_copy(ind.body, 0)
        logger.info("partner_lk_ind: peer addr {}".format(list(map(lambda x: hex(x), list(peer_addr.b8)))))

    def on_nwk_get_short_by_ieee_rsp(self, short_addr):
        self.host.ncp_req_zdo_node_desc(short_addr)

    def zdo_node_desc_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        if rsp.hdr.status_code == 0:
            self.rsp_log(rsp, ncp_hl_zdo_node_desc_t)

    def set_simple_desc_ep2(self):
        cmd = ncp_hl_set_simple_desc_t()
        cmd.hdr.endpoint = 20
        cmd.hdr.profile_id = 0x0109  # SE
        cmd.hdr.device_id = 0x1234
        cmd.hdr.device_version = 9
        # Let it be IHD. Actually we need only KEC now, but let's setup
        # the same set of descriptors as our IHD does - see
        # ZB_SE_DECLARE_IN_HOME_DISPLAY_CLUSTER_LIST()
        cmd.hdr.in_clu_count = 2
        cmd.hdr.out_clu_count = 11
        cmd.clusters[0] = 0x0000  # ZB_ZCL_CLUSTER_ID_BASIC srv
        cmd.clusters[1] = 0x0800  # ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT src
        cmd.clusters[2] = 0x0800  # ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT cli
        cmd.clusters[3] = 0x000a  # ZB_ZCL_CLUSTER_ID_TIME cli
        cmd.clusters[4] = 0x0707  # ZB_ZCL_CLUSTER_ID_CALENDAR cli
        cmd.clusters[5] = 0x0702  # ZB_ZCL_CLUSTER_ID_METERING cli
        cmd.clusters[6] = 0x0700  # ZB_ZCL_CLUSTER_ID_PRICE cli
        cmd.clusters[7] = 0x0701  # ZB_ZCL_CLUSTER_ID_DRLC cli
        cmd.clusters[8] = 0x0025  # ZB_ZCL_CLUSTER_ID_KEEP_ALIVE cli
        cmd.clusters[9] = 0x0709  # ZB_ZCL_CLUSTER_ID_EVENTS cli
        cmd.clusters[10] = 0x0703  # ZB_ZCL_CLUSTER_ID_MESSAGING cli
        cmd.clusters[11] = 0x0706  # ZB_ZCL_CLUSTER_ID_ENERGY_MANAGEMENT cli
        cmd.clusters[12] = 0x070A  # ZB_ZCL_CLUSTER_ID_MDU_PAIRING cli
        cmd.clusters[13] = 0x0705  # ZB_ZCL_CLUSTER_ID_PREPAYMENT cli
        self.set_ep = 2
        self.host.ncp_req_set_simple_desc(cmd)

    def af_simple_desc_set(self, rsp, rsp_len):
        self.rsp_log(rsp)
        if self.set_ep == 1:
            self.set_simple_desc_ep2()
        else:
            self.host.ncp_req_nwk_discovery(0, CHANNEL_MASK, 5)

    def dev_annce_ind(self, ind, ind_len):
        annce = ncp_hl_dev_annce_t.from_buffer_copy(ind.body, 0)
        logger.info("device annce: dev {:#x} ieee {} cap {:#x}".format(annce.short_addr, list(
            map(lambda x: hex(x), list(annce.long_addr))), annce.capability))
        # We are ZR, so the only possible device annce is from another joined device.
        # After got device annce start Partner LK establishment.
        self.partner_addr = annce.short_addr
        self.partner_ieee = annce.long_addr
        sleep(5)
        self.host.ncp_req_establish_partner_lk(self.partner_addr)

    def remote_bind_ind(self, ind, ind_len):
        self.ind_log(ind, ncp_hl_bind_entry_t)
        body = ncp_hl_bind_entry_t.from_buffer_copy(ind.body, 0)
        logger.info("id of new remote binding is {}".format(body.id))

        if body.id < self.remote_bind_offset:
            logger.error("Bind offset not applied")

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
