Bug 12338 - ZB_ZDO_SIGNAL_LEAVE handler
RTP_BDB_10 - ZB_ZDO_SIGNAL_LEAVE signal handling

Objective:

    To confirm that the bdb_start_top_level_commissioning() calling on ZB_ZDO_SIGNAL_LEAVE signal 
        does not cause a failure status of steering or stack failure.

Devices:

	1. TH  - ZC
	2. TH  - ZR
	3. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
	2. Power on DUT ZED
	3. Power on TH ZR
        4. TH ZC sends two mgmt leave request to DUT ZED in ~5 sec one after another

Expected outcome:

	1. TH ZC creates a network

	2. TH ZR and DUT ZED start bdb_top_level_commissioning and get on the network established by TH ZC

        3.1. (Test procedure 4) After first mgmt leave request DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC
        3.2. There is no trace msg in dut's trace: "trigger_steering(): test FAILED"
        3.3. There is trace msg in dut's trace: "Successful steering: test OK"

        4. (Test procedure 4) After second mgmt leave request DUT ZED does not attempt to join any network
