Bug 14951 - Application is never informed about the end of identification
RTP_BDB_02 - Identify notification handler

Objective:

	To confirm that the identify_handler is called with param ZB_FALSE when identification finished.

Devices:

	1. TH  - ZC [Finding & binding initiator]
	2. DUT - ZR [Finding & binding target]

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZR

Expected outcome:

	1. ZC creates a network

	2. ZR starts bdb_top_level_commissioning and gets on the network established by ZC

        3. TH ZC initiates F&B process

    4. There is trace msg in dut's trace: "identify_handler(): identification is cancelled on an endpoint - test OK"

-----------------------------------------------------------------------------------------------

Bug 14807 - Multiprotocol ZED rejoins
RTP_BDB_02

Objective:

    To confirm that DUT clears address map entry after one unsuccessful attempt to join.

Devices:

	1. CTH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on CTH ZC
        2. Power on DUT ZR

Expected outcome:

    1. DUT ZR successfully joins after one unsuccessful join attempt (Transport Key was missed)
