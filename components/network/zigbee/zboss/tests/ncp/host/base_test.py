# /***************************************************************************
# *                  ZBOSS Zigbee software protocol stack                    *
# *                                                                          *
# *          Copyright (c) 2012-2019 DSR Corporation Denver CO, USA.         *
# *                       http://www.dsr-zboss.com                           *
# *                       http://www.dsr-corporation.com                     *
# *                                                                          *
# *                            All rights reserved.                          *
# *                                                                          *
# *                                                                          *
# * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR   *
# * Corporation                                                              *
# *                                                                          *
# * Commercial Usage                                                         *
# * Licensees holding valid DSR Commercial licenses may use                  *
# * this file in accordance with the DSR Commercial License                  *
# * Agreement provided with the Software or, alternatively, in accordance    *
# * with the terms contained in a written agreement between you and          *
# * DSR.                                                                     *
# *                                                                          *
# ****************************************************************************
# PURPOSE: basic host side test running functionality
# */

from host.ncp_hl import *



logger = logging.getLogger(__name__)

PAN_ID = 0x5043


class BaseTest:

    def __init__(self, channel_mask: int, role: ncp_hl_role_e, pan_id=PAN_ID):
        self.reset_state = ncp_hl_rst_state_e.TURNED_ON
        self.erase_nvram_on_start = True
        self.channel_mask = channel_mask
        self.ieee_addr = None
        self.nwk_addr = None
        self.role = role
        self.pan_id = pan_id
        self.child_ieee_addr = ncp_hl_ieee_addr_t()

        self.install_code = bytes([0x83, 0xFE, 0xD3, 0x40, 0x7A, 0x93, 0x97, 0x23,
                                   0xA5, 0xC6, 0x39, 0xB2, 0x69, 0x16, 0xD5, 0x05,
                                   0xC3, 0xB5])
        self.ic_ieee_addr = bytes([0x11, 0x11, 0xef, 0xcd, 0xab, 0x50, 0x50, 0x50])
        self.nwk_key = bytes([0x11, 0xaa, 0x22, 0xbb, 0x33, 0xcc, 0x44, 0xdd, 0, 0, 0, 0, 0, 0, 0, 0])

        self.rsp_switch = {
            ncp_hl_call_code_e.NCP_HL_GET_ZIGBEE_CHANNEL_MASK: self.get_channel_mask_rsp,
            ncp_hl_call_code_e.NCP_HL_GET_ZIGBEE_CHANNEL: self.get_channel_rsp,
            ncp_hl_call_code_e.NCP_HL_GET_PAN_ID: self.get_pan_id_rsp,
            ncp_hl_call_code_e.NCP_HL_GET_LOCAL_IEEE_ADDR: self.get_local_ieee_addr_rsp,
            ncp_hl_call_code_e.NCP_HL_GET_ZIGBEE_ROLE: self.get_zigbee_role_rsp,
            ncp_hl_call_code_e.NCP_HL_NWK_GET_IEEE_BY_SHORT: self.nwk_get_ieee_by_short_rsp,
            ncp_hl_call_code_e.NCP_HL_NWK_GET_SHORT_BY_IEEE: self.nwk_get_short_by_ieee_rsp,
            ncp_hl_call_code_e.NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE: self.nwk_get_neighbor_by_ieee_rsp,
            ncp_hl_call_code_e.NCP_HL_NCP_RESET: self.ncp_reset_rsp,
            ncp_hl_call_code_e.NCP_HL_GET_MODULE_VERSION: self.get_module_version_rsp,
            ncp_hl_call_code_e.NCP_HL_SET_ZIGBEE_CHANNEL_MASK: self.channel_mask_set,
            ncp_hl_call_code_e.NCP_HL_NWK_FORMATION: self.formation_complete,
            ncp_hl_call_code_e.NCP_HL_NWK_PERMIT_JOINING: self.nwk_permit_joining_complete,
            ncp_hl_call_code_e.NCP_HL_NWK_DISCOVERY: self.nwk_discovery_complete,
            ncp_hl_call_code_e.NCP_HL_NWK_NLME_JOIN: self.nwk_join_complete,
            ncp_hl_call_code_e.NCP_HL_GET_AUTHENTICATED: self.get_authenticated_rsp,
            ncp_hl_call_code_e.NCP_HL_SECUR_ADD_IC: self.secur_add_ic_rsp,
            ncp_hl_call_code_e.NCP_HL_SET_PAN_ID: self.pan_id_set,
            ncp_hl_call_code_e.NCP_HL_SET_ZIGBEE_ROLE: self.zigbee_role_set,
            ncp_hl_call_code_e.NCP_HL_APSDE_DATA_REQ: self.data_conf,
            ncp_hl_call_code_e.NCP_HL_GET_NWK_KEYS: self.get_nwk_keys_rsp,
            ncp_hl_call_code_e.NCP_HL_SET_NWK_KEY: self.set_nwk_key_rsp,
            ncp_hl_call_code_e.NCP_HL_SECUR_JOIN_USES_IC: self.join_uses_ic_rsp
        }

        self.ind_switch = {
            ncp_hl_call_code_e.NCP_HL_ZDO_DEV_ANNCE_IND: self.dev_annce_ind,
            ncp_hl_call_code_e.NCP_HL_APSDE_DATA_IND: self.data_ind,
        }

        self.apsde_data_req_count = 0
        self.apsde_data_req_max = 10
        self.required_packets = []
        self.use_cs = 0  # For SE tests
        self.ignore_apsde_data = False # Checking the calls including APSDE data requests and indications.

    def update_response_switch(self, rsp_switch: dict):
        self.rsp_switch.update(rsp_switch)

    def update_indication_switch(self, ind_switch: dict):
        self.ind_switch.update(ind_switch)

    def init_host(self, ll_so_name):
        self.host = ncp_host(ll_so_name, self.rsp_switch, self.ind_switch, self.required_packets, self.ignore_apsde_data)

    def run(self):
        self.main_loop()

    def main_loop(self):
        while True:
            self.host.wait_for_ncp()
            self.host.run_ll_quant(None, 0)

    def ncp_reset_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        if self.erase_nvram_on_start:
            if self.reset_state == ncp_hl_rst_state_e.TURNED_ON:
                self.reset_state = ncp_hl_rst_state_e.NVRAM_ERASE
                self.host.ncp_req_reset(ncp_hl_reset_opt_e.NVRAM_ERASE)
            elif self.reset_state == ncp_hl_rst_state_e.NVRAM_ERASE:
                self.reset_state = ncp_hl_rst_state_e.NVRAM_HAS_ERASED
                self.host.ncp_req_get_module_version()
        else:
            self.host.ncp_req_get_module_version()

    def get_module_version_rsp(self, rsp, rsp_len):
        ver = rsp.body.uint32
        logger.info("NCP version 0x%x", ver)
        self.host.ncp_req_get_local_ieee_addr()

    def channel_mask_set(self, rsp, rsp_len):
        self.rsp_log(rsp)
        if self.pan_id is not None:
            self.host.ncp_req_set_pan_id(self.pan_id)
        else:
            self.host.ncp_req_set_zigbee_role(self.role)

    def pan_id_set(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.host.ncp_req_set_zigbee_role(self.role)

    def zigbee_role_set(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.begin()

    def formation_complete(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.host.ncp_req_nwk_permit_joining(0xfe)

    def nwk_permit_joining_complete(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.host.ncp_req_secur_join_uses_ic(ncp_hl_on_off_e.ON)

    def join_uses_ic_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)        
        self.host.ncp_req_secur_add_ic(self.ic_ieee_addr, self.install_code)

    def nwk_discovery_complete(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_nwk_discovery_rsp_hdr_t)
        dh = ncp_hl_nwk_discovery_rsp_hdr_t.from_buffer(rsp.body)
        for i in range(0, dh.network_count):
            dsc = ncp_hl_nwk_discovery_dsc_t.from_buffer(rsp.body, sizeof(dh) + i * sizeof(ncp_hl_nwk_discovery_dsc_t))
            self.rsp_log(rsp, ncp_hl_nwk_discovery_dsc_t)
        # In our test we have only 1 network, so join to the first one
        dsc = ncp_hl_nwk_discovery_dsc_t.from_buffer(rsp.body, sizeof(dh))
        # Join thru association to the pan and channel just found
        if self.role == ncp_hl_role_e.NCP_HL_ZR:
            self.host.ncp_req_nwk_join_req(dsc.extpanid, 0, dsc.page, (1 << dsc.channel), 5,
                                       ncl_hl_mac_capability_e.NCP_HL_CAP_ROUTER | ncl_hl_mac_capability_e.NCP_HL_CAP_ALLOCATE_ADDRESS,
                                       0)
        elif self.role == ncp_hl_role_e.NCP_HL_ZED:
            self.host.ncp_req_nwk_join_req(dsc.extpanid, 0, dsc.page, (1 << dsc.channel), 5,
                                           #ncl_hl_mac_capability_e.NCP_HL_CAP_RX_ON_WHEN_IDLE |
                                           ncl_hl_mac_capability_e.NCP_HL_CAP_ALLOCATE_ADDRESS,
                                           0)

    def nwk_join_complete(self, rsp, rsp_len):
        self.rsp_log(rsp)
        body = ncp_hl_nwk_nlme_join_rsp_t.from_buffer(rsp.body)
        if rsp.hdr.status_code == 0:
            self.rsp_log(rsp, ncp_hl_nwk_nlme_join_rsp_t)
            body = ncp_hl_nwk_nlme_join_rsp_t.from_buffer(rsp.body)
        self.on_nwk_join_complete(body)

    def get_authenticated_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        if rsp.hdr.status_code == 0:
            val = ncp_hl_uint8_t.from_buffer_copy(rsp.body, 0).uint8
            self.authenticated = val != 0
            strval = "False" if val == 0 else "True"
            logger.info("authenticated: %s", strval)

    def secur_add_ic_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.host.ncp_req_get_zigbee_role()

    def set_simple_desc_ep(self, ep_number):
        cmd = ncp_hl_set_simple_desc_t()
        cmd.hdr.endpoint = ep_number
        cmd.hdr.profile_id = 0x0104
        cmd.hdr.device_id = 0x1234
        cmd.hdr.device_version = 8
        cmd.hdr.in_clu_count = 1
        cmd.hdr.out_clu_count = 1
        cmd.clusters[0] = 0x0003
        cmd.clusters[1] = 0x0003
        self.host.ncp_req_set_simple_desc(cmd)

    def zdo_addr_opt(self):
        return 'e' if self.zdo_addr_req_type == ncl_hl_zdo_addr_req_type_e.NCP_HL_ZDO_EXTENDED_REQ else 's'

    def get_zigbee_role_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_role_t)
        self.host.ncp_req_get_channel_mask()

    def get_channel_mask_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_channel_list_t)
        self.host.ncp_req_get_channel()

    def get_channel_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_channel_t)
        self.host.ncp_req_get_pan_id()

    def get_pan_id_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_pan_id_t)
        self.on_get_pan_id_rsp()

    def get_local_ieee_addr_rsp(self, rsp, rsp_len):
        local_addr = self.rsp_rx(rsp, ncp_hl_local_addr_t)
        self.ieee_addr = ncp_hl_ieee_addr_t(local_addr.long_addr)
        self.host.ncp_req_set_zigbee_channel_mask(0, self.channel_mask)

    def nwk_get_ieee_by_short_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_ieee_addr_t)
        if rsp.hdr.status_code == 0:
            ieee_addr = ncp_hl_8b_t.from_buffer_copy(rsp.body, 0)
            self.host.ncp_req_nwk_get_neighbor_by_ieee(ieee_addr.b8)

    def nwk_get_neighbor_by_ieee_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_get_neighbor_by_ieee_t)
        if rsp.hdr.status_code == 0:
            nbt = ncp_hl_get_neighbor_by_ieee_t.from_buffer_copy(rsp.body, 0)
            self.child_ieee_addr = ncp_hl_ieee_addr_t(nbt.ieee_addr)
            self.host.ncp_req_nwk_get_short_by_ieee(nbt.ieee_addr)

    def nwk_get_short_by_ieee_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_nwk_addr_t)
        if rsp.hdr.status_code == 0:
            short_addr = ncp_hl_uint16_t.from_buffer_copy(rsp.body, 0)
            self.on_nwk_get_short_by_ieee_rsp(short_addr.uint16)

    def data_conf(self, rsp, rsp_len):
        if rsp_len <= sizeof(rsp.hdr):
            logger.info("apsde-data.conf status {} {}".format(rsp.hdr.status_category, rsp.hdr.status_code))
        else:
            self.rsp_log(rsp, ncp_hl_apsde_data_conf_t)
            logger.info("{}".format(as_enum(status_id(rsp.hdr.status_category, rsp.hdr.status_code), NCP_HL_STATUS)))
            conf = ncp_hl_apsde_data_conf_t.from_buffer(rsp.body.arr, 0)
            self.on_data_conf(conf)

    #
    # -- INDICATIONS --
    #

    def dev_annce_ind(self, ind, ind_len):
        annce = ncp_hl_dev_annce_t.from_buffer(ind.body, 0)
        logger.info("device annce: dev {:#x} ieee {} cap{:#x}".format(annce.short_addr,
                                                                      list(map(lambda x: hex(x),
                                                                               list(annce.long_addr))),
                                                                      annce.capability))
        self.on_dev_annce_ind(annce)

    def data_ind(self, ind, ind_len):
        dataind = self.ind_rx(ind, ncp_hl_data_ind_t)
        logger.info("  data: {}".format(ncp_hl_dump(dataind.data[:dataind.data_len])))
        self.on_data_ind(dataind)

    #
    # SE related methods
    #

    def get_module_version_rsp_se(self, rsp, rsp_len):
        ver = rsp.body.uint32
        logger.info("NCP ver 0x%x", ver)
        # Use local ieee which hard-coded in the certificate.
        # Certificates from CE spec uses different local addresses.
        if self.use_cs == 1:
            self.zc_ieee = esi_dev_addr_cs1
            self.zr_ieee = ihd_dev_addr_cs1
            self.issuer = esi_certificate_cs1[30:38]
        elif self.use_cs == 2:
            self.zc_ieee = esi_dev_addr_cs2
            self.zr_ieee = ihd_dev_addr_cs2
            self.issuer = esi_certificate_cs2[11:19]
        else:
            logger.error("CS must be 1 or 2!")
        self.on_get_module_version_rsp_se()

    def set_simple_desc_ep1_se(self):
        cmd = ncp_hl_set_simple_desc_t()
        cmd.hdr.endpoint = 1
        cmd.hdr.profile_id = 0x104  # HA & ZB 3.0
        cmd.hdr.device_id = 0x1234
        cmd.hdr.device_version = 8
        # Let it be on/off server. Can test it with our standalone on_off_switch_zed.c
        cmd.hdr.in_clu_count = 5
        cmd.hdr.out_clu_count = 0
        cmd.clusters[0] = 0x0003  # ZB_ZCL_CLUSTER_ID_IDENTIFY
        cmd.clusters[1] = 0x0000  # ZB_ZCL_CLUSTER_ID_BASIC
        cmd.clusters[2] = 0x0006  # ZB_ZCL_CLUSTER_ID_ON_OFF
        cmd.clusters[3] = 0x0004  # ZB_ZCL_CLUSTER_ID_GROUPS
        cmd.clusters[4] = 0x0005  # ZB_ZCL_CLUSTER_ID_SCENES
        self.host.ncp_req_set_simple_desc(cmd)

    def set_simple_desc_ep2_se(self):
        cmd = ncp_hl_set_simple_desc_t()
        cmd.hdr.endpoint = 20
        cmd.hdr.profile_id = 0x0109  # SE
        cmd.hdr.device_id = 0x1234
        cmd.hdr.device_version = 9
        # Let it be ESI - see samples/se/energy_service_interface/se_esi_zc.c
        cmd.hdr.in_clu_count = 10
        cmd.hdr.out_clu_count = 3
        # in
        cmd.clusters[0] = 0x0000  # ZB_ZCL_CLUSTER_ID_BASIC,
        cmd.clusters[1] = 0x0707  # ZB_ZCL_CLUSTER_ID_CALENDAR,
        cmd.clusters[2] = 0x0700  # ZB_ZCL_CLUSTER_ID_PRICE,
        cmd.clusters[3] = 0x000a  # ZB_ZCL_CLUSTER_ID_TIME,
        cmd.clusters[4] = 0x0025  # ZB_ZCL_CLUSTER_ID_KEEP_ALIVE,
        cmd.clusters[5] = 0x0701  # ZB_ZCL_CLUSTER_ID_DRLC,
        cmd.clusters[6] = 0x0704  # ZB_ZCL_CLUSTER_ID_TUNNELING,
        cmd.clusters[7] = 0x0703  # ZB_ZCL_CLUSTER_ID_MESSAGING,
        cmd.clusters[8] = 0x070a  # ZB_ZCL_CLUSTER_ID_MDU_PAIRING,
        cmd.clusters[9] = 0x0800  # ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT,
        # out
        cmd.clusters[10] = 0x0800  # ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT,
        cmd.clusters[11] = 0x0025  # ZB_ZCL_CLUSTER_ID_KEEP_ALIVE,
        cmd.clusters[12] = 0x000a  # ZB_ZCL_CLUSTER_ID_TIME,
        self.host.ncp_req_set_simple_desc(cmd)

    def get_nwk_keys_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp, ncp_hl_nwk_keys_t)               
        self.host.ncp_req_nwk_formation(0, self.channel_mask, 5, 0, 0)

    def set_nwk_key_rsp(self, rsp, rsp_len):
        self.rsp_log(rsp)
        self.host.ncp_req_get_nwk_keys()

    #
    # Auxiliary methods
    #

    def begin(self):
        pass

    def on_data_conf(self, conf):
        pass

    def on_data_ind(self, dataind):
        pass

    def on_dev_annce_ind(self, annce):
        pass

    def on_nwk_get_short_by_ieee_rsp(self, short_addr):
        pass

    def on_get_pan_id_rsp(self):
        pass

    def on_nwk_join_complete(self, body):
        pass

    def on_get_module_version_rsp_se(self):
        pass

    def send_packet_back(self, dataind):
        # Sends a packet back to the sender.
        if self.apsde_data_req_count < self.apsde_data_req_max:
            self.apsde_data_req_count += 1
            params = ncp_hl_data_req_param_t()
            params.dst_addr.short_addr = dataind.src_addr
            params.addr_mode = ncp_hl_addr_mode_e.NCP_HL_ADDR_MODE_16_ENDP_PRESENT
            params.profileid = dataind.profileid
            params.clusterid = dataind.clusterid
            params.dst_endpoint = dataind.src_endpoint
            params.src_endpoint = dataind.dst_endpoint
            params.radius = 30
            params.tx_options = ncp_hl_tx_options_e.NCP_HL_TX_OPT_SECURITY_ENABLED | ncp_hl_tx_options_e.NCP_HL_TX_OPT_ACK_TX
            params.use_alias = 0
            params.alias_src_addr = 0
            params.alias_seq_num = 0
            self.host.ncp_req_apsde_data_request(params, dataind.data, dataind.data_len)

    def log_info(self, info):
        for line in info:
            if line is not None:
                logger.info(line)

    def rsp_log(self, rsp, cls=None, frame=None):
        self.log_info(ncp_hl_rsp_info(rsp, cls, ncp_hl_frame(frame)))

    def rsp_rx(self, rsp, cls):
        self.rsp_log(rsp, cls, ncp_hl_frame(None))
        return ncp_hl_body(rsp, cls)

    def ind_log(self, ind, cls=None, frame = None):
        self.log_info(ncp_hl_ind_info(ind, cls, ncp_hl_frame(frame)))

    def ind_rx(self, ind, cls):
        self.ind_log(ind, cls, ncp_hl_frame(None))
        return ncp_hl_body(ind, cls)
