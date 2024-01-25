Bug 12338 - ZB_ZDO_SIGNAL_LEAVE handler
RTP_BDB_11 - lack of ZB_ZDO_SIGNAL_LEAVE after lost "Transport key"

Objective:

    To confirm that the device does not send ZB_ZDO_SIGNAL_LEAVE signal to application after 
        unsuccessful join due to unreceived Transport key.

Devices:

	1. TH - ZC
	2. TH  - ZR
	3. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
	2. Power on TH ZR
	3. Power on DUT ZED
        4. TH ZC does not send transport key to DUT ZED

Expected outcome:

	1. TH ZC creates a network

	2. TH ZR start bdb_top_level_commissioning and get on the network established by TH ZC

        3. (Test procedure 4) DUT ZED start bdb_top_level_commissioning and attempts to associate with TH ZC

        4. DUT ZED successfully associates with TH ZR

        5. There is no trace msg in dut's trace: "signal: ZB_ZDO_SIGNAL_LEAVE, status [any_status]" after step 4
