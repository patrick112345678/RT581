Bug 15914 - zb_bdb_reset_via_local_action triggers commissioning when node not on the network
RTP_BDB_01 - zb_bdb_reset_via_local_action triggers commissioning when node not on the network

Objective:

	To confirm that the function zb_bdb_reset_via_local_action does not trigger commissioning when the node is not connected to the network.

Devices:

	1. TH  - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZR
        3. Wait for DUT ZR Leaves the network
        4. Wait for 10 seconds

Expected outcome:

	1. ZC creates a network

	2. ZR starts bdb_top_level_commissioning and gets on the network established by ZC

	3.1 ZR calls zb_bdb_reset_via_local_action(0U) to perform factory reset. ZR leaves the network and sends Leave.
	3.2 There is trace msg in dut's trace: "zb_bdb_reset_via_local_action param {buf_id}: send leave to myself"

	4.1 ZR calls zb_bdb_reset_via_local_action(0U) AGAIN while it is not on any network.
	4.2 There is trace msg in dut's trace: "zb_bdb_reset_via_local_action param {buf_id}: send leave to myself"

	5. ZR does not perform commissioning again.
