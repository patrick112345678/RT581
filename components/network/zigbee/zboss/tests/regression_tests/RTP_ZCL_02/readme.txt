Bug 15005 - Impossible to trigger cluster_write_attr_hook if endpoint has both client and server roles of a cluster implemented
RTP_ZCL_02 - cluster_write_attr_hook if endpoint has both client and server roles of a cluster implemented

Objective:

	To confirm that the .cluster_check_value and .cluster_write_attr_hook handlers are correctly stored and launched for devices with server and client roles implemented on the same endpoint.

Devices:

	1. DUT - ZC
	2. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZR
        3. Wait for TH ZR send write attribute request

Expected outcome:

	1. ZC creates a network

	2. ZR starts bdb_top_level_commissioning and gets on the network established by ZC

	3. TH ZR sends Write Attributes command for DUT ZC
               Attribute: OnTime (0x4001)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 1.0 seconds

	4. DUT ZC responds with the Write Attributes Response
               Status: Success (0x00)

	5.1. DUT ZC sends two Link Quality Requests commands with start_index 1:
        5.2. There is trace msg in dut's trace: "test_check_value_on_off_srv(): test OK"
        5.3. There isn't trace msg in dut's trace: "test_check_value_on_off_cli(): test FAILED"

    	6.1. DUT ZC sends two Link Quality Request command with start_index 2:
        6.2. There is trace msg in dut's trace: "test_attr_hook_on_off_srv(): test OK"
        6.3. There isn't trace msg in dut's trace: "test_attr_hook_on_off_cli(): test FAILED"
