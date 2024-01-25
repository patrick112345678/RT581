Bug 13972 - Multiprotocol ZR fails due to NVRAM corruption
RTP_MEMCONF_01 - ZB_SINGLE_TRANS_INDEX_SIZE and ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE

Objective:

	To confirm that memconfig options does not causes nvram corruption at the application start and after power cycle.

Devices:

	1. TH  - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZR
        3. Power cycle DUT ZR

Expected outcome:

	1. TH ZC creates a network

    2. DUT ZR joins to the network created by TH ZC

    3. DUT ZR responds to 8 Binding Table Requests from TH ZC
                Status: Success (0)
                Table Size: 16

        4. (Test procedure 3) DUT ZR successfully starts after power cycle and sending Link Status packets

    5. DUT ZR responds to 8 Binding Table Requests from TH ZC
                Status: Success (0)
                Table Size: 16

    6. There is no trace messages in the dut's trace: "tests_validate_trans_index_size(): TEST FAILED, ZB_SINGLE_TRANS_INDEX_SIZE does not match, ind [any index]"

    7. There is no trace messages in the dut's trace: "tests_validate_trans_index_size(): TEST FAILED, ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE does not match, ind [any index]"

        8. DUT ZR continues operating on the network and sending Link Status packets
