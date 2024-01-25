Bug 14941 - Allow to set wildcard profile on an endpoint
RTP_ZDO_01 - Order of af_data_cb() and stack processing

Objective:

        Confirm that callback which is set by zb_af_set_data_indication() launches before stack handling of ZDO and ZCL frames.

Devices:

	1. DUT - ZC
	2. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.
        2. [APP], [ZCL], [ZDO] trace are enabled on the DUT

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZR
        3. TH sends Read Attributes Command
        4. TH sends MGMT LQI Request Command

Expected outcome:

	1. ZC creates a network

	2. ZR starts bdb_top_level_commissioning and gets on the network established by ZC

    3. DUT ZC successfully binds to TH ZR identify cluster

	4.1. DUT ZC responds for the read attribute from TH ZR with Read Attribute Response:
               Attribute: Unknown (0xfffd)
               Status: Success (0x00)
               Data Type: 16-Bit Unsigned Integer (0x21)
               Uint16: 1 (0x0001)
        4.2. There is trace msg in dut's trace: "test_data_indication_cb(): read_attr received, param %d, aps_counter %d, tsn %d"
        4.3. aps_counter and tsn from the log file is equal to APS Counter and ZCL Sequence Number from wireshark trace
        4.4. There is trace msg in dut's trace AFTER msg from 4.2: "> zb_zcl_process_device_command %hd"
        4.5. Buf id from 4.2 is equal to buf id from 4.4.

        5.1. DUT ZC responds for the LQI Request from TH ZR with LQI Response:
               Status: Success (0)
        5.2. There is trace msg in dut's trace: "test_data_indication_cb(): lqi_req received, param %d, aps_counter %d, tsn %d"
        5.3. aps_counter and tsn from the log file is equal to APS Counter and ZDP Sequence Number from wireshark trace
        5.4. There is trace msg in dut's trace AFTER msg from 5.2: ">> zb_zdo_data_indication %hd clu 0x%hx"
        5.5. Buf id from 5.2 is equal to buf id from 5.4.
